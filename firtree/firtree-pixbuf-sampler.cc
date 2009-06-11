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

    /*
    sampler_class->intl_vtable->get_param = firtree_pixbuf_sampler_get_param;
    sampler_class->intl_vtable->get_sample_function = 
        firtree_pixbuf_sampler_get_sample_function;
        */
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

/* vim:sw=4:ts=4:et:cindent
 */
