# - Find Styx
# This module finds an installed Styx grammar parser and associated
# utilities.  It sets the following variables:
#  STYX_FOUND - set to true if Styx is found
#  STYX_EXECUTABLE - the path to the styx executable
#  STYX_CTOH_EXECUTABLE - the path to styx's ctoh executable
#  STYX_INCLUDE_DIR - where to find the styx headers.
#  STYX_LIBRARIES - the styx libraries to link against.
#  STYX_LIBRARY_DIR - the styx library directory
#
# Set STYX_USE_STATIC_LIBS to TRUE to use static libs.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
IF( STYX_USE_STATIC_LIBS )
 SET( _styx_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
 SET( _styx_ORIG_CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})
 SET( CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} )
 SET( CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_STATIC_LIBRARY_PREFIX} )
ENDIF( STYX_USE_STATIC_LIBS )

FIND_PROGRAM(STYX_EXECUTABLE styx)
FIND_PROGRAM(STYX_CTOH_EXECUTABLE ctoh)
FIND_PATH(STYX_INCLUDE_DIR ptm.h
  /usr/include/styx
  /usr/local/include/styx
  /opt/local/include/styx
)
FIND_LIBRARY(STYX_dstyx_LIBRARY dstyx
  /usr/lib
  /usr/local/lib
  /opt/local/lib
)

IF( STYX_USE_STATIC_LIBS )
 SET( CMAKE_FIND_LIBRARY_SUFFIXES ${_styx_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES} )
 SET( CMAKE_FIND_LIBRARY_PREFIXES ${_styx_ORIG_CMAKE_FIND_LIBRARY_PREFIXES} )
ENDIF( STYX_USE_STATIC_LIBS )

# Assume Styx is found unless anything indicates otherwise
SET( STYX_FOUND "YES" )

IF(NOT STYX_EXECUTABLE)
  SET( STYX_FOUND "NO" )
ENDIF(NOT STYX_EXECUTABLE)

IF(NOT STYX_CTOH_EXECUTABLE)
  SET( STYX_FOUND "NO" )
ENDIF(NOT STYX_CTOH_EXECUTABLE)

IF(NOT STYX_INCLUDE_DIR)
  SET( STYX_FOUND "NO" )
ENDIF(NOT STYX_INCLUDE_DIR)

SET( STYX_LIBRARIES "" )

IF(STYX_dstyx_LIBRARY)
  SET( STYX_LIBRARIES
    ${STYX_LIBRARIES}
    ${STYX_dstyx_LIBRARY}
  )
  GET_FILENAME_COMPONENT(STYX_LIBRARY_DIR ${STYX_dstyx_LIBRARY} PATH)
ELSE(STYX_dstyx_LIBRARY)
  SET( STYX_FOUND "NO" )
ENDIF(STYX_dstyx_LIBRARY)
