PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(PLAYDAR_PATH "../../..")
SET(CMAKE_INSTALL_PREFIX "${PLAYDAR_PATH}/plugins")
SET(CMAKE_MODULE_PATH "${PLAYDAR_PATH}/CMakeModules")

FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS system thread)
FIND_PACKAGE(OsspUuid)

INCLUDE_DIRECTORIES(
                    ${OSSPUUID_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    /usr/local/include
                    /usr/include
                    ${PLAYDAR_PATH}/deps/sqlite3pp-read-only
                    ${PLAYDAR_PATH}/deps/json_spirit_v3.00
                    ${PLAYDAR_PATH}/deps/pdl-0.3.0/include
                    ${PLAYDAR_PATH}/src
                   )

SET(CMAKE_VERBOSE_MAKEFILE ON)
INCLUDE(InstallRequiredSystemLibraries)

ADD_DEFINITIONS(-Wall -ggdb) 

ADD_LIBRARY(    lan_udp SHARED
                lan_udp.cpp
                ${PLAYDAR_PATH}/src/application/application.cpp
                ${PLAYDAR_PATH}/src/resolvers/playable_item.cpp
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  lan_udp PROPERTIES 
                        PREFIX "" 
                        SUFFIX ".resolver")

TARGET_LINK_LIBRARIES( lan_udp
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                     )

INSTALL(TARGETS lan_udp LIBRARY DESTINATION .)
