# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds a trivial kernelcompiler capable of dumping
# the resultant IR or the compiled GLSL.

SET(TEST_COMPILER_HEADERS
)

SET(TEST_COMPILER_SOURCES
    ${TEST_COMPILER_HEADERS}

    kernelcompile.cc
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../")
ADD_EXECUTABLE(kernelcompile ${TEST_COMPILER_SOURCES})
TARGET_LINK_LIBRARIES(kernelcompile firtree)

# Vim modeline
# vim:ts=4:sw=4:et
