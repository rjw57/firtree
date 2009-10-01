/* firtree-vector.h */

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

#ifndef __FIRTREE_VECTOR_H__
#define __FIRTREE_VECTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef	struct	_FirtreeVec2	FirtreeVec2;
typedef	struct	_FirtreeVec3	FirtreeVec3;
typedef	struct	_FirtreeVec4	FirtreeVec4;

struct _FirtreeVec2
{
	float 	x;
	float	y;
};

GType firtree_vec2_get_type();

/**
 * FIRTREE_TYPE_VEC2:
 *
 * The GType for the boxed version of FirtreeVec2.
 */
#define FIRTREE_TYPE_VEC2 (firtree_vec2_get_type())

struct _FirtreeVec3
{
	float 	x;
	float	y;
	float	z;
};

GType firtree_vec3_get_type();

/**
 * FIRTREE_TYPE_VEC3:
 *
 * The GType for the boxed version of FirtreeVec3.
 */
#define FIRTREE_TYPE_VEC3 (firtree_vec3_get_type())

struct _FirtreeVec4
{
	float 	x;
	float	y;
	float	z;
	float	w;
};

GType firtree_vec4_get_type();

/**
 * FIRTREE_TYPE_VEC4:
 *
 * The GType for the boxed version of FirtreeVec4.
 */
#define FIRTREE_TYPE_VEC4 (firtree_vec4_get_type())

G_END_DECLS
#endif				/* __FIRTREE_VECTOR_H__ */
/* vim:sw=8:ts=8:noet:cindent
 */
