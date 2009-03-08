PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(PLAYDAR_PATH "../../..")
SET(CMAKE_INSTALL_PREFIX "${PLAYDAR_PATH}/plugins")

SET(CMAKE_MODULE_PATH "${PLAYDAR_PATH}/CMakeModules")

FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options)
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

#add definitions, compiler switches, etc.
ADD_DEFINITIONS(-Wall -ggdb) 

ADD_LIBRARY(    lan_udp.resolver SHARED
                lan_udp.cpp
                ${PLAYDAR_PATH}/src/application/application.cpp
                ${PLAYDAR_PATH}/src/resolvers/playable_item.cpp
            )

TARGET_LINK_LIBRARIES( lan_udp.resolver
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_REGEX_LIBRARY} # Boost license
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_FILESYSTEM_LIBRARY} 
                       ${Boost_THREAD_LIBRARY}
                       ${Boost_PROGRAM_OPTIONS_LIBRARY}
                     )

INSTALL(TARGETS lan_udp.resolver LIBRARY DESTINATION .)
