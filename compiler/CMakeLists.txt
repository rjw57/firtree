# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/../"
	"${CMAKE_CURRENT_SOURCE_DIR}/../glsl-frontend/"
)

ADD_SUBDIRECTORY(frontend)
ADD_SUBDIRECTORY(backends)

SET(COMPILER_SOURCES
	include/compiler.h 	include/main.h
	include/kernel.h

	compiler.cpp		main.cpp
	kernel.cpp
)

ADD_LIBRARY(compiler ${COMPILER_SOURCES})
TARGET_LINK_LIBRARIES(compiler glslang frontend irbackend glslbackend -lpthread)
