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

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Instructions.h>
#include <llvm/Linker.h>

#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LinkAllPasses.h"

#include <firtree/main.h>
#include <firtree/linker/sampler_provider.h>
#include <firtree/value.h>
#include "internal/image-int.h"
#include <firtree/kernel.h>

#include "../compiler/llvm-code-gen/llvm_frontend.h"
#include "../compiler/llvm-code-gen/llvm_private.h"

namespace llvm { class Module; }

namespace Firtree { class LLVMFrontend; }

namespace Firtree { namespace LLVM {

using namespace llvm;

//===================================================================
llvm::Value* _ConstantVector(float* v, int n)
{
    std::vector<Constant*> elements;

    for ( int i=0; i<n; i++ ) {
        elements.push_back( ConstantFP::get( Type::FloatTy, 
                    (double)(v[i])));
    }

    llvm::Value* val = ConstantVector::get( elements );
    return val;
}

//===================================================================
llvm::Value* _ConstantVector(float x, float y, float z, float w)
{
    float v[] = {x,y,z,w};
    return _ConstantVector(v,4);
}

//===========================================================================
/// Specialised kernel sampler provider class.
class KernelImageSamplerProvider : public SamplerProvider
{
    public:
        //===================================================================
        KernelImageSamplerProvider(const Image* image)
            :   SamplerProvider(image)
            ,   m_Kernel(NULL)
        {
            const Firtree::Internal::ImageImpl* imImpl = 
                dynamic_cast<const Firtree::Internal::ImageImpl*>(image);
            if(imImpl == NULL) {
                FIRTREE_ERROR("Passed an invalid image.");
            }

            // See if this image has a kernel.
            Kernel* kernel = imImpl->GetKernel();
            if(!kernel) {
                FIRTREE_ERROR("Asked to construct kernel image sampler "
                        "provider from non-kernel image.");
            }

            m_Kernel = kernel;
            FIRTREE_SAFE_RETAIN(m_Kernel);
        }

        //===================================================================
        virtual ~KernelImageSamplerProvider()
        {
            FIRTREE_SAFE_RELEASE(m_Kernel);
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
            if(!m_Kernel->IsValid()) {
                FIRTREE_WARNING("Cannot create sampler module for invalid "
                        "sampler.");
                return NULL;
            }

            // Create the kernel module.
            llvm::Module* new_module = m_Kernel->CreateSamplerModule(prefix);

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
            return m_Kernel->GetFunctionRecord().Parameters.begin();
        }

        //===================================================================
		/// Return an iterator pointing to the end of the parameter list.
		virtual const_iterator end() const 
        {
            return m_Kernel->GetFunctionRecord().Parameters.end();
        }

        //===================================================================
		/// Return a flag indicating if the sampler is in a valid state.
		/// This usually means that all the static parameters have been
		/// set to some non-NULL value. The return values of 
		/// Get{.*}Function() are undefined if this flag is false.
		virtual bool IsValid() const
        {
            return m_Kernel->IsValid();
        }

        //===================================================================
        virtual Kernel* GetKernel() const 
        {
            return m_Kernel;
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
            m_Kernel->SetValueForKey(value, param->Name.c_str());

            // FIXME
            return false;
        }

        //===================================================================
		/// Get the value of the referenced parameter.
		virtual const Value* GetParameterValue(
                const const_iterator& param) const
        {
            return m_Kernel->GetValueForKey(param->Name.c_str());
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
            m_Kernel->SetSamplerProviderForKey(value, param->Name.c_str());
        }

        //===================================================================
		/// Get the sampler for the referenced parameter.
		virtual SamplerProvider* GetParameterSampler(
                const const_iterator& param) const
        {
            return const_cast<SamplerProvider*>(m_Kernel->GetSamplerProviderForKey(param->Name.c_str()));
        }

    private:
        Kernel*     m_Kernel;
};

//===========================================================================
SamplerProvider* SamplerProvider::CreateFromKernelImage(
        const Image* image)
{
    return new KernelImageSamplerProvider(image);
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
