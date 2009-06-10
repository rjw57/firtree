/* firtree-image-sampler.cc */

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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>

#include <common/uuid.h>

#include "internal/firtree-image-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-image-sampler.h"

G_DEFINE_TYPE (FirtreeImageSampler, firtree_image_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_IMAGE_SAMPLER, FirtreeImageSamplerPrivate))

typedef struct _FirtreeImageSamplerPrivate FirtreeImageSamplerPrivate;

struct _FirtreeImageSamplerPrivate {
    FirtreeImage*  image;
    llvm::Function* cached_function;
};

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_image_sampler_invalidate_llvm_cache(FirtreeImageSampler* self)
{
    FirtreeImageSamplerPrivate* p = GET_PRIVATE(self);
    if(p && p->cached_function) {
        delete p->cached_function->getParent();
        p->cached_function = NULL;
    }
}

static void
firtree_image_sampler_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_image_sampler_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_image_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_image_sampler_parent_class)->dispose (object);

    /* drop any references to any image we might have. */
    firtree_image_sampler_set_image((FirtreeImageSampler*)object, NULL);

    /* dispose of any LLVM modules we might have. */
    _firtree_image_sampler_invalidate_llvm_cache((FirtreeImageSampler*)object);
}

static void
firtree_image_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_image_sampler_parent_class)->finalize (object);
}

static FirtreeSamplerIntlVTable _firtree_image_sampler_class_vtable;

static void
firtree_image_sampler_class_init (FirtreeImageSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeImageSamplerPrivate));

    object_class->get_property = firtree_image_sampler_get_property;
    object_class->set_property = firtree_image_sampler_set_property;
    object_class->dispose = firtree_image_sampler_dispose;
    object_class->finalize = firtree_image_sampler_finalize;

    /* override the sampler virtual functions with our own */
    FirtreeSamplerClass* sampler_class = FIRTREE_SAMPLER_CLASS(klass);

    sampler_class->intl_vtable = &_firtree_image_sampler_class_vtable;

    /*
    sampler_class->intl_vtable->get_param = firtree_image_sampler_get_param;
    sampler_class->intl_vtable->get_sample_function = 
        firtree_image_sampler_get_sample_function;
        */
}

static void
firtree_image_sampler_init (FirtreeImageSampler *self)
{
    FirtreeImageSamplerPrivate* p = GET_PRIVATE(self);
    p->image = NULL;
    p->cached_function = NULL;
}

FirtreeImageSampler*
firtree_image_sampler_new (void)
{
    return (FirtreeImageSampler*)
        g_object_new (FIRTREE_TYPE_IMAGE_SAMPLER, NULL);
}

void
firtree_image_sampler_set_image (FirtreeImageSampler* self,
        FirtreeImage* image)
{
    FirtreeImageSamplerPrivate* p = GET_PRIVATE(self);
    /* unref any image we already have. */

    if(p->image) {
        g_object_unref(p->image);
        p->image = NULL;
    }

    if(image) {
        g_assert(FIRTREE_IS_IMAGE(image));
        g_object_ref(image);
        p->image = image;
        
        /* FIXME: Connect image changed signals. */
    }

    _firtree_image_sampler_invalidate_llvm_cache(self);
}

FirtreeImage*
firtree_image_sampler_get_image (FirtreeImageSampler* self)
{
    FirtreeImageSamplerPrivate* p = GET_PRIVATE(self);
    return p->image;
}

/* vim:sw=4:ts=4:et:cindent
 */
