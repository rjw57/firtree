# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

add_library(common
    lock-free.h     lock-free.c
    system-info.h   system-info.c
    threading.h     threading.c
    uuid.h          uuid.cpp
    common.h        common.cpp)

target_link_libraries(common ${GLIB_LIBRARIES})

# vim:sw=4:ts=4:autoindent:et

