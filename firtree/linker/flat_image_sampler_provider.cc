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
// This file implements the Firtree flat image sampler provider.
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
class FlatImageSamplerProvider : public SamplerProvider
{
    public:
        //===================================================================
        FlatImageSamplerProvider(const Image* image)
            :   SamplerProvider()
        {
        }

        //===================================================================
        virtual ~FlatImageSamplerProvider()
        {
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

            // Create the module.
            llvm::Module* new_module = new Module(prefix);

            /*
            Function* kernel_function = new_module->getFunction(
                    m_ClonedFunction.Function->getName());
            if(!kernel_function) {
                FIRTREE_ERROR("Internal error: no kernel function.");
                return NULL;
            }
            */

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
            FunctionType *sample_FT = FunctionType::get(
                    VectorType::get( Type::FloatTy, 4 ),
                    sample_params, false );
            Function* sample_F = LLVM_CREATE( Function, sample_FT,
                    Function::ExternalLinkage,
                    prefix + "Sample",
                    new_module );	
            BasicBlock *sample_BB = LLVM_CREATE( BasicBlock, "entry", 
                    sample_F );

            llvm::Value* sample_val = ConstantVector(0,1,0,1);
            LLVM_CREATE( ReturnInst, sample_val, sample_BB );

            return new_module;
        }

        //===================================================================
		/// Return a const iterator pointing to the parameter named
		/// 'name'. If no such parameter exists, return the
		/// same iterator as end().
		virtual const_iterator find(const std::string& name) const
        {
            return end();
        }

        //===================================================================
		/// Return an iterator pointing to the start of the parameter list.
		virtual const_iterator begin() const
        {
            return m_EmptyList.begin();
        }

        //===================================================================
		/// Return an iterator pointing to the end of the parameter list.
		virtual const_iterator end() const 
        {
            return m_EmptyList.end();
        }

        //===================================================================
		/// Return a flag indicating if the sampler is in a valid state.
		/// This usually means that all the static parameters have been
		/// set to some non-NULL value. The return values of 
		/// Get{.*}Function() are undefined if this flag is false.
		virtual bool IsValid() const
        {
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
            return false;
        }

        //===================================================================
		/// Get the value of the referenced parameter.
		virtual const Value* GetParameterValue(
                const const_iterator& param) const
        {
            return NULL;
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
            /* nothing */
        }

        //===================================================================
		/// Get the sampler for the referenced parameter.
		virtual SamplerProvider* GetParameterSampler(
                const const_iterator& param) const
        {
            return NULL;
        }

    private:
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

        std::vector<Firtree::LLVM::KernelParameter> m_EmptyList;
};

//===========================================================================
SamplerProvider* SamplerProvider::CreateFromFlatImage(
        const Image* image)
{
    return new FlatImageSamplerProvider(image);
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
