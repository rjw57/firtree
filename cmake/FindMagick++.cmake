# - try to find cg library and include files
#  MAGICKPP_INCLUDE_DIR, where to find Cg/cg.h, etc.
#  MAGICKPP_LIBRARIES, the libraries to link against
#  MAGICKPP_FOUND, If false, do not try to use CG.
# Also defined, but not for general use are:
#  MAGICKPP_magickpp_LIBRARY = the full path to the magick++ library.
#  MAGICKPP_magick_LIBRARY = the full path to the core magick library.
#  MAGICKPP_wand_LIBRARY = the full path to the core magick library.

IF (WIN32)

  IF(CYGWIN)

    FIND_PATH( MAGICKPP_INCLUDE_DIR Magick++/Image.h
      /usr/include
      /usr/local/include
    )

    FIND_LIBRARY( MAGICKPP_magickpp_LIBRARY Magick++
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

    FIND_LIBRARY( MAGICKPP_magick_LIBRARY Magick
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

    FIND_LIBRARY( MAGICKPP_wand_LIBRARY wand
      /usr/lib
      /usr/lib/w32api
      /usr/local/lib
      /usr/X11R6/lib
    )

  ELSE(CYGWIN)
    GET_FILENAME_COMPONENT(MAGICK_BIN_PATH  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ImageMagick\\Current;BinPath]" ABSOLUTE CACHE)
    GET_FILENAME_COMPONENT(MAGICK_LIB_PATH  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ImageMagick\\Current;LibPath]" ABSOLUTE CACHE)

    FIND_PATH( MAGICKPP_INCLUDE_DIR 
        Magick++/Image.h
            PATHS ${MAGICK_BIN_PATH}/include
    )

    FIND_LIBRARY( MAGICKPP_magickpp_LIBRARY 
        NAMES CORE_RL_Magick++_
            PATHS ${MAGICK_LIB_PATH}
    )

    FIND_LIBRARY( MAGICKPP_magick_LIBRARY 
        NAMES CORE_RL_magick_
            PATHS ${MAGICK_LIB_PATH}
    )

    FIND_LIBRARY( MAGICKPP_wand_LIBRARY 
        NAMES CORE_RL_wand_
            PATHS ${MAGICK_LIB_PATH}
    )

    # MESSAGE(STATUS "Path: ${MAGICK_BIN_PATH}, Lib: ${MAGICKPP_magickpp_LIBRARY}")
  ENDIF(CYGWIN)

ELSE (WIN32)

    FIND_PATH( MAGICKPP_INCLUDE_DIR Magick++/Image.h
      /usr/include
      /usr/local/include
      /usr/openwin/share/include
      /usr/openwin/include
      /usr/X11R6/include
      /usr/include/X11
    )

    FIND_LIBRARY( MAGICKPP_magickpp_LIBRARY Magick++
      /usr/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

    FIND_LIBRARY( MAGICKPP_magick_LIBRARY Magick
      /usr/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

    FIND_LIBRARY( MAGICKPP_wand_LIBRARY Wand
      /usr/lib
      /usr/local/lib
      /usr/openwin/lib
      /usr/X11R6/lib
    )

ENDIF (WIN32)

SET( MAGICKPP_FOUND "NO" )
IF(MAGICKPP_INCLUDE_DIR)
  IF(MAGICKPP_magickpp_LIBRARY AND MAGICKPP_magick_LIBRARY)
    SET( MAGICKPP_LIBRARIES 
        ${MAGICKPP_magickpp_LIBRARY} 
        ${MAGICKPP_magick_LIBRARY}
        ${MAGICKPP_wand_LIBRARY} )
    SET( MAGICKPP_FOUND "YES" )
  ENDIF(MAGICKPP_magickpp_LIBRARY AND MAGICKPP_magick_LIBRARY)
ENDIF(MAGICKPP_INCLUDE_DIR)

MARK_AS_ADVANCED(
  MAGICKPP_INCLUDE_DIR
  MAGICKPP_magickpp_LIBRARY
  MAGICKPP_magick_LIBRARY
)
