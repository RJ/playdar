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

TARGET_LINK_LIBRARIES( demo playdarengine )
