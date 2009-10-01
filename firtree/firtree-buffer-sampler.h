/* firtree-buffer-sampler.h */

/* Firtree - A generic buffer processing library
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

#ifndef __FIRTREE_BUFFER_SAMPLER_H__
#define __FIRTREE_BUFFER_SAMPLER_H__

#include <glib-object.h>

#include "firtree-types.h"
#include "firtree-sampler.h"

G_BEGIN_DECLS

#define FIRTREE_TYPE_BUFFER_SAMPLER 		firtree_buffer_sampler_get_type()
#define FIRTREE_BUFFER_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSampler))
#define FIRTREE_BUFFER_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSamplerClass))
#define FIRTREE_IS_BUFFER_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_BUFFER_SAMPLER))
#define FIRTREE_IS_BUFFER_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_BUFFER_SAMPLER))
#define FIRTREE_BUFFER_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_BUFFER_SAMPLER, FirtreeBufferSamplerClass))

typedef struct _FirtreeBufferSampler 		FirtreeBufferSampler;
typedef struct _FirtreeBufferSamplerClass	FirtreeBufferSamplerClass;
typedef struct _FirtreeBufferSamplerPrivate 	FirtreeBufferSamplerPrivate;

struct _FirtreeBufferSampler 
{
	FirtreeSampler 		parent;
};

struct _FirtreeBufferSamplerClass
{
	FirtreeSamplerClass 	parent_class;
};

GType 			 firtree_buffer_sampler_get_type(void);

FirtreeBufferSampler 	*firtree_buffer_sampler_new	(void);

gboolean		 firtree_buffer_sampler_get_do_interpolation
							(FirtreeBufferSampler 	*self);

void			 firtree_buffer_sampler_set_do_interpolation
							(FirtreeBufferSampler 	*self,
							 gboolean		 do_interp);

void			 firtree_buffer_sampler_set_buffer
							(FirtreeBufferSampler 	*self,
				 			 gpointer		 buffer, 
							 guint			 width, 
							 guint			 height,
				 			 guint			 stride, 
							 FirtreeBufferFormat 	 format);

void			 firtree_buffer_sampler_set_buffer_no_copy
							(FirtreeBufferSampler	*self,
				 			 gpointer		 buffer, 
							 guint			 width, 
							 guint			 height,
				 			 guint			 stride, 
							 FirtreeBufferFormat 	 format);

void			 firtree_buffer_sampler_unset_buffer
							(FirtreeBufferSampler * self);

G_END_DECLS

#endif				/* __FIRTREE_BUFFER_SAMPLER_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
