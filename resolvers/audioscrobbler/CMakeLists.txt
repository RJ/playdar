IF(APPLE)
    SET(SCROBSUB_SOURCES scrobsub.c scrobsub-cocoa.m)
    SET(SCROBSUB_LDFLAGS -framework Security -framework Cocoa)
ELSE(APPLE)
    SET(SCROBSUB_SOURCES scrobsub.c scrobsub-curl.c)
    SET(SCROBSUB_LDFLAGS -lxml -lopenssl -lcurl)
ENDIF(APPLE)

ADD_LIBRARY( audioscrobbler SHARED audioscrobbler_plugin.cpp ${SCROBSUB_SOURCES} )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( audioscrobbler PROPERTIES 
                       PREFIX "" 
                       SUFFIX ".resolver" )

IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( audioscrobbler
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                       ${APPLE_PLUGIN_LDFLAGS}
                       ${SCROBSUB_LDFLAGS}
                     )
