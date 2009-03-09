#include <string>
#include <stdio.h>
#include <iostream>
#include "config.hpp"

using namespace std;
int main (int argc, char** argv)
{
    playdar::Config c(argv[1]);
    cout << c.str() <<endl;
    cout    << c.get<string>("name","DEF")
            << c.get<string>("plugins.blah","unknown0")  
            << c.get<string>("plugins.blah.foo","unknown1")  
            << c.get<string>("plugins.blah.foox","unknown2")  
            << c.get<string>("plugins.blahx.foo","unknown3")  
            << c.get<int>("http_port",0) 
            << endl;
    c.set<string>("name", "RJ");
    cout << c.str() << endl;
    
//    sm.set<string>("app.name", "RJ_home");
 //   sm.set<int>("app.http_port",8888);
  //  sm.set<string>("app.db","./collection.db");
   // sm.save_config();
    return 0;
}

