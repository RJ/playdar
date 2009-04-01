PROJECT(lanresolver)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(PLAYDAR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../")
SET(CMAKE_MODULE_PATH "${PLAYDAR_PATH}/CMakeModules")
SET(DEPS "${PLAYDAR_PATH}/deps")

FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options)
FIND_PACKAGE(OsspUuid REQUIRED)

INCLUDE_DIRECTORIES(
                    ${DEPS}/json_spirit_v3.00       # json parser, using boost spirit
                    ${PLAYDAR_PATH}/includes        # playdar includes
                    ${OSSPUUID_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    /usr/local/include
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

ADD_EXECUTABLE(lanresolver
               main.cpp
               # json parser:
               ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp  
               ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_value.cpp  
               ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp
              )
TARGET_LINK_LIBRARIES(lanresolver 
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                    )

