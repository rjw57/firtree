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

namespace llvm { class Module; }

namespace Firtree { class LLVMFrontend; }

namespace Firtree { namespace LLVM {

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
//===========================================================================

//===========================================================================
SamplerLinker::SamplerLinker()
    :   m_LinkedModule(NULL)
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
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
