# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds the DLL intitialisation portion of the 3Dlabs
# Inc. Ltd. GLSL front-end implementation.

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

