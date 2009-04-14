/***************************************************************************
 *   Copyright 2008-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "IPod.h"
#include <sys/stat.h>
#include <iostream>
#include <sstream>

#ifndef WIN32
    #include <dirent.h>
#endif


const COMMON_STD_STRING 
IPod::twiddlyFlags() const
{
    LFM_STRINGSTREAM ss;
    
    ss << "--device ";
    ss << device();
    
    ss << " --connection ";
    switch( m_connectionType )
    {
        case usb:
            ss << "usb";
            break;
            
        case fireWire:
            ss << "fireWire";
            break;
    }
    
    ss << " --pid " << m_pid;
    ss << " --vid " << m_vid;
    ss << " --serial " << m_serial;
    
    if( m_manualMode )
        ss << " --manual";

    return ss.str();
}


COMMON_STD_STRING
IPod::device() const
{
    #ifdef WIN32
        #define _(x) L##x
    #else
        #define _(x) x
    #endif

    switch( m_type )
    {
        case iPod:   return _( "ipod" );
        case iTouch: return _( "itouch" );
        case iPhone: return _( "iphone" );
        default:     return _( "unknown" );
    }

    #undef _
}
