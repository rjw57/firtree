/* firtree.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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

#ifndef _FIRTREE_VECTOR
#define _FIRTREE_VECTOR

#include <glib-object.h>

G_BEGIN_DECLS

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
typedef struct {
    float x, y;
} FirtreeVec2;

/**
 * firtree_vec2_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec2.
 *
 * Returns: A GType
 */
GType firtree_vec2_get_type();

/**
 * FIRTREE_TYPE_VEC2:
 *
 * The GType for the boxed version of FirtreeVec2.
 */
#define FIRTREE_TYPE_VEC2 (firtree_vec2_get_type())

/**
 * FirtreeVec3:
 * @x: The x-co-ordinate of the vector.
 * @y: The y-co-ordinate of the vector.
 * @z: The z-co-ordinate of the vector.
 *
 * A three-dimensional vector.
 */
typedef struct {
    float x, y, z;
} FirtreeVec3;


/**
 * firtree_vec3_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec3.
 *
 * Returns: A GType
 */
GType firtree_vec3_get_type();

/**
 * FIRTREE_TYPE_VEC3:
 *
 * The GType for the boxed version of FirtreeVec3.
 */
#define FIRTREE_TYPE_VEC3 (firtree_vec3_get_type())

/**
 * FirtreeVec4:
 * @x: The x-co-ordinate of the vector.
 * @y: The y-co-ordinate of the vector.
 * @z: The z-co-ordinate of the vector.
 * @w: The w-co-ordinate of the vector.
 *
 * A four-dimensional vector.
 */
typedef struct {
    float x, y, z, w;
} FirtreeVec4;

/**
 * firtree_vec4_get_type:
 *
 * Retrieve the GType for the boxed version of FirtreeVec4.
 *
 * Returns: A GType
 */
GType firtree_vec4_get_type();

/**
 * FIRTREE_TYPE_VEC4:
 *
 * The GType for the boxed version of FirtreeVec4.
 */
#define FIRTREE_TYPE_VEC4 (firtree_vec4_get_type())

G_END_DECLS

#endif /* _FIRTREE_VECTOR */

/* vim:sw=4:ts=4:et:cindent
 */
