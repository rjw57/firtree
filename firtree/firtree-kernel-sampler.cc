/* firtree-kernel-sampler.cc */

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

#include "internal/firtree-kernel-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-kernel-sampler.h"

G_DEFINE_TYPE (FirtreeKernelSampler, firtree_kernel_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerPrivate))

typedef struct _FirtreeKernelSamplerPrivate FirtreeKernelSamplerPrivate;

struct _FirtreeKernelSamplerPrivate {
    FirtreeKernel*  kernel;
    llvm::Module* cached_module;
    llvm::Function* cached_function;
};

gboolean
firtree_kernel_sampler_get_param(FirtreeSampler* self, guint param, 
        gpointer dest, guint dest_size);

llvm::Function*
firtree_kernel_sampler_get_function(FirtreeSampler* self);

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_kernel_sampler_invalidate_llvm_cache(FirtreeKernelSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    if(p && p->cached_module) {
        delete p->cached_module;
        p->cached_module = NULL;
        p->cached_function = NULL;
    }
}

static void
firtree_kernel_sampler_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_kernel_sampler_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_kernel_sampler_dispose (GObject *object)
{
    G_OBJECT_CLASS (firtree_kernel_sampler_parent_class)->dispose (object);

    /* drop any references to any kernel we might have. */
    firtree_kernel_sampler_set_kernel((FirtreeKernelSampler*)object, NULL);

    /* dispose of any LLVM modules we might have. */
    _firtree_kernel_sampler_invalidate_llvm_cache((FirtreeKernelSampler*)object);
}

static void
firtree_kernel_sampler_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_kernel_sampler_parent_class)->finalize (object);
}

static FirtreeSamplerIntlVTable _firtree_kernel_sampler_class_vtable;

static void
firtree_kernel_sampler_class_init (FirtreeKernelSamplerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (FirtreeKernelSamplerPrivate));

    object_class->get_property = firtree_kernel_sampler_get_property;
    object_class->set_property = firtree_kernel_sampler_set_property;
    object_class->dispose = firtree_kernel_sampler_dispose;
    object_class->finalize = firtree_kernel_sampler_finalize;

    /* override the sampler virtual functions with our own */
    FirtreeSamplerClass* sampler_class = FIRTREE_SAMPLER_CLASS(klass);

    sampler_class->intl_vtable = &_firtree_kernel_sampler_class_vtable;
    sampler_class->intl_vtable->get_param = firtree_kernel_sampler_get_param;
    sampler_class->intl_vtable->get_function = firtree_kernel_sampler_get_function;
}

static void
firtree_kernel_sampler_init (FirtreeKernelSampler *self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    p->kernel = NULL;
    p->cached_function = NULL;
}

FirtreeKernelSampler*
firtree_kernel_sampler_new (void)
{
    return (FirtreeKernelSampler*)
        g_object_new (FIRTREE_TYPE_KERNEL_SAMPLER, NULL);
}

void
firtree_kernel_sampler_set_kernel (FirtreeKernelSampler* self,
        FirtreeKernel* kernel)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    /* unref any kernel we already have. */

    if(p->kernel) {
        g_object_unref(p->kernel);
        p->kernel = NULL;
    }

    if(kernel) {
        g_assert(FIRTREE_IS_KERNEL(kernel));
        g_object_ref(kernel);
        p->kernel = kernel;
        
        /* FIXME: Connect kernel changed signals. */
    }
}

FirtreeKernel*
firtree_kernel_sampler_get_kernel (FirtreeKernelSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    return p->kernel;
}

gboolean
firtree_kernel_sampler_get_param(FirtreeSampler* self, guint param, 
        gpointer dest, guint dest_size)
{
    return FALSE;
}

llvm::Function*
firtree_kernel_sampler_get_function(FirtreeSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    if(p->cached_function) {
        return p->cached_function;
    }

    /* if we get here, we need to create our function. */
    llvm::Module* m = new llvm::Module("sampler");
    p->cached_module = m;

    /* work out the function name. */
    std::string func_name("sampler_");
    gchar uuid[37];
    generate_random_uuid(uuid, '_');
    func_name += uuid;

    /* create the function */
    std::vector<const llvm::Type*> params;
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy,2));
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            func_name.c_str(), m);
    p->cached_function = f;

    /* FIXME: this is a stub */
    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", f);

    std::vector<llvm::Constant*> elements;
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 1.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 0.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 0.0));
    elements.push_back(llvm::ConstantFP::get(llvm::Type::FloatTy, 1.0));
    llvm::Constant* rv = llvm::ConstantVector::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4),
            elements);

    llvm::ReturnInst::Create(rv, bb);

    return p->cached_function;
}

/* vim:sw=4:ts=4:et:cindent
 */
