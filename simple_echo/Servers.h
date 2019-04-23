#pragma once
#define DEBUG 1
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <string.h>
#include <boost/thread.hpp>
#include <stdio.h>
#include <Windows.h>
#include <type_traits>
#include <functional>
#include <cstdlib>
#include <iostream>

using boost::this_thread::get_id;
using namespace std;

using boost::asio::ip::tcp;

#include <boost/fiber/condition_variable.hpp>
using namespace boost::fibers;
void run_threads(int number);

/*! \class paragraph
*  \brief paragraph class to contain paragraphs of the text
*
*  Used in page class.
*/
class paragraph {
protected:
	//! paragraph's text storage
	char *str;	 

	//! length of the text in paragraph
	int len;	 
public:
	//! default constructor that puts nullptr into str
	paragraph() :str(nullptr), len(0) {};	 

	//! constructor that stores the paragraph. 
	//! copies incoming text from s and stores it into str.
	paragraph(char *s) { len = strlen(s); str = new char[len+4]; strcpy(str, s); }

	//! default destructor
	~paragraph() { if (len) { delete[] str; len = 0; } }
															 
	//! returns the length of loaded paragraph after removing old text
	//! and replacing it by new incoming text from val
	int fill_par(char *val);	
								 
	//! deletes paragraph and frees memory. returns 0 if OK, 1 if is empty
	int del_par();				
								

	//! adds text to the end of the paragraph
	int add_text_to_par(char *text);

	//! gets length in bytes of the current paragraph
	inline int get_length() { return len; }	

	//! gets current paragraph entity
	inline char * get_par() { return str; }	
};

//! Length of the Buffer used for storing temporary data
#define BUF_LEN 6000

/*! \class page
*  \brief page class contains array of paragraphs and function to work with them
*/
class page {
protected:

	//! \var array of paragraphs
	std::array<paragraph, 100> par_arr;

	//! \var number of paragraphs in current page
	int numpar;			//number of paragraphs in opened page

	//! \var name of the current page
	char *pagename;		//name of opened page

	//! hiden function to open page by the name in pagename
	int open_page();	//returns number of paragraphs

	//! hiden function to destroy all allocated memory if any
	int del_page();		//frees all memory including paragraphs
public:
	//! default constructor that puts 0's to member fields
	page() :numpar(0), pagename(nullptr) {};

	//! constructor
	//! pname --- name of the page to open and process
	//! stores pname in pagename, also opens this page
	page(char *pname) { int plen = strlen(pname); pagename = new char[plen+4]; strcpy(pagename, pname); open_page(); }

	//! destructor that calls del_page
	~page() { del_page(); }

	//! returns current page name
	inline char *get_page_name() { return pagename; }

	//! compares current page name with name
	inline bool is_page(char *name) { return (strcmp(name, pagename) == 0); }

	//! opens page
	//! pname --- name of the page to open
	int open_page(char *pname);	//opens page by name, returns number of paragraphs

	//! deletes current page, returns 0 if OK, 1 if not
	int delete_page();			

	//! deletes current paragraph by its number
	//! num --- the number of paragraph to delete, returns 0 if OK, 1 if not
	int del_paragraph(int num);

	//! adds text to paragraph
	//! num --- the number of the paragraph to which we add text
	//! text --- text to add to paragraph, returns length of new paragraph if OK, -1 if not
	int add_to_par(int num, char *text);

	//! adds new paragraph at the end of the array, returns length of text if OK, -1 if not
	int insert_paragraph_at_end(char *text); 

	//! writes changes of the page on the disk, returns 0 if OK, 1 if not
	int write_page();

	//! deletes page from the drive by its name
	static int delete_page(char *name) { return remove(name); }
};

/*! \class session
*  \brief operates with requests and sends the respond
*/
class session
{
public:
	//! standard constructor
	//! io_context --- context of opened connection
	//! thr_num --- this thread's number to store
	session(boost::asio::io_context& io_context, int thr_num) : socket_(io_context),
		thread_number(thr_num)
	{	};

	//! returns current opened socket
	tcp::socket& socket()
	{		return socket_;	}

	//! function that initializes waiting for requests and all variables
	void start();

	//! returns current thread's number
	int get_thread_num() { return thread_number; }

private:
	//! \var this thread's number
	int thread_number;

	//! the function that answers for the event of incoming request,
	//! it reads the buffer
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred);
	
	//! the function that is called in case of event of writing something in the socket
	void handle_write(const boost::system::error_code& error);

	//! \var current socket for this session
	tcp::socket socket_;
	enum { max_length = 28024 };

	//! \var buffer for storing requests and responds
	char data_[max_length];

	//! \var variables for storing some data
	char html_code[max_length], name_of[1000];

	//! prepares data_ section and fills it with GET header and entity (page or image)
	int send_get_header(int leng);

	//! prepares data_ section and fills it with HEAD header (no entity, just information)
	int send_head_header(int leng); 

	//! prepares data_ section and fills it with post header and runs some action in respond to request
	int send_post_header(int leng); 
};

/*! \class server
*  \brief HTTP server's main class
*/
class server
{
public:
	//! standard constructor, runs initializer
	//! accepts opened io_context
	//! port --- number, default is 80
	//! the number of threads to run
	server(boost::asio::io_context& io_context, short port, int num_threads)
		: io_context_(io_context),
		acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
		number_of_threads(num_threads)
	{
		init();
	};

	//! standard destructor, runs free_resources
	~server()
	{
		free_resources();
	}

	//! returns current acceptor
	tcp::acceptor& get_acceptor() { return acceptor_; }

	//! function that is called in answer to the event of accepting new incoming connection
	//! gets link to new_session object, error code and the number of the thread
	void handle_accept(session* new_session,
		const boost::system::error_code& error, int num);
private:
	//! initializes all resources
	void init();

	//! frees all the memory used by the server
	void free_resources();

	//! function that starts the connection and puts some handlers
	void start_accept();
	
	//! \var the array of links to threads
	boost::thread **threads;

	//! \var the number of threads in this server
	int number_of_threads;

	//! \var the current context
	boost::asio::io_context& io_context_;

	//! \var the current acceptor
	tcp::acceptor acceptor_;
};

/*! \class cond_var
*  \brief condition variables are used for communication between threads
*/
class cond_var : public boost::fibers::condition_variable
{
private:
	//! \var current io_context
	boost::asio::io_context* iocontext;

	//! \var pointer to current server
	server* serverlink;

	//! \var pointer to current session
	session* new_sess;
public:
	//! standard constructor
	cond_var() : condition_variable() { iocontext = NULL; serverlink = NULL; };

	//! standard destructor, no extra memory is used
	~cond_var() { };

	//! initializes the values of this class' object's members
	void set_data(boost::asio::io_context& iocont, server *serv, session *s)
	{
		new_sess = s;
		serverlink = serv;
		iocontext = &(iocont);
	}

	//! returns the current io_context
	boost::asio::io_context* get_context() { return iocontext; }

	//! returns the pointer to current server
	server* get_server() { return serverlink; }

	//! returns the pointer to current session
	session* get_session() { return new_sess; }
};
