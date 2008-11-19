# CMake buildfile for the Firtree compiler LLVM code generator. 
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

SET(LLVM_CODE_GEN_HEADERS
  prodnames.incl
  llvm_frontend.h       llvm_emit_decl.h  llvm_type_cast.h
  llvm_emit_constant.h  llvm_private.h

  compiler.h
)

SET(LLVM_CODE_GEN_SOURCES
  ${LLVM_CODE_GEN_HEADERS}
  
  llvm_emit_binop_arith.cc    llvm_emit_return.cc
  llvm_emit_binop_assign.cc   llvm_emit_swizzle.cc
  llvm_emit_binop_cmp.cc      llvm_emit_unary_incdec.cc
  llvm_emit_binop_logic.cc    llvm_emit_variable.cc
  llvm_emit_constant.cc       llvm_emit_variable_declaration.cc
  llvm_emit_decl.cc           llvm_expression.cc
  llvm_emit_expr_list.cc      llvm_frontend.cc
  llvm_emit_function_call.cc  llvm_type_cast.cc
  llvm_emit_negate.cc         symbol_table.cc

  compiler.cc
)

INCLUDE_DIRECTORIES(${STYX_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FIRTREE_COMPILER_INCLUDE_DIRS})

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
ADD_DEFINITIONS(-DLLVM_MAJOR_VER=${LLVM_MAJOR} -DLLVM_MINOR_VER=${LLVM_MINOR})

ADD_LIBRARY(llvm-code-gen ${LLVM_CODE_GEN_SOURCES})

TARGET_LINK_LIBRARIES(llvm-code-gen styx-parser ${LLVM_LIBRARIES})

ADD_DEPENDENCIES(llvm-code-gen styx-parser)

# vim:sw=2:ts=2:et
