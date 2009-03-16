INCLUDE_DIRECTORIES ( ${DEPS}/boost.process )

ADD_LIBRARY(    gateway_script SHARED
                gateway_script.cpp
            )
#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  gateway_script PROPERTIES
                        PREFIX ""
                        SUFFIX ".resolver")


TARGET_LINK_LIBRARIES( gateway_script playdarengine )
