/* 
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * libcga is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libcga is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/* Prevent multiple inclusion */
#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/** 
 * A test program should implement the following functions
 */

/* Initialise any test-specific stuff */
void initialise_test(int *argc, char **argv, int window);
/* Finalise (destroy) any test-specific stuff */
void finalise_test();
/* Render a single frame of the test, time is the time epoch of the
 * frame (monotonically increaes with time) */
void render(float epoch);
/* Called when the OpenGL context has been created */
void context_created();
/* Called when a key has been pressed */
void key_pressed(unsigned char key, int x, int y);

/* UTILITIES */
bool InitialiseTextureFromFile(unsigned int texObj, const char* pFileName);


#define PI 3.1412
#define FALSE 0
#define TRUE 1

extern int mainWindow;
extern int width, height;

#endif /* __COMMON_H */
