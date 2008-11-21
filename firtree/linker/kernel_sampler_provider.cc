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
#include <llvm/ADT/hash_map>
#include <llvm/Instructions.h>

namespace llvm { class Module; }

namespace Firtree { class LLVMFrontend; }

namespace Firtree { namespace LLVM {

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
            ,   m_SampleFunction(NULL)
        {
            if(kernel == NULL) {
                FIRTREE_ERROR("Passed a NULL kernel.");
            }

            CompiledKernel::const_iterator kernel_it;

            if(kernel_name.empty()) {
                kernel_it = kernel->begin();
            } else {
                kernel_it = kernel->find(kernel_name);
            }

            if(kernel_it == kernel->end())
            {
                FIRTREE_ERROR("No such kernel '%s'.",
                        kernel_name.c_str());
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
            for(const_iterator i=begin(); i!=end(); ++i)
            {
                SetParameterValue(i, NULL);
            }

            delete m_ClonedKernelModule;
        }

        //===================================================================
		/// Get the sample function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetSampleFunction() const
        {
            return m_SampleFunction;
        }

        //===================================================================
		/// Get the transform function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetTransformFunction() const
        {
            return NULL;
        }

        //===================================================================
		/// Get the extent function. The returned value may be 
		/// invalidated after a static parameter is set.
		virtual const llvm::Function* GetExtentFunction() const
        {
            return NULL;
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
            // Iterate over all parameters and ensure static params.
            // have non-NULL values.
            for(const_iterator i=begin(); i!=end(); ++i)
            {
                if(i->IsStatic) {
                    const Value* v = GetParameterValue(i);
                    if(v == NULL) {
                        return false;
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

    private:
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
                                        llvm:Type::FloatTy, 
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
            // Remove the sample function from it's parent if
            // necessary
            if(m_SampleFunction != NULL)
            {
                m_SampleFunction->removeFromParent();

                // FIXME: Do we need to do this of will the module do it
                // for us?
                delete m_SampleFunction;
            }

            // ===== Build a function for sampling. =====
            
            // A vector which will hold the types of our non-static
            // parameters.
            std::vector<const llvm::Type*> param_llvm_types;
            for(const_iterator it=begin(); it!=end(); ++it)
            {
                if(!it->IsStatic) {
                    FullType ft(TyQualNone, it->Type);
                    const llvm::Type* param_type = ft.ToLLVMType(NULL);
                    param_llvm_types.push_back(param_type);
                }
            }

            // Create a LLVM type for this function.
            FullType return_type(TyQualNone, TySpecVec4);
            llvm::FunctionType *FT = llvm::FunctionType::get(
                    return_type.ToLLVMType( NULL ),
                    param_llvm_types, false );

            // Create the function proper.
            m_SampleFunction = LLVM_CREATE( llvm::Function, FT,
                    llvm::Function::ExternalLinkage,
                    "FIXME-make-unique", m_ClonedKernelModule );

            // Create a basic block to hold the function body.
            llvm::BasicBlock *BB = LLVM_CREATE( llvm::BasicBlock, "entry",
                    m_SampleFunction );

            // Create a vector which will store LLVM values for the
            // kernel arguments. On the way, set all the non-static
            // parameters' names.
            std::vector<llvm::Value*> kernel_args;
            llvm::Function::arg_iterator ai = m_SampleFunction->arg_begin();
            for(const_iterator pi=begin(); pi != end(); ++pi)
            {
                if(pi->IsStatic) {
                    kernel_args.push_back(GetParamAsLLVMConstant(pi));
                } else {
                    ai->setName(pi->Name);
                    kernel_args.push_back(llvm::cast<llvm::Value>(ai)); 
                    ++ai;
                }
            }

            // Emit a call instruction which calls the kernel itself.
            llvm::Value* func_ret_val = LLVM_CREATE( llvm::CallInst,
                    m_ClonedFunction.Function,
                    kernel_args.begin(), kernel_args.end(),
                    "returnvalue", BB);

            // Emit a return instruction.
            LLVM_CREATE( llvm::ReturnInst, func_ret_val, BB );
        }

        typedef HASH_NAMESPACE::hash_map<std::string, Value*> 
            ParameterValueMap;

        llvm::Module*       m_ClonedKernelModule;
        KernelFunction      m_ClonedFunction;
        ParameterValueMap   m_ParameterValues;

        llvm::Function*     m_SampleFunction;
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
