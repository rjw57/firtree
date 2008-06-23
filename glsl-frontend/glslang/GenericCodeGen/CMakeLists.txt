# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds the trivial 'NULL' compiler from the 3Dlabd Inc.  Ltd. GLSL
# front-end implementation.

SET(CODEGEN_SOURCES
	CodeGen.cpp
	Link.cpp
)

SET(CODEGEN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

ADD_LIBRARY(CodeGen ${CODEGEN_SOURCES})

