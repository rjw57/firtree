# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

set(STYX_USE_STATIC_LIBS TRUE)
include(UseStyx)

# Generate firtree_{int,pim,lim}.c
add_styx_grammars(firtree.sty)

set(STYX_PARSER_HEADERS
    firtree_int.h firtree_pim.h firtree_lim.h
)
set(${STYX_PARSER_HEADERS} PROPERTIES GENERATED TRUE)

set(STYX_PARSER_SOURCES
    ${STYX_PARSER_HEADERS}
    firtree_int.c firtree_pim.c firtree_lim.c
)

if(APPLE)
    set(STYX_LIBRARIES ${STYX_LIBRARIES} -liconv)
endif(APPLE)

include_directories(${STYX_INCLUDE_DIR})
include_directories(${FIRTREE_COMPILER_INCLUDE_DIRS})
add_library(styx-parser ${STYX_PARSER_SOURCES})
target_link_libraries(styx-parser ${STYX_LIBRARIES})

# vim:sw=4:ts=4:autoindent:et
