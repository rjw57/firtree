# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

SET(EXAMPLE_HEADERS
)

SET(EXAMPLE_SOURCES
    ${EXAMPLE_HEADERS}

    common.cpp   main.cpp
)

# INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../")
ADD_EXECUTABLE(simple-glsl ${EXAMPLE_SOURCES})
TARGET_LINK_LIBRARIES(simple-glsl compiler -lGL -lglut -lGLU -lGLEW)

# Vim modeline
# vim:ts=4:sw=4:et
