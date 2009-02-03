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
// This file implements a LLVM pass for canonicalizing calls to samplers.
//===========================================================================

#include <limits.h>
#include <string.h>
#include <vector>

#include <llvm/Pass.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/Function.h>
#include <llvm/Constants.h>

#include <firtree/main.h>

#include "../compiler/llvm-code-gen/llvm_private.h"

#include "canonicalize_sampler_calls.h"

using namespace llvm;

namespace Firtree { namespace LLVM {

//===========================================================================
CanonicalizeSamplerCallsPass::CanonicalizeSamplerCallsPass(
        const SamplerDescList& list)
    :   BasicBlockPass((intptr_t)&ID) 
    ,   m_List(list)
{
}

//===========================================================================
CanonicalizeSamplerCallsPass::~CanonicalizeSamplerCallsPass() {}

//===========================================================================
bool CanonicalizeSamplerCallsPass::runOnBasicBlock(BasicBlock &BB) {
    bool bModified = false;

    std::vector<llvm::Instruction*> to_erase;

    for(BasicBlock::iterator i=BB.begin(); i!=BB.end(); ++i) {
        if(llvm::isa<CallInst>(i)) {
            CallInst* ci = llvm::cast<CallInst>(i);
            Function* f = ci->getCalledFunction();

            // Unless we match, we don't want to replace the 
            // call.
            Function* to_replace_with = NULL;
            bool append_free_params = false;

            int64_t sampler_idx = -1;
            if((ci->getNumOperands() > 1) && 
                    (llvm::isa<ConstantInt>(ci->getOperand(1)))) {
                sampler_idx = llvm::cast<ConstantInt>(ci->getOperand(1))->
                    getValue().getSExtValue();
            }

            {
                // It is a but naughty to be using the pointer like this
                // and I have no real reason to use strcmp. This is mostly
                // so that the Daily WTF still have articles to 
                // print :).
                const char* name = f->getName().c_str();
                if(0 == strcmp("sample_sv2", name)) {
                    assert(sampler_idx >= 0);
                    to_replace_with = m_List[sampler_idx].sampleFunc;
                    append_free_params = true;
                } else if(0 == strcmp("samplerTransform_sv2", name)) {
                    assert(sampler_idx >= 0);
                    to_replace_with = m_List[sampler_idx].transformFunc;
                } else if(0 == strcmp("samplerExtent_sv2", name)) {
                    assert(sampler_idx >= 0);
                    to_replace_with = m_List[sampler_idx].extentFunc;
                }
            }

            if(to_replace_with != NULL) {
                std::vector<llvm::Value*> params;

                llvm::User::op_iterator it = ci->op_begin();
                ++it; // Skip function
                ++it; // Skip sampler parameter
                while(it != ci->op_end()) {
                    params.push_back(*it);
                    ++it;
                }

                const SamplerDesc& desc = m_List[sampler_idx];
                if(append_free_params) {
                    for(std::vector<FreeParam>::const_iterator fpi=
                            desc.freeParams.begin();
                            fpi != desc.freeParams.end();
                            ++fpi) {
                        llvm::Value* rv = LLVM_CREATE( llvm::CallInst, 
                                fpi->accessorFunc, 
                                fpi->accessorParams.begin(),
                                fpi->accessorParams.end(),
                                "tmp", ci );
                        params.push_back(rv);
                    }

                }

                llvm::Value* new_inst = LLVM_CREATE( llvm::CallInst, 
                        to_replace_with, params.begin(), params.end(),
                        "tmp", ci);
                ci->replaceAllUsesWith(new_inst);

                to_erase.push_back(ci);

                bModified = true;
            }
        }
    }

    for(std::vector<llvm::Instruction*>::iterator i=to_erase.begin();
            i!=to_erase.end(); ++i) {
        (*i)->eraseFromParent();
    }

    return bModified;
}

//===========================================================================
char CanonicalizeSamplerCallsPass::ID = 0;

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
