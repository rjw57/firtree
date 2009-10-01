/* firtree-sampler.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.    See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA    02110-1301, USA
 */

#ifndef __FIRTREE_SAMPLER_H__
#define __FIRTREE_SAMPLER_H__

#include <glib-object.h>

#include <firtree/firtree-affine-transform.h>
#include <firtree/firtree-vector.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_SAMPLER 		firtree_sampler_get_type()
#define FIRTREE_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_SAMPLER, FirtreeSampler))
#define FIRTREE_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_SAMPLER, FirtreeSamplerClass))
#define FIRTREE_IS_SAMPLER(obj) 	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_SAMPLER))
#define FIRTREE_IS_SAMPLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_SAMPLER))
#define FIRTREE_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_SAMPLER, FirtreeSamplerClass))

typedef struct 	_FirtreeSampler 		FirtreeSampler;
typedef struct 	_FirtreeSamplerClass 		FirtreeSamplerClass;
typedef struct 	_FirtreeSamplerPrivate 		FirtreeSamplerPrivate;
typedef struct 	_FirtreeSamplerIntlVTable 	FirtreeSamplerIntlVTable;

struct _FirtreeSampler
{
	GObject 	parent;
};

struct _FirtreeSamplerClass 
{
	GObjectClass 	parent_class;

	void 		(*contents_changed) 	(FirtreeSampler *sampler);
	void 		(*module_changed)	(FirtreeSampler *sampler);
	void 		(*extents_changed) 	(FirtreeSampler *sampler);
	void 		(*transform_changed) 	(FirtreeSampler *sampler);

	/* Publically overridable virtual methods. */

	FirtreeVec4	(*get_extent) 		(FirtreeSampler *sampler);
	gboolean	(*lock) 		(FirtreeSampler *sampler);
	void 		(*unlock) 		(FirtreeSampler *sampler);

	/* internal table of virtual methods. These are not publically
	 * overridable. */
	FirtreeSamplerIntlVTable *intl_vtable;
};

GType 		 firtree_sampler_get_type		(void);

FirtreeSampler 	*firtree_sampler_new			(void);

FirtreeVec4 	 firtree_sampler_get_extent		(FirtreeSampler *self);

FirtreeAffineTransform *firtree_sampler_get_transform
							(FirtreeSampler *self);

void		 firtree_sampler_contents_changed	(FirtreeSampler *self);

void		 firtree_sampler_module_changed		(FirtreeSampler *self);

void		 firtree_sampler_extents_changed	(FirtreeSampler *self);

void		 firtree_sampler_transform_changed	(FirtreeSampler *self);

G_END_DECLS

#endif				/* __FIRTREE_SAMPLER_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
