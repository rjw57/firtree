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
llvm::Module* SamplerProvider::LinkSamplerModule()
{
    std::vector< std::pair<std::string,SamplerProvider*> > sampler_queue;
    sampler_queue.push_back(
            std::pair<std::string,SamplerProvider*>("", this));

    llvm::Linker* linker = new llvm::Linker("firtree", "module");

    int sampler_idx = 0;
    while(sampler_queue.size() > 0) {
        std::pair<std::string,SamplerProvider*> next = sampler_queue.back();
        sampler_queue.pop_back();

        SamplerProvider* next_provider = next.second;

        if(!(next_provider->IsValid())) {
            FIRTREE_ERROR("Not all samplers valid.");
            return NULL;
        }

        for(const_iterator i=next_provider->begin();
                i!=next_provider->end(); ++i)
        {
            if(i->Type == Firtree::TySpecSampler) {
                std::ostringstream prefix(std::ostringstream::out);
                prefix << "sampler_" << sampler_idx << "_";

                sampler_queue.push_back(
                        std::pair<std::string,SamplerProvider*>(
                            prefix.str(),
                            next_provider->GetParameterSampler(i)));

                Firtree::Value* val = Firtree::Value::
                    CreateIntValue(sampler_idx);
                next_provider->SetParameterValue(i, val);
                FIRTREE_SAFE_RELEASE(val);

                ++sampler_idx;
            }
        }

        llvm::Module* module = next_provider->CreateSamplerModule(next.first);
        std::string err;
        linker->LinkInModule(module, &err);
        if(!err.empty()) {
            FIRTREE_ERROR("Error linking samplers: %s", err.c_str());
            return NULL;
        }
    }

    llvm::Module* ret_val = linker->releaseModule();
    delete linker;

    return ret_val;
}

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
