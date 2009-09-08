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
#include "playdar/application.h"
#include "playdar/resolver.h"
#include "playdar/logger.h"
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <playdar/playdar_request_handler.h>

using namespace std;

namespace playdar {

MyApplication::MyApplication(Config c)
    : m_config(c)
{
    string db_path = conf()->get<string>("db", "");
    m_resolver  = new Resolver(this);
}

MyApplication::~MyApplication()
{
    log::info() << "DTOR MyApplication" << endl;
    log::info() << "Stopping resolver..." << endl;
    delete(m_resolver);
}
     
void
MyApplication::shutdown(int sig)
{

    cout << "Stopping http(" << sig << ")..." << endl;
    if(m_stop_http) m_stop_http();
}

Resolver *
MyApplication::resolver()
{
    return m_resolver;
}

} // ns
