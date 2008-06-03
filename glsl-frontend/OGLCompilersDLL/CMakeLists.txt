# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the 3dLabs GLSL frontend.

SET(INITDLL_SOURCES
	InitializeDll.h
	InitializeDll.cpp
)

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/../glslang"
	"${CMAKE_CURRENT_SOURCE_DIR}/../glslang/Include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../glslang/OSDependent/Linux"
)

ADD_LIBRARY(InitializeDll ${INITDLL_SOURCES})

