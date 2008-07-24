# CMake project file for firtree
#
# Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
#
# See LICENSE file for distribution rights.

# CMake file to build the various third-party utilities

SET(SELOG_SOURCES
	selog/selog_api.c  
	selog/selog_unix.c   
	selog/selog.c     
	selog/selog.h    
)

# Add the selog target.
ADD_DEFINITIONS( -std=gnu99 )
ADD_LIBRARY(selog ${SELOG_SOURCES})
