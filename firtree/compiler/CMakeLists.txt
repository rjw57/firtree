# CMake buildfile for the Firtree compiler. 
#
# (C) Copyright 2008 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

SET(FIRTREE_COMPILER_INCLUDE_DIRS 
  "${CMAKE_CURRENT_SOURCE_DIR}/llvm-code-gen"
  "${CMAKE_CURRENT_SOURCE_DIR}/parser")

ADD_SUBDIRECTORY(llvm-code-gen)
ADD_SUBDIRECTORY(parser)
ADD_SUBDIRECTORY(targets)

INCLUDE_DIRECTORIES(${FIRTREE_COMPILER_INCLUDE_DIRS})
ADD_EXECUTABLE(compiletest tools/kernelcompile.cc)
ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
TARGET_LINK_LIBRARIES(compiletest firtree)

# vim:sw=2:ts=2:et
