ADD_LIBRARY( lan SHARED
             lan.cpp
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( lan PROPERTIES 
                       PREFIX "" 
                       SUFFIX ".resolver" )

IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( lan
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_LIBRARIES}
                       ${APPLE_PLUGIN_LDFLAGS}
					   playdarlib
                     )
