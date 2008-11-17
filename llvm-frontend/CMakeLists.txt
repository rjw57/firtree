# CMakeLists file for LLVM frontend.

SET(STYX_USE_STATIC_LIBS TRUE)

FIND_PACKAGE(LLVM)
IF(NOT LLVM_FOUND)
  MESSAGE(SEND_ERROR "LLVM required for Firtree.")
ENDIF(NOT LLVM_FOUND)

INCLUDE(UseStyx)

# Generate firtree_{int,pim,lim}.c
ADD_STYX_GRAMMARS(styx/firtree.sty)

SET(STYX_PARSER_HEADERS
  styx/firtree_int.h styx/firtree_pim.h styx/firtree_lim.h
)

SET(STYX_PARSER_SOURCES
  ${STYX_PARSER_HEADERS}
  styx/firtree_int.c styx/firtree_pim.c styx/firtree_lim.c
)

INCLUDE_DIRECTORIES(${STYX_INCLUDE_DIR})
ADD_LIBRARY(styx-parser ${STYX_PARSER_SOURCES})
TARGET_LINK_LIBRARIES(styx-parser ${STYX_LIBRARIES})

SET(LLVM_FRONTEND_HEADERS
  ${STYX_PARSER_HEADERS}
  prodnames.incl
  llvm_frontend.h       llvm_emit_decl.h  llvm_type_cast.h
  llvm_emit_constant.h  llvm_private.h

  compiler.h
)

SET(LLVM_FRONTEND_SOURCES
  ${LLVM_FRONTEND_HEADERS}
  
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

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS -DLLVM_MAJOR_VER=${LLVM_MAJOR} -DLLVM_MINOR_VER=${LLVM_MINOR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
ADD_LIBRARY(llvm-frontend ${LLVM_FRONTEND_SOURCES})
TARGET_LINK_LIBRARIES(llvm-frontend styx-parser ${LLVM_LIBRARIES})

ADD_SUBDIRECTORY(glsl-target)

ADD_EXECUTABLE(compiletest test/kernelcompile.cc)
TARGET_LINK_LIBRARIES(compiletest llvm-frontend glsl-target firtree)

# vim:sw=2:ts=2:et
