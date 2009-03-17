ADD_LIBRARY(    demo SHARED
                demo.cpp
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  demo PROPERTIES 
                        PREFIX "" 
                        SUFFIX ".resolver")

IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)


TARGET_LINK_LIBRARIES( demo
                       ${OSSPUUID_LIBRARIES}
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                       ${APPLE_PLUGIN_LDFLAGS}
                     )
