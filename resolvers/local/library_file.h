#ifndef LIBRARY_FILE_H
#define LIBRARY_FILE_H

#include <string>

namespace playdar {

// a "File" from the playdar local library db
class LibraryFile
{
public:
    std::string url;
    int size;
    std::string mimetype;
    int duration;
    int bitrate;
    int piartid;
    int pialbid;
    int pitrkid;
};

}

#endif
