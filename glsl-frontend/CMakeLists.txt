# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the 3Dlabs Inc. Ltd. GLSL frontend.

SET(GLSLANG_INCLUDE_DIRS 
	"${GLSLANG_INCLUDE_DIRS}"
	"${CMAKE_CURRENT_SOURCE_DIR}/OGLCompilersDLL"
)

ADD_SUBDIRECTORY(OGLCompilersDLL)

ADD_SUBDIRECTORY(glslang)

ADD_SUBDIRECTORY(StandAlone)
