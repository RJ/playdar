PROJECT(playdarproject)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(PLAYDAR_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PLAYDAR_PATH}/CMakeModules")

SET(SRC "${PLAYDAR_PATH}/src")
SET(DEPS "${PLAYDAR_PATH}/deps")
SET(RESOLVERS_DIR "${PLAYDAR_PATH}/resolvers")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PLAYDAR_PATH}/bin")

# Don't really do 'make install' properly yet:
SET(CMAKE_INSTALL_PREFIX "/usr/local")

# on linux x86-64 set this to OFF if you are having (-fPIC...) build problems.
# then: cd build/ && rm -rf CMakeCache.txt CMakeFiles/ cmake_install.cmake resolvers/
# then: cmake .. ; make
# alternatively, recompile boost with -fPIC like so:
# bjam --v2 cxxflags=-fPIC stage
#SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_STATIC_LIBS OFF)

SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options date_time)
FIND_PACKAGE(Taglib 1.5.0 REQUIRED)
FIND_PACKAGE(Sqlite3 REQUIRED)
FIND_PACKAGE(CURL REQUIRED)
INCLUDE_DIRECTORIES(
                    ${PLAYDAR_PATH}/includes  # playdar includes
                    ${Boost_INCLUDE_DIR}
                    ${SQLITE3_INCLUDE_DIR}
                    ${TAGLIB_INCLUDES}
                    ${CURL_INCLUDE_DIR}
                    /usr/local/include
                    ${DEPS}/moost_http/include     # httpd server component, GPL, forked from code from fawx.com
                    ${DEPS}/sqlite3pp-read-only    # cpp wrapper for sqlite api, patched for 64bit compile fix
                    ${DEPS}/json_spirit_v3.00      # json parser, using boost spirit, patched for better utf8
                    ${DEPS}/pdl-0.3.0/include      # x-platform dynamic loading of libraries, minor fixes applied
                    ${DEPS}/boost.process          # (unofficial) spawning resolver scripts
                    ${DEPS}/boost.uuid             # (unofficial) UUID generation library
                   )

# Always check local/lib first:
LINK_DIRECTORIES(   /usr/local/lib 
                ) 

SET(CMAKE_VERBOSE_MAKEFILE ON)
INCLUDE(InstallRequiredSystemLibraries)

#add definitions, compiler switches, etc.
ADD_DEFINITIONS(-DBOOST_SPIRIT_THREADSAFE)

# PDL doesn't know apple mac = posix, this tells it:
IF(APPLE)
    ADD_DEFINITIONS(-Dunix)
ENDIF(APPLE)

IF(WIN32)
	ADD_DEFINITIONS(/DNOMINMAX)
	ADD_DEFINITIONS(/DWIN32_LEAN_AND_MEAN)
ENDIF(WIN32)

IF(NOT WIN32)
    # ADD_DEFINITIONS(-ggdb)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-g )
    ADD_DEFINITIONS(-fno-operator-names)
    ADD_DEFINITIONS(-fPIC)
ENDIF(NOT WIN32)

                     
ADD_EXECUTABLE( playdar
                ${SRC}/application.cpp
                ${SRC}/resolver.cpp
                ${SRC}/rs_script.cpp
                
                ${SRC}/utils/uuid.cpp
                ${SRC}/utils/levenshtein.cpp

                ${SRC}/playdar_request_handler.cpp
                ${SRC}/playdar_request.cpp
                ${SRC}/main.cpp
                ${SRC}/auth.cpp

                # json parser:
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp  
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_value.cpp  
                ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp
                
                # Portable Dynamic Loader:
                ${DEPS}/pdl-0.3.0/src/DynamicLibrary.cpp
                ${DEPS}/pdl-0.3.0/src/DynamicLibraryManager.cpp
                ${DEPS}/pdl-0.3.0/src/DynamicLoader.cpp
                ${DEPS}/pdl-0.3.0/src/LoaderException.cpp
            
                # Wrapper for sqlite API
                ${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp
                
                # moost httpd:
                ${DEPS}/moost_http/src/http/filesystem_request_handler.cpp
                ${DEPS}/moost_http/src/http/mime_types.cpp
                ${DEPS}/moost_http/src/http/reply.cpp
                ${DEPS}/moost_http/src/http/request_parser.cpp
              )

TARGET_LINK_LIBRARIES( playdar 
                       ${CURL_LIBRARIES}      # MIT/X derivate
                       ${SQLITE3_LIBRARIES}   # public domain
                       ${TAGLIB_LIBRARIES}    # LGPL/MPL
                       ${SQLITE3_LIBRARIES}   # public domain
					   ${Boost_LIBRARIES}	  # Boost license
                     )

INSTALL(TARGETS playdar RUNTIME DESTINATION bin)

#
# Resolver Plugins
#
SET(LIBRARY_OUTPUT_PATH "${PLAYDAR_PATH}/plugins")

IF(APPLE)
    SET( PLAYDAR_PLUGIN_LDFLAGS -undefined\ dynamic_lookup )
ENDIF(APPLE)

FILE( GLOB PLUGINDIRS "${RESOLVERS_DIR}/*" )
FOREACH( PLUGDIR ${PLUGINDIRS} )
    IF( IS_DIRECTORY ${PLUGDIR} )
        MESSAGE( STATUS "Adding plugin directory: ${PLUGDIR}" )
        ADD_SUBDIRECTORY( ${PLUGDIR} )
    ENDIF( IS_DIRECTORY ${PLUGDIR} )
ENDFOREACH( PLUGDIR )
