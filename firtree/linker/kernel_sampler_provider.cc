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
// This file implements the Firtree kernel sampler provider.
//===========================================================================

#include <firtree/main.h>
#include <firtree/linker/sampler_provider.h>
#include <firtree/value.h>

#include "../compiler/llvm-code-gen/llvm_frontend.h"
#include "../compiler/llvm-code-gen/llvm_private.h"

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

namespace llvm { class Module; }

namespace Firtree { class LLVMFrontend; }

namespace Firtree { namespace LLVM {

using namespace llvm;

//===========================================================================
/// Specialised kernel sampler provider class.
class KernelSamplerProvider : public SamplerProvider
{
    public:
        //===================================================================
        KernelSamplerProvider(const CompiledKernel* kernel,
				const std::string& kernel_name)
            :   SamplerProvider()
            ,   m_ClonedKernelModule(NULL)
        {
            if(kernel == NULL) {
                FIRTREE_ERROR("Passed a NULL kernel.");
            }

            if(!kernel->GetCompileStatus()) {
                return;
            }

            CompiledKernel::const_iterator kernel_it;

            if(kernel_name.empty()) {
                kernel_it = kernel->begin();
            } else {
                kernel_it = kernel->find(kernel_name);
            }

            if(kernel_it == kernel->end())
            {
                // No kernel, sampler is invalid.
                return;
            }

            // Clone the kernel's module.
            m_ClonedKernelModule = llvm::CloneModule(
                    kernel_it->Function->getParent());

            // Clone the kernel function record and store the
            // newly cloned function.
            m_ClonedFunction = *(kernel_it);
            m_ClonedFunction.Function = 
                m_ClonedKernelModule->getFunction(
                        kernel_it->Function->getName());
        }

        //===================================================================
        virtual ~KernelSamplerProvider()
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

            if(m_ClonedKernelModule) {
                delete m_ClonedKernelModule;
            }
        }

        //===================================================================
		/// Create a LLVM module which *only* has three exported functions:
		///
		/// vec4 ${prefix}Sample(vec2 coord)
		/// vec2 ${prefix}Transform(vec2 coord)
		/// vec4 ${prefix}Extent()
        ///
		/// The caller now 'owns' the returned module and must call 'delete'
		/// on it.
        virtual llvm::Module* CreateSamplerModule(const std::string& prefix,
                uint32_t)
        {
            if(!IsValid()) {
                FIRTREE_WARNING("Cannot create sampler module for invalid "
                        "sampler.");
                return NULL;
            }

            // Clone the kernel module.
            llvm::Module* new_module = 
                llvm::CloneModule(m_ClonedKernelModule);

            Function* kernel_function = new_module->getFunction(
                    m_ClonedFunction.Function->getName());
            if(!kernel_function) {
                FIRTREE_ERROR("Internal error: no kernel function.");
                return NULL;
            }

            // Start adding some functions.

            // EXTENT function FIXME
            std::vector<const Type*> extent_params;
            FunctionType *extent_FT = FunctionType::get(
                    VectorType::get( Type::FloatTy, 4 ),
                    extent_params, false );
            Function* extent_F = LLVM_CREATE( Function, extent_FT,
                    Function::ExternalLinkage,
                    prefix + "Extent",
                    new_module );	
            BasicBlock *extent_BB = LLVM_CREATE( BasicBlock, "entry", 
                    extent_F );

            llvm::Value* extent_val = ConstantVector(0,0,0,0);
            LLVM_CREATE( ReturnInst, extent_val, extent_BB );

            // TRANSFORM function FIXME
            std::vector<const Type*> trans_params;
            trans_params.push_back(VectorType::get( Type::FloatTy, 2 ));
            FunctionType *trans_FT = FunctionType::get(
                    VectorType::get( Type::FloatTy, 2 ),
                    trans_params, false );
            Function* trans_F = LLVM_CREATE( Function, trans_FT,
                    Function::ExternalLinkage,
                    prefix + "Transform",
                    new_module );	
            BasicBlock *trans_BB = LLVM_CREATE( BasicBlock, "entry", 
                    trans_F );

            LLVM_CREATE( ReturnInst, 
                    llvm::cast<llvm::Value>(trans_F->arg_begin()),
                    trans_BB );

            // SAMPLE function
            std::vector<const Type*> sample_params;

            sample_params.push_back( VectorType::get( Type::FloatTy, 2 ) );
            for(const_iterator param_it=begin(); param_it!=end(); ++param_it)
            {
                if(!param_it->IsStatic) {
                    sample_params.push_back(
                            KernelTypeToLLVM(param_it->Type));
                }
            }

            FunctionType *sample_FT = FunctionType::get(
                    VectorType::get( Type::FloatTy, 4 ),
                    sample_params, false );
            Function* sample_F = LLVM_CREATE( Function, sample_FT,
                    Function::ExternalLinkage,
                    prefix + "Sample",
                    new_module );	
            BasicBlock *sample_BB = LLVM_CREATE( BasicBlock, "entry", 
                    sample_F );

            // Set the requested co-ordinate parameter.
            sample_F->arg_begin()->setName("coord");

            std::vector<llvm::Value*> kernel_params;
            Function::arg_iterator arg_it = sample_F->arg_begin();

            // Push the implicit co-ordinate parameter for the
            // kernel.
            kernel_params.push_back(llvm::cast<llvm::Value>(arg_it));
            ++arg_it;

            for(const_iterator param_it=begin(); param_it!=end(); ++param_it)
            {
                if(param_it->IsStatic) {
                    kernel_params.push_back(
                            GetParamAsLLVMConstant(param_it));
                } else {
                    kernel_params.push_back(
                            llvm::cast<llvm::Value>(arg_it));
                    ++arg_it;
                }
            }

            llvm::Value* sample_val = LLVM_CREATE(CallInst,
                    kernel_function,
                    kernel_params.begin(), kernel_params.end(),
                    "tmp", sample_BB);

            LLVM_CREATE( ReturnInst, sample_val, sample_BB );

            RunOptimiser(new_module);

            return new_module;
        }

        //===================================================================
		/// Return a const iterator pointing to the parameter named
		/// 'name'. If no such parameter exists, return the
		/// same iterator as end().
		virtual const_iterator find(const std::string& name) const
        {
            for(const_iterator i = begin(); i != end(); ++i)
            {
                if(i->Name == name) {
                    return i;
                }
            }
            return end();
        }

        //===================================================================
		/// Return an iterator pointing to the start of the parameter list.
		virtual const_iterator begin() const
        {
            return m_ClonedFunction.Parameters.begin();
        }

        //===================================================================
		/// Return an iterator pointing to the end of the parameter list.
		virtual const_iterator end() const 
        {
            return m_ClonedFunction.Parameters.end();
        }

        //===================================================================
		/// Return a flag indicating if the sampler is in a valid state.
		/// This usually means that all the static parameters have been
		/// set to some non-NULL value. The return values of 
		/// Get{.*}Function() are undefined if this flag is false.
		virtual bool IsValid() const
        {
            if(!m_ClonedKernelModule) {
                return false;
            }

            // Iterate over all parameters and ensure static params.
            // have non-NULL values.
            for(const_iterator i=begin(); i!=end(); ++i)
            {
                if(i->Type == Firtree::TySpecSampler) {
                    assert(i->IsStatic);
                    if(m_ParameterSamplers.count(i->Name) == 0) {
                        return false;
                    }
                } else {
                    if(i->IsStatic) {
                        const Value* v = GetParameterValue(i);
                        if(v == NULL) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        //===================================================================
		/// Set the parameter pointed to by a given iterator to the passed
		/// value. Static parameters can cause the value of 
		/// Get{.*}Function() to change. As a convenience this should 
		/// return 'true' if a static parameter was changed.
		/// Throws an error is param == end().
		virtual bool SetParameterValue(const const_iterator& param,
				const Value* value)
        {
            if(param == end()) {
                FIRTREE_ERROR("Cannot set parameter value from invalid "
                        "iterator.");
            }

            if(m_ParameterValues.count(param->Name) != 0)
            {
                FIRTREE_SAFE_RELEASE(m_ParameterValues[param->Name]);
            }

            if(value == NULL) {
                m_ParameterValues[param->Name] = NULL;
            } else {
                m_ParameterValues[param->Name] = value->Clone();
            }

            // If all the static parameters are set and we've just set
            // one, re-compile.
            if(IsValid() && (param->IsStatic)) {
                ReCompile();
            }

            return param->IsStatic;
        }

        //===================================================================
		/// Get the value of the referenced parameter.
		virtual const Value* GetParameterValue(
                const const_iterator& param) const
        {
            ParameterValueMap::const_iterator i = m_ParameterValues.
                find(param->Name);
            if(i == m_ParameterValues.end()) {
                return NULL;
            }
            return i->second;
        }

        //===================================================================
		/// Set the parameter pointed to by a given iterator to be
		/// associated with the sampler provider 'provider'. It is an 
		/// error for the parameter iterator to point to a non-sampler
		/// parameter.
		///
		/// The provider can be NULL in which case the semantic is to 'unset'
		/// the parameter and release any references associated with it.
		/// Throws an error is param == end().
		virtual void SetParameterSampler(const const_iterator& param,
				SamplerProvider* value)
        {
            if(param == end()) {
                FIRTREE_ERROR("Cannot set parameter value from invalid "
                        "iterator.");
            }

            if(m_ParameterSamplers.count(param->Name) != 0)
            {
                FIRTREE_SAFE_RELEASE(m_ParameterSamplers[param->Name]);
            }

            if(value == NULL) {
                m_ParameterSamplers[param->Name] = NULL;
            } else {
                m_ParameterSamplers[param->Name] = value;
                FIRTREE_SAFE_RETAIN(value);
            }

            // If all the static parameters are set, re-compile.
            if(IsValid()) {
                ReCompile();
            }
        }

        //===================================================================
		/// Get the sampler for the referenced parameter.
		virtual SamplerProvider* GetParameterSampler(
                const const_iterator& param) const
        {
            ParameterSamplerMap::const_iterator i = m_ParameterSamplers.
                find(param->Name);
            if(i == m_ParameterSamplers.end()) {
                return NULL;
            }
            return i->second;
        }

    private:
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
        llvm::Value* ConstantVector(float x, float y, float z, float w)
        {
            float v[] = {x,y,z,w};
            return ConstantVector(v,4);
        }
  
        //===================================================================
        llvm::Value* ConstantVector(float* v, int n)
        {
            std::vector<Constant*> elements;

            for ( int i=0; i<n; i++ ) {
#if LLVM_AT_LEAST_2_3
                elements.push_back( ConstantFP::get( Type::FloatTy, 
                            (double)(v[i])));
#else
                elements.push_back( ConstantFP::get( Type::FloatTy,
                            APFloat( v[i] ) ) );
#endif
            }

            llvm::Value* val = ConstantVector::get( elements );
            return val;
        }

        //===================================================================
        /// Returns the value of the specified parameter as a LLVM constant.
        llvm::Value* GetParamAsLLVMConstant(const const_iterator& param)
        {
            const Value* ft_val = GetParameterValue(param);
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
#if LLVM_AT_LEAST_2_3
                    return llvm::ConstantFP::get( llvm::Type::FloatTy,
                            (double)(ft_val->GetFloatValue()) );
#else
                    return llvm::ConstantFP::get( llvm::Type::FloatTy,
                            llvm::APFloat( ft_val->GetFloatValue() ) );
#endif
                    break;
                case Firtree::TySpecVec2:
                case Firtree::TySpecVec3:
                case Firtree::TySpecVec4:
                    {
                        unsigned arity = ft_val->GetArity();
                        std::vector<llvm::Constant*> elements;
                        for ( unsigned i=0; i<arity; ++i ) {
#if LLVM_AT_LEAST_2_3
                            elements.push_back( 
                                    llvm::ConstantFP::get( 
                                        llvm::Type::FloatTy, 
                                        (double)(ft_val->GetVectorValue(i))));
#else
                            elements.push_back( 
                                    llvm::ConstantFP::get( 
                                        llvm::Type::FloatTy,
                                        llvm::APFloat( ft_val->GetVectorValue(i) ) ) );
#endif
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

        //===================================================================
        /// Called when the sampler functions, etc need recompiling.
        void ReCompile()
        {
        }

        typedef HASH_NAMESPACE::hash_map<std::string, Value*> 
            ParameterValueMap;
        typedef HASH_NAMESPACE::hash_map<std::string, SamplerProvider*> 
            ParameterSamplerMap;

        llvm::Module*       m_ClonedKernelModule;
        KernelFunction      m_ClonedFunction;
        ParameterValueMap   m_ParameterValues;
        ParameterSamplerMap   m_ParameterSamplers;
};

//===========================================================================
SamplerProvider* SamplerProvider::CreateFromCompiledKernel(
        const CompiledKernel* kernel,
        const std::string& kernel_name)
{
    return new KernelSamplerProvider(kernel, kernel_name);
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
