/* firtree-cairo-surface-sampler.h */

/* Firtree - A generic cairo_surface processing library
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

#ifndef __FIRTREE_CAIRO_SURFACE_SAMPLER_H__
#define __FIRTREE_CAIRO_SURFACE_SAMPLER_H__

#include <firtree/firtree.h>

#if FIRTREE_HAVE_CAIRO

#include <glib-object.h>
#include <cairo/cairo.h>

#include <firtree/firtree-sampler.h>

G_BEGIN_DECLS

#define FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER 		firtree_cairo_surface_sampler_get_type()
#define FIRTREE_CAIRO_SURFACE_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSampler))
#define FIRTREE_CAIRO_SURFACE_SAMPLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSamplerClass))
#define FIRTREE_IS_CAIRO_SURFACE_SAMPLER(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER))
#define FIRTREE_IS_CAIRO_SURFACE_SAMPLER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER))
#define FIRTREE_CAIRO_SURFACE_SAMPLER_GET_CLASS(obj) 	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSamplerClass))

typedef struct _FirtreeCairoSurfaceSampler 		FirtreeCairoSurfaceSampler;
typedef struct _FirtreeCairoSurfaceSamplerClass 	FirtreeCairoSurfaceSamplerClass;
typedef struct _FirtreeCairoSurfaceSamplerPrivate 	FirtreeCairoSurfaceSamplerPrivate;

struct _FirtreeCairoSurfaceSampler 
{
	FirtreeSampler 		parent;
};

struct _FirtreeCairoSurfaceSamplerClass 
{
	FirtreeSamplerClass parent_class;
};

GType 				 firtree_cairo_surface_sampler_get_type	(void);

FirtreeCairoSurfaceSampler 	*firtree_cairo_surface_sampler_new	(void);

void				 firtree_cairo_surface_sampler_set_cairo_surface
									(FirtreeCairoSurfaceSampler 	*self,
									 cairo_surface_t		*cairo_surface);

cairo_surface_t			*firtree_cairo_surface_sampler_get_cairo_surface
									(FirtreeCairoSurfaceSampler   	*self);

gboolean			 firtree_cairo_surface_sampler_get_do_interpolation
									(FirtreeCairoSurfaceSampler	*self);

void				 firtree_cairo_surface_sampler_set_do_interpolation
									(FirtreeCairoSurfaceSampler 	*self,
									 gboolean do_interp);

G_END_DECLS

#endif				/* FIRTREE_HAVE_CAIRO */

#endif				/* __FIRTREE_CAIRO_SURFACE_SAMPLER_H__ */

/* vim:sw=8:ts=8:tw=78:noet:cindent
 */
