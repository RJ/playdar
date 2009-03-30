# ################################################################################
# Project name
#

PROJECT(PDL)

# ################################################################################
# Project version
#

SET(PDL_VERSION_MAJOR 0)
SET(PDL_VERSION_MINOR 3)
SET(PDL_VERSION_PATCH 0)

# ################################################################################
# CMake version check
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2 FATAL_ERROR)

# ################################################################################
# Check Includes
#

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckCXXSourceCompiles)

INCLUDE_DIRECTORIES(include)

# ################################################################################
# Check libraries
#

IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

#DL_LIBRARY
FIND_LIBRARY(DL_LIBRARY 
  NAMES dl
  PATHS /opt/lib /usr/lib /usr/local/lib
  DOC "Can't find DL library")

ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

# ################################################################################
# Compiler settings
#

IF(WIN32)
  IF(NOT UNIX)
    IF(NOT BORLAND)
      IF(NOT MINGW )
        SET(CMAKE_BUILD_ON_VISUAL_STUDIO 1)
      ENDIF(NOT MINGW )
    ENDIF(NOT BORLAND)
  ENDIF(NOT UNIX)
ENDIF(WIN32)

IF("${CMAKE_COMPILER_IS_GNUCXX}" MATCHES "1")
SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} -Wall -O3 -funroll-loops -pedantic -Wno-long-long")
#SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} -Wall -O3 -funroll-loops -pedantic -Wno-long-long -D_DEBUG")
#SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} -Wall -Wno-long-long -Weffc++ -pedantic -Wno-long-long -g")
#SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} -Wall -Wno-long-long -pedantic -Wno-long-long -g")
ENDIF("${CMAKE_COMPILER_IS_GNUCXX}" MATCHES "1")

# ################################################################################
# Library source
#

SET(LIBSRC                                       src/DynamicLibrary.cpp
                                                 src/DynamicLibraryManager.cpp
                                                 src/DynamicLoader.cpp
                                                 src/LoaderException.cpp )

# ################################################################################
# Library binary
#

ADD_LIBRARY( pdl SHARED                          ${LIBSRC} )
IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
TARGET_LINK_LIBRARIES( pdl c                     ${DL_LIBRARY})
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
SET_TARGET_PROPERTIES( pdl PROPERTIES            OUTPUT_NAME pdl )
SET_TARGET_PROPERTIES( pdl PROPERTIES            LINKER_LANGUAGE CXX )
SET_TARGET_PROPERTIES( pdl PROPERTIES            DEFINE_SYMBOL PDL_EXPORT )
SET_TARGET_PROPERTIES( pdl PROPERTIES            COMPILE_FLAGS -DPDL_SHARED )
SET_TARGET_PROPERTIES( pdl PROPERTIES            VERSION ${PDL_VERSION_MAJOR}.${PDL_VERSION_MINOR}.${PDL_VERSION_PATCH}
                                                 SOVERSION ${PDL_VERSION_MAJOR} )

ADD_LIBRARY( pdl-st STATIC                       ${LIBSRC} )
IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
TARGET_LINK_LIBRARIES( pdl-st c                  ${DL_LIBRARY} )
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
SET_TARGET_PROPERTIES( pdl-st PROPERTIES         OUTPUT_NAME pdl-st )
SET_TARGET_PROPERTIES( pdl-st PROPERTIES         LINKER_LANGUAGE CXX )
SET_TARGET_PROPERTIES( pdl-st PROPERTIES         VERSION ${PDL_VERSION_MAJOR}.${PDL_VERSION_MINOR}.${PDL_VERSION_PATCH}
                                                 SOVERSION ${PDL_VERSION_MAJOR} )

# ################################################################################
# Installation

# Install headers
INSTALL( FILES                                   include/platform.h
                                                 include/DynamicClass.hpp
                                                 include/DynamicLoader.hpp
                                                 include/LoaderException.hpp
         DESTINATION                             include/pdl)

# Install binaries
INSTALL( TARGETS                                 pdl
                                                 pdl-st
         DESTINATION                             lib )


# ################################################################################
# Test suite
#

ENABLE_TESTING()

# Test libraries
ADD_LIBRARY( test_module1 SHARED                 tests/TestClass1.cpp )
SET_TARGET_PROPERTIES( test_module1 PROPERTIES   OUTPUT_NAME test_module1 )
ADD_LIBRARY( test_module2 SHARED                 tests/TestClass2.cpp )
SET_TARGET_PROPERTIES( test_module2 PROPERTIES   OUTPUT_NAME test_module2 )

# Test LoaderException class
ADD_EXECUTABLE( TestLoaderException              tests/TestLoaderException.cpp
                                                 ${LIBSRC} )
IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
TARGET_LINK_LIBRARIES( TestLoaderException       ${DL_LIBRARY})
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
ADD_TEST( TestLoaderException                    TestLoaderException )

# Test DynamicLibrary class
ADD_EXECUTABLE( TestDynamicLibrary               tests/TestDynamicLibrary.cpp
                                                 ${LIBSRC} )
IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
TARGET_LINK_LIBRARIES( TestDynamicLibrary        ${DL_LIBRARY})
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
IF (WIN32)
ADD_TEST( TestDynamicLibrary                     TestDynamicLibrary test_module1.dll )
ELSE (WIN32)
ADD_TEST( TestDynamicLibrary                     TestDynamicLibrary ./libtest_module1 )
ENDIF (WIN32)

# Test DynamicLibraryManager class
ADD_EXECUTABLE( TestDynamicLibraryManager        tests/TestDynamicLibraryManager.cpp
                                                 ${LIBSRC} )
IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
TARGET_LINK_LIBRARIES( TestDynamicLibraryManager ${DL_LIBRARY})
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
IF (WIN32)
ADD_TEST( TestDynamicLibraryManager              TestDynamicLibraryManager test_module1.dll test_module2.dll )
ELSE (WIN32)
ADD_TEST( TestDynamicLibraryManager              TestDynamicLibraryManager ./libtest_module1 ./libtest_module2 )
ENDIF (WIN32)

# Test DynamicLoader class
ADD_EXECUTABLE( TestDynamicLoader                tests/TestDynamicLoader.cpp )
TARGET_LINK_LIBRARIES( TestDynamicLoader         pdl )
IF (WIN32)
ADD_TEST( TestDynamicLoader1                     TestDynamicLoader test_module1.dll MyTestClass1 MyTestClass2 )
ADD_TEST( TestDynamicLoader2                     TestDynamicLoader test_module2.dll MyTestClass3 MyTestClass4 )
ELSE (WIN32)
ADD_TEST( TestDynamicLoader1                     TestDynamicLoader ./libtest_module1 MyTestClass1 MyTestClass2 )
ADD_TEST( TestDynamicLoader2                     TestDynamicLoader ./libtest_module2 MyTestClass3 MyTestClass4 )
ENDIF (WIN32)
