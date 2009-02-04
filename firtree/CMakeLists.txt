# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds the main Firtree library.

SET(LLVM_CONFIG_COMPONENTS instrumentation scalaropts ipo linker engine)

FIND_PACKAGE(LLVM)
IF(NOT LLVM_FOUND)
  MESSAGE(SEND_ERROR "LLVM required for Firtree.")
ENDIF(NOT LLVM_FOUND)

MESSAGE(STATUS "LLVM libs: ${LLVM_STATIC_LIBS}")
MESSAGE(STATUS "LLVM objs: ${LLVM_STATIC_OBJS}")

# Build the compiler and linker
ADD_SUBDIRECTORY(compiler)
ADD_SUBDIRECTORY(linker)

# Find required libraries.
FIND_PACKAGE(OpenGL)
FIND_PACKAGE(OSMesa)
FIND_PACKAGE(GLUT)
FIND_PACKAGE(Magick++)

SET(FIRTREE_TARGET_LIBRARIES "")
SET(FIRTREE_TARGET_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

IF (OPENGL_LIBRARIES)
    SET(FIRTREE_TARGET_LIBRARIES ${FIRTREE_TARGET_LIBRARIES} ${OPENGL_LIBRARIES})
    SET(FIRTREE_TARGET_INCLUDES ${FIRTREE_TARGET_INCLUDES} ${OPENGL_INCLUDE_DIR})
    SET(FIRTREE_HAVE_OPENGL 1)
ELSE(OPENGL_LIBRARIES)
    MESSAGE(SEND_ERROR "OpenGL is required to build FIRTREE.")
    SET(FIRTREE_HAVE_OPENGL 0)
ENDIF (OPENGL_LIBRARIES)

IF(GLUT_FOUND)
    SET(FIRTREE_TARGET_LIBRARIES ${FIRTREE_TARGET_LIBRARIES} ${GLUT_LIBRARIES})
    SET(FIRTREE_TARGET_INCLUDES ${FIRTREE_TARGET_INCLUDES} ${GLUT_INCLUDE_DIR})
    SET(FIRTREE_HAVE_GLUT 1)
ELSE(GLUT_FOUND)
    MESSAGE(SEND_ERROR "GLUT is required to build FIRTREE.")
    SET(FIRTREE_HAVE_GLUT 0)
ENDIF(GLUT_FOUND)

# OSMesa can be installed with Apple's X11 but doesn't play nice with the OpenGL Framework.
IF(OSMESA_FOUND AND NOT APPLE)
    SET(FIRTREE_TARGET_LIBRARIES ${FIRTREE_TARGET_LIBRARIES} ${OSMESA_LIBRARIES})
    SET(FIRTREE_TARGET_INCLUDES ${FIRTREE_TARGET_INCLUDES} ${OSMESA_INCLUDE_DIR})
    SET(FIRTREE_HAVE_OSMESA 1)
ELSE(OSMESA_FOUND AND NOT APPLE)
    MESSAGE(STATUS "OSMesa is not found. Software-only rendering is disabled.")
    SET(FIRTREE_HAVE_OSMESA 0)
ENDIF(OSMESA_FOUND AND NOT APPLE)

IF(MAGICKPP_FOUND)
    SET(FIRTREE_TARGET_LIBRARIES ${FIRTREE_TARGET_LIBRARIES} ${MAGICKPP_LIBRARIES})
    SET(FIRTREE_TARGET_INCLUDES ${FIRTREE_TARGET_INCLUDES} ${MAGICKPP_INCLUDE_DIR})
ELSE(MAGICKPP_FOUND)
    MESSAGE(SEND_ERROR "The ImageMagick libraries are required to build this example.")
ENDIF(MAGICKPP_FOUND)

INCLUDE_DIRECTORIES(
	${FIRTREE_TARGET_INCLUDES}
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/../"
	"${CMAKE_CURRENT_SOURCE_DIR}/../third-party/"
)

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

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/firtree/platform.h.in")
    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/firtree/platform.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/firtree/platform.h"
        @ONLY)
ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/firtree/platform.h.in")

IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc"
        @ONLY)
ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc.in")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/firtree.pc" DESTINATION lib/pkgconfig)

SET(FIRTREE_HEADERS
	include/firtree/main.h		include/firtree/platform.h	include/firtree/opengl.h
	include/firtree/blob.h		include/firtree/math.h		include/firtree/kernel.h
	include/firtree/image.h		include/firtree/glsl-runtime.h  include/firtree/value.h
	include/firtree/cpu-runtime.h
)

SET(FIRTREE_SOURCES
	${FIRTREE_HEADERS}

	internal/image-int.h
	internal/lru-cache.h
	internal/render-to-texture.h
	internal/pbuffer.h

	runtime/glsl/sha1.h
	runtime/glsl/glsl-runtime.cpp	runtime/glsl/sha1.c

	runtime/cpu/cpu-runtime.cpp

	main.cpp		math.cpp		blob.cpp
	kernel.cpp		image.cpp

	image-int.cpp		render-to-texture.cpp	pbuffer.cpp
	value.cpp

	${LLVM_STATIC_OBJS}
)

ADD_LIBRARY(firtree SHARED ${FIRTREE_SOURCES})

TARGET_LINK_LIBRARIES(firtree 
	${LLVM_STATIC_LIBS}
	selog
	${FIRTREE_TARGET_LIBRARIES}
	llvm-code-gen glsl-target
	llvm-linker
	compiler glslang frontend irbackend glslbackend)

# Set the version and soversion properties for Firtree
# so that library versioning works.
SET_TARGET_PROPERTIES(firtree PROPERTIES
	VERSION "${FIRTREE_VERSION}"
	SOVERSION "${FIRTREE_VERSION_MAJOR}.${FIRTREE_VERSION_MINOR}")

# The magic runes required to install a library.
INSTALL(TARGETS firtree
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

INSTALL(FILES ${FIRTREE_HEADERS} DESTINATION include/firtree)
