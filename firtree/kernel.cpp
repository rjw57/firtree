// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
/// \file kernel.cpp This file implements the FIRTREE kernel interface.
//=============================================================================

#define __STDC_LIMIT_MACROS
#include <limits.h>

#include <llvm/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include <sstream>

#include <float.h>
#include <string.h>
#include <firtree/main.h>
#include <firtree/kernel.h>
#include <firtree/value.h>

// LLVM
#include <firtree/compiler/llvm_compiled_kernel.h>
#include <firtree/linker/sampler_provider.h>

/// Kernels are implemented in Firtree by keeping an internal compiled 
/// representation for all possible runtimes. This has the advantage of
/// keeping all the kernel housekeeping within the kernel object itself
/// but is a little mucky as the kernel object needs to 'know' about all
/// Firtree runtimes.

using namespace llvm;

namespace Firtree {

//=============================================================================
/// An implementation of the ExtentProvider interface that provide a 'standard'
/// algorithm.
class StandardExtendProvider : public ExtentProvider
{
    public:
        StandardExtendProvider(const char* samplerName, float deltaX, 
                float deltaY)
            :   ExtentProvider()
            ,   m_Delta(deltaX, deltaY)
        {
            // If the passed sampler name is non-NULL, initialise
            // our copy of it.
            if(samplerName != NULL)
            {
                m_SamplerName = samplerName;
            }
        }

        virtual ~StandardExtendProvider()
        {
        }

        virtual Rect2D ComputeExtentForKernel(Kernel* kernel)
        {
            return Rect2D::MakeInfinite();

            // FIXME
#if 0
            if(kernel == NULL)
            {
                FIRTREE_ERROR("Passed a null kernel.");
                return Rect2D::MakeInfinite();
            }

            //FIRTREE_DEBUG("%p -> compute extent.", kernel);

            const char* parameterName = NULL;
            if(!m_SamplerName.empty())
            {
                parameterName = m_SamplerName.c_str();
            }

            // If we don't have a parameter name, find each sampler the kernel
            // uses and form a union.
            if(parameterName == NULL)
            {
                //FIRTREE_DEBUG("No specified parameter, searching.");

                Rect2D extentRect = Rect2D::MakeInfinite();
                bool foundOneSampler = false;
                const std::vector<std::string>& paramNames = 
                    kernel->GetParameterNames();

                for(std::vector<std::string>::const_iterator i = paramNames.begin();
                        i != paramNames.end(); i++)
                {
                    SamplerParameter* sp = dynamic_cast<SamplerParameter*>
                        (kernel->GetParameterForKey((*i).c_str()));
                    if(sp != NULL)
                    {
                        Rect2D samplerExtent = sp->GetExtent();
                        /*
                        FIRTREE_DEBUG("Found: '%s' rect = %f,%f+%f+%f",
                                (*i).c_str(), 
                                samplerExtent.Origin.X, samplerExtent.Origin.Y,
                                samplerExtent.Size.Width, samplerExtent.Size.Height);
                                */
                        if(foundOneSampler) {
                            extentRect = Rect2D::Union(extentRect, samplerExtent);
                        } else {
                            extentRect = samplerExtent;
                        }
                        foundOneSampler = true;
                    }
                }

                return Rect2D::Inset(extentRect, m_Delta.Width, m_Delta.Height);
            }

            //FIRTREE_DEBUG("Using specified parameter: %s", m_SamplerName.c_str());

            if(parameterName == NULL)
            {
                FIRTREE_WARNING("Could not find valid sampler parameter in kernel from "
                         "which to use extent.");
                return Rect2D::MakeInfinite();
            }

            SamplerParameter* sp = dynamic_cast<SamplerParameter*>
                    (kernel->GetParameterForKey(parameterName));

            Rect2D samplerExtent = sp->GetExtent();

            return Rect2D::Inset(samplerExtent, m_Delta.Width, m_Delta.Height);
#endif
        }

    private:
        std::string     m_SamplerName;
        Size2D          m_Delta;
};

//=============================================================================
ExtentProvider* ExtentProvider::CreateStandardExtentProvider(const char* samplerName,
        float deltaX, float deltaY)
{
    return new StandardExtendProvider(samplerName, deltaX, deltaY);
}

//=============================================================================
Kernel::Kernel(const char* source)
    :   ReferenceCounted()
{
    // Create a LLVM-based kernel and keep a reference.
    LLVM::CompiledKernel* llvm_kernel = LLVM::CompiledKernel::Create();
    m_bCompileStatus = llvm_kernel->Compile(&source, 1);

    std::ostringstream log_string(std::ostringstream::out);
    uint32_t line_count = 0;
    const char *const * log = llvm_kernel->GetCompileLog(&line_count);
    while(line_count > 0) {
        log_string << *log << "\n";
        --line_count;
        ++log;
    }
    m_CompileLog = log_string.str();

    m_CompiledSource = source;

    m_CompiledKernel = llvm_kernel;
}

//=============================================================================
Kernel::~Kernel()
{
    // Free any parameter values.
    for(ParameterValueMap::iterator i=m_ParameterValues.begin();
            i!=m_ParameterValues.end(); ++i)
    {
        if(i->second) {
            i->second->Release();
        }
    }
    m_ParameterValues.clear();

    // Free any parameter samplers.
    for(ParameterSamplerMap::iterator i=m_ParameterSamplers.begin();
            i!=m_ParameterSamplers.end(); ++i)
    {
        if(i->second) {
            i->second->Release();
        }
    }
    m_ParameterSamplers.clear();

    FIRTREE_SAFE_RELEASE(m_CompiledKernel);
}

//=============================================================================
const LLVM::KernelFunction& Kernel::GetFunctionRecord() const
{
    return *(m_CompiledKernel->begin());
}

//=============================================================================
Kernel* Kernel::CreateFromSource(const char* source)
{
    return new Kernel(source);
}

//=============================================================================
bool Kernel::GetStatus() const
{
    return m_bCompileStatus;
}

//=============================================================================
const char* Kernel::GetCompileLog() const
{
    return m_CompileLog.c_str();
}

//=============================================================================
const char* Kernel::GetSource() const
{
    return m_CompiledSource.c_str();
}

//=============================================================================
const Value* Kernel::GetValueForKey(const char* key) const
{
    ParameterValueMap::const_iterator i = m_ParameterValues.find(key);
    if(i == m_ParameterValues.end()) {
        return NULL;
    }
    return i->second;
}

//=============================================================================
bool Kernel::IsValid() const
{
    if(!GetStatus()) {
        return false;
    }

    // Iterate over all parameters and ensure static params.
    // have non-NULL values.
    const LLVM::KernelFunction& func = GetFunctionRecord();
    for(LLVM::KernelParameterList::const_iterator i=func.Parameters.begin();
            i!=func.Parameters.end(); ++i)
    {
        if(i->Type == Firtree::TySpecSampler) {
            assert(i->IsStatic);
            if(m_ParameterSamplers.count(i->Name) == 0) {
                return false;
            }
        } else {
            if(i->IsStatic) {
                const Value* v = GetValueForKey(i->Name.c_str());
                if(v == NULL) {
                    return false;
                }
            }
        }
    }
    return true;
}

//===================================================================
const llvm::Type* KernelTypeToLLVM(KernelTypeSpecifier type)
{
    switch(type) {
        case TySpecFloat:
            return Type::FloatTy;

        case TySpecInt:
        case TySpecSampler:
            return Type::Int32Ty;

        case TySpecBool:
            return Type::Int1Ty;

        case TySpecVec2:
            return VectorType::get( Type::FloatTy, 2 );

        case TySpecVec3:
            return VectorType::get( Type::FloatTy, 3 );

        case TySpecVec4:
        case TySpecColor:
            return VectorType::get( Type::FloatTy, 4 );

        case TySpecVoid:
            return Type::VoidTy;

        default:
            FIRTREE_ERROR("Unknown type: %i", type);
            break;
    }

    return NULL;
}

//===================================================================
void RunOptimiser(llvm::Module* m) 
{
    if(m == NULL)
    {
        return;
    }

    PassManager PM;

    PM.add(new TargetData(m));

    PM.add(createVerifierPass());

    PM.add(createStripSymbolsPass(true));
    PM.add(createRaiseAllocationsPass());     // call %malloc -> malloc inst
    PM.add(createCFGSimplificationPass());    // Clean up disgusting code
    PM.add(createPromoteMemoryToRegisterPass());// Kill useless allocas
    PM.add(createGlobalOptimizerPass());      // Optimize out global vars
    PM.add(createGlobalDCEPass());            // Remove unused fns and globs
    PM.add(createIPConstantPropagationPass());// IP Constant Propagation
    PM.add(createDeadArgEliminationPass());   // Dead argument elimination
    PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
    PM.add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

    PM.add(createPruneEHPass());              // Remove dead EH info

    PM.add(createFunctionInliningPass());   // Inline small functions
    PM.add(createArgumentPromotionPass());    // Scalarize uninlined fn args

    PM.add(createTailDuplicationPass());      // Simplify cfg by copying code
    PM.add(createInstructionCombiningPass()); // Cleanup for scalarrepl.
    PM.add(createCFGSimplificationPass());    // Merge & remove BBs
    PM.add(createScalarReplAggregatesPass()); // Break up aggregate allocas
    PM.add(createInstructionCombiningPass()); // Combine silly seq's
    PM.add(createCondPropagationPass());      // Propagate conditionals

    PM.add(createTailCallEliminationPass());  // Eliminate tail calls
    PM.add(createCFGSimplificationPass());    // Merge & remove BBs
    PM.add(createReassociatePass());          // Reassociate expressions
    PM.add(createLoopRotatePass());
    PM.add(createLICMPass());                 // Hoist loop invariants
    PM.add(createLoopUnswitchPass());         // Unswitch loops.
    PM.add(createLoopIndexSplitPass());       // Index split loops.
    PM.add(createInstructionCombiningPass()); // Clean up after LICM/reassoc
    PM.add(createIndVarSimplifyPass());       // Canonicalize indvars
    PM.add(createLoopUnrollPass());           // Unroll small loops
    PM.add(createInstructionCombiningPass()); // Clean up after the unroller
    PM.add(createGVNPass());                  // Remove redundancies
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

//===================================================================
llvm::Value* ConstantVector(float* v, int n)
{
    std::vector<Constant*> elements;

    for ( int i=0; i<n; i++ ) {
        elements.push_back( ConstantFP::get( Type::FloatTy, 
                    (double)(v[i])));
    }

    llvm::Value* val = llvm::ConstantVector::get( elements );
    return val;
}

//===================================================================
llvm::Value* ConstantVector(float x, float y, float z, float w)
{
    float v[] = {x,y,z,w};
    return ConstantVector(v,4);
}

//===================================================================
/// Returns the value of the specified parameter as a LLVM constant.
llvm::Value* GetValueAsLLVMConstant(const Value* ft_val)
{
    if(ft_val == NULL) { 
        FIRTREE_ERROR("Internal error. GetParamAsLLVMConstant() "
                "value is unset.");
        return NULL;
    }

    switch(ft_val->GetType()) {
        case Firtree::TySpecInt:
            return llvm::ConstantInt::get( llvm::Type::Int32Ty, 
                    ft_val->GetIntValue() );
            break;
        case Firtree::TySpecBool:
            return llvm::ConstantInt::get( llvm::Type::Int1Ty, 
                    ft_val->GetIntValue() );
            break;
        case Firtree::TySpecFloat:
            return llvm::ConstantFP::get( llvm::Type::FloatTy,
                    (double)(ft_val->GetFloatValue()) );
            break;
        case Firtree::TySpecVec2:
        case Firtree::TySpecVec3:
        case Firtree::TySpecVec4:
            {
                unsigned arity = ft_val->GetArity();
                std::vector<llvm::Constant*> elements;
                for ( unsigned i=0; i<arity; ++i ) {
                    elements.push_back( 
                            llvm::ConstantFP::get( 
                                llvm::Type::FloatTy, 
                                (double)(ft_val->GetVectorValue(i))));
                }
                return llvm::ConstantVector::get( elements );
            }
            break;
        default:
            FIRTREE_ERROR("Internal error. GetParamAsLLVMConstant() "
                    "parameter value is of invalid type.");
            break;
    }

    return NULL;
}

//=============================================================================
llvm::Module* Kernel::CreateSamplerModule(const std::string& prefix)
{
    if(!IsValid()) {
        FIRTREE_WARNING("Cannot create module for invalid kernel.");
        return NULL;
    }

    const LLVM::KernelFunction& func_record = GetFunctionRecord();

    // Clone the kernel module.
    llvm::Module* new_module = llvm::CloneModule(
            func_record.Function->getParent());
    Function* kernel_function = new_module->getFunction(
            func_record.Function->getName());

    if(!kernel_function) {
        FIRTREE_ERROR("Internal error: no kernel function.");
        return NULL;
    }

    // Start adding some functions.

    // SAMPLE function
    std::vector<const Type*> sample_params;

    sample_params.push_back( VectorType::get( Type::FloatTy, 2 ) );
    for(LLVM::KernelParameterList::const_iterator 
            param_it=func_record.Parameters.begin(); 
            param_it!=func_record.Parameters.end(); ++param_it)
    {
        if(!param_it->IsStatic) {
            sample_params.push_back(
                    KernelTypeToLLVM(param_it->Type));
        }
    }

    FunctionType *sample_FT = FunctionType::get(
            VectorType::get( Type::FloatTy, 4 ),
            sample_params, false );
    Function* sample_F = Function::Create( sample_FT,
            Function::ExternalLinkage,
            prefix + "Sample",
            new_module );	
    BasicBlock *sample_BB = BasicBlock::Create( "entry", 
            sample_F );

    // Set the requested co-ordinate parameter.
    sample_F->arg_begin()->setName("coord");

    std::vector<llvm::Value*> kernel_params;
    Function::arg_iterator arg_it = sample_F->arg_begin();

    // Push the implicit co-ordinate parameter for the
    // kernel.
    kernel_params.push_back(llvm::cast<llvm::Value>(arg_it));
    ++arg_it;

    for(LLVM::KernelParameterList::const_iterator 
            param_it=func_record.Parameters.begin(); 
            param_it!=func_record.Parameters.end(); ++param_it)
    {
        if(param_it->IsStatic) {
            const Value* val = GetValueForKey(param_it->Name.c_str());
            kernel_params.push_back(GetValueAsLLVMConstant(val));
        } else {
            kernel_params.push_back(
                    llvm::cast<llvm::Value>(arg_it));
            ++arg_it;
        }
    }

    llvm::Value* sample_val = CallInst::Create(
            kernel_function,
            kernel_params.begin(), kernel_params.end(),
            "tmp", sample_BB);

    ReturnInst::Create( sample_val, sample_BB );

    RunOptimiser(new_module);

    return new_module;
}

//=============================================================================
void Kernel::SetValueForKey(Image* image, const char* key)
{
    // Firstly see if the value for this key is alreay this image.
    // If so, don't bother creating a new sampler.
#if 0
    if(m_ParameterSamplers.count(key) > 0) {
        if(m_ParameterSamplers[key]->GetImage() == image) {
            return;
        }
    }
#endif

    FIRTREE_DEBUG("Performance hint: re-wiring pipeline is expensive.");

    LLVM::SamplerProvider* sampler_prov = LLVM::SamplerProvider::
        CreateFromImage(image);

    SetSamplerProviderForKey(sampler_prov, key);

    FIRTREE_SAFE_RELEASE(sampler_prov);
}

//=============================================================================
void Kernel::SetSamplerProviderForKey(LLVM::SamplerProvider* sampler_prov, 
        const char* key)
{
    const LLVM::KernelParameterList& params = GetFunctionRecord().Parameters;

    for(LLVM::KernelParameterList::const_iterator param = params.begin();
            param != params.end(); ++param) {
        if(param->Name == key) {
            ParameterSamplerMap::iterator entry = 
                m_ParameterSamplers.find(param->Name);

            if(entry != m_ParameterSamplers.end()) {
                FIRTREE_SAFE_RELEASE(entry->second);
                FIRTREE_SAFE_RETAIN(sampler_prov);
                entry->second = sampler_prov;
            } else {
                FIRTREE_SAFE_RETAIN(sampler_prov);
                m_ParameterSamplers[param->Name] = sampler_prov;
            }

            return;
        }
    }

    FIRTREE_WARNING("Attempt to set unknown parameter '%s'.\n", key);
}

//=============================================================================
const LLVM::SamplerProvider* Kernel::GetSamplerProviderForKey(
        const char* key) const
{
    ParameterSamplerMap::const_iterator i = m_ParameterSamplers.find(key);
    if(i == m_ParameterSamplers.end()) {
        return NULL;
    }
    return i->second;
}

//=============================================================================
void Kernel::SetValueForKey(const Value* value, const char* key)
{
    const LLVM::KernelParameterList& params = GetFunctionRecord().Parameters;

    for(LLVM::KernelParameterList::const_iterator param = params.begin();
            param != params.end(); ++param) {
        if(param->Name == key) {
            ParameterValueMap::iterator entry = 
                m_ParameterValues.find(param->Name);

            Value* cloned_val = NULL;
            if(value) {
                cloned_val = value->Clone();
            }

            if(entry != m_ParameterValues.end()) {
                FIRTREE_SAFE_RELEASE(entry->second);
                entry->second = cloned_val;
            } else {
                m_ParameterValues[param->Name] = cloned_val;
            }

            return;
        }
    }

    FIRTREE_WARNING("Attempt to set unknown parameter '%s'.\n", key);
}

//=============================================================================
void Kernel::SetValueForKey(float value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, const char* key)
{
    float v[] = {x, y};
    SetValueForKey(v, 2, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, float z, const char* key)
{
    float v[] = {x, y, z};
    SetValueForKey(v, 3, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, float z, float w, 
        const char* key)
{
    float v[] = {x, y, z, w};
    SetValueForKey(v, 4, key);
}

//=============================================================================
void Kernel::SetValueForKey(const float* value, int count, const char* key)
{
    Value* v = Value::CreateVectorValue(value, count);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

//=============================================================================
void Kernel::SetValueForKey(int value, const char* key)
{
    Value* v = Value::CreateIntValue(value);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

//=============================================================================
void Kernel::SetValueForKey(bool value, const char* key)
{
    Value* v = Value::CreateBoolValue(value);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
