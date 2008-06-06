# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

ADD_SUBDIRECTORY(frontend)

SET(COMPILER_SOURCES
	include/compiler.h 	include/main.h

	compiler.cpp		main.cpp
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../glsl-frontend/")

ADD_LIBRARY(compiler ${COMPILER_SOURCES})
TARGET_LINK_LIBRARIES(compiler glslang frontend -lpthread)
