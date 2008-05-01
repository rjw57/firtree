# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Test compiler
SET(TEST_COMPILER_HEADERS
)

SET(TEST_COMPILER_SOURCES
    ${TEST_COMPILER_HEADERS}

    kernelcompile.cc
)

ADD_DEFINITIONS(-DFIRTREE)
GET_TARGET_PROPERTY(MESA_INCLUDES mesa INCLUDE_DIRS)
INCLUDE_DIRECTORIES(${MESA_INCLUDES})
ADD_EXECUTABLE(kernelcompile ${TEST_COMPILER_SOURCES})
TARGET_LINK_LIBRARIES(kernelcompile mesa)

# Vim modeline
# vim:ts=4:sw=4:et
