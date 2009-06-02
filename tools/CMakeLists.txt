# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# The simple kernel compiler.
add_executable(kernel2llvm kernel2llvm.cc)
target_link_libraries(kernel2llvm llvm-frontend)

# vim:sw=4:ts=4:autoindent:et

