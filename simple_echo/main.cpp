//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#include "Servers.h"

extern bool verbose;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//This is multi-thread HTTP server
//In order to construct it in the way, I needed conditional variables,
//global variables to be accessed by different threads
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//array of values: free_threads[i]=0 means thread_i is busy, otherwise =1.
int *free_threads;
//mutex for locking critical sections
boost::fibers::mutex mtx, mtx2;
//array of values: data_ready[i]=true if data for thread_i is ready, otherwise =false
bool *data_ready;
//conditional variables that contain special values
cond_var *vars, start_var;
//array of values: if ready to start the thread
bool *fresh_start;

void session::start()
{
if(verbose){
	std::cout << "Thread " << get_id() << " called start session with " << thread_number << std::endl;
}
	//register handler after reading from socket
	socket_.async_read_some(boost::asio::buffer(data_, max_length),
		boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error,
	size_t bytes_transferred)
{
	if (!error)//no error
	{
		int n;	//length of output
if(verbose){
		data_[bytes_transferred] = 0;
		data_[bytes_transferred+1] = 0;
		data_[bytes_transferred + 2] = 0;
		std::cout << "Thread " << get_id() << std::endl;	//we print current id of the thread
		printf("REQUEST = %s\n", data_);
}
		if ((data_[0] == 'G') && (data_[1] == 'E') && (data_[2] == 'T'))
		{//GET request send to handler
			n = send_get_header(bytes_transferred);
		}
		else if ((data_[0] == 'H') && (data_[1] == 'E') && (data_[2] == 'A') && (data_[3] == 'D'))
		{//HEAD request send to handler
			n = send_head_header(bytes_transferred);
		}
		else if ((data_[0] == 'P') && (data_[1] == 'O') && (data_[2] == 'S') && (data_[3] == 'T'))
		{//HEAD request send to handler
			n = send_post_header(bytes_transferred);
		}
		else
		{//unknown request
			sprintf(html_code, "HTTP / 1.1 404 ERROR\r\nServer: %s\r\nContent - Length: %d\r\nContent - Type: %s\r\nConnection: %s\r\n<html><head><title>ERROR</title></head><body><h1>Don't understand you yet</h1></body></html>\r\n",
				"my  (Win32)",
				strlen(html_code), "text/html", "close\r\n");
			strcpy(data_, html_code);
			n = strlen(html_code);
		}
		//register handler to do after write
		boost::asio::async_write(socket_,
			boost::asio::buffer(data_, n),
			boost::bind(&session::handle_write, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//if error close connection
		try {
			socket_.close();
			free_threads[this->thread_number] = 1;
			fresh_start[this->thread_number] = true;
			start_var.notify_all();
			delete this;
		}
		catch (...)
		{

		}
	}
}

void session::handle_write(const boost::system::error_code& error)
{
	if (!error)
	{
			try {
				//if here then all data is written close connection
				socket_.close();
				free_threads[this->thread_number] = 1;
				fresh_start[this->thread_number] = true;
				start_var.notify_all();
				delete this;
			}
			catch (...)
			{
				//do nothing
			}
	}
	else {
		try {
			socket_.close();
			fresh_start[this->thread_number] = true;
			free_threads[this->thread_number] = 1;
			start_var.notify_all();
			delete this;
		}
		catch (...)
		{
			//do nothing
		}
	}
}

int session::send_get_header(int leng)
	{
		int i, n;
		i = 5;//shift determined by 'GET /' = 5 letters
		while ((data_[i] != ' ') && (i < leng - 1)
			&& (data_[i] != '\n') && (data_[i] != 0)) {
			name_of[i - 5] = data_[i];
			i++;
		}
		name_of[i - 5] = 0;//obtain name of file to open
		FILE *f = fopen(name_of, "rb");
if(verbose){
		printf("%s\n", name_of);
}

		if (!f)
		{//no such file
			//we prepare special respond with error message
			sprintf(html_code, "<html><head><title>ERROR</title></head><body><h1>404 Not Found</h1>%s</body></html>\r\n", name_of);
			sprintf(data_, "HTTP/1.1 404 ERROR\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				strlen(html_code), "text/html", "close\r\n");
			n = strlen(data_);
			//we write the text
			strcat(data_, html_code);
			n = strlen(data_);
		}
		else
		{
			char type[16];
			//we open the file and get its type
			int len_name = strlen(name_of), flag = 0;
			if ((strcmp(name_of + len_name - 3, "bmp") == 0) || (strcmp(name_of + len_name - 3, "png") == 0)) {
				sprintf(type, "image/png");
				flag = 1;
			}
			else sprintf(type, "text/html");
			char buf[256];
			i = 0;
			//read file until end and copy to html_code buffer
			while (1) {
				n = fread(buf, 1, 256, f);
				memcpy(html_code + i, buf, n);
				i += n;
				if (i >= max_length - 3)
					break;
				if (n < 256)
					break;
			}
			if (!flag) {//if text/html we write the end of the entity to output, so browser doesn't wait for continuation
				html_code[i++] = '\r';
				html_code[i++] = '\n';
				html_code[i] = 0;
			}
			n = i;//size of the file, preparing the HEADer
			sprintf(data_, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
				"my  (Win32)",
				n, type, "close\r\n");
			int cur_len = strlen(data_);
			//copy data to data_
			memcpy(data_+ cur_len, html_code, n);
			n += cur_len+1;
			data_[n] = 0;
			data_[n - 1] = 0;
if(verbose){
			printf("FINAL_DATA = %s\n", data_);
}
			fclose(f);	//close file
		}
if(verbose){
		printf("LENGTH = %d\n", n);
}
		return n;
	}
int session::send_head_header(int leng)
{//the same things like with GET request, but we don't write the entity to the respond
	int i, n;
	i = 6;
	while ((data_[i] != ' ') && (i < leng - 1)
		&& (data_[i] != '\n') && (data_[i] != 0)) {
		name_of[i - 6] = data_[i];
		i++;
	}
	name_of[i - 6] = 0;
	FILE *f = fopen(name_of, "rb");
if(verbose){
		printf("%s\n", name_of);
}
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
		sprintf(data_, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
			"my  (Win32)",
			buff_stat.st_size, type, "close\r\n");
		int cur_len = strlen(data_);
		memcpy(data_ + cur_len, html_code, n);
		n += cur_len + 1;
if(verbose){
			printf("FINAL_DATA = %s\n", data_);
}
			fclose(f);
		}
		data_[n] = 0;
		data_[n - 1] = 0;
if(verbose){
		printf("LENGTH = %d\n", n);
}
	return n;
}

/////////////POST
//LANGUAGE:
/*
delpage page_name					1
delpar page_name par_number			2
addpar page_name par_number text	3
*/

//! returns the code of the command from input
int get_command(char *cmnd)
{
	if ((cmnd[0] == 'd') && (cmnd[1] == 'e') && (cmnd[2] == 'l') && (cmnd[3] == 'p') && (cmnd[4] == 'a') && (cmnd[5] == 'g') && (cmnd[1] == 'e'))
		return 1;
	else if ((cmnd[0] == 'd') && (cmnd[1] == 'e') && (cmnd[2] == 'l') && (cmnd[3] == 'p') && (cmnd[4] == 'a') && (cmnd[5] == 'r'))
		return 2;
	else if ((cmnd[0] == 'a') && (cmnd[1] == 'd') && (cmnd[2] == 'd') && (cmnd[3] == 'p') && (cmnd[4] == 'a') && (cmnd[5] == 'r'))
		return 3;
	else return 0;
}

//! if everything is OK, than we have standard message
void fill_OK_msg(char *_data)
{
	sprintf(_data, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: %s\r\n",
		"my  (Win32)", 0, "close\r\n");
	return;
}

//! if it is not OK, we also have the standard respond
void fill_ERROR_msg(char *_data)
{
	sprintf(_data, "HTTP/1.1 404 ERROR\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n",
		"my  (Win32)", 0, "text/html", "close\r\n");
	return;
}

int session::send_post_header(int leng)
	{
		int i, n, j = 0, cm, result;
		i = 5;

		char command[10];
		sscanf(data_+i, "%s", command);
		i += strlen(command) + 1;
		cm = get_command(command);	//we get the command
		if (verbose) {
			printf("COMMAND ID = %d\n", cm);
		}
		//getting the name of the file
		while ((data_[i] != ' ') && (i < leng - 1)
			&& (data_[i] != '\n') && (data_[i] != 0)) {
			name_of[j] = data_[i];
			i++; j++;
		}
		name_of[j] = 0;

		if (verbose) {
			printf("NAME = %s\n", name_of);
		}

		name_of[j] = 0;
		char type[16];
		switch (cm) {
		case 0:
			if (verbose) {
				printf("WRONG command\n");
			}
			fill_ERROR_msg(data_);
			break;
		case 1:
			n = 0;
			if (verbose) {
				printf("DEL PAGE\n");
			}
			result = page::delete_page(name_of);
			if (!result)
			{
				if (verbose) {
					printf("FILE DELETED\n");
				}
				fill_OK_msg(data_);
			}
			else
			{
				if (verbose) {
					printf("NO FILE FOUND\n");
				}
				fill_ERROR_msg(data_);
			}
			break;
		case 2:	//deleting paragraph from the page
		{
			page xpage(name_of);
			result = sscanf(data_ + i, "%d", &j);
			if (result < 1) {
				//error
				fill_ERROR_msg(data_);
			}
			else
			{
				result = xpage.del_paragraph(j);
				xpage.write_page();
				fill_OK_msg(data_);
			}
		}
			break;
		case 3:
		{
			if (verbose) {
				printf("ADD PARAGRAPH\n");
			}
			page xpage(name_of);
			result = sscanf(data_ + i, "%d", &j);
			if (verbose) {
				printf("PAGE name = %s\r\nPAR No = %d\n", name_of, j);
			}
			if (result < 1)
			{
				if (verbose) {
					printf("WRONG\n");
				}
				fill_ERROR_msg(data_);
			}
			else {
				i++;
				while (data_[i] != ' ')
					i++;
				i++;
				n = i;
				while (data_[n] != '\n')
					n++;
				data_[n] = 0;
				if (verbose) {
					printf("TEXT = %s\n", data_+i);
				}
				xpage.add_to_par(j, data_ + i);
				xpage.write_page();
				fill_OK_msg(data_);
			}
		}
			break;
		}
		n = strlen(data_) + 1;
if(verbose){
		printf("%s\n", name_of);
}	
if(verbose){
			printf("FINAL_DATA = %s\n", data_);
}
		data_[n] = 0;
		data_[n - 1] = 0;
if(verbose){
		printf("LENGTH = %d\n", n);
}
		return n;
}

void server::init()
{
	//initialization of data
	free_threads = new int[number_of_threads];
	data_ready = new bool[number_of_threads];
	fresh_start = new bool[number_of_threads];
	for (int i = 0; i < number_of_threads; i++) {
		data_ready[i] = false;
		free_threads[i] = 1;
		fresh_start[i] = false;
	}
	vars = new cond_var[number_of_threads];
	threads = new boost::thread*[number_of_threads];

	for (int i = 0; i < number_of_threads; i++) {
		threads[i] = new boost::thread{ run_threads, i };
		threads[i]->detach();
	}
	if (verbose) {
		std::cout << "Thread " << get_id() << std::endl;
	}
	start_accept();
}

void server::free_resources(){
	std::unique_lock< boost::fibers::mutex > lk(mtx);
	fresh_start = false;
		for (int i = 0; i < number_of_threads; i++)
			delete threads[i];
		delete[] threads;
		delete[] vars;
		delete[] data_ready;
		delete[] free_threads;
		threads = NULL;
		vars = NULL;
		data_ready = NULL;
		free_threads = NULL;
}

void server::start_accept()
{
	start_var.set_data(io_context_, this, NULL);
	int i;
	{
		std::unique_lock< boost::fibers::mutex > lk(mtx);
		for (i = 0; i < this->number_of_threads; i++) {
			if (free_threads[i])
				break;
			else
				fresh_start[i] = false;
		}
		if (i < number_of_threads)
			fresh_start[i] = true;
		start_var.notify_all();
	}
}

void server::handle_accept(session* new_session,
	const boost::system::error_code& error, int num)
{
	if (!error)
	{
		int i = num;
		if (i < number_of_threads)
		{
			{
				std::unique_lock< boost::fibers::mutex > lk(mtx);
				vars[i].set_data(io_context_, this, new_session);
				data_ready[i] = true;
				free_threads[i] = 0;
				vars[i].notify_all();
			}
		}
	}
	else
	{
		delete new_session;
	}

	start_accept();
}

void run_threads(int number)
{
	boost::thread::id id_thr = get_id();
	if (verbose) {
		std::cout << "Thread " << id_thr << " is " << number << std::endl;
	}
	boost::asio::io_context io_context_cur;
	while (1) {
		{
			std::unique_lock< boost::fibers::mutex > lk(mtx2);
			while (!fresh_start[number]) {
				start_var.wait(lk);
			}
		}
		free_threads[number] = 0;
		fresh_start[number] = false;
		session* new_session = new session(io_context_cur, number);
		start_var.get_server()->get_acceptor().async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, start_var.get_server(), new_session,
				boost::asio::placeholders::error, number));
		{
			std::unique_lock< boost::fibers::mutex > lk(mtx);
			while (!data_ready[number]) {
				if (vars != NULL)
					vars[number].wait(lk);
				else
					break;
			}
		}
		if (vars != NULL) {
			vars[number].get_session()->start();
			data_ready[number] = false;
			io_context_cur.restart();
			io_context_cur.run();
		}
		else
			break;
	}
}

/*********PARAGRAPH*********/
//PARAGRAPH methods
//void fill_par(char *val);
int paragraph::fill_par(char *val)
{
	if (str != nullptr) {
		delete[] str;
		len = 0;
	}
	len = strlen(val);
	str = new char[len+4];
	strcpy(str, val);
	return len;
}

//int del_par();
int paragraph::del_par()
{
	if (str != nullptr) {
		delete[] str;
		str = nullptr;
		len = 0;
		return 0;
	}
	else
		return 1;
}
//int add_text_to_par(char *text);
int paragraph::add_text_to_par(char *text) {
	int tlen = strlen(text), i;
	char *tmp = new char[len + tlen + 4];
	strcpy(tmp, str);
	if (str != nullptr)
		delete[] str;
	str = tmp;
	for (i = 0; i < tlen; i++)
	{
		str[len - 1 + i] = text[i];
	}
	str[len - 1 + i] = '\r';
	i++;
	str[len - 1 + i] = '\n';
	i++;
	str[len - 1 + i] = 0;
	len = strlen(str);
	return len;
}

/*********PAGE*********/
//PAGE methods
//int open_page();
int page::open_page()
{
	if (pagename == nullptr)
	{
		//throw std::domain_error("No page name specified");
		return 0;
	}
	else
	{
		FILE *f = fopen(pagename, "r");
		if (!f)
			return 0;
		numpar = 0;
		int symnum = 0, readn;
		char symb, *buf = new char[BUF_LEN];
		while (true) {
			//reading entity
			readn = fscanf(f, "%c", &symb);
			if (readn < 1)
			{
				//EOF
				break;
			}
			else
			{
				buf[symnum++] = symb;
				if (symb == '\n')
				{
					buf[symnum] = 0;
					par_arr[numpar++].fill_par(buf);
					symnum = 0;
				}	
			}
		}
		if (verbose) {
			printf("EOF %d\n", symnum);
		}
		fclose(f);
		if (symnum > 0)
		{
			par_arr[numpar++].fill_par(buf);
			return numpar;
		}
		//EOF
		fclose(f);
		return numpar;
	}
}

//int open_page(char *pname);
int page::open_page(char *pname)
{
	if (pagename != nullptr)
	{
		del_page();
	}
	int plen = strlen(pname); 
	pagename = new char[plen+2]; 
	strcpy(pagename, pname); 
	return open_page();
}
//int del_page();
int page::del_page()
{
	if (pagename != nullptr) {
		delete[] pagename;
		pagename = nullptr;
		for (int i = 0; i < numpar; i++)
		{
			par_arr[i].del_par();
		}
		numpar = 0;
		return 0;
	}
	return 1;
}
//int del_paragraph(int num);
int page::del_paragraph(int num)
{
	if ((num >= numpar) || (num < 1))	//cannot delete paragraph by this number
	{
		return 1;
	}
	else
	{
		return par_arr[num].del_par();
	}
}
//int insert_paragraph_at_end(char *text);
int page::insert_paragraph_at_end(char *text)
{
	if (numpar > 0)
	{
		return par_arr[numpar++].fill_par(text);
	}
	else
		return -1;
}

//int add_to_par(int num, char *text);
int page::add_to_par(int num, char *text) {
	if ((num >= numpar) || (num < 1))
	{
		return -1;
	}
	else
	{
		return par_arr[num].add_text_to_par(text);
	}
}

//int write_page()
int page::write_page()
{
	if (pagename == nullptr) {
		//throw invalid_argument("No opened page to write");
		return 1;
	}
	FILE *f = fopen(pagename, "wb");
	if (!f)
		return 1;
	char *text;
	int len;
	for (int i = 0; i < numpar; i++)
	{
		len = par_arr[i].get_length();
		if (len > 0) {
			text = par_arr[i].get_par();
			fwrite(text, 1, len, f);
		}
	}
	fclose(f);
	return 0;
}

//int delete_page()
int page::delete_page() {
	if (pagename != nullptr)
	{
		remove(pagename);
		del_page();
		pagename = nullptr;
		return 0;
	}
	else return 1;
}


//// TEST
void TestPage::run() {
	FILE *fn;
	char sname[125];
	/// SEARCH for the file name that does not exist
	int i = rand() % 1000;
	sprintf(sname, "pge%d.html", i);
	fn = fopen(sname, "r");
	while (fn) {
		i = i * i + 1;
		sprintf(sname, "pge%d.html", i);
		fn = fopen(sname, "r");
	}
	/// FOUND it. Create new file
	fn = fopen(sname, "w");
	fprintf(fn, "<html>\n<head>\nWOW, it is a page!\n</head>\n<body>\n<br>HI, I AM a PAGE\n<br>TRY to catch me\n</body></html>\n");
	fclose(fn);
	/// CREATED
	cout << "PAGE NAME : " << sname << '\n';
	cout << "PRESS ENTER after you check the file\n";
	getchar();
	/// TEST 1. Open the page
	page xy(sname);
	cout << "TEST 1\nCOMPARE NAMES: " << (strcmp(xy.get_page_name(), sname) == 0 ? "OK" : "NOT OK") << '\n';
	
	/// TEST 2. Add text to paragraph 5
	cout << "TEST 2\nADD paragraph : BEFORE : " << xy.get_paragraph(5) << '\n';
	cout << "RESULT CODE : " << (xy.add_to_par(5, " , Oh my") == -1? "NOT OK" : "OK") << '\n';
	cout << "GOT : " << xy.get_paragraph(5) << '\n';
	xy.write_page();
	cout << "PRESS ENTER after you check the file\n";
	getchar();
	xy.open_page(sname);

	/// TEST 3. Delete paragraph 5
	cout << "TEST 3\nDELETE paragraph : " << xy.del_paragraph(5) << '\n';
	xy.write_page();
	cout << "PRESS ENTER after you check the file\n";
	getchar();
	xy.open_page(sname);
	cout << "NOW : " << xy.get_paragraph(5) << '\n';

	/// TEST 5. DELETE page
	cout << "TEST 5\nDELETE page : " << xy.delete_page() << '\n';
	cout << "CHECK : " << (fopen(sname, "r") == 0 ? "OK" : "NOT OK") << '\n';
	return;
}