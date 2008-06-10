# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

SET(BACKEND_SOURCES
	glsl.h	glsl.cpp
	../utils.cpp ../utils.h
)

ADD_LIBRARY(glslbackend ${BACKEND_SOURCES})

