ADD_LIBRARY( lan_udp SHARED
             lan_udp.cpp
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( lan_udp PROPERTIES 
                       PREFIX "" 
                       SUFFIX ".resolver" )

IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( lan_udp
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_LIBRARIES}
                       ${APPLE_PLUGIN_LDFLAGS}
					   playdarlib
                     )
