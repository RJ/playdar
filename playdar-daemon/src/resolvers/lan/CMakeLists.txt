ADD_LIBRARY(    lan_udp SHARED
                lan_udp.cpp
                ${PLAYDAR_PATH}/src/application/application.cpp
                ${PLAYDAR_PATH}/src/resolvers/playable_item.cpp
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  lan_udp PROPERTIES 
                        PREFIX "" 
                        SUFFIX ".resolver")

TARGET_LINK_LIBRARIES( lan_udp
                       ${OSSPUUID_LIBRARIES}  # BSD-like
                       ${Boost_SYSTEM_LIBRARY}
                       ${Boost_THREAD_LIBRARY}
                     )
