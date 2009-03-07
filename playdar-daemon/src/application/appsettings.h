#ifndef __APPSETTINGS_H__
#define __APPSETTINGS_H__
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <map>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

using namespace std;

class SettingsManager
{
public:

    SettingsManager(string configfile) 
        : m_filename(configfile)
    {
        load_config();
    }
    
    template <typename T>
    void set(string k, T v)
    {
        string s = boost::lexical_cast<string>(v);
        m_settings[k]=s; 
    }
    
    template <typename T>
    T get(string k, T def)
    {
        if(m_settings.find(k)==m_settings.end())
        {
            return def;
        }
        return boost::lexical_cast<T>(m_settings[k]);
    }
    
    bool load_config()
    {
        return parse_config(m_filename, false);
    }
    
    bool save_config()
    {
        ifstream ifs;
        ofstream ofs;
        ifs.open(m_filename.c_str(), ios::in);
        string ofilename = m_filename;
        ofilename += ".new";
        ofs.open(ofilename.c_str(), ios::out);
        if(ifs.fail()) return false;
        if(ofs.fail()) return false;
        string line;
        // keep comments:
        while (getline(ifs,line))
        {
            boost::trim(line);
            if(line.length() && line.at(0)=='#')
            {
                ofs << line << endl;
            }
        }    
        typedef std::pair<string,string> pt;
        BOOST_FOREACH(pt p, m_settings)
        {
            cout << p.first << "=" << p.second << endl;
            ofs << p.first << "=" << p.second << endl;
        }
        // move .new to filename.
        boost::filesystem::path oldf(filename);
        boost::filesystem::path newf(ofilename);
        boost::filesystem::remove(oldf);
        boost::filesystem::rename(newf, oldf);
        return true;
    }
    
private:
    // category -> key -> value
    map< string, map< string, string > > m_settings;
    string m_filename;
        
    // if rewrite, will replace config with existing settings in m_settings
    bool parse_config(string filename, bool rewrite)
    {
        ifstream ifs;
        ofstream ofs;
        ifs.open(filename.c_str(), ios::in);
        string ofilename = filename;
        ofilename += ".new";
        if(rewrite)
        {
            ofs.open(ofilename.c_str(), ios::out);
            if(ofs.fail()) return false;
        }
        if(ifs.fail()) return false;
        string category;
        string line;
        while (getline(ifs,line))
        {
            boost::trim(line);
            //cout << "Parsing: " << line << endl;
            string key,value;
            for(size_t i = 0; i<line.length(); i++)
            {
                if(line.at(0)=='#') break;
                if(line.at(0)=='[')
                {
                    category = line.substr(1, line.length()-2);
                    break;
                }
                else if(key.length()==0)
                {
                    if(line.at(i)=='=')
                    {
                        key=line.substr(0,i);
                        if(i == line.length()-1)
                        {
                            value="";
                        }
                        else if(line.at(i+1)=='"' && line.at(line.length()-1)=='"')
                        {
                            value=line.substr(i+2, line.length()-i-3);
                        }
                        else
                        {
                            value=line.substr(i+1);
                        }
                        break;
                    }
                }
            }
            if(rewrite) 
            {
                string k = category;
                if(k.length()) k+=".";
                k+=key;
                if(key.length() == 0)
                {
                    // non-KV line, replicate:
                    ofs << line << endl;
                }else{
                    string newval = get<string>(k,"");
                    ofs << key << "=" << newval << endl;
                }
            }
            else if(key.length()) //update settings:
            {
                string k = category;
                if(k.length()) k+=".";
                k+=key;
                cout << "Loaded: '" << k << "' = '" << value << "'" << endl;
                m_settings[k]=value;
            }
        }
        if(rewrite)
        {
            // move .new to filename.
            boost::filesystem::path oldf(filename);
            boost::filesystem::path newf(ofilename);
            boost::filesystem::remove(oldf);
            boost::filesystem::rename(newf, oldf);
        }
    }


};



#endif
