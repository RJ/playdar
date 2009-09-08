/* 
   Playdar - music content resolver 
   Copyright (C) 2009  Richard Jones 
   Copyright (C) 2009  Last.fm Ltd. 

   This program is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 

   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 

   You should have received a copy of the GNU General Public License 
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */
#ifndef PLAYDAR_LOG_H_
#define PLAYDAR_LOG_H_

#include <map>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>

namespace playdar {
namespace log {

typedef boost::shared_ptr<std::ofstream> stream_ptr;
template< class T > class base_log {
public:
    template<typename V> base_log& operator<<( const V& in ) {
        *m_stream << " " << in;
        return *this;
    }
   
    //Required to allow mutators to work (endl etc):
    base_log& 
    operator<<( std::ostream& ( *pf )( std::ostream& ))
    {
        pf( *(m_stream) );
        return *this;
    }
    

protected:
    stream_ptr m_stream;
    base_log( const std::string& filename = "playdar.log" ) {
        //LOCK 
        if( s_streamMap.find( filename ) == s_streamMap.end() || 
            !s_streamMap.find( filename )->second->is_open()) { 

            std::cout << "Opening file: " << filename << std::endl; 
            s_streamMap[ filename ] = stream_ptr( new std::ofstream( filename.c_str(), std::ios_base::out | std::ios_base::app ));
        } 

        m_stream = s_streamMap[ filename ]; 

        *m_stream << std::endl << T::logType() << ":"; 
        //UNLOCK
    }
    static std::map< std::string, stream_ptr > s_streamMap;
};

template<class T>
std::map< std::string, stream_ptr > base_log<T>::s_streamMap;

class warning: public base_log< warning > {
public:
    warning(): base_log<warning>(){};
    warning( const std::string& file ): base_log<warning>( file ){};
    static const char* logType(){ return "WARNING";}
};

class error: public base_log< error > {
public:
    error(): base_log<error>(){};
    error( const std::string& file ): base_log<error>( file ){};
    static const char* logType(){ return "ERROR";}

};

class info: public base_log< info > {
public:
    info(): base_log<info>(){};
    info( const std::string& file ): base_log<info>( file ){};
    static const char* logType(){ return "INFO";}

};

}} // playdar::log

#endif //PLAYDAR_LOG_H_
