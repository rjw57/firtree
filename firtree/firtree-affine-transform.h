/* firtree-affine-transform.h */

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

#ifndef __FIRTREE_AFFINE_TRANSFORM_H__
#define __FIRTREE_AFFINE_TRANSFORM_H__

#include <glib-object.h>
#include <firtree/firtree-vector.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_AFFINE_TRANSFORM			firtree_affine_transform_get_type()
#define FIRTREE_AFFINE_TRANSFORM(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransform))
#define FIRTREE_AFFINE_TRANSFORM_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransformClass))
#define FIRTREE_IS_AFFINE_TRANSFORM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM))
#define FIRTREE_IS_AFFINE_TRANSFORM_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_AFFINE_TRANSFORM))
#define FIRTREE_AFFINE_TRANSFORM_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_AFFINE_TRANSFORM, FirtreeAffineTransformClass))

typedef struct _FirtreeAffineTransform FirtreeAffineTransform;
typedef struct _FirtreeAffineTransformClass FirtreeAffineTransformClass;

struct _FirtreeAffineTransform
{
	GObject parent;
	float m11, m12;
	float m21, m22;
	float tx, ty;
};

struct _FirtreeAffineTransformClass 
{
	GObjectClass parent_class;
};

GType		 firtree_affine_transform_get_type	(void) G_GNUC_CONST;

FirtreeAffineTransform	*firtree_affine_transform_new	(void);

FirtreeAffineTransform	*firtree_affine_transform_clone	(FirtreeAffineTransform	*self);

gboolean	 firtree_affine_transform_is_identity	(FirtreeAffineTransform	*self);

void		 firtree_affine_transform_append_transform	
							(FirtreeAffineTransform	*self,
							 FirtreeAffineTransform	*trans);

void		 firtree_affine_transform_prepend_transform
							(FirtreeAffineTransform	*self,
							 FirtreeAffineTransform	*trans);

gboolean	 firtree_affine_transform_invert	(FirtreeAffineTransform	*self);

void		 firtree_affine_transform_rotate_by_degrees
							(FirtreeAffineTransform	*self,
							 float 			 angle);

void		 firtree_affine_transform_rotate_by_radians
							(FirtreeAffineTransform	*self,
							 float 			 angle);

void		 firtree_affine_transform_scale_by
							(FirtreeAffineTransform *self,
							 float			 sx,
							 float			 sy);

void		 firtree_affine_transform_translate_by	(FirtreeAffineTransform	*self,
							 float			 tx,
							 float			 ty);

FirtreeVec2	 firtree_affine_transform_transform_point
							(FirtreeAffineTransform *self,
							 float			 x,
							 float			 y);

FirtreeVec2	 firtree_affine_transform_transform_size
							(FirtreeAffineTransform *self,
							 float			 width,
							 float			 height);

void		 firtree_affine_transform_set_identity	(FirtreeAffineTransform *self);

void		 firtree_affine_transform_set_rotation_by_degrees
							(FirtreeAffineTransform *self,
							 float			 angle);

void		 firtree_affine_transform_set_rotation_by_radians
							(FirtreeAffineTransform *self,
							 float			 angle);

void		 firtree_affine_transform_set_scaling_by
							(FirtreeAffineTransform *self,
							 float			 sx,
							 float			 sy);

void		 firtree_affine_transform_set_translation_by
							(FirtreeAffineTransform *self,
							 float			 tx,
							 float			 ty);

void		 firtree_affine_transform_set_elements	(FirtreeAffineTransform *self,
							 float			 m11,
							 float			 m12,
							 float			 m21,
							 float			 m22,
							 float			 tx,
							 float			 ty);

void		 firtree_affine_transform_get_elements	(FirtreeAffineTransform *self,
							 float			*m11, 
							 float			*m12, 
							 float 			*m21,
							 float 			*m22, 
							 float 			*tx, 
							 float 			*ty);

void		 firtree_affine_transform_set_transform	(FirtreeAffineTransform *self,
							 FirtreeAffineTransform *src);

G_END_DECLS

#endif		/* __FIRTREE_AFFINE_TRANSFORM_H__ */

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
