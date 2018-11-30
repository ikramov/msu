//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <string.h>

using boost::asio::ip::tcp;

class session
{
public:
	session(boost::asio::io_context& io_context)
		: socket_(io_context)
	{
		
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			boost::bind(&session::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

private:
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{
			int i, n;
			if ((data_[0] == 'G') && (data_[1] == 'E') && (data_[2] == 'T'))
			{//GET request
				i = 5;
				//printf("%d\n", bytes_transferred);
				while ((data_[i] != ' ') && (i < bytes_transferred-1)
					&& (data_[i] != '\n') && (data_[i]!=0)) {
					name_of[i - 5] = data_[i];
					i++;
				}
				name_of[i-5] = 0;
				FILE *f = fopen(name_of, "rb");
				if (!f)
				{//no such file
					sprintf(html_code, "<html><head><title>ERROR</title></head><body><h1>404 Not Found</h1>%s</body></html>\n\r", name_of);
					strcpy(data_, html_code);
					n = strlen(html_code);
				}
				else
				{
					char buf[256];
					i = 0;
					while (1) {
						n = fread(buf, 1, 256, f);
						memcpy(data_ + i, buf, n);
						i+=n;
						if (i >= max_length-3)
							break;
						if (n < 256)
							break;
					}
					data_[i++] = '\n';
					data_[i++] = '\r';
					data_[i] = 0;
					n = i;
					fclose(f);		
				}
			}
			else
			{
				sprintf(html_code, "<html><head><title>ERROR</title></head><body><h1>Don't understand you yet</h1></body></html>\n\r");
				strcpy(data_, html_code);
				n = strlen(html_code);
			}
			boost::asio::async_write(socket_,
				boost::asio::buffer(data_, n),
				boost::bind(&session::handle_write, this,
					boost::asio::placeholders::error));
		}
		else
		{
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

	tcp::socket socket_;
	enum { max_length = 28024 };
	char data_[max_length];
	char html_code[max_length], name_of[1000];
};

class server
{
public:
	server(boost::asio::io_context& io_context, short port)
		: io_context_(io_context),
		acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		start_accept();
	}

private:
	void start_accept()
	{
		session* new_session = new session(io_context_);
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	void handle_accept(session* new_session,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept();
	}

	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
			return 1;
		}

		boost::asio::io_context io_context;

		using namespace std; // For atoi.
		server s(io_context, atoi(argv[1]));

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}