# CMake buildfile for the Firtree compiler parser. 
#
# (C) Copyright 2008 Rich Wareham <richwareham@gmail.com>
# See LICENSE file for distribution rights.

SET(STYX_USE_STATIC_LIBS TRUE)
INCLUDE(UseStyx)

# Generate firtree_{int,pim,lim}.c
ADD_STYX_GRAMMARS(firtree.sty)

SET(STYX_PARSER_HEADERS
  firtree_int.h firtree_pim.h firtree_lim.h
)
SET_SOURCE_FILES_PROPERTIES(${STYX_PARSER_HEADERS} PROPERTIES GENERATED TRUE)

SET(STYX_PARSER_SOURCES
  ${STYX_PARSER_HEADERS}
  firtree_int.c firtree_pim.c firtree_lim.c
)

INCLUDE_DIRECTORIES(${STYX_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FIRTREE_COMPILER_INCLUDE_DIRS})
ADD_LIBRARY(styx-parser ${STYX_PARSER_SOURCES})
TARGET_LINK_LIBRARIES(styx-parser ${STYX_LIBRARIES})

# vim:sw=2:ts=2:et
