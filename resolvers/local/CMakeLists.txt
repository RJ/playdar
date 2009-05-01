ADD_LIBRARY( local SHARED
             rs_local_library.cpp
             library.cpp
             ${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp
             ${DEPS}/json_spirit_v3.00/json_spirit/json_spirit_writer.cpp             
             )

SET_TARGET_PROPERTIES( local PROPERTIES
                       PREFIX ""
                       SUFFIX ".resolver"
                       CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PLAYDAR_DIR}/plugins" )

TARGET_LINK_LIBRARIES(local 
                      ${Boost_LIBRARIES}
                      ${PLAYDAR_PLUGIN_LDFLAGS}
                      ${SQLITE3_LIBRARIES}
					  )

ADD_EXECUTABLE(scanner
               scanner/scanner.cpp
               library.cpp         # because library.cpp uses HTTPStreamingStrategy
               ${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp
              )
			  
TARGET_LINK_LIBRARIES(scanner 
                      ${SQLITE3_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${CURL_LIBRARIES}
                      ${TAGLIB_LIBRARIES})
                    
INSTALL(TARGETS scanner RUNTIME DESTINATION bin)
