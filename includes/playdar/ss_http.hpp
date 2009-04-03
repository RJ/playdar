#ifndef __HTTP_STRAT_H__
#define __HTTP_STRAT_H__
#include <boost/asio.hpp>
#include "playdar/streaming_strategy.h"
#include <boost/algorithm/string.hpp>

#include "playdar/utils/base64.h"

using namespace boost::asio::ip;
/*
    Consider this a nasty hack until I find a decent c++ http library

    HTTP Basic Auth:
    Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
    base64(username:password)

*/
class HTTPStreamingStrategy : public StreamingStrategy
{
public:

    HTTPStreamingStrategy(string url)
    {   
        reset();
        if(!parse_url(url)) throw;
    }
    
    HTTPStreamingStrategy(string host, unsigned short port, string url)
        : m_host(host), m_url(url), m_port(port)
    {
        reset();
    }
    
    bool parse_url(string url)
    {
        boost::regex re("http://(.*@)?(.[^/^:]*)\\:?([0-9]*)/(.*)");
        boost::cmatch matches;
        if(boost::regex_match(url.c_str(), matches, re))
        {
            m_host = matches[2];
            unsigned int colpos;
            // does it start with user:pass@ (ie, http basic auth)
            if( matches[1] != "" )
            {
                string authbit = matches[1]; // user:pass@
                string username, password;
                colpos = authbit.find(':');
                if(colpos == authbit.npos)
                {
                    username = authbit.substr(0, authbit.length()-1);
                    password = "";
                }
                else
                {
                    username = authbit.substr(0, colpos);
                    password = authbit.substr(colpos+1, authbit.length()-colpos-2);
                }
                // construct basic auth header
                m_authheader = "Authorization: Basic " +
                                playdar::utils::base64_encode
                                       (username + ":" + password);
            }
            m_port = matches[3]==""
                ? 80 // default when port not specified
                : boost::lexical_cast<int>(matches[3]);
            m_url = "/" + matches[4];
            cout << "URL: " << m_host << ":" << m_port << " " << m_url << endl;
            return true;
        }
        else
        {
            cerr << "Invalid URL: " << url << endl;
            return false;
        }
    }


    
    ~HTTPStreamingStrategy(){  }
    

    int read_bytes(char * buf, int size)
    {
        if(!m_connected) do_connect();
        if(!m_connected)
        {
            cout << "Failed to fetch over http" << endl;
            reset();
            return 0;
        }
        int p = m_partial.length();
        if(p)
        {
            if(p <= size)
            {
                memcpy(buf, m_partial.c_str(), p);
                m_partial="";
                m_bytesreceived+=p;
                return p;
            }else{
                memcpy(buf, m_partial.c_str(), size);
                m_partial = m_partial.substr(size-1, string::npos);
                m_bytesreceived+=size;
                return size;
            }
        }
        boost::system::error_code error;
        size_t len = m_socket->read_some(boost::asio::buffer(buf, size), error);

        if (error == boost::asio::error::eof)
        {
            m_bytesreceived+=len;
            cout << "Clean shutdown, bytes recvd: " << m_bytesreceived << endl;
            reset();
            return len; // Connection closed cleanly by peer.
        }
        else if (error)
        {
            m_bytesreceived+=len;
            cout << "Unclean shutdown, bytes recvd: " << m_bytesreceived << endl;
            reset();
            throw boost::system::system_error(error); // Some other error.
        }   
        return len;
    }

    
    string debug()
    { 
        ostringstream s;
        s<< "HTTPStreamingStrategy( host='"<<m_host<<"' port='"<<m_port<<"' url='"<<m_url<<"')";
        return s.str();
    }
    
    void reset()
    {
        m_socket.reset();
        m_partial="";
        m_connected=false;
        m_bytesreceived = 0;
        m_numredirects = 0;
    }
    
private:



    void do_connect()
    {
        cout << debug() << endl; 

        tcp::resolver resolver(m_io_service);
        tcp::endpoint ep;
        boost::regex re("[0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*");
        boost::cmatch matches;

        // if it's a numeric IP, connect, otherwise resolve name first:
        if(boost::regex_match(m_host.c_str(), matches, re))
        {
            boost::asio::ip::address_v4 ip = boost::asio::ip::address_v4::from_string(m_host);
            ep = tcp::endpoint(ip, m_port);
        }
        else
        {
            tcp::resolver::query query(m_host, "http");
            tcp::resolver::iterator resolve_iter = resolver.resolve(query);
            tcp::resolver::iterator end_marker;
            while (resolve_iter != end_marker)
            { 
                ep = *resolve_iter++;
            }
            ep.port(m_port);
        }
        m_socket = boost::shared_ptr<boost::asio::ip::tcp::socket>
                                    (new tcp::socket(m_io_service));

        boost::system::error_code error = boost::asio::error::host_not_found;
        m_socket->connect(ep, error);
        if (error) throw boost::system::system_error(error);

        boost::system::error_code werror;

        ostringstream rs;
        rs  << "GET " << m_url << " HTTP/1.0\r\n"
            << "Host: " <<m_host<<":"<<m_port<<"\r\n"
            << "Accept: */*\r\n"
            << "Connection: close\r\n";
        if(m_authheader.length() && m_numredirects == 0)
        {
            rs << m_authheader << "\r\n";
        }
        rs  << "\r\n";
        boost::asio::write(*m_socket, boost::asio::buffer(rs.str()), boost::asio::transfer_all(), werror);
        if(werror)
        {
            cerr << "Error making request" << endl;
            throw boost::system::system_error(werror);
            return;
        }
        boost::asio::streambuf response;
    
        boost::asio::read_until(*m_socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            std::cout << "Invalid response\n";
            return;
        }
        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(*m_socket, response, "\r\n\r\n");

        cout << "Status code: " << status_code << endl
             << "Status message: " << status_message << endl;
        // Process the response headers.
        map<string,string> headers;
        std::string line;
        while (std::getline(response_stream, line) && line!= "\r")
        {
            size_t offset = line.find(':');
            string key = boost::to_lower_copy(line.substr(0,offset));
            string value = boost::trim_copy(line.substr(offset+1));
            headers[key]=value;
            cout << "'"<< key <<"' = '" << value << "'" << "\n";
        }
        
        // was it a redirect?
        if(status_code == 301 || status_code == 302)
        {
            if(headers.find("location")==headers.end())
            {
                cerr << "HTTP redirect given with no Location header!" 
                     << endl;
                return;
            }
            if(m_numredirects++==3)
            {
                cerr << "HTTP Redirect limit of 3 reached. Failed." 
                     << endl;
                return;
            }
            
            string newurl = headers["location"];
            if(newurl.at(0)=='/')
            {
                // if it starts with / it's on same domain, rebuild full url
                ostringstream oss;
                oss << "http://" << m_host << ":" << m_port 
                    << headers["location"];
                newurl = oss.str();
            }
            cout << "Following HTTP redirect to: " << newurl << endl;
            // recurse - this resets connection and tries again.
            parse_url(newurl);
            do_connect();
            return;
        }
        
        // hack for stupid 404 pages that use the 200 code:
        if(headers.find("content-type")!=headers.end() && status_code==200)
        {
            if(boost::to_lower_copy(headers["content-type"]) == "text/html")
            {
                cout << "Page claims 200 OK but is HTML -> pretend it's 404." 
                     << endl;
                status_code = 404;
            }
        }
        
        
        if (status_code != 200)
        {
            cerr << "HTTP status code " << status_code << " was not OK\n";
            return;
        }
        
        // save whatever content we already have.
        if (response.size() > 0)
        {
            ostringstream part;
            part << &response;
            m_partial = string(part.str());
            //cout << "partial size: " << m_partial.length() << " partial='"<< part.str() <<"'" << endl;
        }
        m_connected = true;
    }
    
    boost::asio::io_service m_io_service;
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    bool m_connected;
    string m_partial;
    string m_host;
    string m_url;
    unsigned short m_port;
    int m_numredirects;
    size_t m_bytesreceived;
    string m_authheader;
};

#endif
