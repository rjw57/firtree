/* firtree-engine.cc */

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

#include <common/uuid.h>

#include "internal/firtree-engine-intl.hh"

#include <firtree/firtree-vector.h>

llvm::Function*
firtree_engine_create_sample_image_buffer_prototype(llvm::Module* module, 
        gboolean interp)
{
    static const char* nn_function_name = "sample_image_buffer_nn";
    static const char* interp_function_name = "sample_image_buffer_lerp";
    const char* function_name = interp ? interp_function_name : nn_function_name;

    g_assert(module);
    if(module->getFunction(function_name) != NULL) {
        return module->getFunction(function_name);
    }

    std::vector<const llvm::Type*> params;
    params.push_back(llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); /* buffer */
    params.push_back(llvm::Type::Int32Ty); /* format */
    params.push_back(llvm::Type::Int32Ty); /* width */
    params.push_back(llvm::Type::Int32Ty); /* height */
    params.push_back(llvm::Type::Int32Ty); /* stride */
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            function_name, module);

    g_assert(f);

    return f;
}

llvm::Function*
firtree_engine_create_sample_cogl_texture_prototype(llvm::Module* module)
{
    static const char* function_name = "sample_cogl_texture";

    g_assert(module);
    if(module->getFunction(function_name) != NULL) {
        return module->getFunction(function_name);
    }
    
    std::vector<const llvm::Type*> params;
    params.push_back(llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); /* handle */
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            function_name, module);

    g_assert(f);

    return f;
}

llvm::Function*
firtree_engine_create_sample_function_prototype(llvm::Module* module)
{
    /* work out the function name. */
    std::string func_name("sampler_");
    gchar uuid[37];
    generate_random_uuid(uuid, '_');
    func_name += uuid;

    g_assert(module);
    g_assert(module->getFunction(func_name.c_str()) == NULL);

    std::vector<const llvm::Type*> params;
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            func_name.c_str(), module);

    g_assert(f);

    return f;
}

llvm::Value*
firtree_engine_get_constant_for_kernel_argument(GValue* kernel_arg)
{
    GType type = G_VALUE_TYPE(kernel_arg);

    /* Can't use a switch here because the FIRTREE_TYPE_SAMPLER macro
     * doesn't expand to a constant. */
    if(type == G_TYPE_FLOAT) {
        gfloat float_val = g_value_get_float(kernel_arg);
        llvm::Value* llvm_float = llvm::ConstantFP::get(
                llvm::Type::FloatTy, (double)float_val);
        return llvm_float;
    } else if(type == G_TYPE_INT) {
        gint int_val = g_value_get_int(kernel_arg);
        llvm::Value* llvm_int = llvm::ConstantInt::get(
                llvm::Type::Int32Ty, (int64_t)int_val, true);
        return llvm_int;
    } else if(type == G_TYPE_BOOLEAN) {
        gboolean bool_val = g_value_get_boolean(kernel_arg);
        llvm::Value* llvm_bool = llvm::ConstantInt::get(
                llvm::Type::Int1Ty, (uint64_t)bool_val, false);
        return llvm_bool;
    } else if(type == FIRTREE_TYPE_VEC2) {
        FirtreeVec2* vec_box = (FirtreeVec2*)g_value_get_boxed(kernel_arg);
        std::vector<llvm::Constant*> vec_vals;
        vec_vals.push_back(llvm::ConstantFP::get(
                    llvm::Type::FloatTy, (double)vec_box->x));
        vec_vals.push_back(llvm::ConstantFP::get(
                    llvm::Type::FloatTy, (double)vec_box->y));
        llvm::Value* vec_val = llvm::ConstantVector::get(vec_vals);
        return vec_val;
    } else if(type == FIRTREE_TYPE_VEC3) {
        FirtreeVec3* vec_box = (FirtreeVec3*)g_value_get_boxed(kernel_arg);
        std::vector<llvm::Constant*> vec_vals;
        vec_vals.push_back(llvm::ConstantFP::get(
                    llvm::Type::FloatTy, (double)vec_box->x));
        vec_vals.push_back(llvm::ConstantFP::get(
                    llvm::Type::FloatTy, (double)vec_box->y));
        vec_vals.push_back(llvm::ConstantFP::get(
                    llvm::Type::FloatTy, (double)vec_box->z));
        llvm::Value* vec_val = llvm::ConstantVector::get(vec_vals);
        return vec_val;
    } else if(type == FIRTREE_TYPE_VEC4) {
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
        return vec_val;
    } else {
        g_error("Don't know how to deal with argument of type %s.\n",
                g_type_name(type));
    }

    return NULL;
}

void
firtree_engine_create_standard_optimization_passes(
        llvm::PassManager *PM,
        guint OptimizationLevel,
        gboolean OptimizeSize,
        gboolean UnitAtATime,
        gboolean UnrollLoops,
        gboolean SimplifyLibCalls,
        gboolean HaveExceptions,
        llvm::Pass *InliningPass)
{
    if (OptimizationLevel == 0) {
        if (InliningPass)
            PM->add(InliningPass);
    } else {
        if (UnitAtATime)
            PM->add(llvm::createRaiseAllocationsPass());    // call %malloc -> malloc inst
        PM->add(llvm::createCFGSimplificationPass());     // Clean up disgusting code
        // Kill useless allocas
        PM->add(llvm::createPromoteMemoryToRegisterPass());
        if (UnitAtATime) {
            PM->add(llvm::createGlobalOptimizerPass());     // Optimize out global vars
            PM->add(llvm::createGlobalDCEPass());           // Remove unused fns and globs
            // IP Constant Propagation
            PM->add(llvm::createIPConstantPropagationPass());
            PM->add(llvm::createDeadArgEliminationPass());  // Dead argument elimination
        }
        PM->add(llvm::createInstructionCombiningPass());  // Clean up after IPCP & DAE
        PM->add(llvm::createCFGSimplificationPass());     // Clean up after IPCP & DAE
        if (UnitAtATime) {
            if (HaveExceptions)
                PM->add(llvm::createPruneEHPass());           // Remove dead EH info
            PM->add(llvm::createFunctionAttrsPass());       // Set readonly/readnone attrs
        }
        if (InliningPass)
            PM->add(InliningPass);
        if (OptimizationLevel > 2)
            PM->add(llvm::createArgumentPromotionPass());   // Scalarize uninlined fn args
        if (SimplifyLibCalls)
            PM->add(llvm::createSimplifyLibCallsPass());    // Library Call Optimizations
        PM->add(llvm::createInstructionCombiningPass());  // Cleanup for scalarrepl.
        PM->add(llvm::createJumpThreadingPass());         // Thread jumps.
        PM->add(llvm::createCFGSimplificationPass());     // Merge & remove BBs
        PM->add(llvm::createScalarReplAggregatesPass());  // Break up aggregate allocas
        PM->add(llvm::createInstructionCombiningPass());  // Combine silly seq's
        PM->add(llvm::createCondPropagationPass());       // Propagate conditionals
        PM->add(llvm::createTailCallEliminationPass());   // Eliminate tail calls
        PM->add(llvm::createCFGSimplificationPass());     // Merge & remove BBs
        PM->add(llvm::createReassociatePass());           // Reassociate expressions
        PM->add(llvm::createLoopRotatePass());            // Rotate Loop
        PM->add(llvm::createLICMPass());                  // Hoist loop invariants
        PM->add(llvm::createLoopUnswitchPass(OptimizeSize));
        PM->add(llvm::createLoopIndexSplitPass());        // Split loop index
        PM->add(llvm::createInstructionCombiningPass());  
        PM->add(llvm::createIndVarSimplifyPass());        // Canonicalize indvars
        PM->add(llvm::createLoopDeletionPass());          // Delete dead loops
        if (UnrollLoops)
            PM->add(llvm::createLoopUnrollPass());          // Unroll small loops
        PM->add(llvm::createInstructionCombiningPass());  // Clean up after the unroller
        PM->add(llvm::createGVNPass());                   // Remove redundancies
        PM->add(llvm::createMemCpyOptPass());             // Remove memcpy / form memset
        PM->add(llvm::createSCCPPass());                  // Constant prop with SCCP

        // Run instcombine after redundancy elimination to exploit opportunities
        // opened up by them.
        PM->add(llvm::createInstructionCombiningPass());
        PM->add(llvm::createCondPropagationPass());       // Propagate conditionals
        PM->add(llvm::createDeadStoreEliminationPass());  // Delete dead stores
        PM->add(llvm::createAggressiveDCEPass());         // Delete dead instructions
        PM->add(llvm::createCFGSimplificationPass());     // Merge & remove BBs

        if (UnitAtATime) {
            PM->add(llvm::createStripDeadPrototypesPass()); // Get rid of dead prototypes
            PM->add(llvm::createDeadTypeEliminationPass()); // Eliminate dead types
        }

        if (OptimizationLevel > 1 && UnitAtATime)
            PM->add(llvm::createConstantMergePass());       // Merge dup global constants
    }
}

/* vim:sw=4:ts=4:et:cindent
 */
