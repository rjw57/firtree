/* firtree-cogl-texture-sampler.h */

/* Firtree - A generic cogl_texture processing library
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

#ifndef __FIRTREE_COGL_TEXTURE_SAMPLER_H__
#define __FIRTREE_COGL_TEXTURE_SAMPLER_H__

#include "firtree.h"

#if FIRTREE_HAVE_CLUTTER

#include <glib-object.h>
#include <cogl/cogl.h>
#include <clutter/clutter.h>

#include "firtree-sampler.h"

G_BEGIN_DECLS

#define FIRTREE_TYPE_COGL_TEXTURE_SAMPLER 		firtree_cogl_texture_sampler_get_type()
#define FIRTREE_COGL_TEXTURE_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSampler))
#define FIRTREE_COGL_TEXTURE_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSamplerClass))
#define FIRTREE_IS_COGL_TEXTURE_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER))
#define FIRTREE_IS_COGL_TEXTURE_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER))
#define FIRTREE_COGL_TEXTURE_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_COGL_TEXTURE_SAMPLER, FirtreeCoglTextureSamplerClass))

typedef struct _FirtreeCoglTextureSampler		FirtreeCoglTextureSampler;
typedef struct _FirtreeCoglTextureSamplerClass		FirtreeCoglTextureSamplerClass;
typedef struct _FirtreeCoglTextureSamplerPrivate	FirtreeCoglTextureSamplerPrivate;

struct _FirtreeCoglTextureSampler
{
	FirtreeSampler parent;
};

struct _FirtreeCoglTextureSamplerClass
{
	FirtreeSamplerClass parent_class;
};

GType 				 firtree_cogl_texture_sampler_get_type	(void);

FirtreeCoglTextureSampler 	*firtree_cogl_texture_sampler_new	(void);

void				 firtree_cogl_texture_sampler_set_cogl_texture
									(FirtreeCoglTextureSampler 	*self,
									 CoglHandle 			 texture);

CoglHandle			 firtree_cogl_texture_sampler_get_cogl_texture
									(FirtreeCoglTextureSampler 	*self);

void				 firtree_cogl_texture_sampler_set_clutter_texture
									(FirtreeCoglTextureSampler 	*self,
									 ClutterTexture 		*texture);

ClutterTexture 			*firtree_cogl_texture_sampler_get_clutter_texture
									(FirtreeCoglTextureSampler 	*self);

G_END_DECLS

#endif				/* FIRTREE_HAVE_CLUTTER */

#endif				/* __FIRTREE_COGL_TEXTURE_SAMPLER_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
