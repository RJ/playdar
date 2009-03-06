#include "application/application.h"
#include "resolvers/darknet/rs_darknet.h"
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
    // won't work with >1 worker atm, because library db code isn't
    // yet threadsafe, amongst other things.
    moost::http::server<playdar_request_handler> s(ip, port, 3);
    s.request_handler().init(app);
    s.run();
    cout << "http_server thread exiting." << endl; 
}

int main(int ac, char *av[])
{
    try {
        int opt;

        // Declare a group of options that will be
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("config,c",    po::value<string>(),
                            "path to config file")
            ("version,v",   "print version string")
            ("help,h",      "print this message")
            ;
        // Declare a group of options that will be
        // allowed both on command line and in
        // config file
        po::options_description config("App config");
        config.add_options()
            ("app.name",        po::value<string>(),
                 "Name used for your collection on the network")
            ("app.db",          po::value<string>(),
                 "Path to your database file")
            ("app.http_port",   po::value<int>(&opt)->default_value(8888),
                  "Port used for local webserver")
            ;
            
            /*
            ("app.private_ip",  po::value<string>()->default_value("127.0.0.1"),
                 "For LAN resolving - set this to your 192.168.x or 10.x address")
            ;
            */
        // options to configure various resolvers
        po::options_description resolver("Resolver Options");
        resolver.add_options()
            ("resolver.remote_http.enabled",   po::value<string>()->default_value("no"),
                  "Enable this resolver yes/no")
            ("resolver.remote_http.ip",   po::value<string>(),
                  "IP to a remote Playdar install")
            ("resolver.remote_http.port",   po::value<int>(&opt),
                  "Port to a remote Playdar install (HTTP API)")
            
            ("resolver.lan_udp.enabled",   po::value<string>()->default_value("no"),
                  "Enable this resolver yes/no")
            ("resolver.lan_udp.multicast",   po::value<string>()->default_value("239.255.0.1"),
                  "Multicast IP")
            ("resolver.lan_udp.port",   po::value<int>(&opt)->default_value(8888),
                  "UDP port to bind to")
            
            ("resolver.darknet.enabled",   po::value<string>()->default_value("no"),
                  "Enable this resolver yes/no")
            ("resolver.darknet.port",   po::value<int>(&opt)->default_value(9999),
                  "TCP port for darknet resolver")
            ("resolver.darknet.remote_ip",   po::value<string>()->default_value(""),
                  "Remote peer IP address")
            ("resolver.darknet.remote_port",   po::value<int>(&opt)->default_value(9999),
                  "Remote peer port number")
            ;
            
        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(resolver);

        po::options_description config_file_options;
        config_file_options.add(config).add(resolver);

        po::options_description visible("Playdar configuration");
        visible.add(generic).add(config);
/*
        po::positional_options_description p;
        p.add("input-file", -1);
*/
        po::variables_map vm;
        po::parsed_options parsedopts_cmd = 
            po::command_line_parser(ac, av).
              options(cmdline_options).allow_unregistered().run();
              
        store(parsedopts_cmd, vm);
        notify(vm);

        
        if(!vm.count("config"))
        {
            cerr << "You must use a config file." << endl;
            return 1;
        }
        string configfile = vm["config"].as<string>();
        cout << "Using config file: " << configfile << endl;
        ifstream ifs(configfile.c_str());
        if(!ifs.is_open())
        {
            cerr << "Could not open: " << configfile << endl;
            return 1;
        }
        po::parsed_options parsedopts_ini = po::parse_config_file(ifs, config_file_options, true/*allow unreg*/);
        store(parsedopts_ini , vm);
        // FIXME does allow unrecognied even work with config files? who knows.
        /*
        vector<string> unrec = collect_unrecognized(vm, po::exclude_positional);
        BOOST_FOREACH(string & s, unrec)
        {
            cout << "Unrec: " << s << endl;
        }
        */
        notify(vm);
    
        
        cout << "Parsed config options."<<endl;
        
        if (vm.count("help")) {
            cout << visible << "\n";
            return 0;
        }
        if (vm.count("version")) {
            cout << "0.1\n";
            return 0;
        }
        if( !vm.count("app.name") )
        {
            cout << visible << endl;
            return 1;
        }
    /*
        if( !vm.count("foo") )
        {
            cout << "foo detected" << endl;
            
            vector<string> to_pass_further = collect_unrecognized(parsedopts_ini.options, po::exclude_positional);
            
            cout << "collected unrec: " << to_pass_further.size() << endl;
            BOOST_FOREACH(string & k, to_pass_further)
            {
                cout <<"opt: "<< k << endl;
            }
            //cout << "Foo: " << vm["foo"].as<string>() << endl;
            return 1;
        }
        
*/

        MyApplication * app = new MyApplication(vm);
        

        // start http server:
        string ip = "0.0.0.0"; //app->private_ip().to_string();
        
        boost::thread http_thread(&start_http_server, ip, app->http_port(), 1, app);
        
        cout    << endl
                << "Now check http://"<< app->private_ip().to_string() << ":" 
                << app->http_port() <<"/" << endl
                << "Or see a live demo on http://www.playdar.org/" << endl << endl;
        // Lame interactive mode for debugging:
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
        }
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

