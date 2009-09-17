/* firtree-cpu-render.h */

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

#ifndef _FIRTREE_CPU_RENDERER
#define _FIRTREE_CPU_RENDERER

#include <glib-object.h>

#include <firtree/firtree.h>
#include <firtree/firtree-sampler.h>

#if FIRTREE_HAVE_CAIRO
#   include <cairo/cairo.h>
#endif

#if FIRTREE_HAVE_GDK_PIXBUF
#   include <gdk-pixbuf/gdk-pixbuf.h>
#endif

/**
 * SECTION:firtree-cpu-render
 * @short_description: A rendering engine which uses a CPU JIT.
 * @include: firtree/engines/cpu/firtree-cpu-render.h
 *
 * A rendering engine which uses a CPU JIT.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_CPU_RENDERER firtree_cpu_renderer_get_type()

#define FIRTREE_CPU_RENDERER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_CPU_RENDERER, FirtreeCpuRenderer))

#define FIRTREE_CPU_RENDERER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_CPU_RENDERER, FirtreeCpuRendererClass))

#define FIRTREE_IS_CPU_RENDERER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_CPU_RENDERER))

#define FIRTREE_IS_CPU_RENDERER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_CPU_RENDERER))

#define FIRTREE_CPU_RENDERER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_CPU_RENDERER, FirtreeCpuRendererClass))

/**
 * FirtreeCpuRenderer:
 * @parent: The parent GObject.
 *
 * A structure representing a FirtreeCpuRenderer object.
 */
typedef struct {
    GObject parent;
} FirtreeCpuRenderer;

typedef struct {
    GObjectClass parent_class;
} FirtreeCpuRendererClass;

GType firtree_cpu_renderer_get_type (void);

/**
 * firtree_cpu_renderer_new:
 *
 * Construct an uninitialised kernel sampler. Until this has been associated
 * with a kernel via firtree_cpu_renderer_new_set_kernel(), the sampler is
 * invalid.
 *
 * Returns: A new FirtreeCpuRenderer.
 */
FirtreeCpuRenderer* 
firtree_cpu_renderer_new (void);

#if FIRTREE_HAVE_GDK_PIXBUF

/**
 * firtree_cpu_renderer_render_into_pixbuf:
 * @self: A FirtreeCpuRenderer object.
 * @extents: The extents of the sampler to render.
 * @pixbuf: The pixbuf to render into
 *
 * Render the engine's sampler into the passed pixbuf.
 *
 * Returns: TRUE if rendereding succeeded.
 */
gboolean 
firtree_cpu_renderer_render_into_pixbuf (FirtreeCpuRenderer* self,
        FirtreeVec4* extents, GdkPixbuf* pixbuf);

#endif 

#if FIRTREE_HAVE_CAIRO

/**
 * firtree_cpu_renderer_render_into_cairo_surface:
 * @self: A FirtreeCpuRenderer object.
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
firtree_cpu_renderer_render_into_cairo_surface (FirtreeCpuRenderer* self,
        FirtreeVec4* extents, cairo_surface_t* surface);

#endif

/**
 * firtree_cpu_renderer_set_sampler:
 * @self: A FirtreeCpuRenderer object.
 * @sampler: A FirtreeSampler to associate with the engine.
 *
 * Associate the sampler @sampler with the engine.
 */
void
firtree_cpu_renderer_set_sampler (FirtreeCpuRenderer* self, FirtreeSampler* sampler);

/**
 * firtree_cpu_renderer_get_sampler:
 * @self: A FirtreeCpuRenderer object.
 *
 * Retrieve the FirtreeSampler associated with the engine. 
 *
 * Returns: The sampler related with the engine.
 */
FirtreeSampler*
firtree_cpu_renderer_get_sampler (FirtreeCpuRenderer* self);

/**
 * firtree_cpu_renderer_render_into_buffer:
 * @self: A FirtreeCpuRenderer.
 * @extents: The extents of the sampler to render.
 * @buffer: The location of the buffer in memory or NULL to unset the buffer.
 * @width: The buffer width in pixels.
 * @height: The buffer height in rows.
 * @stride: The size of one row in bytes.
 * @format: The format of the buffer.
 *
 * Render directly into a buffer in memory.
 *
 * Returns: TRUE if rendereding succeeded.
 */
gboolean 
firtree_cpu_renderer_render_into_buffer (FirtreeCpuRenderer* self,
        FirtreeVec4* extents,
        gpointer buffer, guint width, guint height,
        guint stride, FirtreeBufferFormat format);

/**
 * firtree_debug_dump_cpu_renderer_function:
 * @engine: A FirtreeCpuRenderer.
 *
 * Dump the compiled LLVM associated with @engine into a string and
 * return it. The string must be released via g_string_free() after use.
 *
 * If the engine is invalid, or there is no LLVM function, this returns
 * NULL.
 *
 * Returns: NULL or a GString.
 */
GString*
firtree_debug_dump_cpu_renderer_function(FirtreeCpuRenderer* engine);

G_END_DECLS

#endif /* _FIRTREE_CPU_RENDERER */

/* vim:sw=4:ts=4:et:cindent
 */
