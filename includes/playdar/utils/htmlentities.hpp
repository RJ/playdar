/*
    Playdar - music content resolver
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
#ifndef HTML_ESCAPE_HPP
#define HTML_ESCAPE_HPP

#include <string>
#include <boost/algorithm/string.hpp>

namespace playdar {
namespace utils { 

// return a string with &<>" characters replaced with html entities
std::string htmlentities(const std::string& a)
{
    std::string b(a);
    boost::replace_all(b, "&", "&amp;");
    boost::replace_all(b, "<", "&lt;");
    boost::replace_all(b, ">", "&gt;");
    boost::replace_all(b, "\"", "&quot;");
    return b;
}

}}

#endif
