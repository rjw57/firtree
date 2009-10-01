/* firtree-affine-transform.c */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
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

#include "firtree-affine-transform.h"

#include <math.h>

/**
 * SECTION:firtree-affine-transform
 * @short_description: 2D affine transform support.
 * @include: firtree/firtree-affine-transform.h
 *
 * A FirtreeAffineTransform encapsulates a 2D affine transform.
 */

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

G_DEFINE_TYPE(FirtreeAffineTransform, firtree_affine_transform, G_TYPE_OBJECT)

static void
firtree_affine_transform_class_init(FirtreeAffineTransformClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
}

static void firtree_affine_transform_init(FirtreeAffineTransform *self)
{
	firtree_affine_transform_set_identity(self);
}

/**
 * firtree_affine_transform_new:
 *
 * Create a new affine transform representing the identity
 * transform (x,y) -> (x,y).
 *
 * Returns: A newly instantiated FirtreeAffineTransform.
 */
FirtreeAffineTransform *firtree_affine_transform_new(void)
{
	return g_object_new(FIRTREE_TYPE_AFFINE_TRANSFORM, NULL);
}

/**
 * firtree_affine_transform_clone:
 * @self: An affine transform.
 *
 * Create a new affine transform and copy the value of @self into it.
 * Return the new transform.
 *
 * Returns: A newly instantiated FirtreeAffineTransform.
 */
FirtreeAffineTransform *firtree_affine_transform_clone(FirtreeAffineTransform *
						       self)
{
	FirtreeAffineTransform *new_trans = firtree_affine_transform_new();
	firtree_affine_transform_set_transform(new_trans, self);
	return new_trans;
}

/**
 * firtree_affine_transform_is_identity:
 * @self: An affine transform.
 *
 * Determine if @self represents the identity transform.
 *
 * Returns: TRUE is @self is an identity transform, FALSE otherwise.
 */
gboolean firtree_affine_transform_is_identity(FirtreeAffineTransform *self)
{
	if ((self->m11 == 1.f) && (self->m12 == 0.f) &&
	    (self->m21 == 0.f) && (self->m22 == 1.f) &&
	    (self->tx == 0.f) && (self->ty == 0.f)) {
		return TRUE;
	}
	return FALSE;
}

/* Computes c <- a*b */
static void
_firtree_affine_transform_multiply(FirtreeAffineTransform *a,
				   FirtreeAffineTransform *b,
				   FirtreeAffineTransform *c)
{
	float m11 = (a->m11 * b->m11) + (a->m12 * b->m21);
	float m12 = (a->m11 * b->m12) + (a->m12 * b->m22);
	float m21 = (a->m21 * b->m11) + (a->m22 * b->m21);
	float m22 = (a->m21 * b->m12) + (a->m22 * b->m22);
	float tx = (a->m11 * b->tx) + (a->m12 * b->ty) + a->tx;
	float ty = (a->m21 * b->tx) + (a->m22 * b->ty) + a->ty;
	firtree_affine_transform_set_elements(c, m11, m12, m21, m22, tx, ty);
}

/**
 * firtree_affine_transform_append_transform:
 * @self: An affine transform.
 * @trans: An affine transform.
 *
 * Modify @self so that it represents the transform @self followed by @trans.
 */
void
firtree_affine_transform_append_transform(FirtreeAffineTransform *self,
					  FirtreeAffineTransform *trans)
{
	/* self <- trans * self */
	_firtree_affine_transform_multiply(trans, self, self);
}

/**
 * firtree_affine_transform_prepend_transform:
 * @self: An affine transform.
 * @trans: An affine transform.
 *
 * Modify @self so that it represents the transform @trans followed by @self.
 */
void
firtree_affine_transform_prepend_transform(FirtreeAffineTransform *self,
					   FirtreeAffineTransform *trans)
{
	/* self <- self * trans */
	_firtree_affine_transform_multiply(self, trans, self);
}

/**
 * firtree_affine_transform_invert:
 * @self: An affine transform.
 *
 * Is @self is an invertable transform, modify it so that it becomes its
 * inverse.
 *
 * Returns: TRUE if @self is invertable, FALSE otherwise.
 */
gboolean firtree_affine_transform_invert(FirtreeAffineTransform *self)
{
	float d = (self->m11 * self->m22) - (self->m12 * self->m21);
	if (d == 0.f) {
		return FALSE;
	}
	float ood = 1.f / d;

	float m11 = ood * self->m22;
	float m22 = ood * self->m11;
	float m12 = ood * -self->m12;
	float m21 = ood * -self->m21;

	float tx = ood * (self->m12 * self->ty - self->m22 * self->tx);
	float ty = ood * (self->m21 * self->tx - self->m11 * self->ty);

	firtree_affine_transform_set_elements(self, m11, m12, m21, m22, tx, ty);

	return TRUE;
}

/**
 * firtree_affine_transform_rotate_by_degrees:
 * @self: An affine transform.
 * @angle: An angle in degrees.
 *
 * Modifies @self so that it represents the transform @self *followed* by
 * a rotation about the origin of @angle degrees.
 */
void
firtree_affine_transform_rotate_by_degrees(FirtreeAffineTransform *self,
					   float angle)
{
	FirtreeAffineTransform *tmp_trans = firtree_affine_transform_new();
	firtree_affine_transform_set_rotation_by_degrees(tmp_trans, angle);
	firtree_affine_transform_append_transform(self, tmp_trans);
	g_object_unref(tmp_trans);
}

/**
 * firtree_affine_transform_rotate_by_radians:
 * @self: An affine transform.
 * @angle: An angle in radians.
 *
 * Modifies @self so that it represents the transform @self *followed* by
 * a rotation about the origin of @angle radians.
 */
void
firtree_affine_transform_rotate_by_radians(FirtreeAffineTransform *self,
					   float angle)
{
	FirtreeAffineTransform *tmp_trans = firtree_affine_transform_new();
	firtree_affine_transform_set_rotation_by_radians(tmp_trans, angle);
	firtree_affine_transform_append_transform(self, tmp_trans);
	g_object_unref(tmp_trans);
}

/**
 * firtree_affine_transform_scale_by:
 * @self: An affine transform.
 * @sx: Scale factor in the x-direction.
 * @sy: Scale factor in the y-direction.
 *
 * Modifies @self so that it represents the transform @self *followed* by
 * a scaling about the origin of @sx along the x-direction and @sy along
 * the y-direction.
 */
void
firtree_affine_transform_scale_by(FirtreeAffineTransform *self,
				  float sx, float sy)
{
	FirtreeAffineTransform *tmp_trans = firtree_affine_transform_new();
	firtree_affine_transform_set_scaling_by(tmp_trans, sx, sy);
	firtree_affine_transform_append_transform(self, tmp_trans);
	g_object_unref(tmp_trans);
}

/**
 * firtree_affine_transform_translate_by:
 * @self: An affine transform.
 * @tx: Offset along the x-direction.
 * @ty: Offset along the y-direction.
 *
 * Modifies @self so that it represents the transform @self *followed* by
 * a translation of @tx along the x-direction and @ty along the y-direction.
 */
void
firtree_affine_transform_translate_by(FirtreeAffineTransform *self,
				      float tx, float ty)
{
	FirtreeAffineTransform *tmp_trans = firtree_affine_transform_new();
	firtree_affine_transform_set_translation_by(tmp_trans, tx, ty);
	firtree_affine_transform_append_transform(self, tmp_trans);
	g_object_unref(tmp_trans);
}

/**
 * firtree_affine_transform_transform_point:
 * @self: An affine transform.
 * @x: The x-co-ordinate of the point to transform.
 * @y: The y-co-ordinate of the point to transform.
 *
 * Performs the mapping
 *
 * (x,y) -> (m11*x+m21*y+tx, m21*x+m22*y+ty)
 *
 * This effectively 'applies' the transform to the point.
 *
 * Returns: A FirtreeVec2 representing the transformed point.
 */
FirtreeVec2
firtree_affine_transform_transform_point(FirtreeAffineTransform *self,
					 float x, float y)
{
	FirtreeVec2 vec = {
		self->m11 * x + self->m12 * y + self->tx,
		self->m21 * x + self->m22 * y + self->ty,
	};
	return vec;
}

/**
 * firtree_affine_transform_transform_size:
 * @self: An affine transform.
 * @width: The width of the size to transform.
 * @height: The height of the size to transform.
 *
 * Performs the mapping
 *
 * (x,y) -> (m11*width+m21*height, m21*width+m22*height)
 *
 * This effectively 'applies' the transform to the size. This is
 * the same as applying the transform to a point but without the translational
 * component.
 *
 * Returns: A FirtreeVec2 representing the transformed size.
 */
FirtreeVec2
firtree_affine_transform_transform_size(FirtreeAffineTransform *self,
					float width, float height)
{
	FirtreeVec2 vec = {
		self->m11 * width + self->m12 * height,
		self->m21 * width + self->m22 * height,
	};
	return vec;
}

/**
 * firtree_affine_transform_set_identity:
 * @self: An affine transform.
 *
 * Set the elements of @self so that it represents the identity transform
 * (i.e. one which maps a point to itself, (x,y) -> (x,y)).
 */
void firtree_affine_transform_set_identity(FirtreeAffineTransform *self)
{
	firtree_affine_transform_set_elements(self, 1, 0, 0, 1, 0, 0);
}

/**
 * firtree_affine_transform_set_rotation_by_degrees:
 * @self: An affine transform.
 * @angle: An angle in degrees.
 *
 * Set the elements of @self so that it represents a rotation anti-clockwise
 * about the origin by @angle degrees.
 */
void
firtree_affine_transform_set_rotation_by_degrees(FirtreeAffineTransform *self,
						 float angle)
{
	const float deg2rad = 2.f * G_PI / 360.f;
	firtree_affine_transform_set_rotation_by_radians(self, angle * deg2rad);
}

/**
 * firtree_affine_transform_set_rotation_by_radians:
 * @self: An affine transform.
 * @angle: An angle in radians.
 *
 * Set the elements of @self so that it represents a rotation anti-clockwise
 * about the origin by @angle radians.
 */
void
firtree_affine_transform_set_rotation_by_radians(FirtreeAffineTransform *self,
						 float angle)
{
	firtree_affine_transform_set_identity(self);
	float s = sin(angle);
	float c = cos(angle);
	self->m11 = c;
	self->m12 = -s;
	self->m21 = s;
	self->m22 = c;
}

/**
 * firtree_affine_transform_set_scaling_by:
 * @self: An affine transform.
 * @sx: Scale factor in the x-direction.
 * @sy: Scale factor in the y-direction.
 *
 * Set the elements of @self so that it represents a scaling about the origin
 * of @sx in the x-direction and @sy in the y-direction.
 */
void
firtree_affine_transform_set_scaling_by(FirtreeAffineTransform *self,
					float sx, float sy)
{
	firtree_affine_transform_set_identity(self);
	self->m11 = sx;
	self->m22 = sy;
}

/**
 * firtree_affine_transform_set_translation_by:
 * @self: An affine transform.
 * @tx: Translation in the x-direction.
 * @ty: Translation in the y-direction.
 *
 * Set the elements of @self so that it represents a translation of
 * @tx along the x-direction and @ty along the y-direction.
 */
void
firtree_affine_transform_set_translation_by(FirtreeAffineTransform *self,
					    float tx, float ty)
{
	firtree_affine_transform_set_identity(self);
	self->tx = tx;
	self->ty = ty;
}

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
firtree_affine_transform_set_elements(FirtreeAffineTransform *self,
				      float m11, float m12, float m21,
				      float m22, float tx, float ty)
{
	self->m11 = m11;
	self->m12 = m12;
	self->m21 = m21;
	self->m22 = m22;
	self->tx = tx;
	self->ty = ty;
}

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
firtree_affine_transform_get_elements(FirtreeAffineTransform *self,
				      float *m11, float *m12, float *m21,
				      float *m22, float *tx, float *ty)
{
	*m11 = self->m11;
	*m12 = self->m12;
	*m21 = self->m21;
	*m22 = self->m22;
	*tx = self->tx;
	*ty = self->ty;
}

/**
 * firtree_affine_transform_set_transform:
 * @self: An affine transform.
 * @src: An affine transform.
 * 
 * Modify @self so that it is identical to the transform @src.
 */
void
firtree_affine_transform_set_transform(FirtreeAffineTransform *self,
				       FirtreeAffineTransform *src)
{
	self->m11 = src->m11;
	self->m12 = src->m12;
	self->m21 = src->m21;
	self->m22 = src->m22;
	self->tx = src->tx;
	self->ty = src->ty;
}

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
