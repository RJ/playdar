ADD_LIBRARY( f2f SHARED
                 f2f.cpp
                 jbot.cpp
           )

# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( f2f PROPERTIES
                       PREFIX ""
                       SUFFIX ".resolver" )

TARGET_LINK_LIBRARIES( f2f
                       gloox
                       ${PLAYDAR_PLUGIN_LDFLAGS}
                       ${Boost_LIBRARIES}
                     )

