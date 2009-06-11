/* firtree-pixbuf-sampler.cc */

/* Firtree - A generic pixbuf processing library
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
#include "firtree-pixbuf-sampler.h"

G_DEFINE_TYPE (FirtreePixbufSampler, firtree_pixbuf_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_PIXBUF_SAMPLER, FirtreePixbufSamplerPrivate))

typedef struct _FirtreePixbufSamplerPrivate FirtreePixbufSamplerPrivate;

struct _FirtreePixbufSamplerPrivate {
    GdkPixbuf*          pixbuf;
    llvm::Function*     cached_function;
};

llvm::Function*
firtree_pixbuf_sampler_get_sample_function(FirtreeSampler* self);

llvm::Function*
_firtree_pixbuf_sampler_create_sample_function(FirtreePixbufSampler* self);

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_pixbuf_sampler_invalidate_llvm_cache(FirtreePixbufSampler* self)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    if(p && p->cached_function) {
        delete p->cached_function->getParent();
        p->cached_function = NULL;
    }
}

static void
firtree_pixbuf_sampler_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_pixbuf_sampler_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_pixbuf_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_pixbuf_sampler_parent_class)->dispose (object);

    /* drop any references to any pixbuf we might have. */
    firtree_pixbuf_sampler_set_pixbuf((FirtreePixbufSampler*)object, NULL);

    /* dispose of any LLVM modules we might have. */
    _firtree_pixbuf_sampler_invalidate_llvm_cache((FirtreePixbufSampler*)object);
}

static void
firtree_pixbuf_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_pixbuf_sampler_parent_class)->finalize (object);
}

static FirtreeSamplerIntlVTable _firtree_pixbuf_sampler_class_vtable;

static void
firtree_pixbuf_sampler_class_init (FirtreePixbufSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreePixbufSamplerPrivate));

    object_class->get_property = firtree_pixbuf_sampler_get_property;
    object_class->set_property = firtree_pixbuf_sampler_set_property;
    object_class->dispose = firtree_pixbuf_sampler_dispose;
    object_class->finalize = firtree_pixbuf_sampler_finalize;

    /* override the sampler virtual functions with our own */
    FirtreeSamplerClass* sampler_class = FIRTREE_SAMPLER_CLASS(klass);

    sampler_class->intl_vtable = &_firtree_pixbuf_sampler_class_vtable;

    sampler_class->intl_vtable->get_sample_function = 
        firtree_pixbuf_sampler_get_sample_function;
}

static void
firtree_pixbuf_sampler_init (FirtreePixbufSampler *self)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    p->pixbuf = NULL;
    p->cached_function = NULL;
}

FirtreePixbufSampler*
firtree_pixbuf_sampler_new (void)
{
    return (FirtreePixbufSampler*)
        g_object_new (FIRTREE_TYPE_PIXBUF_SAMPLER, NULL);
}

void
firtree_pixbuf_sampler_set_pixbuf (FirtreePixbufSampler* self,
        GdkPixbuf* pixbuf)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    /* unref any pixbuf we already have. */

    if(p->pixbuf) {
        gdk_pixbuf_unref(p->pixbuf);
        p->pixbuf = NULL;
    }

    if(pixbuf) {
        gdk_pixbuf_ref(pixbuf);
        p->pixbuf = pixbuf;
    }

    _firtree_pixbuf_sampler_invalidate_llvm_cache(self);
}

GdkPixbuf*
firtree_pixbuf_sampler_get_pixbuf (FirtreePixbufSampler* self)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    return p->pixbuf;
}

llvm::Function*
firtree_pixbuf_sampler_get_sample_function(FirtreeSampler* self)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    if(p->cached_function) {
        return p->cached_function;
    }

    return _firtree_pixbuf_sampler_create_sample_function(
            FIRTREE_PIXBUF_SAMPLER(self));
}

/* Create the sampler function */
llvm::Function*
_firtree_pixbuf_sampler_create_sample_function(FirtreePixbufSampler* self)
{
    FirtreePixbufSamplerPrivate* p = GET_PRIVATE(self);
    if(!p->pixbuf) {
        return NULL;
    }

    _firtree_pixbuf_sampler_invalidate_llvm_cache(self);

    llvm::Module* m = new llvm::Module("pixbuf");

    /* declare the sample_image_buffer() function which will be implemented
     * by the engine. */
    llvm::Function* sample_buffer_func = 
        firtree_engine_create_sample_image_buffer_prototype(m);
    g_assert(sample_buffer_func);

    /* declare the sample() function which we shall implement. */
    llvm::Function* sample_func = 
        firtree_engine_create_sample_function_prototype(m);
    g_assert(sample_func);

    /* Work out the width, height, stride, etc of the buffer. */
    int channels = gdk_pixbuf_get_n_channels(p->pixbuf);
    if((channels != 3) && (channels != 4)) {
        g_debug("FirtreePixbufSampler only supports 3 and 4 channel pixbufs.");
        return FALSE;
    }
    int width = gdk_pixbuf_get_width(p->pixbuf);
    int height = gdk_pixbuf_get_height(p->pixbuf);
    int stride = gdk_pixbuf_get_rowstride(p->pixbuf);
    guchar* data = gdk_pixbuf_get_pixels(p->pixbuf);

    llvm::Value* llvm_width = llvm::ConstantInt::get(llvm::Type::Int32Ty, 
            (uint64_t)width, false);
    llvm::Value* llvm_height = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)height, false);
    llvm::Value* llvm_stride = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)stride, false);
    llvm::Value* llvm_format = llvm::ConstantInt::get(llvm::Type::Int32Ty,
            (uint64_t)FIRTREE_FORMAT_BGR24, false);

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
