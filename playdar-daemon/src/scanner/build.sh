 g++    -ltag -lsqlite3 -lboost_filesystem \
        -fpermissive \
        -o scanner \
        -I../../deps/sqlite3pp-read-only/ \
        -I/usr/include \
        -I/usr/include/taglib \
        -I../library \
        ../../deps/sqlite3pp-read-only/sqlite3pp.cpp \
        ../library/library.cpp \
        scanner.cpp
