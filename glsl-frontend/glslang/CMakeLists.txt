# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# This file builds the 3Dlabs Inc. Ltd. GLSL front-end.

SET(GLSLANG_INCLUDE_DIRS 
	"${GLSLANG_INCLUDE_DIRS}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

# Add the include directory
INCLUDE_DIRECTORIES(${GLSLANG_INCLUDE_DIRS})

# Compile the OS Dependent bits.
ADD_SUBDIRECTORY(OSDependent)

# Compile the generic code gen
ADD_SUBDIRECTORY(GenericCodeGen)

# Compile the actual library
ADD_SUBDIRECTORY(MachineIndependent)
