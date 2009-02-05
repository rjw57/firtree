# - try to find DevIL library and include files
#  DEVIL_INCLUDE_DIR, where to find GL/IL/il.h, etc.
#  DEVIL_LIBRARIES, the libraries to link against
#  DEVIL_FOUND, If false, do not try to use DEVIL.
# Also defined, but not for general use are:
#  DEVIL_DevIL_LIBRARY = the full path to the cg library.

IF (WIN32)

  IF(CYGWIN)

    FIND_PATH( DEVIL_INCLUDE_DIR IL/il.h
      /usr/include
    )

    FIND_LIBRARY( DEVIL_DevIL_LIBRARY DEVIL
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

  ELSE(CYGWIN)

    FIND_PATH( DEVIL_INCLUDE_DIR IL/il.h
      $ENV{DEVIL_INC_PATH}
    )

    FIND_LIBRARY( DEVIL_DevIL_LIBRARY DEVIL
      $ENV{DEVIL_LIB_PATH}
    )

  ENDIF(CYGWIN)

ELSE (WIN32)

    FIND_PATH( DEVIL_INCLUDE_DIR IL/il.h
      /usr/include
      /usr/include/devil
      /opt/local/include
      /opt/local/include/devil
      /usr/local/include
      /usr/openwin/share/include
      /usr/openwin/include
      /usr/X11R6/include
      /usr/include/X11
    )

    FIND_LIBRARY( DEVIL_DevIL_LIBRARY IL
      /usr/lib
      /opt/local/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

ENDIF (WIN32)

SET( DEVIL_FOUND "NO" )
IF(DEVIL_INCLUDE_DIR)
  IF(DEVIL_DevIL_LIBRARY)
    SET( DEVIL_LIBRARIES
      ${DEVIL_DevIL_LIBRARY}
    )
    SET( DEVIL_FOUND "YES" )
  ENDIF(DEVIL_DevIL_LIBRARY)
ENDIF(DEVIL_INCLUDE_DIR)

MARK_AS_ADVANCED(
  DEVIL_INCLUDE_DIR
  DEVIL_DevIL_LIBRARY
)
