# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Build the parser
add_subdirectory(styx-parser)

set(LLVM_CONFIG_COMPONENTS ipo instrumentation)

# We need the styx libraries and LLVM
find_package(Styx)
find_package(LLVM)

# Moan if we can't find our dependencies
if(NOT LLVM_FOUND)
    message(SEND_ERROR "LLVM is required.")
endif(NOT LLVM_FOUND)
if(NOT STYX_FOUND)
    message(SEND_ERROR "Styx is required.")
endif(NOT STYX_FOUND)

set(LLVM_FRONTEND_HEADERS
  prodnames.incl
  llvm-frontend.h       llvm-emit-decl.h  llvm-type-cast.h
  llvm-emit-constant.h  llvm-private.h
  llvm-compiled-kernel.h
)

set(LLVM_FRONTEND_SOURCES
  ${LLVM_FRONTEND_HEADERS}

  llvm-compiled-kernel.cc     llvm-emit-return.cc
  llvm-emit-binop-arith.cc    llvm-emit-selection.cc
  llvm-emit-binop-assign.cc   llvm-emit-swizzle.cc
  llvm-emit-binop-cmp.cc      llvm-emit-unary-incdec.cc
  llvm-emit-binop-logic.cc    llvm-emit-variable.cc
  llvm-emit-constant.cc       llvm-emit-variable-declaration.cc
  llvm-emit-decl.cc           llvm-expression.cc
  llvm-emit-expr-list.cc      llvm-frontend.cc
  llvm-emit-function-call.cc  llvm-type-cast.cc
  llvm-emit-loop.cc           symbol-table.cc
  llvm-emit-negate.cc

  ${LLVM_STATIC_OBJS}
)

include_directories(${STYX_INCLUDE_DIR})
include_directories(${LLVM_INCLUDE_DIR})
include_directories(${FIRTREE_COMPILER_INCLUDE_DIRS})

add_definitions("-D__STDC_LIMIT_MACROS")
add_definitions("-DLLVM_MAJOR_VER=${LLVM_MAJOR}" "-DLLVM_MINOR_VER=${LLVM_MINOR}")

add_library(llvm-frontend ${LLVM_FRONTEND_SOURCES})

target_link_libraries(llvm-frontend
    styx-parser common 
    -ldl
    ${GLIB_LIBRARIES}
    ${LLVM_STATIC_LIBS})

# vim:sw=4:ts=4:autoindent:et

