/* firtree-cpu-engine.cc */

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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <firtree/firtree-debug.h>
#include "firtree-cpu-engine.h"
#include "firtree-cpu-jit.hh"

#include <firtree/internal/firtree-engine-intl.hh>
#include <firtree/internal/firtree-sampler-intl.hh>

#include <common/system-info.h>
#include <common/threading.h>
#include <common/lock-free.h>

#include <sstream>

G_DEFINE_TYPE (FirtreeCpuEngine, firtree_cpu_engine, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CPU_ENGINE, FirtreeCpuEnginePrivate))

typedef struct _FirtreeCpuEnginePrivate FirtreeCpuEnginePrivate;

struct _FirtreeCpuEnginePrivate {
    FirtreeSampler*             sampler;
    gulong                      sampler_handler_id;
    FirtreeCpuJit*              jit;

    FirtreeCpuJitRenderFunc     cached_render_func;
    FirtreeBufferFormat         cached_render_func_format;
};

struct FirtreeCpuEngineRenderRequest {
    FirtreeCpuJitRenderFunc      func;
    unsigned char*  buffer;
    unsigned int    row_width;
    unsigned int    num_rows;
    unsigned int    row_stride;
    float           extents[4];
};

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_cpu_engine_invalidate_llvm_cache(FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self);
    p->cached_render_func = NULL;
}

static void*
_firtree_cpu_lazy_function_creator(const std::string& name) {
    if(name == "exp_f") {
        return (void*)expf;
    }
    if(name == "atan_f") {
        return (void*)atanf;
    }
    if(name == "atan_ff") {
        return (void*)atan2f;
    }
    g_debug("Do not know what function to use for '%s'.", name.c_str());
    return NULL;
}

static void
firtree_cpu_engine_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_cpu_engine_parent_class)->dispose (object);
    FirtreeCpuEngine* cpu_engine = FIRTREE_CPU_ENGINE(object);
    firtree_cpu_engine_set_sampler(cpu_engine, NULL);
    _firtree_cpu_engine_invalidate_llvm_cache(cpu_engine);

    FirtreeCpuEnginePrivate* p = GET_PRIVATE(cpu_engine); 
    if(p && p->jit) {
        g_object_unref(p->jit);
        p->jit = NULL;
    }

    p->cached_render_func = NULL;
}

static void
firtree_cpu_engine_class_init (FirtreeCpuEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    g_type_class_add_private (klass, sizeof (FirtreeCpuEnginePrivate));
    object_class->dispose = firtree_cpu_engine_dispose;
}

static void
firtree_cpu_engine_init (FirtreeCpuEngine *self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    p->sampler = NULL;
    p->jit = firtree_cpu_jit_new();
    p->cached_render_func = NULL;
}

FirtreeCpuEngine*
firtree_cpu_engine_new (void)
{
    return (FirtreeCpuEngine*)
        g_object_new (FIRTREE_TYPE_CPU_ENGINE, NULL);
}

static void
firtree_cpu_engine_sampler_module_changed_cb(gpointer sampler,
        gpointer data)
{
    FirtreeCpuEngine* self = FIRTREE_CPU_ENGINE(data);
    _firtree_cpu_engine_invalidate_llvm_cache(self);
}

void
firtree_cpu_engine_set_sampler (FirtreeCpuEngine* self,
        FirtreeSampler* sampler)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(p->sampler) {
        /* disconnect the original handler */
        g_signal_handler_disconnect(p->sampler, p->sampler_handler_id);

        g_object_unref(p->sampler);
        p->sampler = NULL;
    }

    if(sampler) {
        p->sampler = sampler;
        g_object_ref(p->sampler);

        p->sampler_handler_id = g_signal_connect(p->sampler, 
                "module-changed",
                G_CALLBACK(firtree_cpu_engine_sampler_module_changed_cb), self);
    }

    _firtree_cpu_engine_invalidate_llvm_cache(self);
}

FirtreeSampler*
firtree_cpu_engine_get_sampler (FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 
    return p->sampler;
}

FirtreeCpuJitRenderFunc
firtree_cpu_engine_get_renderer_func(FirtreeCpuEngine* self, FirtreeBufferFormat format)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(!p->sampler) {
        g_warning("No sampler");
        return NULL;
    }

    if(p->cached_render_func && (p->cached_render_func_format == format)) {
        return p->cached_render_func;
    }

    p->cached_render_func_format = format;
    p->cached_render_func = firtree_cpu_jit_get_render_function_for_sampler(p->jit,
            format, p->sampler, _firtree_cpu_lazy_function_creator);

    return p->cached_render_func;
}

GString*
firtree_debug_dump_cpu_engine_function(FirtreeCpuEngine* self)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(!p->sampler) {
        return NULL;
    }

    llvm::Function* f = firtree_sampler_get_sample_function(p->sampler);
    if(!f) {
        return NULL;
    }
    llvm::Module* m = f->getParent();
    
    std::ostringstream out;

    m->print(out, NULL);

    /* This is non-optimal, invlving a copy as it does but
     * production code shouldn't be using this function anyway. */
    return g_string_new(out.str().c_str());
}

static void
_call_render_func(guint row, FirtreeCpuEngineRenderRequest* request)
{
    float dy = request->extents[3] / (float)(request->num_rows);
    float extents[] = { 
        request->extents[0], request->extents[1] + (dy * (float)row),
        request->extents[2], dy };
    request->func(request->buffer + (row * request->row_stride),
            request->row_width, 1,
            request->row_stride, extents);
}

static void
firtree_cpu_engine_perform_render(FirtreeCpuEngine* self,
        FirtreeCpuJitRenderFunc func, unsigned char* buffer, unsigned int row_width, 
        unsigned int num_rows, unsigned int row_stride, 
        float* extents) 
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(!func) {
        return;
    }

    if(!p->sampler) {
        return;
    }

    if(!firtree_sampler_lock(p->sampler)) {
        g_debug("Failed to lock sampler.");
        return;
    }

    FirtreeCpuEngineRenderRequest request = {
        func, buffer, row_width, num_rows, row_stride,
        { extents[0], extents[1], extents[2], extents[3] },
    };

    threading_apply(num_rows, (ThreadingApplyFunc) _call_render_func, &request);

    firtree_sampler_unlock(p->sampler);
}

#if FIRTREE_HAVE_GDK_PIXBUF

gboolean
firtree_cpu_engine_render_into_pixbuf (FirtreeCpuEngine* self,
        FirtreeVec4* extents, GdkPixbuf* pixbuf)
{
    FirtreeCpuEnginePrivate* p = GET_PRIVATE(self); 

    if(!extents) { return FALSE; }
    if(!pixbuf) { return FALSE; }

    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    guint width = gdk_pixbuf_get_width(pixbuf);
    guint height = gdk_pixbuf_get_height(pixbuf);
    guint stride = gdk_pixbuf_get_rowstride(pixbuf);
    guint channels = gdk_pixbuf_get_n_channels(pixbuf);

    if((channels != 4) && (channels != 3)) {
        g_debug("Only 3 and 4 channel GdkPixbufs supported.");
        return FALSE;
    }

    if(p->sampler == NULL) {
        return FALSE;
    }

    FirtreeCpuJitRenderFunc render = NULL;

    if(gdk_pixbuf_get_has_alpha(pixbuf)) {
        /* GdkPixbufs use non-premultiplied alpha. */
        render = (FirtreeCpuJitRenderFunc)firtree_cpu_engine_get_renderer_func(self, 
                FIRTREE_FORMAT_RGBA32);
    } else {
        /* Use the render function optimised for ignored alpha. */
        render = (FirtreeCpuJitRenderFunc)firtree_cpu_engine_get_renderer_func(self, 
                FIRTREE_FORMAT_RGB24);
    }

    if(!render) {
        return FALSE;
    }

    firtree_cpu_engine_perform_render(self, render,
            pixels, width, height, stride, (float*)extents);

    return TRUE;
}

#endif

#if FIRTREE_HAVE_CAIRO
gboolean 
firtree_cpu_engine_render_into_cairo_surface (FirtreeCpuEngine* self,
        FirtreeVec4* extents, cairo_surface_t* surface)
{
    if(!extents) { return FALSE; }
    if(!surface) { return FALSE; }

    guchar* data = cairo_image_surface_get_data(surface);
    if(!data) {
        g_debug("Surface is not an image surface with data.");
        return FALSE;
    }

    cairo_format_t format = cairo_image_surface_get_format(surface);
    guint width = cairo_image_surface_get_width(surface);
    guint height = cairo_image_surface_get_height(surface);
    guint stride = cairo_image_surface_get_stride(surface);

    FirtreeCpuJitRenderFunc render = NULL;

    switch(format) {
        case CAIRO_FORMAT_ARGB32:
            render = (FirtreeCpuJitRenderFunc)firtree_cpu_engine_get_renderer_func(self, 
                    FIRTREE_FORMAT_BGRA32_PREMULTIPLIED);
            break;
        case CAIRO_FORMAT_RGB24:
            render = (FirtreeCpuJitRenderFunc)firtree_cpu_engine_get_renderer_func(self, 
                    FIRTREE_FORMAT_BGRX32);
            break;
        default:
            g_debug("Invalid Cairo format.");
            return FALSE;
            break;
    }

    if(!render) {
        return FALSE;
    }

    firtree_cpu_engine_perform_render(self, render,
            data, width, height, stride, (float*)extents);

    return TRUE;
}
#endif

gboolean 
firtree_cpu_engine_render_into_buffer (FirtreeCpuEngine* self,
        FirtreeVec4* extents,
        gpointer buffer, guint width, guint height,
        guint stride, FirtreeBufferFormat format)
{
    g_assert(self);

    if(!buffer) { return FALSE; }

    FirtreeCpuJitRenderFunc render = NULL;

    switch(format) {
        case FIRTREE_FORMAT_ARGB32:
        case FIRTREE_FORMAT_ARGB32_PREMULTIPLIED:
        case FIRTREE_FORMAT_XRGB32:
        case FIRTREE_FORMAT_RGBA32:
        case FIRTREE_FORMAT_RGBA32_PREMULTIPLIED:
        case FIRTREE_FORMAT_ABGR32:
        case FIRTREE_FORMAT_ABGR32_PREMULTIPLIED:
        case FIRTREE_FORMAT_XBGR32:
        case FIRTREE_FORMAT_BGRA32:
        case FIRTREE_FORMAT_BGRA32_PREMULTIPLIED:
        case FIRTREE_FORMAT_RGB24:
        case FIRTREE_FORMAT_BGR24:
        case FIRTREE_FORMAT_RGBX32:
        case FIRTREE_FORMAT_BGRX32:
            render = (FirtreeCpuJitRenderFunc)firtree_cpu_engine_get_renderer_func(self, format);
            break;
        default:
            g_warning("Attempt to render to buffer in unsupported format.");
            return FALSE;
            break;
    }

    if(!render) {
        return FALSE;
    }

    firtree_cpu_engine_perform_render(self, render,
            (unsigned char*)buffer, width, height, stride, (float*)extents);

    return TRUE;
}

/* vim:sw=4:ts=4:et:cindent
 */
