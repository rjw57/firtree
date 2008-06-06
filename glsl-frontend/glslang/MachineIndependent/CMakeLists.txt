# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the 3dLabs GLSL frontend.

INCLUDE(FlexAndBison)

SET(GLSLANG_HEADERS
	preprocessor/atom.h     preprocessor/parser.h      preprocessor/symbols.h
	preprocessor/compile.h  preprocessor/preprocess.h  preprocessor/tokens.h
	preprocessor/cpp.h      preprocessor/scanner.h
	preprocessor/memory.h   preprocessor/slglobals.h

	Initialize.h         ParseHelper.h     SymbolTable.h
	localintermediate.h  QualifierAlive.h  unistd.h
	MMap.h               RemoveTree.h
)

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

ADD_FLEX_FILES(GLSLANG_SOURCES glslang.lpp)

ADD_BISON_FILES(GLSLANG_SOURCES glslang.ypp)

SET(LIBGLSLANG_INCLUDE_DIRS CACHE 
	${GLSLANG_INCLUDE_DIRS}
	"${CMAKE_CURRENT_SOURCE_DIR}/../Public"
	"${CMAKE_CURRENT_SOURCE_DIR}/../OSDependent/Linux"
)

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/../Public"
	"${CMAKE_CURRENT_SOURCE_DIR}/../OSDependent/Linux"
)

ADD_LIBRARY(glslang ${GLSLANG_SOURCES})
TARGET_LINK_LIBRARIES(glslang Ossource InitializeDll)
