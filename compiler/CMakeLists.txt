# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLUT)

IF(NOT OPENGL_LIBRARIES)
    MESSAGE(SEND_ERROR "OpenGL required.")
    SET(FIRTREE_HAVE_OPENGL 0)
ELSE(NOT OPENGL_LIBRARIES)
    SET(FIRTREE_HAVE_OPENGL 1)
ENDIF(NOT OPENGL_LIBRARIES)

IF(NOT GLUT_LIBRARIES)
    MESSAGE(SEND_ERROR "GLUT required.")
    SET(FIRTREE_HAVE_GLUT 0)
ELSE(NOT GLUT_LIBRARIES)
    SET(FIRTREE_HAVE_GLUT 1)
ENDIF(NOT GLUT_LIBRARIES)

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/../"
	"${CMAKE_CURRENT_SOURCE_DIR}/../glsl-frontend/"
)

ADD_SUBDIRECTORY(frontend)
ADD_SUBDIRECTORY(backends)

# Set platform-specific variables
IF(APPLE)
        SET(FIRTREE_APPLE 1)
ENDIF(APPLE)
IF(UNIX)
        SET(FIRTREE_UNIX 1)
ENDIF(UNIX)
IF(WIN32)
        SET(FIRTREE_WIN32 1)
ENDIF(WIN32)
IF(CYGWIN)
        SET(FIRTREE_CYGWIN 1)
ENDIF(CYGWIN)

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/platform.h.in")
    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform.h"
        @ONLY)
ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/platform.h.in")

SET(COMPILER_SOURCES
	include/compiler.h 

	compiler.cpp
)

ADD_LIBRARY(compiler ${COMPILER_SOURCES})

TARGET_LINK_LIBRARIES(compiler glslang frontend irbackend glslbackend -lpthread)
