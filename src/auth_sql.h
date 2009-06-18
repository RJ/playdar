/*
    This file was automatically generated from auth.sql on Thu Jun 18 17:34:23 GMTDT 2009.
*/
namespace playdar {

static const char * auth_schema_sql = 
"DROP TABLE IF EXISTS playdar_auth;"
"CREATE TABLE IF NOT EXISTS playdar_auth ("
"    token TEXT NOT NULL PRIMARY KEY,"
"    website TEXT NOT NULL,"
"    name TEXT NOT NULL,"
"    ua TEXT NOT NULL,"
"    mtime INTEGER NOT NULL,"
"    permissions TEXT NOT NULL"
");"
"CREATE TABLE IF NOT EXISTS settings ("
"    key TEXT NOT NULL PRIMARY KEY,"
"    value TEXT NOT NULL DEFAULT ''"
");"
"INSERT INTO settings(key ,value) VALUES('schema_version', '2');"
    ;

const char * get_auth_sql()
{
    return auth_schema_sql;
}

} // namespace

