# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds the glslang library which contains the 3Dlabs Inc.
# Ltd. GLSL front-end implementation.

# Include support for flex & bison.
INCLUDE(FlexAndBison)

# Specify the C header files for the target.
SET(GLSLANG_HEADERS
	preprocessor/atom.h     preprocessor/parser.h      preprocessor/symbols.h
	preprocessor/compile.h  preprocessor/preprocess.h  preprocessor/tokens.h
	preprocessor/cpp.h      preprocessor/scanner.h
	preprocessor/memory.h   preprocessor/slglobals.h

	Initialize.h         ParseHelper.h     SymbolTable.h
	localintermediate.h  QualifierAlive.h  unistd.h
	MMap.h               RemoveTree.h
)

# Specify the C/C++ source files for the target.
SET(GLSLANG_SOURCES
	${GLSLANG_HEADERS}

	preprocessor/atom.c       preprocessor/memory.c   preprocessor/tokens.c
	preprocessor/cpp.c        preprocessor/scanner.c
	preprocessor/cppstruct.c  preprocessor/symbols.c

	InfoSink.cpp      IntermTraverse.cpp  QualifierAlive.cpp
	Initialize.cpp    parseConst.cpp      RemoveTree.cpp
	Intermediate.cpp  ParseHelper.cpp     ShaderLang.cpp
	intermOut.cpp     PoolAlloc.cpp       SymbolTable.cpp
)

# Add flex and bison sources to generate the parser.
ADD_FLEX_FILES(GLSLANG_SOURCES glslang.lpp)
ADD_BISON_FILES(GLSLANG_SOURCES glslang.ypp)

# Add the front-end header directories to the include path.
INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Public"
	"${CMAKE_CURRENT_SOURCE_DIR}/../OSDependent/Linux"
)

# Add the glslang target.
ADD_LIBRARY(glslang ${GLSLANG_SOURCES})
TARGET_LINK_LIBRARIES(glslang Ossource InitializeDll)
