# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Grammar parsing
SET(GRAMMAR_HEADERS
	grammar/grammar_crt.h 
	grammar/grammar.h 
	grammar/grammar_syn.h
	
	# C files included by others
	grammar/grammar.c
)

SET(GRAMMAR_SOURCES
	grammar/grammar_crt.c 
)

# Kernel language parser
SET(SLANG_HEADERS
	slang/slang_builtin.h
	slang/slang_codegen.h
	slang/slang_compile_function.h
	slang/slang_compile.h
	slang/slang_compile_operation.h
	slang/slang_compile_struct.h
	slang/slang_compile_variable.h
	slang/slang_emit.h
	slang/slang_ir.h
	slang/slang_label.h
	slang/slang_library_noise.h
	slang/slang_link.h
	slang/slang_log.h
	slang/slang_mem.h
	slang/slang_preprocess.h
	slang/slang_print.h
	slang/slang_simplify.h
	slang/slang_storage.h
	slang/slang_typeinfo.h
	slang/slang_utility.h
	slang/slang_vartable.h
)

SET(SLANG_SOURCES
    slang/slang_builtin.c
    slang/slang_codegen.c
    slang/slang_compile.c
    slang/slang_compile_function.c
    slang/slang_compile_operation.c
    slang/slang_compile_struct.c
    slang/slang_compile_variable.c
    # slang/slang_emit.c
    slang/slang_ir.c
    slang/slang_label.c
    slang/slang_library_noise.c
    # slang/slang_link.c
    slang/slang_log.c
    slang/slang_mem.c
    slang/slang_preprocess.c
    slang/slang_print.c
    slang/slang_simplify.c
    slang/slang_storage.c
    slang/slang_typeinfo.c
    slang/slang_utility.c
    slang/slang_vartable.c
)

SET(MATH_SOURCES
    mesa/math/m_matrix.h       mesa/math/m_matrix.c
)

SET(KERNEL_PARSE_INCLUDES
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/grammar
	${CMAKE_CURRENT_SOURCE_DIR}/mesa
	${CMAKE_CURRENT_SOURCE_DIR}/mesa/main
	${CMAKE_CURRENT_SOURCE_DIR}/mesa/glapi
	${CMAKE_CURRENT_SOURCE_DIR}/mesa/shader
)

INCLUDE_DIRECTORIES(${KERNEL_PARSE_INCLUDES})

SET(KERNEL_PARSE_SOURCES
    mesa/main/version.h mesa/main/config.h
    mesa/main/glheader.h mesa/main/mtypes.h
    mesa/main/bitset.h mesa/main/macros.h
    mesa/main/dd.h
    mesa/main/imports.c mesa/main/imports.h

	${MATH_SOURCES}
	${GRAMMAR_SOURCES}
	${SLANG_SOURCES}
)

ADD_LIBRARY(kparse STATIC ${KERNEL_PARSE_SOURCES})
SET_TARGET_PROPERTIES(kparse PROPERTIES INCLUDE_DIRS "${KERNEL_PARSE_INCLUDES}")

# Vim modeline
# vim:ts=4:sw=4:et
