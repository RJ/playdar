PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_MODULE_PATH "CMakeModules")

FIND_PACKAGE(Taglib 1.5.0 REQUIRED)
FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options)
FIND_PACKAGE(OsspUuid)
#FIND_PACKAGE(Uriparser 0.7.0)
FIND_PACKAGE(Sqlite3)

INCLUDE_DIRECTORIES(#${URIPARSER_INCLUDE_DIR}
                    ${OSSPUUID_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ${SQLITE3_INCLUDE_DIR}
                    ${TAGLIB_INCLUDES}
                    /usr/local/include
                    src
                    deps/moost_http/include     # our httpd server, from erikf's moost
                    deps/sqlite3pp-read-only    # cpp wrapper for sqlite api
                    deps/json_spirit_v3.00      # json parser, using boost spirit
                    deps/pdl-0.3.0/include      # x-platform dynamic loading of libraries
                   )

LINK_DIRECTORIES(/usr/local/lib) # for uriparser


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
                src/library/library.cpp
                src/playdar/playdar_request_handler.cpp
                src/playdar/main.cpp

                # sqlite wrapper:
                deps/sqlite3pp-read-only/sqlite3pp.cpp

                # json parser:
                deps/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp  
                deps/json_spirit_v3.00/json_spirit/json_spirit_value.cpp  
                deps/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp

                # moost http:
                deps/moost_http/src/http/filesystem_request_handler.cpp
                deps/moost_http/src/http/mime_types.cpp
                deps/moost_http/src/http/reply.cpp
                deps/moost_http/src/http/request_parser.cpp

                # Portable Dynamic Loader:
                deps/pdl-0.3.0/src/DynamicLibrary.cpp
                deps/pdl-0.3.0/src/DynamicLibraryManager.cpp
                deps/pdl-0.3.0/src/DynamicLoader.cpp
                deps/pdl-0.3.0/src/LoaderException.cpp
              )

TARGET_LINK_LIBRARIES( playdar 
                       ${TAGLIB_LIBRARIES}    # LGPL/MPL
                       ${SQLITE3_LIBRARIES}   # public domain
                       #${URIPARSER_LIBRARIES} # BSD
                       uriparser
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_REGEX_LIBRARY} # Boost license
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_FILESYSTEM_LIBRARY} 
                       ${Boost_THREAD_LIBRARY}
                       ${Boost_PROGRAM_OPTIONS_LIBRARY}
                     )

INSTALL(TARGETS scanner RUNTIME DESTINATION bin)
INSTALL(TARGETS playdar RUNTIME DESTINATION bin)

