/* firtree-cairo-surface-sampler.cc */

/* Firtree - A generic cairo_surface processing library
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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <common/uuid.h>

#include "internal/firtree-engine-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-cairo-surface-sampler.h"

G_DEFINE_TYPE (FirtreeCairoSurfaceSampler, firtree_cairo_surface_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, FirtreeCairoSurfaceSamplerPrivate))

typedef struct _FirtreeCairoSurfaceSamplerPrivate FirtreeCairoSurfaceSamplerPrivate;

struct _FirtreeCairoSurfaceSamplerPrivate {
    cairo_surface_t*          cairo_surface;
    gboolean            do_interp;
    llvm::Function*     cached_function;
};

llvm::Function*
firtree_cairo_surface_sampler_get_sample_function(FirtreeSampler* self);

llvm::Function*
_firtree_cairo_surface_sampler_create_sample_function(FirtreeCairoSurfaceSampler* self);

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_cairo_surface_sampler_invalidate_llvm_cache(FirtreeCairoSurfaceSampler* self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    if(p && p->cached_function) {
        delete p->cached_function->getParent();
        p->cached_function = NULL;
    }
}

static void
firtree_cairo_surface_sampler_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_cairo_surface_sampler_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_cairo_surface_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_cairo_surface_sampler_parent_class)->dispose (object);

    /* drop any references to any cairo_surface we might have. */
    firtree_cairo_surface_sampler_set_cairo_surface((FirtreeCairoSurfaceSampler*)object, NULL);

    /* dispose of any LLVM modules we might have. */
    _firtree_cairo_surface_sampler_invalidate_llvm_cache((FirtreeCairoSurfaceSampler*)object);
}

static void
firtree_cairo_surface_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_cairo_surface_sampler_parent_class)->finalize (object);
}

static FirtreeSamplerIntlVTable _firtree_cairo_surface_sampler_class_vtable;

static void
firtree_cairo_surface_sampler_class_init (FirtreeCairoSurfaceSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeCairoSurfaceSamplerPrivate));

    object_class->get_property = firtree_cairo_surface_sampler_get_property;
    object_class->set_property = firtree_cairo_surface_sampler_set_property;
    object_class->dispose = firtree_cairo_surface_sampler_dispose;
    object_class->finalize = firtree_cairo_surface_sampler_finalize;

    /* override the sampler virtual functions with our own */
    FirtreeSamplerClass* sampler_class = FIRTREE_SAMPLER_CLASS(klass);

    sampler_class->intl_vtable = &_firtree_cairo_surface_sampler_class_vtable;

    sampler_class->intl_vtable->get_sample_function = 
        firtree_cairo_surface_sampler_get_sample_function;
}

static void
firtree_cairo_surface_sampler_init (FirtreeCairoSurfaceSampler *self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    p->cairo_surface = NULL;
    p->do_interp = FALSE;
    p->cached_function = NULL;
}

FirtreeCairoSurfaceSampler*
firtree_cairo_surface_sampler_new (void)
{
    return (FirtreeCairoSurfaceSampler*)
        g_object_new (FIRTREE_TYPE_CAIRO_SURFACE_SAMPLER, NULL);
}

void
firtree_cairo_surface_sampler_set_cairo_surface (FirtreeCairoSurfaceSampler* self,
        cairo_surface_t* cairo_surface)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    /* unref any cairo_surface we already have. */

    if(p->cairo_surface) {
        cairo_surface_destroy(p->cairo_surface);
        p->cairo_surface = NULL;
    }

    if(cairo_surface) {
        cairo_surface_reference(cairo_surface);
        p->cairo_surface = cairo_surface;
    }

    _firtree_cairo_surface_sampler_invalidate_llvm_cache(self);
}

cairo_surface_t*
firtree_cairo_surface_sampler_get_cairo_surface (FirtreeCairoSurfaceSampler* self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    return p->cairo_surface;
}

gboolean
firtree_cairo_surface_sampler_get_do_interpolation (FirtreeCairoSurfaceSampler* self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    return p->do_interp;
}

void
firtree_cairo_surface_sampler_set_do_interpolation (FirtreeCairoSurfaceSampler* self,
        gboolean do_interpolation)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    if(do_interpolation != p->do_interp) {
        p->do_interp = do_interpolation;
        _firtree_cairo_surface_sampler_invalidate_llvm_cache(self);
    }
}

llvm::Function*
firtree_cairo_surface_sampler_get_sample_function(FirtreeSampler* self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    if(p->cached_function) {
        return p->cached_function;
    }

    return _firtree_cairo_surface_sampler_create_sample_function(
            FIRTREE_CAIRO_SURFACE_SAMPLER(self));
}

/* Create the sampler function */
llvm::Function*
_firtree_cairo_surface_sampler_create_sample_function(FirtreeCairoSurfaceSampler* self)
{
    FirtreeCairoSurfaceSamplerPrivate* p = GET_PRIVATE(self);
    if(!p->cairo_surface) {
        return NULL;
    }

    unsigned char* data = cairo_image_surface_get_data(p->cairo_surface);
    if(!data) {
        g_debug("Cairo surface is either not an image surface or has been destroyed.");
        return NULL;
    }

    /* Work out the width, height, stride, etc of the buffer. */
    int width = cairo_image_surface_get_width(p->cairo_surface);
    int height = cairo_image_surface_get_height(p->cairo_surface);
    int stride = cairo_image_surface_get_stride(p->cairo_surface);

    cairo_format_t format = cairo_image_surface_get_format(p->cairo_surface);

    FirtreeEngineBufferFormat firtree_format = FIRTREE_FORMAT_LAST;
    switch(format) {
        case CAIRO_FORMAT_ARGB32:
            firtree_format = FIRTREE_FORMAT_ARGB32_PREMULTIPLIED;
            break;
        case CAIRO_FORMAT_RGB24:
            firtree_format = FIRTREE_FORMAT_XRGB32;
            break;
        default:
            g_debug("Unsupported Cairo image format.");
            return NULL;
            break;
    }

    _firtree_cairo_surface_sampler_invalidate_llvm_cache(self);

    llvm::Module* m = new llvm::Module("cairo_surface");

    /* declare the sample_image_buffer() function which will be implemented
     * by the engine. */
    llvm::Function* sample_buffer_func = 
        firtree_engine_create_sample_image_buffer_prototype(m, p->do_interp);
    g_assert(sample_buffer_func);

    /* declare the sample() function which we shall implement. */
    llvm::Function* sample_func = 
        firtree_engine_create_sample_function_prototype(m);
    g_assert(sample_func);

    llvm::Value* llvm_width = llvm::ConstantInt::get(llvm::Type::Int32Ty, 
            (uint64_t)width, false);
    llvm::Value* llvm_height = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)height, false);
    llvm::Value* llvm_stride = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)stride, false);
    llvm::Value* llvm_format = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)firtree_format, false);

    /* This looks dirty but is apparently valid.
     *   See: http://www.nabble.com/Creating-Pointer-Constants-td22401381.html */
    llvm::Constant* llvm_data_int = llvm::ConstantInt::get(llvm::Type::Int64Ty, 
            (uint64_t)data, false);
    llvm::Value* llvm_data = llvm::ConstantExpr::getIntToPtr(llvm_data_int,
            llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); 

    /* Implement the sample function. */
    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", sample_func);
    
    std::vector<llvm::Value*> func_args;
    func_args.push_back(llvm_data);
    func_args.push_back(llvm_format);
    func_args.push_back(llvm_width);
    func_args.push_back(llvm_height);
    func_args.push_back(llvm_stride);
    func_args.push_back(sample_func->arg_begin());

    llvm::Value* ret_val = llvm::CallInst::Create(sample_buffer_func,
            func_args.begin(), func_args.end(), "rv", bb);

    llvm::ReturnInst::Create(ret_val, bb);

    p->cached_function = sample_func;
    return sample_func;
}

/* vim:sw=4:ts=4:et:cindent
 */
