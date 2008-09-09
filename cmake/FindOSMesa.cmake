# - try to find OSMesa library and include files
#  OSMESA_INCLUDE_DIR, where to find GL/OSMesa.h, etc.
#  OSMESA_LIBRARIES, the libraries to link against
#  OSMESA_FOUND, If false, do not try to use OSMESA.
# Also defined, but not for general use are:
#  OSMESA_OSMesa_LIBRARY = the full path to the cg library.

IF (WIN32)

  IF(CYGWIN)

    FIND_PATH( OSMESA_INCLUDE_DIR GL/osmesa.h
      /usr/include
    )

    FIND_LIBRARY( OSMESA_OSMesa_LIBRARY OSMESA
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

  ELSE(CYGWIN)

    FIND_PATH( OSMESA_INCLUDE_DIR GL/osmesa.h
      $ENV{OSMESA_INC_PATH}
    )

    FIND_LIBRARY( OSMESA_OSMesa_LIBRARY OSMesa
      $ENV{OSMESA_LIB_PATH}
    )

  ENDIF(CYGWIN)

ELSE (WIN32)

    FIND_PATH( OSMESA_INCLUDE_DIR GL/osmesa.h
      /usr/include
      /usr/include/OSMesa
      /usr/local/include
      /usr/openwin/share/include
      /usr/openwin/include
      /usr/X11R6/include
      /usr/include/X11
    )

    FIND_LIBRARY( OSMESA_OSMesa_LIBRARY OSMesa
      /usr/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

ENDIF (WIN32)

SET( OSMESA_FOUND "NO" )
IF(OSMESA_INCLUDE_DIR)
  IF(OSMESA_OSMesa_LIBRARY)
    SET( OSMESA_LIBRARIES
      ${OSMESA_OSMesa_LIBRARY}
    )
    SET( OSMESA_FOUND "YES" )
  ENDIF(OSMESA_OSMesa_LIBRARY)
ENDIF(OSMESA_INCLUDE_DIR)

MARK_AS_ADVANCED(
  OSMESA_INCLUDE_DIR
  OSMESA_OSMesa_LIBRARY
)
