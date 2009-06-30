# CMake project file for firtree
#
# Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# Build the internal documentation.
add_subdirectory(internal)

# Build the public documentation.
add_subdirectory(public)

# Add a custom target that builds all the documentation
add_custom_target(doc)
add_dependencies(doc doc-internal doc-firtree)

# vim:sw=4:ts=4:autoindent:et
