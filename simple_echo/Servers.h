#pragma once
#define DEBUG 1
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <string.h>
#include <boost/thread.hpp>
using boost::this_thread::get_id;
using namespace std;

using boost::asio::ip::tcp;

#include <boost/fiber/condition_variable.hpp>
using namespace boost::fibers;
void run_threads(int number);

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

