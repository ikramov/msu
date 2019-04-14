#include <boost/program_options.hpp>
#include "Servers.h"

bool verbose = true;

using namespace boost::program_options;

class server_manager{

public: 
        variables_map vm;
        int v=0;

        //boost::asio::io_context io_context;
        //server new_server(io_context, atoi(argv[1]));

        int cmline_parser(int argc, char *argv[])
        {
            try{
                options_description desc("Options");
                desc.add_options()
                    ("ip,i",value<std::string>()->default_value("0.0.0.0"),"IP")
                    ("port,p",value<int>()->default_value(8000),"PORT")
                    ("workers,w",value<int>()->default_value(1),"Count of workers")
                    ("help,h", "HELP");
                    ("verbose,v",value<bool>()->default_value(0),"Print log");
            
                store(parse_command_line(argc, argv, desc), this->vm);
                notify(this->vm);
                
                if (this->vm.count("help")){
                    std::cout << desc << '\n';
                    return -1;}
                else {
                    if (this->vm.count("verbose"))
                        v=1;
                }
            }
            catch (const error &ex)
                {
                    std::cerr << ex.what() << '\n';
                }

            return 0;	
        } 

       ~server_manager(){};

        server_manager(int argc, char *argv[])
        {
            if (cmline_parser(argc, argv)!=-1)
            {
               
            }
            else this->~server_manager();
        }   

        int run_server_test()
        {
            return 0;
        }   
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
		server s(io_context, atoi(argv[1]), 3);
		while (true) {
			io_context.run();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}