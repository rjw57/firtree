# CMake buildfile for the Firtree compiler GLSL target. 
#
# (C) Copyright 2008 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

FIND_PACKAGE(Styx)

SET(LLVM_CONFIG_COMPONENTS 
  instrumentation bitwriter analysis system linker lto)
FIND_PACKAGE(LLVM)
IF(NOT LLVM_FOUND)
  MESSAGE(SEND_ERROR "LLVM required for Firtree.")
ENDIF(NOT LLVM_FOUND)

SET(GLSL_TARGET_HEADERS
  glsl-target.h
)

SET(GLSL_TARGET_SOURCES
  ${GLSL_TARGET_HEADERS}
  glsl-target.cc
)

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
ADD_DEFINITIONS(-DLLVM_MAJOR_VER=${LLVM_MAJOR} -DLLVM_MINOR_VER=${LLVM_MINOR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${STYX_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FIRTREE_COMPILER_INCLUDE_DIRS})
ADD_LIBRARY(glsl-target ${GLSL_TARGET_SOURCES})
TARGET_LINK_LIBRARIES(glsl-target ${LLVM_LIBRARIES})

ADD_DEPENDENCIES(glsl-target styx-parser)

# vim:sw=2:ts=2:et
