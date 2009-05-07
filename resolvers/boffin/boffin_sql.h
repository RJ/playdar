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
/*
    This file was automatically generated from resolvers/boffin/boffin.sql on Tue Apr 28 16:44:27 BST 2009.
*/
namespace playdar {

static const char * boffin_schema_sql = 
"CREATE TABLE IF NOT EXISTS tag ("
"	name TEXT NOT NULL"
");"
"CREATE UNIQUE INDEX tag_name_idx ON tag(name);"
"CREATE TABLE IF NOT EXISTS track_tag ("
"	track INTEGER NOT NULL,"
"	tag INTEGER NOT NULL,"
"	weight FLOAT NOT NULL"
");"
"CREATE INDEX track_tag_track_idx ON track_tag(track);"
"CREATE INDEX track_tag_tag_idx ON track_tag(tag);"
"CREATE TABLE IF NOT EXISTS boffin_system ("
"    key TEXT NOT NULL PRIMARY KEY,"
"    value TEXT NOT NULL DEFAULT ''"
");"
"INSERT INTO boffin_system(key,value) VALUES('schema_version', '1');"
    ;

const char * get_boffin_sql()
{
    return boffin_schema_sql;
}

} // namespace

