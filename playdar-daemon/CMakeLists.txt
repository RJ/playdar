PROJECT(playdar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
SET(PLAYDAR_PATH ${CMAKE_CURRENT_SOURCE_DIR})

SET(SRC "${PLAYDAR_PATH}/src")
SET(DEPS "${PLAYDAR_PATH}/deps")
SET(RESOLVERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resolvers")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PLAYDAR_PATH}/bin")

# Don't really do 'make install' properly yet:
SET(CMAKE_INSTALL_PREFIX "/usr/local")

FIND_PACKAGE(Taglib 1.5.0 REQUIRED)
FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options)
FIND_PACKAGE(OsspUuid REQUIRED)
FIND_PACKAGE(Uriparser 0.7.0 REQUIRED)
FIND_PACKAGE(Sqlite3 REQUIRED)

INCLUDE_DIRECTORIES(
                    ${CMAKE_CURRENT_SOURCE_DIR}/includes  # playdar includes
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/resolvers # only needed for some debug/bootstrap code
                    #${URIPARSER_INCLUDE_DIR}
                    ${OSSPUUID_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ${SQLITE3_INCLUDE_DIR}
                    ${TAGLIB_INCLUDES}
                    /usr/local/include
                    ${DEPS}/moost_http/include     # our httpd server, from erikf's moost
                    ${DEPS}/sqlite3pp-read-only    # cpp wrapper for sqlite api
                    ${DEPS}/json_spirit_v3.00      # json parser, using boost spirit
                    ${DEPS}/pdl-0.3.0/include      # x-platform dynamic loading of libraries
                   )

# Always check local/lib first:
LINK_DIRECTORIES(/usr/local/lib) 

SET(CMAKE_VERBOSE_MAKEFILE ON)
INCLUDE(InstallRequiredSystemLibraries)

#add definitions, compiler switches, etc.
ADD_DEFINITIONS(-Wall -ggdb) 

# PDL doesn't know apple mac = posix, this tells it:
IF(APPLE)
    ADD_DEFINITIONS(-Dunix)
ENDIF(APPLE)

ADD_EXECUTABLE(scanner # EXCLUDE_FROM_ALL
               ${SRC}/scanner/scanner.cpp
               ${SRC}/library.cpp
               ${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp
              )
TARGET_LINK_LIBRARIES(scanner 
                      ${SQLITE3_LIBRARIES}
                      ${Boost_FILESYSTEM_LIBRARY}
                      ${Boost_SYSTEM_LIBRARY}
                      ${Boost_REGEX_LIBRARY}
                      ${Boost_THREAD_LIBRARY}
                      ${TAGLIB_LIBRARIES})

ADD_EXECUTABLE( playdar
                ${SRC}/application.cpp
                ${SRC}/resolver.cpp
                ${SRC}/resolver_service.cpp
                ${SRC}/rs_local_library.cpp
                ${SRC}/library.cpp
                ${SRC}/playdar/playdar_request_handler.cpp
                ${SRC}/playdar/main.cpp

                # sqlite wrapper:
                ${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp

                # json parser:
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp  
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_value.cpp  
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp

                # moost http:
                ${DEPS}/moost_http/src/http/filesystem_request_handler.cpp
                ${DEPS}/moost_http/src/http/mime_types.cpp
                ${DEPS}/moost_http/src/http/reply.cpp
                ${DEPS}/moost_http/src/http/request_parser.cpp

                # Portable Dynamic Loader:
                ${DEPS}/pdl-0.3.0/src/DynamicLibrary.cpp
                ${DEPS}/pdl-0.3.0/src/DynamicLibraryManager.cpp
                ${DEPS}/pdl-0.3.0/src/DynamicLoader.cpp
                ${DEPS}/pdl-0.3.0/src/LoaderException.cpp
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

#
# Resolver Plugins
#
SET(LIBRARY_OUTPUT_PATH "${PLAYDAR_PATH}/plugins")

FILE(GLOB PLUGINDIRS "${RESOLVERS_DIR}/*")
FOREACH(PLUGDIR ${PLUGINDIRS})
    IF(IS_DIRECTORY ${PLUGDIR})
        MESSAGE( STATUS "Adding plugin directory: ${PLUGDIR}" )
        ADD_SUBDIRECTORY(${PLUGDIR})
    ENDIF(IS_DIRECTORY ${PLUGDIR})
ENDFOREACH(PLUGDIR)
