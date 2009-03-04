#ifndef __APPSETTINGS_H__
#define __APPSETTINGS_H__


class SettingsManager
{
    typedef struct {
        bool multi;
        union {
            string single;
            vector<string> multi;
        } val;
    } setting;

    SettingsManager()
    {
    }
    
    template <typename T>
    void set(string k, T v)
    {
        setting s;
        s.multi=false;
        s.val.single = boost::lexical_cast<string>(v);
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
    
    bool load_config(string filename)
    {
        ifstream ifs;
        ifs.open(filename.c_str, ifstream::in);
        if(ifs.fail()) return false;
        string line;
        while (getline(ifs,line))
        {
            cout << "Parsing: " << line << endl;
        }
    }

    private:
        map< string, setting > m_settings;
};


#endif
