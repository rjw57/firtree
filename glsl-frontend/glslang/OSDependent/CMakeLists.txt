# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the 3dLabs GLSL frontend.

SET(OSDEP_SOURCES
	Linux/osinclude.h
	Linux/ossource.cpp
)

SET(OSDEP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/Linux")

INCLUDE_DIRECTORIES("${OSDEP_INCLUDE_DIRS}")

ADD_LIBRARY(Ossource ${OSDEP_SOURCES})

