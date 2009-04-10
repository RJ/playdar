#ifndef __HTTP_STRAT_H__
#define __HTTP_STRAT_H__

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "playdar/utils/base64.h"
#include "playdar/streaming_strategy.h"

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

    HTTPStreamingStrategy(std::string url)
    {   
        reset();
        if(!parse_url(url)) throw;
    }
    
    HTTPStreamingStrategy(std::string host, unsigned short port, std::string url)
        : m_host(host), m_url(url), m_port(port)
    {
        reset();
    }
    
    bool parse_url(std::string url)
    {
        boost::regex re("http://(.*@)?(.[^/^:]*)\\:?([0-9]*)/(.*)");
        boost::cmatch matches;
        if(boost::regex_match(url.c_str(), matches, re))
        {
            m_host = matches[2];
            size_t colpos;
            // does it start with user:pass@ (ie, http basic auth)
            if( matches[1] != "" )
            {
                std::string authbit = matches[1]; // user:pass@
                std::string username, password;
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
                std::string h = "Authorization: Basic " +
                                playdar::utils::base64_encode
                                       (username + ":" + password);
                extra_headers().push_back(h);
            }
            m_port = matches[3]==""
                ? 80 // default when port not specified
                : boost::lexical_cast<int>(matches[3]);
            m_url = "/" + matches[4];
            std::cout << "URL: " << m_host << ":" << m_port << " " << m_url << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Invalid URL: " << url << std::endl;
            return false;
        }
    }

    ~HTTPStreamingStrategy(){  }
    
    std::vector<std::string>& extra_headers() { return m_extra_headers; }

    // this should return size_t, really (as well as having size of the same type).. 
    int read_bytes(char * buf, int size)
    {
        if(!m_connected) do_connect();
        if(!m_connected)
        {
            std::cout << "Failed to fetch over http" << std::endl;
            reset();
            return 0;
        }

        int p = static_cast<int>( m_partial.length() );
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
                m_partial = m_partial.substr(size-1, std::string::npos);
                m_bytesreceived+=size;
                return size;
            }
        }
        boost::system::error_code error;
        size_t len = m_socket->read_some(boost::asio::buffer(buf, size), error);

        if (error == boost::asio::error::eof)
        {
            m_bytesreceived+=len;
            std::cout << "Clean shutdown, bytes recvd: " << m_bytesreceived << std::endl;
            reset();
            return static_cast<int>(len); // Connection closed cleanly by peer.
        }
        else if (error)
        {
            m_bytesreceived+=len;
            std::cout << "Unclean shutdown, bytes recvd: " << m_bytesreceived << std::endl;
            reset();
            throw boost::system::system_error(error); // Some other error.
        }   
        return static_cast<int>(len);
    }

    
    std::string debug()
    { 
        std::ostringstream s;
        s << "HTTPStreamingStrategy( host='" << m_host << "' port='" << m_port << "' url='" << m_url << "')";
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
        std::cout << debug() << std::endl; 

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

        std::ostringstream rs;
        rs  << "GET " << m_url << " HTTP/1.0\r\n"
            << "Host: " << m_host <<":"<< m_port <<"\r\n"
            << "Accept: */*\r\n"
            << "Connection: close\r\n";
        // don't send extra headers if we are redirected, to avoid leaking
        // auth details or cookies to other domains.
        if(m_numredirects==0)
        {
            BOOST_FOREACH( std::string& extra_header, m_extra_headers )
            {
                rs << extra_header << "\r\n";
            }
        }
        rs  << "\r\n";
        
        
        boost::asio::write(*m_socket, boost::asio::buffer(rs.str()), boost::asio::transfer_all(), werror);
        if(werror)
        {
            std::cerr << "Error making request" << std::endl;
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

        std::cout << "Status code: " << status_code << std::endl
                  << "Status message: " << status_message << std::endl;
        // Process the response headers.
        std::map<std::string, std::string> headers;
        std::string line;
        while (std::getline(response_stream, line) && line!= "\r")
        {
            size_t offset = line.find(':');
            std::string key = boost::to_lower_copy(line.substr(0,offset));
            std::string value = boost::trim_copy(line.substr(offset+1));
            headers[key]=value;
            std::cout << "'"<< key <<"' = '" << value << "'" << "\n";
        }
        
        // was it a redirect?
        if(status_code == 301 || status_code == 302)
        {
            if(headers.find("location")==headers.end())
            {
                std::cerr << "HTTP redirect given with no Location header!" 
                          << std::endl;
                return;
            }
            if(m_numredirects++==3)
            {
                std::cerr << "HTTP Redirect limit of 3 reached. Failed." 
                          << std::endl;
                return;
            }
            
            std::string newurl = headers["location"];
            if(newurl.at(0)=='/')
            {
                // if it starts with / it's on same domain, rebuild full url
                std::ostringstream oss;
                oss << "http://" << m_host << ":" << m_port 
                    << headers["location"];
                newurl = oss.str();
            }
            std::cout << "Following HTTP redirect to: " << newurl << std::endl;
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
                std::cout << "Page claims 200 OK but is HTML -> pretend it's 404." 
                     << std::endl;
                status_code = 404;
            }
        }
        
        
        if (status_code != 200)
        {
            std::cerr << "HTTP status code " << status_code << " was not OK\n" << std::endl;
            return;
        }
        
        // save whatever content we already have.
        if (response.size() > 0)
        {
            std::ostringstream part;
            part << &response;
            m_partial = std::string(part.str());
            //cout << "partial size: " << m_partial.length() << " partial='"<< part.str() <<"'" << std::endl;
        }
        m_connected = true;
    }
    
    boost::asio::io_service m_io_service;
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    bool m_connected;
    std::string m_partial;
    std::string m_host;
    std::string m_url;
    unsigned short m_port;
    int m_numredirects;
    size_t m_bytesreceived;
    std::vector<std::string> m_extra_headers;
};

#endif
