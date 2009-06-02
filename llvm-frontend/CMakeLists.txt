# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Build the parser
add_subdirectory(styx-parser)

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
  llvm_frontend.h       llvm_emit_decl.h  llvm_type_cast.h
  llvm_emit_constant.h  llvm_private.h
  llvm-compiled-kernel.h
)

set(LLVM_FRONTEND_SOURCES
  ${LLVM_FRONTEND_HEADERS}
  
  llvm_emit_binop_arith.cc    llvm_emit_return.cc
  llvm_emit_binop_assign.cc   llvm_emit_swizzle.cc
  llvm_emit_binop_cmp.cc      llvm_emit_unary_incdec.cc
  llvm_emit_binop_logic.cc    llvm_emit_variable.cc
  llvm_emit_constant.cc       llvm_emit_variable_declaration.cc
  llvm_emit_decl.cc           llvm_expression.cc
  llvm_emit_expr_list.cc      llvm_frontend.cc
  llvm_emit_function_call.cc  llvm_type_cast.cc
  llvm_emit_selection.cc      llvm_emit_loop.cc
  llvm_emit_negate.cc         symbol_table.cc

  llvm_compiled_kernel.cc
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
    ${LLVM_LIBRARIES} ${GLIB_LIBRARIES})

# vim:sw=4:ts=4:autoindent:et

