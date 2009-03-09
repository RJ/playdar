#include "application/application.h"
//#include "resolvers/darknet/rs_darknet.h"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <iterator>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include "playdar_request_handler.h"

#include <boost/algorithm/string.hpp>

using namespace std;
namespace po = boost::program_options;

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
    return os;
}



void start_http_server(string ip, int port, int conc, MyApplication *app)
{
    cout << "HTTP server starting on: http://" << ip << ":" << port << "/" << endl;
    moost::http::server<playdar_request_handler> s(ip, port, 3);
    s.request_handler().init(app);
    s.run();
    cout << "http_server thread exiting." << endl; 
}

int main(int ac, char *av[])
{
    try {
        int opt;
        po::options_description generic("Generic options");
        generic.add_options()
            ("config,c",    po::value<string>(),
                            "path to config file")
            ("version,v",   "print version string")
            ("help,h",      "print this message")
            ;
/*
            ("name",        po::value<string>(),
                 "Name used for your collection on the network")
            ("db",          po::value<string>(),
                 "Path to your database file")
            ("http_port",   po::value<int>(&opt)->default_value(8888),
                  "Port used for local webserver")
            ;
*/
        po::options_description cmdline_options;
        cmdline_options.add(generic);

        po::options_description visible("Playdar configuration");
        visible.add(generic);

        po::variables_map vm;
        po::parsed_options parsedopts_cmd = 
            po::command_line_parser(ac, av).
              options(cmdline_options).run();
              
        store(parsedopts_cmd, vm);
        notify(vm);
       
        if (vm.count("help")) {
            cout << visible << "\n";
            return 0;
        }
        if (vm.count("version")) {
            cout << "TODO\n";
            return 0;
        }
        
        if(!vm.count("config"))
        {
            cerr << "You must use a config file." << endl;
            return 1;
        }
        string configfile = vm["config"].as<string>();
        cout << "Using config file: " << configfile << endl;
        
        playdar::Config conf(configfile);
        if(conf.get<string>("name")=="YOURNAMEHERE")
        {
            cerr << "Please edit " << configfile << endl;
            cerr << "YOURNAMEHERE is not a valid name." << endl;
            return 42;
        }

        MyApplication * app = new MyApplication(conf);
        

        // start http server:
        string ip = "0.0.0.0"; 
        boost::thread http_thread(&start_http_server, ip,
                                  app->conf()->get<int>("http_port"),
                                  1, 
                                  app);
                                  
        // Lame interactive mode for debugging:
        /*
        RS_darknet * dnet = static_cast<RS_darknet *>( app->resolver()->get_darknet());
        if(dnet)
        {
            cout << "** Darknet resolver active, entering lame interactive mode **" << endl;
            std::string line;
            while(1)
            {
                std::getline(std::cin, line);
                std::vector<std::string> toks;
                boost::split(toks, line, boost::is_any_of(" "));
                // establish new connection:
                if(toks[0]=="connect")
                {
                    boost::asio::ip::address_v4 ip =
                        boost::asio::ip::address_v4::from_string(toks[1]);
                    unsigned short port = boost::lexical_cast<unsigned short>(toks[2]);
                    boost::asio::ip::tcp::endpoint ep(ip, port);
                    dnet->servent()->connect_to_remote(ep);
                }
                
            }
        }else{     
            // this will stop us exiting:
            http_thread.join();
        }*/
        http_thread.join();
        cout << "All threads finished, exiting cleanly" << endl;
        delete(app); 
        return 0;
    }
    catch(exception& e)
    {
        cout << "Playdar main exception: " << e.what() << "\n";
        return 1;
    }    
}

