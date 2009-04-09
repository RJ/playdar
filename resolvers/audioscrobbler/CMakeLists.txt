IF(NOT WIN32)
    FIND_PACKAGE(LibXml2 REQUIRED)

    ADD_LIBRARY( audioscrobbler SHARED audioscrobbler_plugin.cpp scrobsub.c scrobsub-curl.c)
    ADD_DEFINITIONS( -w -std=c99 -DSCROBSUB_SHARED_SECRET="e800f85bb46beeca0af4175724e97b2e" -DSCROBSUB_API_KEY="7a90f6672a04b809ee309af169f34b8b" -DSCROBSUB_CLIENT_ID="pld" -DSCROBSUB_CLIENT_VERSION="0.1" )

    # The lib must be named ^audioscrobbler.resolver$ for it to be loaded by playdar
    SET_TARGET_PROPERTIES( audioscrobbler PROPERTIES 
                           PREFIX "" 
                           SUFFIX ".resolver" )

    INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} )

    TARGET_LINK_LIBRARIES( audioscrobbler
                           ${OSSPUUID_LIBRARIES}
                           ${Boost_SYSTEM_LIBRARY}
                           ${Boost_THREAD_LIBRARY}
                           ${PLAYDAR_PLUGIN_LDFLAGS}
                           ${LIBXML2_LIBRARIES}
                           -lcrypto
                           -lcurl )
ENDIF(WIN32)