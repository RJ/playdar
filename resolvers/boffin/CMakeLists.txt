ADD_LIBRARY(    Boffin SHARED
                Boffin.cpp
				BoffinDb.cpp
                ResultSet.cpp
				SimilarArtists.cpp
				parser/parser.cpp
            )
#
# Ensure the shared library file is called <CLASS NAME>.resolver
# Playdar looks for a class with the same name as the file, minus suffix
#
SET_TARGET_PROPERTIES(  Boffin PROPERTIES
                        PREFIX ""
                        SUFFIX ".resolver"
                        CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PLAYDAR_DIR}/plugins"
                     )


IF(APPLE)
    SET(APPLE_PLUGIN_LDFLAGS -undefined\ dynamic_lookup)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( Boffin
					${OSSPUUID_LIBRARIES}  # BSD-like
					${Boost_LIBRARIES} # Boost license
					${APPLE_PLUGIN_LDFLAGS}
					${SQLITE3_LIBRARIES}
					playdarlib
					)
					
ADD_EXECUTABLE(tagger
	Tagger.cpp
	BoffinDb.cpp
	${DEPS}/sqlite3pp-read-only/sqlite3pp.cpp )

TARGET_LINK_LIBRARIES(tagger
	${SQLITE3_LIBRARIES}
	${Boost_LIBRARIES} )
 
