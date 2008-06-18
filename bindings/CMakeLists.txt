# CMake project file for FIRTREE
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# See if we can locate the SWIG tool.
SET(SWIG_FIND_QUIETLY 1)
FIND_PACKAGE(SWIG)
FIND_PACKAGE(OpenGL)
IF(SWIG_FOUND)
    INCLUDE(UseSWIG)
ENDIF(SWIG_FOUND)

IF(NOT OPENGL_FOUND)
    MESSAGE(SEND_ERROR OpenGL is required to compile bindings.)
ENDIF(NOT OPENGL_FOUND)

# Setup SWIG options in general.
IF(SWIG_FOUND)
    SET(CMAKE_SWIG_FLAGS -Werror)
    # The SWIG-using bindings
    ADD_SUBDIRECTORY(python)
ELSE(SWIG_FOUND)
    MESSAGE(STATUS 
    "=> Could not find SWIG. Make sure the swig executable is in your path.")
    MESSAGE(STATUS 
    "   Without SWIG, the FIRTREE bindings will *not* be built.")
ENDIF(SWIG_FOUND)

# Vim modeline
# vim:ts=4:sw=4:et
