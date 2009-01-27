# CMake buildfile for the Firtree compiler LLVM linker.
#
# (C) Copyright 2008 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

FIND_PACKAGE(Styx)

FIND_PACKAGE(LLVM)
IF(NOT LLVM_FOUND)
  MESSAGE(SEND_ERROR "LLVM required for Firtree.")
ENDIF(NOT LLVM_FOUND)

SET(LINKER_HEADERS
)

SET(LINKER_SOURCES
  ${LINKER_HEADERS}
  
  kernel_sampler_provider.cc
  sampler_provider.cc
)

INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${STYX_INCLUDE_DIR})
INCLUDE_DIRECTORIES(../compiler/parser)

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
ADD_DEFINITIONS(-DLLVM_MAJOR_VER=${LLVM_MAJOR} -DLLVM_MINOR_VER=${LLVM_MINOR})

ADD_LIBRARY(llvm-linker ${LINKER_SOURCES})
ADD_DEPENDENCIES(llvm-linker styx-parser)

TARGET_LINK_LIBRARIES(llvm-linker ${LLVM_LIBRARIES})

ADD_EXECUTABLE(linkertest tools/linkertest.cc)
TARGET_LINK_LIBRARIES(linkertest firtree)

# vim:sw=2:ts=2:et
