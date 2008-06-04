# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the 3dLabs GLSL frontend.


INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../glslang/Public")

SET(STANDALONE_SOURCES
        StandAlone.cpp
)

ADD_EXECUTABLE(StandAlone ${STANDALONE_SOURCES})
TARGET_LINK_LIBRARIES(StandAlone glslang -lpthread)

