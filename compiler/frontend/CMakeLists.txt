# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

SET(FRONTEND_SOURCES
	glsl-compiler.cpp
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../../glsl-frontend/")

ADD_LIBRARY(frontend ${FRONTEND_SOURCES})

