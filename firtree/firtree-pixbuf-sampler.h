/* firtree-pixbuf-sampler.h */

/* Firtree - A generic pixbuf processing library
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

#ifndef ___FIRTREE_PIXBUF_SAMPLER_H__
#define ___FIRTREE_PIXBUF_SAMPLER_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <firtree/firtree-sampler.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_PIXBUF_SAMPLER 		firtree_pixbuf_sampler_get_type()
#define FIRTREE_PIXBUF_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSampler))
#define FIRTREE_PIXBUF_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSamplerClass))
#define FIRTREE_IS_PIXBUF_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER))
#define FIRTREE_IS_PIXBUF_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_PIXBUF_SAMPLER))
#define FIRTREE_PIXBUF_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSamplerClass))

typedef struct 		_FirtreePixbufSampler 		FirtreePixbufSampler;
typedef struct 		_FirtreePixbufSamplerClass 	FirtreePixbufSamplerClass;
typedef struct 		_FirtreePixbufSamplerPrivate 	FirtreePixbufSamplerPrivate;

struct 	_FirtreePixbufSampler 
{
	FirtreeSampler 		parent;
};

struct _FirtreePixbufSamplerClass
{
	FirtreeSamplerClass 	parent_class;
};

GType			 firtree_pixbuf_sampler_get_type	(void);

FirtreePixbufSampler 	*firtree_pixbuf_sampler_new		(void);

void			 firtree_pixbuf_sampler_set_pixbuf	(FirtreePixbufSampler	*self,
								 GdkPixbuf 		*pixbuf);

GdkPixbuf		*firtree_pixbuf_sampler_get_pixbuf	(FirtreePixbufSampler 	*self);

gboolean		 firtree_pixbuf_sampler_get_do_interpolation
								(FirtreePixbufSampler 	*self);

void			 firtree_pixbuf_sampler_set_do_interpolation
								(FirtreePixbufSampler 	*self,
								 gboolean		 do_interpolation);

G_END_DECLS

#endif				/* ___FIRTREE_PIXBUF_SAMPLER_H__ */

/* vim:sw=8:ts=8:noet:cindent
 */
