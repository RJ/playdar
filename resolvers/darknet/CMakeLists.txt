ADD_LIBRARY(    darknet SHARED
                darknet.cpp
                servent.cpp
            )
#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  darknet PROPERTIES
                        PREFIX ""
                        SUFFIX ".resolver"
                        CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PLAYDAR_DIR}/plugins"
                     )


IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( darknet
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_LIBRARIES} # Boost license
                       ${APPLE_PLUGIN_LDFLAGS}
					   playdarlib
                     )
