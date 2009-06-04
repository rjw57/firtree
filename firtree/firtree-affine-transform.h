/* firtree-affine-transform.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.        See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA        02110-1301, USA
 */

#ifndef _FIRTREE_AFFINE_TRANSFORM
#define _FIRTREE_AFFINE_TRANSFORM

#include <glib-object.h>

#include "firtree-vector.h"

/**
 * SECTION:firtree-affine-transform
 * @short_description: 2D affine transform support.
 * @include: firtree/firtree-affine-transform.h
 *
 * A FirtreeAffineTransform encapsulates a 2D affine transform.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_AFFINE_TRANSFORM firtree_affine_transform_get_type()

#define FIRTREE_AFFINE_TRANSFORM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransform))

#define FIRTREE_AFFINE_TRANSFORM_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransformClass))

#define FIRTREE_IS_AFFINE_TRANSFORM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM))

#define FIRTREE_IS_AFFINE_TRANSFORM_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_AFFINE_TRANSFORM))

#define FIRTREE_AFFINE_TRANSFORM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransformClass))

/**
 * FirtreeAffineTransform:
 * @m11: The (1,1) element of the transform matrix.
 * @m12: The (1,2) element of the transform matrix.
 * @m21: The (2,1) element of the transform matrix.
 * @m22: The (2,2) element of the transform matrix.
 * @tx: The translation along x.
 * @ty: The translation along y.
 *
 * A FirtreeAffineTransform encapsulates a 2D affine transform. An affine
 * transform is one of the form
 *
 * (x, y) -> (m11*x + m12*y + tx, m21*x + m22*y + ty).
 */
typedef struct {
    GObject parent;
    float m11, m12;
    float m21, m22;
    float tx,  ty;
} FirtreeAffineTransform;

typedef struct {
    GObjectClass parent_class;
} FirtreeAffineTransformClass;

GType firtree_affine_transform_get_type (void);

/**
 * firtree_affine_transform_new:
 *
 * Create a new affine transform representing the identity
 * transform (x,y) -> (x,y).
 *
 * Returns: A newly instantiated FirtreeAffineTransform.
 */
FirtreeAffineTransform* 
firtree_affine_transform_new (void);

/**
 * firtree_affine_transform_clone:
 * @self: An affine transform.
 *
 * Create a new affine transform and copy the value of @self into it.
 * Return the new transform.
 *
 * Returns: A newly instantiated FirtreeAffineTransform.
 */
FirtreeAffineTransform* 
firtree_affine_transform_clone (FirtreeAffineTransform* self);

/**
 * firtree_affine_transform_is_identity:
 * @self: An affine transform.
 *
 * Determine if @self represents the identity transform.
 *
 * Returns: TRUE is @self is an identity transform, FALSE otherwise.
 */
gboolean
firtree_affine_transform_is_identity (FirtreeAffineTransform* self);

/**
 * firtree_affine_transform_append_transform:
 * @self: An affine transform.
 * @trans: An affine transform.
 *
 * Modify @self so that it represents the transform @self followed by @trans.
 */
void
firtree_affine_transform_append_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans);

/**
 * firtree_affine_transform_append_transform:
 * @self: An affine transform.
 * @trans: An affine transform.
 *
 * Modify @self so that it represents the transform @trans followed by @self.
 */
void
firtree_affine_transform_prepend_transform (FirtreeAffineTransform* self,
        FirtreeAffineTransform* trans);

/**
 * firtree_affine_transform_invert:
 * @self: An affine transform.
 *
 * Is @self is an invertable transform, modify it so that it becomes its
 * inverse.
 *
 * Returns: TRUE if @self is invertable, FALSE otherwise.
 */
gboolean
firtree_affine_transform_invert (FirtreeAffineTransform* self);

void
firtree_affine_transform_rotate_by_degrees (FirtreeAffineTransform* self,
        float angle);

void
firtree_affine_transform_rotate_by_radians (FirtreeAffineTransform* self,
        float angle);

void
firtree_affine_transform_scale_by (FirtreeAffineTransform* self, 
        float sx, float sy);

void
firtree_affine_transform_translate_by (FirtreeAffineTransform* self, 
        float tx, float ty);

FirtreeVec2
firtree_affine_transform_transform_point (FirtreeAffineTransform* self,
        float x, float y);

FirtreeVec2
firtree_affine_transform_transform_size (FirtreeAffineTransform* self,
        float width, float height);

void
firtree_affine_transform_set_identity (FirtreeAffineTransform* self);

void
firtree_affine_transform_set_rotation_by_degrees (FirtreeAffineTransform* self,
        float angle);

void
firtree_affine_transform_set_rotation_by_radians (FirtreeAffineTransform* self,
        float angle);

void
firtree_affine_transform_set_scaling_by (FirtreeAffineTransform* self, 
        float sx, float sy);

void
firtree_affine_transform_set_translation_by (FirtreeAffineTransform* self, 
        float tx, float ty);

/**
 * firtree_affine_transform_set_elements:
 * @self: An affine transform.
 * @m11: The (1,1) element of the transform.
 * @m12: The (1,2) element of the transform.
 * @m21: The (2,1) element of the transform.
 * @m22: The (2,2) element of the transform.
 * @tx: The translation along x.
 * @ty: The translation along y.
 *
 * Set the transform @self so that it represents the transform
 *
 * (x,y) -> ( @m11 * x + @m12 * y + @tx , @m21 * x + @m22 * y + @ty )
 */
void
firtree_affine_transform_set_elements (FirtreeAffineTransform* self, 
        float m11, float m12, float m21, float m22, float tx, float ty);

/**
 * firtree_affine_transform_get_elements:
 * @self: An affine transform.
 * @m11: A pointer to a float to receive the (1,1) element of the transform.
 * @m12: A pointer to a float to receive the (1,2) element of the transform.
 * @m21: A pointer to a float to receive the (2,1) element of the transform.
 * @m22: A pointer to a float to receive the (2,2) element of the transform.
 * @tx: A pointer to a float to receive the translation along x.
 * @ty: A pointer to a float to receive the translation along y.
 *
 * Retrieve the elements of the transform @self.
 */
void
firtree_affine_transform_get_elements (FirtreeAffineTransform* self, 
        float* m11, float* m12, float* m21, float* m22, float* tx, float* ty);

/**
 * firtree_affine_transform_set_transform:
 * @self: An affine transform.
 * @src: An affine transform.
 * 
 * Modify @self so that it is identical to the transform @src.
 */
void
firtree_affine_transform_set_transform (FirtreeAffineTransform* self, 
        FirtreeAffineTransform* src);

G_END_DECLS

#endif /* _FIRTREE_AFFINE_TRANSFORM */

/* vim:sw=4:ts=4:et:cindent
 */
