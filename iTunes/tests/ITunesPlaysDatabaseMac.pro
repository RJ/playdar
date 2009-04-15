CONFIG += qtestlib
TEMPLATE = app
TARGET = 
DEPENDPATH += .

QT -= gui
QT += sql

BIN_DIR = bin
BUILD_DIR = build
DESTDIR = bin

OBJECTS_DIR = $$BUILD_DIR
MOC_DIR = $$BUILD_DIR

mac:CONFIG -= app_bundle

# Input
HEADERS += ../ITunesPlaysDatabaseMac.h \
           ../common/ITunesTrack.h     \
           ../Log.h

SOURCES += TestITunesPlaysDatabaseMac.cpp    \
           ../ITunesPlaysDatabaseCommon.cpp  \
           ../ITunesPlaysDatabaseMac.cpp     \
           ../Moose.cpp                      \
           ../common/ITunesTrack.cpp         \
           ../Log.cpp
           
LIBS += -lsqlite3

INCLUDEPATH += ..
