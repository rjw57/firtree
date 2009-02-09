# CMake buildfile for the Firtree cpu runtime. 
#
# (C) Copyright 2009 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

FIND_PACKAGE(LLVM)
FIND_PACKAGE(PythonInterp)

SET(BIN2H "${CMAKE_SOURCE_DIR}/tools/bin2h.py")

IF(NOT PYTHONINTERP_FOUND)
  MESSAGE(SEND_ERROR "Could not find Python interpreter.")
ENDIF(NOT PYTHONINTERP_FOUND)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/firtree")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/firtree/include")

ADD_CUSTOM_COMMAND(OUTPUT builtins_bc.h 
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${BIN2H} builtins_bc builtins.bc.opt builtins_bc.h
  MAIN_DEPENDENCY builtins.bc.opt
  DEPENDS ${BIN2H}
  )

ADD_CUSTOM_COMMAND(OUTPUT builtins.bc.opt
  COMMAND ${LLVM_OPT_EXECUTABLE} ARGS builtins.bc -f -std-compile-opts -o=builtins.bc.opt
  MAIN_DEPENDENCY builtins.bc
  )

ADD_CUSTOM_COMMAND(OUTPUT builtins.bc 
  COMMAND ${LLVM_AS_EXECUTABLE} ARGS builtins.il -f -o=builtins.bc
  MAIN_DEPENDENCY builtins.il
  )

ADD_DEFINITIONS("-DLLVM_HOST_TARGET=${LLVM_HOST_TARGET}")

ADD_LIBRARY(cpu-runtime
  builtins_bc.h 
  ../../include/firtree/cpu-runtime.h
  cpu-runtime.cpp)

# vim:sw=2:ts=2:et
