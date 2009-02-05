# CMake buildfile for the Firtree cpu runtime. 
#
# (C) Copyright 2009 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/firtree")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/firtree/include")

ADD_LIBRARY(cpu-runtime
  ../../include/firtree/cpu-runtime.h
  cpu-runtime.cpp)

# vim:sw=2:ts=2:et
