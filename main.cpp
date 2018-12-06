//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "Servers.h"

int *free_threads;
boost::fibers::mutex mtx;
bool *data_ready;
cond_var *vars;
bool fresh_start = false;

void session::start()
{
#ifdef DEBUG
	std::cout << "Thread " << get_id() << " called start session" << std::endl;
#endif // DEBUG

	socket_.async_read_some(boost::asio::buffer(data_, max_length),
		boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error,
	size_t bytes_transferred)
{
	if (!error)
	{
		int n;
		if ((data_[0] == 'G') && (data_[1] == 'E') && (data_[2] == 'T'))
		{//GET request
#ifdef DEBUG
			data_[bytes_transferred] = 0;
			std::cout << "Thread " << get_id() << std::endl;
			printf("REQUEST = %s\n", data_);
#endif // DEBUG
			n = send_get_header(bytes_transferred);
		}
		else if ((data_[0] == 'H') && (data_[1] == 'E') && (data_[2] == 'A') && (data_[3] == 'D'))
		{//HEAD request
#ifdef DEBUG
			data_[bytes_transferred] = 0;
			printf("REQUEST = %s\n", data_);
#endif // DEBUG
			n = send_head_header(bytes_transferred);
		}
		else if ((data_[0] == 'P') && (data_[1] == 'O') && (data_[2] == 'S') && (data_[3] == 'T'))
		{//HEAD request
#ifdef DEBUG
			data_[bytes_transferred] = 0;
			printf("REQUEST = %s\n", data_);
#endif // DEBUG
			n = send_post_header(bytes_transferred);
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

void session::handle_write(const boost::system::error_code& error)
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
		free_threads[this->thread_number] = 1;
		delete this;
	}
}

int session::send_get_header(int leng)
	{
		int i, n;
		i = 5;
		//printf("%d\n", bytes_transferred);
		while ((data_[i] != ' ') && (i < leng - 1)
			&& (data_[i] != '\n') && (data_[i] != 0)) {
			name_of[i - 5] = data_[i];
			i++;
		}
		name_of[i - 5] = 0;
		FILE *f = fopen(name_of, "rb");
#ifdef DEBUG
		printf("%s\n", name_of);
#endif // DEBUG

		if (!f)
		{//no such file
			sprintf(html_code, "<html><head><title>ERROR</title></head><body><h1>404 Not Found</h1>%s</body></html>\r\n", name_of);
			sprintf(data_, "HTTP/1.1 404 ERROR\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				strlen(html_code), "text/html", "close\r\n");
			n = strlen(data_);

			strcat(data_, html_code);
			n = strlen(data_);
		}
		else
		{
			char type[16];
			int len_name = strlen(name_of), flag = 0;
			if ((strcmp(name_of + len_name - 3, "bmp") == 0) || (strcmp(name_of + len_name - 3, "png") == 0)) {
				sprintf(type, "image/png");
				flag = 1;
			}
			else sprintf(type, "text/html");
			char buf[256];
			i = 0;
			
			while (1) {
				n = fread(buf, 1, 256, f);
				memcpy(html_code + i, buf, n);
				i += n;
				if (i >= max_length - 3)
					break;
				if (n < 256)
					break;
			}
			if (!flag) {
				html_code[i++] = '\r';
				html_code[i++] = '\n';
				html_code[i] = 0;
			}
			n = i;
			//if (!flag) {
				sprintf(data_, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
					"my  (Win32)",
					n, type, "close\r\n");
			//}
			//else
			//	data_[0] = data_[1] = 0;
			int cur_len = strlen(data_);
#ifdef DEBUG
			printf("DATA = %s\n", data_);
#endif // DEBUG
			memcpy(data_+ cur_len, html_code, n);
			//strcat(data_, html_code);
			n += cur_len+1;
			data_[n] = 0;
			data_[n - 1] = 0;
#ifdef DEBUG
			printf("FINAL_DATA = %s\n", data_);
#endif // DEBUG
			fclose(f);
		}
#ifdef DEBUG
		printf("LENGTH = %d\n", n);
#endif // DEBUG
		return n;
	}
int session::send_head_header(int leng)
	{
		int i, n;
		i = 6;
		//printf("%d\n", bytes_transferred);
		while ((data_[i] != ' ') && (i < leng - 1)
			&& (data_[i] != '\n') && (data_[i] != 0)) {
			name_of[i - 6] = data_[i];
			i++;
		}
		name_of[i - 6] = 0;
		FILE *f = fopen(name_of, "rb");
#ifdef DEBUG
		printf("%s\n", name_of);
#endif // DEBUG

		if (!f)
		{//no such file
			html_code[0] = 0;
			sprintf(data_, "HTTP/1.1 404 ERROR\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				strlen(html_code), "text/html", "close\r\n");
			n = strlen(data_);

			strcat(data_, html_code);
			n = strlen(data_)+1;
		}
		else
		{
			char type[16];
			struct stat buff_stat;
			fstat(fileno(f), &buff_stat);
			int len_name = strlen(name_of), flag = 0;
			if ((strcmp(name_of + len_name - 3, "bmp") == 0) || (strcmp(name_of + len_name - 3, "png") == 0)) {
				sprintf(type, "image/png");
				flag = 1;
			}
			else sprintf(type, "text/html");
			n = 0;
			//if (!flag) {
			sprintf(data_, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				buff_stat.st_size, type, "close\r\n");
			//}
			//else
			//	data_[0] = data_[1] = 0;
			int cur_len = strlen(data_);
#ifdef DEBUG
			printf("DATA = %s\n", data_);
#endif // DEBUG
			memcpy(data_ + cur_len, html_code, n);
			//strcat(data_, html_code);
			n += cur_len + 1;
#ifdef DEBUG
			printf("FINAL_DATA = %s\n", data_);
#endif // DEBUG
			fclose(f);
		}
		data_[n] = 0;
		data_[n - 1] = 0;
#ifdef DEBUG
		printf("LENGTH = %d\n", n);
#endif // DEBUG
		return n;
	}
int session::send_post_header(int leng)
	{
		int i, n;
		i = 6;
		//printf("%d\n", bytes_transferred);
		while ((data_[i] != ' ') && (i < leng - 1)
			&& (data_[i] != '\n') && (data_[i] != 0)) {
			name_of[i - 6] = data_[i];
			i++;
		}
		name_of[i - 6] = 0;
		FILE *f = fopen(name_of, "rb");
#ifdef DEBUG
		printf("%s\n", name_of);
#endif // DEBUG

		if (!f)
		{//no such file
			html_code[0] = 0;
			sprintf(data_, "HTTP/1.1 404 ERROR\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				strlen(html_code), "text/html", "close\r\n");
			n = strlen(data_);

			strcat(data_, html_code);
			n = strlen(data_) + 1;
		}
		else
		{
			char type[16];
			int len_name = strlen(name_of), flag = 0;
			if ((strcmp(name_of + len_name - 3, "bmp") == 0) || (strcmp(name_of + len_name - 3, "png") == 0)) {
				sprintf(type, "image/png");
				flag = 1;
			}
			else sprintf(type, "text/html");
			n = 0;
			//if (!flag) {
			sprintf(data_, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				n, type, "close\r\n");
			//}
			//else
			//	data_[0] = data_[1] = 0;
			int cur_len = strlen(data_);
#ifdef DEBUG
			printf("DATA = %s\n", data_);
#endif // DEBUG
			memcpy(data_ + cur_len, html_code, n);
			//strcat(data_, html_code);
			n += cur_len + 1;
#ifdef DEBUG
			printf("FINAL_DATA = %s\n", data_);
#endif // DEBUG
			fclose(f);
		}
		data_[n] = 0;
		data_[n - 1] = 0;
#ifdef DEBUG
		printf("LENGTH = %d\n", n);
#endif // DEBUG
		return n;
	}

void server::init()
{
	free_threads = new int[number_of_threads];
	data_ready = new bool[number_of_threads];
	for (int i = 0; i < number_of_threads; i++) {
		data_ready[i] = false;
		free_threads[i] = 1;
	}
	vars = new cond_var[number_of_threads];
	io_contexts = new boost::asio::io_context[number_of_threads];
	threads = new boost::thread*[number_of_threads];

	

	for (int i = 0; i < number_of_threads; i++) {
		io_contexts[i].run();
		threads[i] = new boost::thread{ run_threads, i };
	}

	std::cout << "Thread " << get_id() << std::endl;
	//t1 = new boost::thread{ &server::start_accept, this };
	start_accept();
}

void server::free_resources(){
	fresh_start = false;
		for (int i = 0; i < number_of_threads; i++)
			threads[i]->join();
		delete[] threads;
		delete[] vars;
		delete[] data_ready;
		delete[] free_threads;
		delete[] io_contexts;
}

void server::start_accept()
{
	//acceptor_.wait(boost::asio::ip::tcp::acceptor::wait_read);
	boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> ep = acceptor_.local_endpoint();
	//ep.port(0);
	acceptor_.bind(ep);
	std::cout << "PORT " << acceptor_.local_endpoint().port() << endl;
	boost::system::error_code ec;
	boost::system::error_code e1 = acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (!ec) {
		/*fresh_start = true;
		int i = 0;
		while ((!free_threads[i]) && (i < number_of_threads))
			i++;
		if (i < number_of_threads)
		{
			//vars[i].set_data(io_contexts[i], this);
			vars[i].set_data(io_context_, this);
			data_ready[i] = true;
			free_threads[i] = 0;
		}*/
	}
}


void run_threads(int number)
{
	boost::thread::id id_thr = get_id();
#ifdef  DEBUG
	std::cout << "Thread " << id_thr << " is " << number << std::endl;
#endif //  
	while (1) {
		{
			std::unique_lock< boost::fibers::mutex > lk(mtx);
			while ((!data_ready[number]) || (!fresh_start)) {
				vars[number].wait(lk);
			}
		}
		session* new_session = new session(*vars[number].get_context(), number);

		//std::cout << "Local endpoint = " << new_session->socket().local_endpoint().port() << endl;

		vars[number].get_server()->get_acceptor().async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, vars[number].get_server(), new_session,
				boost::asio::placeholders::error));
	}
}