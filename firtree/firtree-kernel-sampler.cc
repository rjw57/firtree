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
#include <llvm/Linker.h>

#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/LinkAllPasses.h>

#include "internal/firtree-engine-intl.hh"
#include "internal/firtree-kernel-intl.hh"
#include "internal/firtree-sampler-intl.hh"
#include "firtree-kernel-sampler.h"

G_DEFINE_TYPE (FirtreeKernelSampler, firtree_kernel_sampler, FIRTREE_TYPE_SAMPLER)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_KERNEL_SAMPLER, FirtreeKernelSamplerPrivate))

typedef struct _FirtreeKernelSamplerPrivate FirtreeKernelSamplerPrivate;

struct _FirtreeKernelSamplerPrivate {
    FirtreeKernel*  kernel;
    llvm::Function* cached_function;
};

gboolean
firtree_kernel_sampler_get_param(FirtreeSampler* self, guint param, 
        gpointer dest, guint dest_size);

llvm::Function*
firtree_kernel_sampler_get_sample_function(FirtreeSampler* self);

/* invalidate (and release) any cached LLVM modules/functions. This
 * will cause them to be re-generated when ..._get_function() is next
 * called. */
static void
_firtree_kernel_sampler_invalidate_llvm_cache(FirtreeKernelSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    if(p && p->cached_function) {
        delete p->cached_function->getParent();
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
    sampler_class->intl_vtable->get_sample_function = 
        firtree_kernel_sampler_get_sample_function;
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

    _firtree_kernel_sampler_invalidate_llvm_cache(self);
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

void
firtree_kernel_sampler_optimise_cached_function(FirtreeSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    if(!p->cached_function) {
        return;
    }

    llvm::Module* m = p->cached_function->getParent();
    
    llvm::PassManager PM;
	PM.add(new llvm::TargetData(m));
    
    /* Firstly internalise all the functions apart from our sampler
     * function. */
    std::vector<const char*> export_list;
    export_list.push_back(p->cached_function->getName().c_str());
    PM.add(llvm::createInternalizePass(export_list));

    /* Now inline functions. */
	PM.add(llvm::createFunctionInliningPass(32768)); 

    /* Agressively remove dead code. */
	PM.add(llvm::createAggressiveDCEPass()); 

    /* Now do some compile optimisations. */
	PM.add(llvm::createStripDeadPrototypesPass());

	PM.add(llvm::createIPConstantPropagationPass());
	PM.add(llvm::createInstructionCombiningPass());
	PM.add(llvm::createCFGSimplificationPass());

	PM.add(llvm::createCondPropagationPass());
	PM.add(llvm::createReassociatePass());
    
	PM.run(*m);
}

void
firtree_kernel_sampler_implement_sample_function(FirtreeSampler* self,
        llvm::Function* sample_func,
        GData** sampler_func_map)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);

    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", sample_func);

    llvm::Function::arg_iterator args = sample_func->arg_begin();
    
    llvm::Value* sampler_id = args;
    ++args;
    
    std::vector<llvm::Value*> remaining_args;
    for(; args != sample_func->arg_end(); ++args) {
        remaining_args.push_back(args);
    }

    /* default return value. */
    llvm::BasicBlock* default_bb = llvm::BasicBlock::Create("default_id", 
            sample_func);
    llvm::ReturnInst::Create(
            llvm::ConstantAggregateZero::get(
                llvm::VectorType::get(llvm::Type::FloatTy, 4)),
            default_bb);

    guint n_arguments = 0;
    GQuark* arg_list = firtree_kernel_list_arguments(p->kernel, &n_arguments);

    /* For each possible input id, add a case to a switch which calls
     * the appropriate sampler. */
    llvm::SwitchInst* sampler_switch = llvm::SwitchInst::Create(
            sampler_id, default_bb, 0, bb);
    for(guint i=0; i<n_arguments; ++i) {
        GQuark arg_quark = arg_list[i];
        FirtreeKernelArgumentSpec* spec = 
            firtree_kernel_get_argument_spec(p->kernel, arg_quark);
        if(spec->type == FIRTREE_TYPE_SAMPLER) {
            FirtreeSampler* sampler = (FirtreeSampler*)
                g_value_get_object(
                        firtree_kernel_get_argument_value(p->kernel, arg_quark));
            g_assert(sampler);

            llvm::Function* sampler_f = (llvm::Function*)
                g_datalist_id_get_data(sampler_func_map, arg_quark);
            g_assert(sampler_f);

            llvm::BasicBlock* sample_bb = llvm::BasicBlock::Create("id",
                    sample_func);

            llvm::Value* ret_val = llvm::CallInst::Create(
                    sampler_f, 
                    remaining_args.begin(), remaining_args.end(),
                    "rv", sample_bb);

            llvm::ReturnInst::Create(ret_val, sample_bb);

            sampler_switch->addCase(
                    llvm::ConstantInt::get(llvm::Type::Int32Ty, arg_quark, false),
                    sample_bb);
        }
    }
}

void
firtree_kernel_sampler_implement_transform_function(FirtreeSampler* self,
        llvm::Function* transform_func)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", transform_func);

    /* extract the input arguments. */
    llvm::Function::const_arg_iterator argit = transform_func->arg_begin();
    const llvm::Value* input_id = argit;
    ++argit;
    const llvm::Value* input_vector = argit;

    /* construct a basic block that 'implements' an identity transform
     * (the default). */
    llvm::BasicBlock* id_bb = llvm::BasicBlock::Create("identity", transform_func);
    llvm::ReturnInst::Create(const_cast<llvm::Value*>(input_vector), id_bb);

    /* Now make a switch statement for each sampler argument. */

    guint n_arguments = 0;
    GQuark* arg_list = firtree_kernel_list_arguments(p->kernel, &n_arguments);

    llvm::SwitchInst* sampler_switch = llvm::SwitchInst::Create(
            const_cast<llvm::Value*>(input_id), id_bb, 0, bb);
    for(guint i=0; i<n_arguments; ++i) {
        GQuark arg_quark = arg_list[i];
        FirtreeKernelArgumentSpec* spec = 
            firtree_kernel_get_argument_spec(p->kernel, arg_quark);
        if(spec->type == FIRTREE_TYPE_SAMPLER) {
            FirtreeSampler* sampler = (FirtreeSampler*)
                g_value_get_object(
                        firtree_kernel_get_argument_value(p->kernel, arg_quark));
            g_assert(sampler);

            FirtreeAffineTransform* transform = 
                firtree_sampler_get_transform(sampler);

            if(firtree_affine_transform_is_identity(transform)) {
                sampler_switch->addCase(
                        llvm::ConstantInt::get(llvm::Type::Int32Ty, 
                            arg_quark, false),
                        id_bb);
            } else {
                g_error("Non-identity transforms not yet implemented.");
                /*
                llvm::BasicBlock* trans_bb = llvm::BasicBlock::Create("trans",
                        sample_func);

                llvm::Value* ret_val = llvm::CallInst::Create(
                        sampler_f, 
                        remaining_args.begin(), remaining_args.end(),
                        "rv", sample_bb);

                llvm::ReturnInst::Create(ret_val, sample_bb);

                sampler_switch->addCase(
                        llvm::ConstantInt::get(llvm::Type::Int32Ty, 
                            arg_quark, false),
                        sample_bb);
                        */
            }

            g_object_unref(transform);
        }
    }

}

llvm::Function*
firtree_kernel_sampler_get_sample_function(FirtreeSampler* self)
{
    FirtreeKernelSamplerPrivate* p = GET_PRIVATE(self);
    if(p->cached_function) {
        return p->cached_function;
    }

    if(!p->kernel) {
        g_debug("No kernel associated with sampler.\n");
        return NULL;
    }

    if(!firtree_kernel_is_valid(p->kernel)) {
        g_debug("firtree_kernel_is_valid() returns false for associated kernel.\n");
        return NULL;
    }

    /* if we get here, we need to create our function. */

    /* get our associate kernel's function. */
    llvm::Function* kernel_func = firtree_kernel_get_function(p->kernel);
    if(!kernel_func) {
        g_debug("No kernel function.\n");
        return NULL;
    }
    
    /* Link the kernel function into a new module. */
    llvm::Linker* linker = new llvm::Linker("kernel-sampler", "module");
    std::string err_str;
    if(linker->LinkInModule(kernel_func->getParent(), &err_str))
    {
        g_error("Error linking function: %s\n", err_str.c_str());
    }

    llvm::Value* new_kernel_func = linker->getModule()->getFunction(
            kernel_func->getName());
    g_assert(new_kernel_func);

    guint n_arguments = 0;
    GQuark* arg_list = firtree_kernel_list_arguments(p->kernel, &n_arguments);

    /* A list which maps sampler argument quarks to their associated function
     * in our new module. */
    GData* sampler_function_list = NULL;
    g_datalist_init(&sampler_function_list);

    /* For each sampler parameter, get the associated sampler function
     * and link that in too. */
    for(guint arg_i=0; arg_i<n_arguments; ++arg_i)
    {
        GQuark arg_quark = arg_list[arg_i];
        FirtreeKernelArgumentSpec* arg_spec =
            firtree_kernel_get_argument_spec(p->kernel, arg_quark);
        g_assert(arg_spec);

        if(arg_spec->type == FIRTREE_TYPE_SAMPLER) {
            GValue* val = firtree_kernel_get_argument_value(p->kernel, arg_quark);
            g_assert(val);
            FirtreeSampler* sampler = (FirtreeSampler*)g_value_get_object(val);
            g_assert(sampler);

            llvm::Function* sampler_f = firtree_sampler_get_sample_function(sampler);
            g_assert(sampler_f);

            if(linker->LinkInModule(sampler_f->getParent(), &err_str))
            {
                g_error("Error linking function: %s\n", err_str.c_str());
            }

            llvm::Function* new_f = linker->getModule()->getFunction(
                    sampler_f->getName());
            g_assert(new_f);

            /* record the new function in the sampler function list. */
            g_datalist_id_set_data(&sampler_function_list, arg_quark, new_f);
        }

    }

    /* We've finished all our linking, release the linker. */
    llvm::Module* m = linker->releaseModule();
    delete linker;

    /* If we have any calls to sample(), replace them with calls to the
     * apropriate sampler function. We do this by implementing the @sample_sv2
     * function. */
    llvm::Function* sample_sv2_f = m->getFunction("sample_sv2");
    if(sample_sv2_f) {
        firtree_kernel_sampler_implement_sample_function(self, sample_sv2_f,
                &sampler_function_list);
    }

    /* Similarly, we implement the samplerTransform_sv2 function. */
    llvm::Function* trans_f = m->getFunction("samplerTransform_sv2");
    if(trans_f) {
        firtree_kernel_sampler_implement_transform_function(self, trans_f);
    }

    g_datalist_clear(&sampler_function_list);

    /* create the function */
    llvm::Function* f = firtree_engine_create_sample_function_prototype(m);
    p->cached_function = f;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create("entry", f);

    llvm::Value* dest_coord = f->arg_begin();

    std::vector<llvm::Value*> arguments;
    arguments.push_back(dest_coord);

    for(guint arg_i=0; arg_i<n_arguments; ++arg_i)
    {
        GQuark arg_quark = arg_list[arg_i];
        FirtreeKernelArgumentSpec* arg_spec =
            firtree_kernel_get_argument_spec(p->kernel, arg_quark);
        g_assert(arg_spec);

        if(!arg_spec->is_static) {
            g_error("FIXME: Non-static argument support not yet written.");
        } else {
            GValue* kernel_arg = firtree_kernel_get_argument_value(p->kernel,
                    arg_quark);
            g_assert(kernel_arg);

            /*
            g_debug("Argument %s is of type %s.\n",
                    g_quark_to_string(arg_list[arg_i]),
                    G_VALUE_TYPE_NAME(kernel_arg));
                    */

            /* Can't use a switch here because the FIRTREE_TYPE_SAMPLER macro
             * doesn't expand to a constant. */
            if(arg_spec->type == FIRTREE_TYPE_SAMPLER) {
                llvm::Value* arg_quark_val = llvm::ConstantInt::get(
                        llvm::Type::Int32Ty, arg_quark, false);
                arguments.push_back(arg_quark_val);
            } else if(arg_spec->type == G_TYPE_FLOAT) {
                gfloat float_val = g_value_get_float(kernel_arg);
                llvm::Value* llvm_float = llvm::ConstantFP::get(
                        llvm::Type::FloatTy, (double)float_val);
                arguments.push_back(llvm_float);
            } else if(arg_spec->type == G_TYPE_INT) {
                gint int_val = g_value_get_int(kernel_arg);
                llvm::Value* llvm_int = llvm::ConstantInt::get(
                        llvm::Type::Int32Ty, (int64_t)int_val, true);
                arguments.push_back(llvm_int);
            } else if(arg_spec->type == G_TYPE_BOOLEAN) {
                gboolean bool_val = g_value_get_boolean(kernel_arg);
                llvm::Value* llvm_bool = llvm::ConstantInt::get(
                        llvm::Type::Int1Ty, (uint64_t)bool_val, false);
                arguments.push_back(llvm_bool);
            } else if(arg_spec->type == FIRTREE_TYPE_VEC2) {
                FirtreeVec2* vec_box = (FirtreeVec2*)g_value_get_boxed(kernel_arg);
                std::vector<llvm::Constant*> vec_vals;
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->x));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->y));
                llvm::Value* vec_val = llvm::ConstantVector::get(vec_vals);
                arguments.push_back(vec_val);
            } else if(arg_spec->type == FIRTREE_TYPE_VEC3) {
                FirtreeVec3* vec_box = (FirtreeVec3*)g_value_get_boxed(kernel_arg);
                std::vector<llvm::Constant*> vec_vals;
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->x));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->y));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->z));
                llvm::Value* vec_val = llvm::ConstantVector::get(vec_vals);
                arguments.push_back(vec_val);
            } else if(arg_spec->type == FIRTREE_TYPE_VEC4) {
                FirtreeVec4* vec_box = (FirtreeVec4*)g_value_get_boxed(kernel_arg);
                std::vector<llvm::Constant*> vec_vals;
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->x));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->y));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->z));
                vec_vals.push_back(llvm::ConstantFP::get(
                            llvm::Type::FloatTy, (double)vec_box->w));
                llvm::Value* vec_val = llvm::ConstantVector::get(vec_vals);
                arguments.push_back(vec_val);
            } else {
                g_error("Don't know how to deal with argument of type %s.\n",
                        g_type_name(arg_spec->type));
            }
        }
    }

    llvm::Value* function_call = llvm::CallInst::Create(
            new_kernel_func, arguments.begin(), arguments.end(),
            "kernel_call", bb);

    llvm::ReturnInst::Create(function_call, bb);

    //p->cached_function->getParent()->dump();

    /* run an agressive inlining pass over the function */
    firtree_kernel_sampler_optimise_cached_function(self);

    //g_debug("*************************\n");

    g_assert(m->getFunction("sample_sv2") == NULL);

    return p->cached_function;
}

/* vim:sw=4:ts=4:et:cindent
 */
