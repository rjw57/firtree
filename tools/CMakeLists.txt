# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

if(NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(CAIRO cairo)
    pkg_check_modules(CLUTTER clutter-1.0)
    if(NOT CLUTTER_FOUND)
        pkg_check_modules(CLUTTER clutter-0.8)
    endif(NOT CLUTTER_FOUND)
endif(NOT APPLE)

include_directories(${CLUTTER_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})

# The simple kernel compiler.
add_executable(kernel2llvm kernel2llvm.cc)
target_link_libraries(kernel2llvm firtree
    ${CLUTTER_LIBRARIES} ${CAIRO_LIBRARIES})

# vim:sw=4:ts=4:autoindent:et

