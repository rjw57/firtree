# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

SET(BACKEND_SOURCES
	irdump.h	irdump.cpp
	../utils.h	../utils.cpp
)

ADD_LIBRARY(irbackend ${BACKEND_SOURCES})

