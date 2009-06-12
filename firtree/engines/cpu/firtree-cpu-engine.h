/* firtree-cpu-engine.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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

#ifndef _FIRTREE_CPU_ENGINE
#define _FIRTREE_CPU_ENGINE

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <firtree/firtree.h>
#include <firtree/firtree-sampler.h>

#if FIRTREE_HAVE_CAIRO
#   include <cairo/cairo.h>
#endif

/**
 * SECTION:firtree-cpu-engine
 * @short_description: A rendering engine which uses a CPU JIT.
 * @include: firtree/engines/cpu/firtree-cpu-engine.h
 *
 * A rendering engine which uses a CPU JIT.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_CPU_ENGINE firtree_cpu_engine_get_type()

#define FIRTREE_CPU_ENGINE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEngine))

#define FIRTREE_CPU_ENGINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEngineClass))

#define FIRTREE_IS_CPU_ENGINE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CPU_ENGINE))

#define FIRTREE_IS_CPU_ENGINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CPU_ENGINE))

#define FIRTREE_CPU_ENGINE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEngineClass))

/**
 * FirtreeCpuEngine:
 * @parent: The parent GObject.
 *
 * A structure representing a FirtreeCpuEngine object.
 */
typedef struct {
    GObject parent;
} FirtreeCpuEngine;

typedef struct {
    GObjectClass parent_class;
} FirtreeCpuEngineClass;

GType firtree_cpu_engine_get_type (void);

/**
 * firtree_cpu_engine_new:
 *
 * Construct an uninitialised kernel sampler. Until this has been associated
 * with a kernel via firtree_cpu_engine_new_set_kernel(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeCpuEngine.
 */
FirtreeCpuEngine* 
firtree_cpu_engine_new (void);

/**
 * firtree_cpu_engine_render_into_pixbuf:
 * @self: A FirtreeCpuEngine object.
 * @extents: The extents of the sampler to render.
 * @pixbuf: The pixbuf to render into
 *
 * Render the engine's sampler into the passed pixbuf.
 *
 * Returns: TRUE if rendereding succeeded.
 */
gboolean 
firtree_cpu_engine_render_into_pixbuf (FirtreeCpuEngine* self,
        FirtreeVec4* extents, GdkPixbuf* pixbuf);

#if FIRTREE_HAVE_CAIRO

/**
 * firtree_cpu_engine_render_into_cairo_surface:
 * @self: A FirtreeCpuEngine object.
 * @extents: The extents of the sampler to render.
 * @surface: The Cairo surface to render into
 *
 * Render the engine's sampler into the passed surface. The surface
 * must be a Cairo image surface and should be either in ARGB32 or
 * RGB24 format.
 *
 * Returns: TRUE if rendereding succeeded.
 */
gboolean 
firtree_cpu_engine_render_into_cairo_surface (FirtreeCpuEngine* self,
        FirtreeVec4* extents, cairo_surface_t* surface);

#endif

/**
 * firtree_cpu_engine_set_sampler:
 * @self: A FirtreeCpuEngine object.
 * @sampler: A FirtreeSampler to associate with the engine.
 *
 * Associate the sampler @sampler with the engine.
 */
void
firtree_cpu_engine_set_sampler (FirtreeCpuEngine* self, FirtreeSampler* sampler);

/**
 * firtree_cpu_engine_get_sampler:
 * @self: A FirtreeCpuEngine object.
 *
 * Retrieve the FirtreeSampler associated with the engine. 
 *
 * Returns: The sampler related with the engine.
 */
FirtreeSampler*
firtree_cpu_engine_get_sampler (FirtreeCpuEngine* self);

G_END_DECLS

#endif /* _FIRTREE_CPU_ENGINE */

/* vim:sw=4:ts=4:et:cindent
 */
