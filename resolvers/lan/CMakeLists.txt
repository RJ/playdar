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

TARGET_LINK_LIBRARIES( lan
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                       ${PLAYDAR_PLUGIN_LDFLAGS}
                     )
