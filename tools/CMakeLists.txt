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

GET_TARGET_PROPERTY(KERNEL_PARSE_INCLUDES kparse INCLUDE_DIRS)
INCLUDE_DIRECTORIES(${KERNEL_PARSE_INCLUDES})
ADD_EXECUTABLE(kernelcompile ${TEST_COMPILER_SOURCES})
TARGET_LINK_LIBRARIES(kernelcompile kparse)

# Vim modeline
# vim:ts=4:sw=4:et
