ADD_LIBRARY( lan SHARED
             lan.cpp
             ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_reader.cpp             
             ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp             
             ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_value.cpp             
            )

#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES( lan PROPERTIES
                       PREFIX ""
                       SUFFIX ".resolver" )

TARGET_LINK_LIBRARIES( lan
                       ${PLAYDAR_PLUGIN_LDFLAGS}
                       ${Boost_LIBRARIES}
                       )
