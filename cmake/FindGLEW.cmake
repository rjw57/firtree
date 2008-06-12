# - try to find glew library and include files
#  GLEW_INCLUDE_DIR, where to find GL/glew.h, etc.
#  GLEW_LIBRARIES, the libraries to link against
#  GLEW_FOUND, If false, do not try to use GLEW.
# Also defined, but not for general use are:
#  GLEW_glew_LIBRARY = the full path to the cg library.

IF (WIN32)

  IF(CYGWIN)

    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
      /usr/include
    )

    FIND_LIBRARY( GLEW_glew_LIBRARY GLEW
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

  ELSE(CYGWIN)

    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
      $ENV{GLEW_INC_PATH}
    )

    FIND_LIBRARY( GLEW_glew_LIBRARY GLEW
      $ENV{GLEW_LIB_PATH}
    )

  ENDIF(CYGWIN)

ELSE (WIN32)

    FIND_PATH( GLEW_INCLUDE_DIR GL/glew.h
      /usr/include
      /usr/include/glew
      /usr/local/include
      /usr/openwin/share/include
      /usr/openwin/include
      /usr/X11R6/include
      /usr/include/X11
    )

    FIND_LIBRARY( GLEW_glew_LIBRARY GLEW
      /usr/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

ENDIF (WIN32)

SET( GLEW_FOUND "NO" )
IF(GLEW_INCLUDE_DIR)
  IF(GLEW_glew_LIBRARY)
    SET( GLEW_LIBRARIES
      ${GLEW_glew_LIBRARY}
    )
    SET( GLEW_FOUND "YES" )
  ENDIF(GLEW_glew_LIBRARY)
ENDIF(GLEW_INCLUDE_DIR)

MARK_AS_ADVANCED(
  GLEW_INCLUDE_DIR
  GLEW_glew_LIBRARY
)
