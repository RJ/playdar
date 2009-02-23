PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED( VERSION 2.6 FATAL_ERROR )

SET(CMAKE_VERBOSE_MAKEFILE ON)
FIND_PACKAGE( Boost 1.35 COMPONENTS filesystem regex)
INCLUDE(InstallRequiredSystemLibraries)

# binaries get installed here
SET(CMAKE_INSTALL_PREFIX "/usr/local")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

#add definitions, compiler switches, etc.
ADD_DEFINITIONS(-Wall -O2) 

#May be necessary on mac?
#ADD_DEFINITIONS(-D_POSIX_C_SOURCE -D_DARWIN_C_SOURCE)

# If you enable this, you don't need the uuid lib
# it *is* in mac ports, and debian apt tho.
# the visual studio project should build it too.
# only use out of desparation:
#ADD_DEFINITIONS( -DLAME_UUIDS_PLEASE ) 

LINK_DIRECTORIES(/usr/local/lib 
                 /usr/lib
                 /opt/local/lib)

INCLUDE_DIRECTORIES(    
                        src 
                        deps/moost_http/include     # our httpd server, from erikf's moost
                        deps/sqlite3pp-read-only    # cpp wrapper for sqlite api
                        deps/json_spirit_v3.00      # json parser, using boost spirit
                        deps/boost.process          # x-platform spawn processes and manage stdin/out

            			/usr/local/include 
                        /usr/local/include/taglib 
                        /usr/include 
                        /usr/include/taglib
            			/opt/local/include 
                        /opt/local/include/taglib 
                    )


ADD_EXECUTABLE( scanner # EXCLUDE_FROM_ALL
                src/scanner/scanner.cpp
                src/library/library.cpp
                deps/sqlite3pp-read-only/sqlite3pp.cpp
              )

TARGET_LINK_LIBRARIES(  scanner 
                        sqlite3 
                        tag 
                        boost_filesystem
                        boost_system
                        boost_thread
                        boost_regex)


ADD_EXECUTABLE( playdar
                src/application/application.cpp
                src/resolvers/playable_item.cpp
                src/resolvers/resolver.cpp
                src/resolvers/resolver_service.cpp
                src/resolvers/rs_local_library.cpp
                src/resolvers/rs_lan_udp.cpp
                src/resolvers/rs_http_gateway_script.cpp
                src/library/library.cpp
                src/playdar/playdar_request_handler.cpp
                deps/sqlite3pp-read-only/sqlite3pp.cpp
                deps/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp  
                deps/json_spirit_v3.00/json_spirit/json_spirit_value.cpp  
                deps/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp
                src/playdar/main.cpp
                # moost http:
                deps/moost_http/src/http/filesystem_request_handler.cpp
                deps/moost_http/src/http/mime_types.cpp
                deps/moost_http/src/http/reply.cpp
                deps/moost_http/src/http/request_parser.cpp
              )

TARGET_LINK_LIBRARIES(  playdar 
                        sqlite3             # public domain
                        tag                 # taglib is LGPL / MPL
                        uriparser           # BSD
                        ossp-uuid           # BSD
                        # boost libs:       # BOOST LICENSE
                        boost_filesystem
                        boost_system
                        boost_thread
                        boost_regex
                        boost_program_options
                     )

#cui18n icuuc icudata icuio #TODO ;)


INSTALL(TARGETS scanner RUNTIME DESTINATION bin)
INSTALL(TARGETS playdar  RUNTIME DESTINATION bin)




#
# CPack stuff follows, this isn't really working properly yet
#

SET(CPACK_GENERATOR "DEB") # "DEB;TGZ;TBZ"...;TGZ;TZ")

SET(CPACK_PACKAGE_VERSION "0.1.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

SET(CPACK_DEBIAN_PACKAGE_NAME "playdar")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "sqlite3 (>=3), libtag1-dev (>=1.5), liburiparser1 (>=0.6), liburiparser-dev (>=0.6), libossp-uuid-dev (>=1.5), libossp-uuid15 (>=1.5)")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "rj@playdar.org")
SET(CPACK_DEBIAN_PACKAGE_SECTION "utils")

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Playdar - the music content resolver service")

SET(CPACK_PACKAGE_VENDOR "www.playdar.org")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "LICENSE.txt")
SET(CPACK_RESOURCE_FILE_WELCOME "README.txt")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Playdar ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "playdar-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}-${CMake_VERSION_PATCH}")
SET(CPACK_SYSTEM_NAME "linux-i686")

IF(UNIX)
    SET(CPACK_STRIP_FILES "bin/playdar;bin/scanner")
    SET(CPACK_SOURCE_STRIP_FILES "")
    SET(CPACK_PACKAGE_EXECUTABLES "playdar" "scanner")
ENDIF(UNIX)
INCLUDE(CPack)
