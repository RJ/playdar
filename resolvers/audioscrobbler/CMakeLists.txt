IF(APPLE)
    SET(SCROBSUB_SOURCES scrobsub-cocoa.m)
    SET(SCROBSUB_LDFLAGS -framework\ Security -framework\ Cocoa -framework\ CoreData)
ELSE(APPLE)
    SET(SCROBSUB_SOURCES scrobsub-curl.c)
    SET(SCROBSUB_LDFLAGS -lxml -lopenssl -lcurl)
ENDIF(APPLE)

ADD_LIBRARY( audioscrobbler SHARED audioscrobbler_plugin.cpp scrobsub.c ${SCROBSUB_SOURCES} )

ADD_DEFINITIONS(-w -std=c99 -DSCROBSUB_SHARED_SECRET="73582dfc9e556d307aead069af110ab8" -DSCROBSUB_API_KEY="c8c7b163b11f92ef2d33ba6cd3c2c3c3" -DSCROBSUB_CLIENT_ID="ass" -DSCROBSUB_CLIENT_VERSION="0.1")

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( audioscrobbler PROPERTIES 
                       PREFIX "" 
                       SUFFIX ".resolver" )

TARGET_LINK_LIBRARIES( audioscrobbler
                       ${OSSPUUID_LIBRARIES}
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                       ${PLUGIN_LDFLAGS}
                       ${SCROBSUB_LDFLAGS}
                     )
