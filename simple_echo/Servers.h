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

/*!
* \file
* \author Alisher Ikramov
* \date   April, 2019
* \brief  HTTP server with support of GET, HEAD, POST requests
*
* \section DESCRIPTION
*
* server --- class for establishing connection between user and system, provides:
* 1.	accepting of connection;
* 2.	receiving the request;
* 3.	sending the respond to the request.
*
* Requires:
* boost v.1.68.0
* netcat (optional)
*/

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
	paragraph(
		char *s		///< text of the paragraph
	) { len = strlen(s); str = new char[len+4]; strcpy(str, s); }

	//! default destructor
	~paragraph() { if (len) { delete[] str; len = 0; } }
															 
	//! returns the length of loaded paragraph after removing old text 
	//! and replacing it by new incoming text from val
	int fill_par(
		char *val	///< text of the paragraph, replaces the entire text
	);	
								 
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
	std::array<paragraph, 100> par_arr;	///< array of the paragraphs of the page

	int numpar;			///< number of paragraphs in opened page

	//! \var name of the current page
	char *pagename;		///< name of currently opened page

	//! hiden function to open page by the name in pagename
	int open_page();	///< returns current number of paragraphs

	//! hiden function to destroy all allocated memory if any
	int del_page();		///< frees all memory including paragraphs
public:
	//! default constructor that puts 0's to member fields
	page() :numpar(0), pagename(nullptr) {};

	//! constructor, 
	//! stores pname in pagename, also opens this page
	page(
		char *pname		///< name of the page to open and process
	) { int plen = strlen(pname); pagename = new char[plen+4]; strcpy(pagename, pname); open_page(); }

	//! destructor that calls del_page
	~page() { del_page(); }

	//! returns current page name
	inline char *get_page_name() { return pagename; }

	//! compares current page name with name
	//! \param[in]	name		name of the page to compare with
	inline bool is_page(
		char *name		///< name of the page to compare with
	) { return (strcmp(name, pagename) == 0); }

	//! opens page and processes all paragraphs
	//! \return		number of paragraphs
	int open_page(
		char *pname		///< name of the page to open
	);	//opens page by name, returns number of paragraphs

	//! deletes current page, returns 0 if OK, 1 if not
	int delete_page();			

	//! deletes current paragraph by its number
	//! \param[in]	num			the number of paragraph to delete
	//! \return		0 if OK, 1 if not
	int del_paragraph(int num);

	//! adds text to paragraph
	//! \param[in]	num			the number of the paragraph to which we add text
	//! \param[in]	text		text to add to paragraph
	//! \return		length of new paragraph if OK, -1 if not
	int add_to_par(int num, char *text);

	//! adds new paragraph at the end of the array
	//! \param[in]	text		new paragraph to add to the page's end
	//! \return		length of text if OK, -1 if not
	int insert_paragraph_at_end(char *text); 

	//! writes changes of the page on the disk
	//! \return		0 if OK, 1 if not
	int write_page();

	//! deletes page from the drive by its name
	//! \return		answer from the remove function
	static int delete_page(char *name) { return remove(name); }
};

/*! \class session
*  \brief operates with requests and sends the respond
*/
class session
{
public:
	//! standard constructor
	//! \param[in]		io_context			context of opened connection
	//! \param[in]		thr_num				this thread's number to store
	session(boost::asio::io_context& io_context, int thr_num) : socket_(io_context),
		thread_number(thr_num)
	{	};

	//!\return			current opened socket
	tcp::socket& socket()
	{		return socket_;	}

	//! function that initializes waiting for requests and all variables
	void start();

	//! returns current thread's number
	int get_thread_num() { return thread_number; }

private:
	int thread_number; ///< this thread's number

	//! the function that answers for the event of incoming request,
	//! it reads the buffer
	//! \param[in]		error					error code returned from the system if any
	//! \param[in]		bytes_transferred		number of bytes that are placed in the buffer
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred);
	
	//! the function that is called in case of event of writing something in the socket
	//! \param[in]		error					error code returned from the system if any
	void handle_write(const boost::system::error_code& error);

	tcp::socket socket_;		///<  current socket for this session
	enum { max_length = 28024 };

	char data_[max_length];		///< buffer for storing requests and responds

	char html_code[max_length], name_of[1000];	///< variables for storing some data

	//! prepares data_ section and fills it with GET header and entity (page or image)
	//! \param[in]		leng		the length of the data in request
	int send_get_header(int leng);

	//! prepares data_ section and fills it with HEAD header (no entity, just information)
	//! \param[in]		leng		the length of the data in request
	int send_head_header(int leng); 

	//! prepares data_ section and fills it with post header and runs some action in respond to request
	//! \param[in]		leng		the length of the data in request
	int send_post_header(int leng); 
};

/*! \class server
*  \brief HTTP server's main class
*/
class server
{
public:
	//! standard constructor, runs initializer
	//! \param[in]		io_context		accepts currently opened io_context
	//! \param[in]		port			number, default is 80
	//! \param[in]		num_threads		number of threads to run server in
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

	//! get function
	//! \return		current acceptor
	tcp::acceptor& get_acceptor() { return acceptor_; }

	//! function that is called in answer to the event of accepting new incoming connection
	//! \param[in]		new_session			link to new_session object
	//! \param[in]		error				error code if any
	//! \param[in]		num					the number of the thread
	void handle_accept(session* new_session,
		const boost::system::error_code& error, int num);
private:
	//! initializes all resources
	void init();

	//! frees all the memory used by the server
	void free_resources();

	//! function that starts the connection and puts some handlers
	void start_accept();
	
	boost::thread **threads;				///< the array of links to threads

	int number_of_threads;					///< the number of threads in this server

	boost::asio::io_context& io_context_;	///< the current context

	tcp::acceptor acceptor_;				///< the current acceptor
};

/*! \class cond_var
*  \brief condition variables are used for communication between threads
*/
class cond_var : public boost::fibers::condition_variable
{
private:
	boost::asio::io_context* iocontext;		///< current io_context

	server* serverlink;						///< pointer to current server

	session* new_sess;						///< pointer to current session
public:
	//! standard constructor
	cond_var() : condition_variable() { iocontext = NULL; serverlink = NULL; };

	//! standard destructor, no extra memory is used
	~cond_var() { };

	//! initializes the values of this class' object's members
	//! \param[in]		iocont			link to current io_context to store
	//! \param[in]		serv			link to server class current object
	//! \param[in]		s				link to session class current object
	void set_data(boost::asio::io_context& iocont, server *serv, session *s)
	{
		new_sess = s;
		serverlink = serv;
		iocontext = &(iocont);
	}

	//! \return		the current io_context
	boost::asio::io_context* get_context() { return iocontext; }

	//! \return		the pointer to current server
	server* get_server() { return serverlink; }

	//! \return		the pointer to current session
	session* get_session() { return new_sess; }
};
