// FIRTREE - A generic image processing library Copyright (C) 2007, 2008
// Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//===========================================================================
// This file implements the Firtree sampler provider.
//===========================================================================

#include <iostream>
#include <sstream>
#include <limits.h>

#include <firtree/main.h>
#include <firtree/linker/sampler_provider.h>
#include <firtree/value.h>

#include "../compiler/llvm-code-gen/llvm_frontend.h"
#include "../compiler/llvm-code-gen/llvm_private.h"

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/hash_map>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include <internal/image-int.h>

namespace llvm { class Module; }

namespace Firtree { class LLVMFrontend; }

namespace Firtree { namespace LLVM {

using namespace llvm;

//===========================================================================
SamplerProvider::SamplerProvider()
    :   ReferenceCounted()
{
}

//===========================================================================
SamplerProvider::~SamplerProvider()
{
}

//===========================================================================
SamplerProvider* SamplerProvider::CreateFromImage(const Image* image)
{
    const Firtree::Internal::ImageImpl* imImpl = 
        dynamic_cast<const Firtree::Internal::ImageImpl*>(image);
    if(imImpl == NULL) {
        FIRTREE_WARNING("Passed an invalid image.");
        return NULL; 
    }

    // See if this image has a kernel.
    Kernel* kernel = imImpl->GetKernel();

    if(!kernel) {
        // If not, create one (temp. hack).
        kernel = Kernel::CreateFromSource(
                "kernel vec4 foo() { return vec4(1,0,0,1); }");
    } else {
        FIRTREE_SAFE_RETAIN(kernel);
    }

    SamplerProvider* rv = kernel->GetSampler();
    FIRTREE_SAFE_RETAIN(rv);

    FIRTREE_SAFE_RELEASE(kernel);

    return rv;
}

//===========================================================================
//===========================================================================

//===========================================================================
SamplerLinker::SamplerLinker()
    :   m_RunOptimiser(true)
    ,   m_LinkedModule(NULL)
{
}

//===========================================================================
SamplerLinker::~SamplerLinker()
{
    if(m_LinkedModule) {
        delete m_LinkedModule;
        m_LinkedModule = NULL;
    }
}

//===========================================================================
bool SamplerLinker::CanLinkSampler(SamplerProvider* sampler)
{
    std::vector<SamplerProvider*> sampler_queue;
    sampler_queue.push_back(sampler);
    while(sampler_queue.size() > 0) {
        SamplerProvider* next_provider = sampler_queue.back();
        sampler_queue.pop_back();

        if(!(next_provider->IsValid())) {
            return false;
        }

        for(SamplerProvider::const_iterator i=next_provider->begin();
                i!=next_provider->end(); ++i)
        {
            if(i->Type == Firtree::TySpecSampler) {
                SamplerProvider* param_samp = 
                    next_provider->GetParameterSampler(i);
                sampler_queue.push_back(param_samp);
            }
        }
    }

    return true;
}

//===========================================================================
void SamplerLinker::Reset()
{
    m_FreeParameters.clear();
    m_SamplerTable.clear();

    if(m_LinkedModule) {
        delete m_LinkedModule;
        m_LinkedModule = NULL;
    }
}

//===========================================================================
void SamplerLinker::LinkSampler(SamplerProvider* sampler)
{
    Reset();

    m_SamplerTable.push_back(sampler);

    std::vector< std::pair<std::string,SamplerProvider*> > sampler_queue;
    sampler_queue.push_back(
            std::pair<std::string,SamplerProvider*>("sampler_0_", sampler));

    llvm::Linker* linker = new llvm::Linker("firtree", "module");

    while(sampler_queue.size() > 0) {
        std::pair<std::string,SamplerProvider*> next = sampler_queue.back();
        sampler_queue.pop_back();

        SamplerProvider* next_provider = next.second;

        if(!(next_provider->IsValid())) {
            FIRTREE_ERROR("Not all samplers valid.");
            return;
        }

        for(SamplerProvider::const_iterator i=next_provider->begin();
                i!=next_provider->end(); ++i)
        {
            if(i->Type == Firtree::TySpecSampler) {
                std::ostringstream prefix(std::ostringstream::out);
                prefix << "sampler_" << m_SamplerTable.size() << "_";

                Firtree::Value* val = Firtree::Value::
                    CreateIntValue(m_SamplerTable.size());
                next_provider->SetParameterValue(i, val);
                FIRTREE_SAFE_RELEASE(val);

                SamplerProvider* param_samp = 
                    next_provider->GetParameterSampler(i);
                sampler_queue.push_back(
                        std::pair<std::string,SamplerProvider*>(
                            prefix.str(), param_samp));
                m_SamplerTable.push_back(param_samp);
            } else if(! i->IsStatic) {
                // This is a free parameter
                m_FreeParameters.push_back(ParamSpec(
                            next_provider, i->Name));
            }
        }

        llvm::Module* module = next_provider->CreateSamplerModule(next.first);
        std::string err;
        linker->LinkInModule(module, &err);
        if(!err.empty()) {
            FIRTREE_ERROR("Error linking samplers: %s", err.c_str());
            return;
        }
    }

    m_LinkedModule = linker->releaseModule();
    delete linker;

    Function* trans_F = WriteTransform();
    /* Function* extent_F = */ WriteExtent();
    Function* sample_F = WriteSample();

    // Now write the main kernel function
    std::vector<const Type*> kernel_params;
    kernel_params.push_back(VectorType::get( Type::FloatTy, 2 ));
    FunctionType* kernel_FT = FunctionType::get( 
            VectorType::get( Type::FloatTy, 4 ),
            kernel_params, false );
    Function* kernel_F = LLVM_CREATE( Function, kernel_FT,
            Function::ExternalLinkage,
            "kernel", m_LinkedModule );

    kernel_F->arg_begin()->setName("coord");

    llvm::BasicBlock *kernel_BB = LLVM_CREATE( llvm::BasicBlock, "entry",
            kernel_F );

    std::vector<llvm::Value*> tparams;
    tparams.push_back(ConstantInt::get(Type::Int32Ty, 0));
    tparams.push_back(llvm::cast<llvm::Value>(
                kernel_F->arg_begin()));
    llvm::Value* sample_coord = LLVM_CREATE( llvm::CallInst, 
            trans_F, tparams.begin(), tparams.end(),
            "coord", kernel_BB );

    std::vector<llvm::Value*> sparams;
    sparams.push_back(ConstantInt::get(Type::Int32Ty, 0));
    sparams.push_back(sample_coord);
    llvm::Value* rv = LLVM_CREATE( llvm::CallInst, 
            sample_F, sparams.begin(), sparams.end(),
            "rv", kernel_BB );

    LLVM_CREATE( llvm::ReturnInst, rv, kernel_BB );

    // We're donw if we don't need to optimise
    if(!m_RunOptimiser) {
        return;
    }

    RunOptimiser();

    m_LinkedModule->dump();

    // By this point, all usages of sample_x_{Transform,Sample,Extent}
    // for any sampler should have been inlined. Check this
    // and remove them.
    for(size_t idx = 0; idx<m_SamplerTable.size(); ++idx)
    {
        std::ostringstream trans_name(std::ostringstream::out);
        trans_name << "sampler_" << idx << "_Transform";
        llvm::Function *samp_trans_F = 
            m_LinkedModule->getFunction(trans_name.str());

        if(!samp_trans_F->hasNUses(0)) {
            FIRTREE_ERROR("Internal error: sampler has uses left "
                    "after inlining.");
        }
        samp_trans_F->eraseFromParent();

        std::ostringstream extent_name(std::ostringstream::out);
        extent_name << "sampler_" << idx << "_Extent";
        llvm::Function *samp_extent_F = 
            m_LinkedModule->getFunction(extent_name.str());

        if(!samp_extent_F->hasNUses(0)) {
            FIRTREE_ERROR("Internal error: sampler has uses left "
                    "after inlining.");
        }
        samp_extent_F->eraseFromParent();
        
        std::ostringstream sample_name(std::ostringstream::out);
        sample_name << "sampler_" << idx << "_Sample";
        llvm::Function *samp_sample_F = 
            m_LinkedModule->getFunction(sample_name.str());

        if(!samp_sample_F->hasNUses(0)) {
            FIRTREE_ERROR("Internal error: sampler has uses left "
                    "after inlining.");
        }
        samp_sample_F->eraseFromParent();
    }

}

//===================================================================
llvm::Value* ConstantVector(float* v, int n)
{
    std::vector<llvm::Constant*> elements;

    for ( int i=0; i<n; i++ ) {
#if LLVM_AT_LEAST_2_3
        elements.push_back( llvm::ConstantFP::get( llvm::Type::FloatTy, 
                    (double)(v[i])));
#else
        elements.push_back( llvm::ConstantFP::get( llvm::Type::FloatTy,
                    llvm::APFloat( v[i] ) ) );
#endif
    }

    llvm::Value* val = llvm::ConstantVector::get( elements );
    return val;
}

//===================================================================
llvm::Value* ConstantVector(float x, float y)
{
    float v[] = {x,y};
    return ConstantVector(v,2);
}

//===================================================================
llvm::Value* ConstantVector(float x, float y, float z)
{
    float v[] = {x,y,z};
    return ConstantVector(v,3);
}

//===================================================================
llvm::Value* ConstantVector(float x, float y, float z, float w)
{
    float v[] = {x,y,z,w};
    return ConstantVector(v,4);
}

//===========================================================================
llvm::Function* SamplerLinker::WriteTransform()
{
    llvm::Module* module = GetModule();
    if(!module) {
        return NULL;
    }

    llvm::Function* trans_F = module->getFunction("samplerTransform_sv2");

    std::vector<const Type*> trans_params;
    trans_params.push_back(Type::Int32Ty);
    trans_params.push_back(VectorType::get( Type::FloatTy, 2 ));
    FunctionType *trans_FT = FunctionType::get(
            VectorType::get( Type::FloatTy, 2 ),
            trans_params, false );
    Function* new_trans_F = LLVM_CREATE( Function, trans_FT,
            Function::InternalLinkage,
            "samplerTransform_sv2",
            module );	

    if(trans_F) {
        trans_F->replaceAllUsesWith(new_trans_F);
        trans_F->eraseFromParent();
    }

    trans_F = new_trans_F;

    // Start the function with an entry block.
    llvm::BasicBlock *main_BB = LLVM_CREATE( llvm::BasicBlock, "entry",
            trans_F );

    // If we have no samplers (unlikely if we actually *use*
    // samplerTransform) just return 0,0.
    if(m_SamplerTable.size() == 0) {
        LLVM_CREATE( llvm::ReturnInst, ConstantVector(0.f,0.f), main_BB );
        return trans_F;
    }

    // For each sampler...
    int idx = 0;
    for(SamplerList::iterator i = m_SamplerTable.begin();
            i != m_SamplerTable.end(); ++i, ++idx)
    {
        // Get the transform function associated with this sampler.
        std::ostringstream func_name(std::ostringstream::out);
        func_name << "sampler_" << idx << "_Transform";
        llvm::Function *samp_trans_F = module->getFunction(func_name.str());
        assert(samp_trans_F);

        // Copy the arguments from the function.
        std::vector<llvm::Value*> params;
        llvm::Function::arg_iterator args = trans_F->arg_begin();
        ++args; // Skip sampler index.
        params.push_back(llvm::cast<llvm::Value>(args));

        // If this is not the last sampler, return the value if
        // the sampler index matches, otherwise just assume the sampler
        // index must mactch. This is done so that if a single sampler
        // is used, we will shortcut any comparisons.
        if(idx+1 < static_cast<int>(m_SamplerTable.size())) {
            llvm::Value* icmp = new llvm::ICmpInst(
                    llvm::ICmpInst::ICMP_EQ,
                    llvm::cast<llvm::Value>(trans_F->arg_begin()),
                    llvm::ConstantInt::get(llvm::Type::Int32Ty, idx),
                    "cmp", main_BB);

            llvm::BasicBlock *exit_BB = LLVM_CREATE( llvm::BasicBlock, "exit",
                    trans_F );

            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_trans_F, 
                    params.begin(), params.end(),
                    "trans", exit_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, exit_BB );

            llvm::BasicBlock *cont_BB = LLVM_CREATE( llvm::BasicBlock, "cont",
                    trans_F );

            LLVM_CREATE( llvm::BranchInst, exit_BB, cont_BB, icmp, main_BB );
            main_BB = cont_BB;
        } else {
            // Fall through to last sampler.
            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_trans_F, 
                    params.begin(), params.end(),
                    "trans", main_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, main_BB );
        }
    }

    return trans_F;
}

//===========================================================================
llvm::Function* SamplerLinker::WriteExtent()
{
    llvm::Module* module = GetModule();
    if(!module) {
        return NULL;
    }

    llvm::Function* extent_F = module->getFunction("samplerExtent_s");

    // If so, create a new /internal/ function which will replace it.
    std::vector<const Type*> extent_params;
    extent_params.push_back(Type::Int32Ty);
    FunctionType *extent_FT = FunctionType::get(
            VectorType::get( Type::FloatTy, 4 ),
            extent_params, false );
    Function* new_extent_F = LLVM_CREATE( Function, extent_FT,
            Function::InternalLinkage,
            "samplerExtent_s",
            module );	

    if(extent_F) {
        extent_F->replaceAllUsesWith(new_extent_F);
        extent_F->eraseFromParent();
    }
    extent_F = new_extent_F;

    // Start the function with an entry block.
    llvm::BasicBlock *main_BB = LLVM_CREATE( llvm::BasicBlock, "entry",
            extent_F );

    // If we have no samplers (unlikely if we actually *use*
    // samplerTransform) just return 0,0.
    if(m_SamplerTable.size() == 0) {
        LLVM_CREATE( llvm::ReturnInst, ConstantVector(0.f,0.f,0.f,0.f), 
                main_BB );
        return extent_F;
    }

    // For each sampler...
    int idx = 0;
    for(SamplerList::iterator i = m_SamplerTable.begin();
            i != m_SamplerTable.end(); ++i, ++idx)
    {
        // Get the extent function associated with this sampler.
        std::ostringstream func_name(std::ostringstream::out);
        func_name << "sampler_" << idx << "_Extent";
        llvm::Function *samp_extent_F = module->getFunction(func_name.str());
        assert(samp_extent_F);

        // If this is not the last sampler, return the value if
        // the sampler index matches, otherwise just assume the sampler
        // index must mactch. This is done so that if a single sampler
        // is used, we will shortcut any comparisons.
        if(idx+1 < static_cast<int>(m_SamplerTable.size())) {
            llvm::Value* icmp = new llvm::ICmpInst(
                    llvm::ICmpInst::ICMP_EQ,
                    llvm::cast<llvm::Value>(extent_F->arg_begin()),
                    llvm::ConstantInt::get(llvm::Type::Int32Ty, idx),
                    "cmp", main_BB);

            llvm::BasicBlock *exit_BB = LLVM_CREATE( llvm::BasicBlock, "exit",
                    extent_F );

            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_extent_F, 
                    "extent", exit_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, exit_BB );

            llvm::BasicBlock *cont_BB = LLVM_CREATE( llvm::BasicBlock, "cont",
                    extent_F );

            LLVM_CREATE( llvm::BranchInst, exit_BB, cont_BB, icmp, main_BB );
            main_BB = cont_BB;
        } else {
            // Fall through to last sampler.
            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_extent_F, 
                    "extent", main_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, main_BB );
        }
    }

    return NULL;
}

//===========================================================================
llvm::Function* SamplerLinker::WriteSample()
{
    llvm::Module* module = GetModule();
    if(!module) {
        return NULL;
    }

    // Do we use the sample function?
    llvm::Function* sample_F = module->getFunction("sample_sv2");

    WriteFreeParamFuncs();

    // If so, create a new /internal/ function which will replace it.
    std::vector<const Type*> sample_params;
    sample_params.push_back(Type::Int32Ty);
    sample_params.push_back(VectorType::get( Type::FloatTy, 2 ));
    FunctionType *sample_FT = FunctionType::get(
            VectorType::get( Type::FloatTy, 4 ),
            sample_params, false );
    Function* new_sample_F = LLVM_CREATE( Function, sample_FT,
            Function::InternalLinkage,
            "sample_sv2",
            module );	

    if(sample_F) {
        sample_F->replaceAllUsesWith(new_sample_F);
        sample_F->eraseFromParent();
    }
    sample_F = new_sample_F;

    // Start the function with an entry block.
    llvm::BasicBlock *main_BB = LLVM_CREATE( llvm::BasicBlock, "entry",
            sample_F );

    // If we have no samplers (unlikely if we actually *use*
    // sample) just return 0,0,0,0.
    if(m_SamplerTable.size() == 0) {
        LLVM_CREATE( llvm::ReturnInst, ConstantVector(0.f,0.f,0.f,0.f),
                main_BB );
        return sample_F;
    }

    // For each sampler...
    int idx = 0;
    for(SamplerList::iterator i = m_SamplerTable.begin();
            i != m_SamplerTable.end(); ++i, ++idx)
    {
        // Get the sample function associated with this sampler.
        std::ostringstream func_name(std::ostringstream::out);
        func_name << "sampler_" << idx << "_Sample";
        llvm::Function *samp_sample_F = module->getFunction(func_name.str());
        assert(samp_sample_F);

        // Copy the arguments from the function.
        std::vector<llvm::Value*> params;
        llvm::Function::arg_iterator args = sample_F->arg_begin();
        ++args; // Skip sampler index.
        params.push_back(llvm::cast<llvm::Value>(args));

        // Handle the free parameters.
        SamplerProvider::const_iterator start = (*i)->begin();
        SamplerProvider::const_iterator end = (*i)->end();
        for(SamplerProvider::const_iterator 
                param = start; param != end; ++param) {
            // Skip static parameters
            if(param->IsStatic) {
                continue;
            }

            // Look for this free parameter
            int free_param_idx = 0;
            bool found = false;
            for(ParamSpecList::iterator fpit = m_FreeParameters.begin();
                    (fpit != m_FreeParameters.end()) && !found;
                    ++fpit, ++free_param_idx)
            {
                // Cheap test first...
                if((fpit->first == *i) && (fpit->second == param->Name)) {
                    found = true;

                    // Find the free parameter function.
                    std::ostringstream func_name(std::ostringstream::out);
                    func_name << "freeparam_";
                    switch(param->Type) {
                        case Firtree::TySpecFloat:
                            func_name << "f";
                            break;
                        case Firtree::TySpecInt:
                            func_name << "i";
                            break;
                        case Firtree::TySpecBool:
                            func_name << "b";
                            break;
                        case Firtree::TySpecVec2:
                            func_name << "v2";
                            break;
                        case Firtree::TySpecVec3:
                            func_name << "v3";
                            break;
                        case Firtree::TySpecVec4:
                        case Firtree::TySpecColor:
                            func_name << "v4";
                            break;
                        default:
                            FIRTREE_ERROR("Unsupported free parameter "
                                    "type: %i.", param->Type);
                            break;
                    }

                    llvm::Function* param_F = module->getFunction(
                            func_name.str());
                    if(!param_F)
                    {
                        FIRTREE_ERROR("Could not find free parameter "
                                "accessor.");
                    }

                    std::vector<llvm::Value*> fp_params;
                    fp_params.push_back(
                            ConstantInt::get(Type::Int32Ty, free_param_idx));

                    llvm::Value* func_call = 
                        LLVM_CREATE( llvm::CallInst, param_F, 
                            fp_params.begin(), fp_params.end(),
                            "trans", main_BB );
                    params.push_back(func_call);
                }
            }

            assert(found && "Failed to find parameter in free param table.");
        }

        // If this is not the last sampler, return the value if
        // the sampler index matches, otherwise just assume the sampler
        // index must mactch. This is done so that if a single sampler
        // is used, we will shortcut any comparisons.
        if(idx+1 < static_cast<int>(m_SamplerTable.size())) {
            llvm::Value* icmp = new llvm::ICmpInst(
                    llvm::ICmpInst::ICMP_EQ,
                    llvm::cast<llvm::Value>(sample_F->arg_begin()),
                    llvm::ConstantInt::get(llvm::Type::Int32Ty, idx),
                    "cmp", main_BB);

            llvm::BasicBlock *exit_BB = LLVM_CREATE( llvm::BasicBlock, "exit",
                    sample_F );

            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_sample_F, 
                    params.begin(), params.end(),
                    "sample", exit_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, exit_BB );

            llvm::BasicBlock *cont_BB = LLVM_CREATE( llvm::BasicBlock, "cont",
                    sample_F );

            LLVM_CREATE( llvm::BranchInst, exit_BB, cont_BB, icmp, main_BB );
            main_BB = cont_BB;
        } else {
            // Fall through to last sampler.
            llvm::Value* rv = LLVM_CREATE( llvm::CallInst, samp_sample_F, 
                    params.begin(), params.end(),
                    "sample", main_BB );

            LLVM_CREATE( llvm::ReturnInst, rv, main_BB );
        }
    }

    return sample_F;
}

//===================================================================
// Write declarations for a set of functions which will return 
// free parameter values.
void SamplerLinker::WriteFreeParamFuncs()
{
    llvm::Module* module = GetModule();
    if(!module) {
        return;
    }

    FunctionType* FT;
    Function* F;

    std::vector<const Type*> params;
    params.push_back(Type::Int32Ty);

    // bool
    FT = FunctionType::get( Type::Int1Ty,
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_b", module );

    // int
    FT = FunctionType::get( Type::Int32Ty,
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_i", module );

    // float
    FT = FunctionType::get( Type::FloatTy,
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_f", module );
    // vec2
    FT = FunctionType::get( VectorType::get( Type::FloatTy, 2 ),
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_v2", module );
    // vec3
    FT = FunctionType::get( VectorType::get( Type::FloatTy, 3 ),
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_v3", module );
    // vec4
    FT = FunctionType::get( VectorType::get( Type::FloatTy, 4 ),
            params, false );
    F = LLVM_CREATE( Function, FT, Function::ExternalLinkage,
            "freeparam_v4", module );
}

//===================================================================
void SamplerLinker::RunOptimiser()
{
    llvm::Module* m = GetModule();
    if(!m) {
        return;
    }

    PassManager PM;

    PM.add(new TargetData(m));

    PM.add(createVerifierPass());

	PM.add(createStripSymbolsPass(true));

    PM.add(createFunctionInliningPass(32768));   // Inline most functions
    PM.add(createArgumentPromotionPass());    // Scalarize uninlined fn args

	PM.add(createCFGSimplificationPass());    // Clean up disgusting code
	PM.add(createPromoteMemoryToRegisterPass());// Kill useless allocas
    PM.add(createGlobalOptimizerPass());      // Optimize out global vars
    PM.add(createGlobalDCEPass());            // Remove unused fns and globs
	PM.add(createIPConstantPropagationPass());// IP Constant Propagation
	PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
	PM.add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

    PM.add(createTailDuplicationPass());      // Simplify cfg by copying code
    PM.add(createInstructionCombiningPass()); // Cleanup for scalarrepl.
    PM.add(createCFGSimplificationPass());    // Merge & remove BBs
    PM.add(createScalarReplAggregatesPass()); // Break up aggregate allocas
    PM.add(createInstructionCombiningPass()); // Combine silly seq's
    PM.add(createCondPropagationPass());      // Propagate conditionals

	PM.add(createCondPropagationPass());      // Propagate conditionals
	PM.add(createReassociatePass());          // Reassociate expressions

	PM.add(createLoopRotatePass());
	PM.add(createLoopSimplifyPass());
	PM.add(createIndVarSimplifyPass());       

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());    

	PM.add(createLoopRotatePass());
	PM.add(createLoopSimplifyPass());
	PM.add(createIndVarSimplifyPass());     
	PM.add(createLoopStrengthReducePass());
	PM.add(createLoopIndexSplitPass());
	PM.add(createLoopDeletionPass());          

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());   

	PM.add(createLoopUnrollPass());           // Unroll small loops

	PM.add(createInstructionCombiningPass()); 
	PM.add(createCFGSimplificationPass());    

	PM.add(createSCCPPass());                 // Constant prop with SCCP

    // Run instcombine after redundancy elimination to exploit opportunities
    // opened up by them.
    PM.add(createInstructionCombiningPass());
    PM.add(createCondPropagationPass());      // Propagate conditionals

    PM.add(createDeadStoreEliminationPass()); // Delete dead stores
    PM.add(createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
    PM.add(createCFGSimplificationPass());    // Merge & remove BBs
    PM.add(createSimplifyLibCallsPass());     // Library Call Optimizations
    PM.add(createDeadTypeEliminationPass());  // Eliminate dead types
    PM.add(createConstantMergePass());        // Merge dup global constants

    PM.run(*m);
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
