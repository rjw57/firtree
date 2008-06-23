# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds a trivial stand-alone compiler which uses the 
# 3Dlabs Inc. Ltd. GLSL frontend.

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../glslang/Public")

SET(STANDALONE_SOURCES
        StandAlone.cpp
)

ADD_EXECUTABLE(StandAlone ${STANDALONE_SOURCES})
TARGET_LINK_LIBRARIES(StandAlone glslang CodeGen -lpthread)

