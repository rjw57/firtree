# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}")

SET(BACKEND_SOURCES
	glsl.h		glsl.cpp
	glutil.h	glutil.cpp
	../utils.cpp 	../utils.h
)

ADD_LIBRARY(glslbackend ${BACKEND_SOURCES})

