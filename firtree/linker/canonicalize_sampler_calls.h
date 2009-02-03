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
// This file defines a LLVM pass for canonicalizing calls to samplers.
//===========================================================================

#include <limits.h>
#include <vector>

#include <llvm/Pass.h>

namespace Firtree { namespace LLVM {

struct FreeParam {
    llvm::Function*                 accessorFunc;
    std::vector<llvm::Value*>       accessorParams;
};

struct SamplerDesc {
    llvm::Function*     sampleFunc;
    llvm::Function*     transformFunc;
    llvm::Function*     extentFunc;

    std::vector<FreeParam> freeParams;

    SamplerDesc(llvm::Function* s, llvm::Function* t, llvm::Function* e) 
        :   sampleFunc(s)
        ,   transformFunc(t)
        ,   extentFunc(e) 
    { }
};

typedef std::vector<SamplerDesc> SamplerDescList;

//===========================================================================
/// This pass takes a vector of SamplerDesc structs and replaces calls
/// to sample(), samplerTransform() and samplerExtent() with appropriate
/// calls to elements of that vector.
struct CanonicalizeSamplerCallsPass : public llvm::BasicBlockPass {
    static char ID;
    CanonicalizeSamplerCallsPass(const SamplerDescList& list);
    ~CanonicalizeSamplerCallsPass();

    virtual bool runOnBasicBlock(llvm::BasicBlock &BB);

    private:

    const SamplerDescList& m_List;
};

} } // namespace Firtree::LLVM

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
