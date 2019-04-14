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

class paragraph {
protected:
	char *str;
	int len;
public:
	paragraph() :str(nullptr), len(0) {};
	paragraph(char *s) { len = strlen(s); str = new char[len+4]; strcpy(str, s); }
	~paragraph() { if (len) { delete[] str; len = 0; } }
	int fill_par(char *val);	//returns length of loaded paragraph
	int del_par();				//returns 0 if OK, 1 if is empty
	int add_text_to_par(char *text);	//adds text to the end of the paragraph
	inline int get_length() { return len; }	//returns the length of the paragraph
	inline char * get_par() { return str; }	//gets paragraph's text
};


#define BUF_LEN 6000
class page {
protected:
	std::array<paragraph, 100> par_arr;
	int numpar;			//number of paragraphs in opened page
	char *pagename;		//name of opened page
	int open_page();	//returns number of paragraphs
	int del_page();		//frees all memory including paragraphs
public:
	page() :numpar(0), pagename(nullptr) {};
	page(char *pname) { int plen = strlen(pname); pagename = new char[plen+4]; strcpy(pagename, pname); open_page(); }
	~page() { del_page(); }
	inline char *get_page_name() { return pagename; }
	inline bool is_page(char *name) { return (strcmp(name, pagename) == 0); }
	int open_page(char *pname);	//opens page by name, returns number of paragraphs
	int delete_page();			//deletes whole current page, returns 0 if OK, 1 if not
	int del_paragraph(int num); //deletes paragraph[num], returns 0 if OK, 1 if not
	int add_to_par(int num, char *text);	//adds text to paragraph[num], returns length of new paragraph if OK, -1 if not
	int insert_paragraph_at_end(char *text); //returns length of text if OK, -1 if not
	int write_page();			//writes changes on the disk, returns 0 if OK, 1 if not
	static int delete_page(char *name) { return remove(name); }
};

class session
{
public:
	session(boost::asio::io_context& io_context, int thr_num) : socket_(io_context),
		thread_number(thr_num)
	{	};

	tcp::socket& socket()
	{		return socket_;	}

	void start();

	int get_thread_num() { return thread_number; }

private:
	int thread_number;
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred);
	
	void handle_write(const boost::system::error_code& error);

	tcp::socket socket_;
	enum { max_length = 28024 };
	char data_[max_length];
	char html_code[max_length], name_of[1000];
	int send_get_header(int leng);	//prepare data_ section and fill it with get header and entity
	int send_head_header(int leng); //prepare data_ section and fill it with set header
	int send_post_header(int leng); //prepare data_ section and fill it with post header
};

class server
{
public:
	server(boost::asio::io_context& io_context, short port, int num_threads)
		: io_context_(io_context),
		acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
		number_of_threads(num_threads)
	{
		init();
	};
	~server()
	{
		free_resources();
	}
	tcp::acceptor& get_acceptor() { return acceptor_; }
	void handle_accept(session* new_session,
		const boost::system::error_code& error, int num);
private:
	void init();
	void free_resources();
	void start_accept();
	
	boost::thread **threads;
	int number_of_threads;

	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
};

class cond_var : public boost::fibers::condition_variable
{
private:
	//data
	boost::asio::io_context* iocontext;
	server* serverlink;
	session* new_sess;
public:
	cond_var() : condition_variable() { iocontext = NULL; serverlink = NULL; };
	~cond_var() { };
	void set_data(boost::asio::io_context& iocont, server *serv, session *s)
	{
		new_sess = s;
		serverlink = serv;
		iocontext = &(iocont);
	}
	boost::asio::io_context* get_context() { return iocontext; }
	server* get_server() { return serverlink; }
	session* get_session() { return new_sess; }
};

