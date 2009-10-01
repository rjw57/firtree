/* firtree-vector.c */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "firtree-vector.h"

#include <string.h>

static GType _firtree_vec2_type = 0;
static GType _firtree_vec3_type = 0;
static GType _firtree_vec4_type = 0;

/**
 * SECTION:firtree-vector
 * @short_description: Vector types used in Firtree.
 * @include: firtree/firtree-vector.h
 *
 * Firtree uses 2, 3 and 4 dimensional vectors as kernel parameters. Firtree
 * also defines appropriate boxing and unboxing code for passing these as
 * parameters for firtree_kernel_set_argument_value(), etc.
 */

/**
 * FirtreeVec2:
 * @x: The x-co-ordinate of the vector.
 * @y: The y-co-ordinate of the vector.
 *
 * A two-dimensional vector.
 */

gpointer _firtree_vec2_copy(gpointer ptr)
{
	FirtreeVec2 *new_vec2 = g_slice_new(FirtreeVec2);
	memcpy(new_vec2, ptr, sizeof(FirtreeVec2));
	return new_vec2;
}

void _firtree_vec2_free(gpointer ptr)
{
	g_slice_free(FirtreeVec2, ptr);
}

/**
 * firtree_vec2_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec2.
 *
 * Returns: A GType
 */
GType firtree_vec2_get_type()
{
	if (0 == _firtree_vec2_type) {
		_firtree_vec2_type =
		    g_boxed_type_register_static("firtree_vec2",
						 _firtree_vec2_copy,
						 _firtree_vec2_free);
	}
	return _firtree_vec2_type;
}

/**
 * FirtreeVec3:
 * @x: The x-co-ordinate of the vector.
 * @y: The y-co-ordinate of the vector.
 * @z: The z-co-ordinate of the vector.
 *
 * A three-dimensional vector.
 */

gpointer _firtree_vec3_copy(gpointer ptr)
{
	FirtreeVec3 *new_vec3 = g_slice_new(FirtreeVec3);
	memcpy(new_vec3, ptr, sizeof(FirtreeVec3));
	return new_vec3;
}

void _firtree_vec3_free(gpointer ptr)
{
	g_slice_free(FirtreeVec3, ptr);
}

/**
 * firtree_vec3_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec3.
 *
 * Returns: A GType
 */
GType firtree_vec3_get_type()
{
	if (0 == _firtree_vec3_type) {
		_firtree_vec3_type =
		    g_boxed_type_register_static("firtree_vec3",
						 _firtree_vec3_copy,
						 _firtree_vec3_free);
	}
	return _firtree_vec3_type;
}

/**
 * FirtreeVec4:
 * @x: The x-co-ordinate of the vector.
 * @y: The y-co-ordinate of the vector.
 * @z: The z-co-ordinate of the vector.
 * @w: The w-co-ordinate of the vector.
 *
 * A four-dimensional vector.
 */

gpointer _firtree_vec4_copy(gpointer ptr)
{
	FirtreeVec4 *new_vec4 = g_slice_new(FirtreeVec4);
	memcpy(new_vec4, ptr, sizeof(FirtreeVec4));
	return new_vec4;
}

void _firtree_vec4_free(gpointer ptr)
{
	g_slice_free(FirtreeVec4, ptr);
}

/**
 * firtree_vec4_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec4.
 *
 * Returns: A GType
 */
GType firtree_vec4_get_type()
{
	if (0 == _firtree_vec4_type) {
		_firtree_vec4_type =
		    g_boxed_type_register_static("firtree_vec4",
						 _firtree_vec4_copy,
						 _firtree_vec4_free);
	}
	return _firtree_vec4_type;
}

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
