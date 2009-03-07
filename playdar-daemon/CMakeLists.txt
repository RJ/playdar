PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_MODULE_PATH "CMakeModules")

FIND_PACKAGE(Taglib 1.5.0 REQUIRED)
FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options)
FIND_PACKAGE(OsspUuid)
FIND_PACKAGE(UriParser 0.7.0)
FIND_PACKAGE(Sqlite3)

INCLUDE_DIRECTORIES(${URIPARSER_INCLUDE_DIR}
                    ${OSSPUUID_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ${SQLITE3_INCLUDE_DIR}
                    ${TAGLIB_INCLUDES}
                    src
                    deps/moost_http/include     # our httpd server, from erikf's moost
                    deps/sqlite3pp-read-only    # cpp wrapper for sqlite api
                    deps/json_spirit_v3.00      # json parser, using boost spirit
                    deps/boost.process          # x-platform spawn processes and manage stdin/out
                   )

SET(CMAKE_VERBOSE_MAKEFILE ON)
INCLUDE(InstallRequiredSystemLibraries)

# binaries get installed here
SET(CMAKE_INSTALL_PREFIX "/usr/local")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

#add definitions, compiler switches, etc.
ADD_DEFINITIONS(-Wall -ggdb) 

#May be necessary on mac?
#ADD_DEFINITIONS(-D_POSIX_C_SOURCE -D_DARWIN_C_SOURCE)


###########scanner
ADD_EXECUTABLE(scanner # EXCLUDE_FROM_ALL
               src/scanner/scanner.cpp
               src/library/library.cpp
               deps/sqlite3pp-read-only/sqlite3pp.cpp
              )
TARGET_LINK_LIBRARIES(scanner 
                      ${SQLITE3_LIBRARIES}
                      ${Boost_FILESYSTEM_LIBRARY}
                      ${Boost_SYSTEM_LIBRARY}
                      ${Boost_REGEX_LIBRARY}
                      ${Boost_THREAD_LIBRARY}
                      ${TAGLIB_LIBRARIES})

###########playdar
ADD_EXECUTABLE( playdar
                src/application/application.cpp
                src/resolvers/playable_item.cpp
                src/resolvers/resolver.cpp
                src/resolvers/resolver_service.cpp
                src/resolvers/rs_local_library.cpp
                src/resolvers/rs_lan_udp.cpp
                src/resolvers/rs_http_gateway_script.cpp
                src/resolvers/darknet/rs_darknet.cpp
                src/resolvers/darknet/servent.cpp
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


TARGET_LINK_LIBRARIES( playdar 
                       ${TAGLIB_LIBRARIES}    # LGPL/MPL
                       ${SQLITE3_LIBRARIES}   # public domain
                       ${URIPARSER_LIBRARIES} # BSD
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_REGEX_LIBRARY} # Boost license
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_FILESYSTEM_LIBRARY} 
                       ${Boost_THREAD_LIBRARY}
                       ${Boost_PROGRAM_OPTIONS_LIBRARY}
                     )

#cui18n icuuc icudata icuio #TODO ;)


INSTALL(TARGETS scanner RUNTIME DESTINATION bin)
INSTALL(TARGETS playdar RUNTIME DESTINATION bin)



########### CPack
# this isn't really working properly yet

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

#IF(UNIX)
#    SET(CPACK_STRIP_FILES "bin/playdar;bin/scanner")
#    SET(CPACK_SOURCE_STRIP_FILES "")
#    SET(CPACK_PACKAGE_EXECUTABLES "playdar" "scanner")
#ENDIF(UNIX)
INCLUDE(CPack)
