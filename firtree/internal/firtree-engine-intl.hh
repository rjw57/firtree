/* firtree-engine-intl.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.    See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA    02110-1301, USA
 */

#ifndef _FIRTREE_ENGINE_INTL
#define _FIRTREE_ENGINE_INTL

#include <glib-object.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/LinkAllPasses.h>

namespace llvm { class PassManager; class Pass; }

/**
 * SECTION:firtree-engine-intl
 * @short_description: Internal types and constants for Firtree Engines.
 * @include: firtree/firtree-engine-intl.h
 *
 */

G_BEGIN_DECLS

/**
 * firtree_engine_create_sample_image_buffer_prototype:
 * @module: An LLVM module.
 * @interp: Whether to use linear interpolation.
 *
 * Create a prototype for the sample_image_buffer() engine intrinsic
 * described in builtins-desc.txt. The C-style ptototype would be:
 *
 *   vec4 sample_image_buffer(guchar* buffer, FirtreeEngineBufferFormat format, 
 *      unsigned int width, unsigned int height, unsigned int stride,
 *      vec2 location);
 *
 * Returns: A new LLVM function.
 */
llvm::Function*
firtree_engine_create_sample_image_buffer_prototype(llvm::Module* module,
        gboolean interp);

/**
 * firtree_engine_create_sample_cogl_texture_prototype:
 * @module: An LLVM module.
 *
 * Create a prototype for the sample_cogl_texture() engine intrinsic
 * described in builtins-desc.txt. The C-style ptototype would be:
 *
 *   vec4 sample_cogl_texture(FirtreeCoglTextureSampler* sampler, vec2 location);
 *
 * Returns: A new LLVM function.
 */
llvm::Function*
firtree_engine_create_sample_cogl_texture_prototype(llvm::Module* module);

/**
 * firtree_engine_create_sample_function_prototype:
 * @module: An LLVM module.
 *
 * Create a prototype for a sampler's sample() function. The C-style
 * prototype would be:
 *
 *   vec4 sample(vec2 location);
 *
 * Note that the function name is constructed via a UUID so can be assumed to
 * be unique between modules.
 *
 * Returns: A new LLVM function.
 */
llvm::Function*
firtree_engine_create_sample_function_prototype(llvm::Module* module);

/**
 * firtree_engine_get_constant_for_kernel_argument:
 * @kernel_arg: The value of the kernel's argument.
 *
 * Return a LLVM constant which has the value in @kernel_arg.
 */
llvm::Value*
firtree_engine_get_constant_for_kernel_argument(GValue* kernel_arg);

/**
 * firtree_engine_create_standard_optimization_passes:
 *
 * Create a number of standard optimisation passes which may be applied to a module
 * before code-generation.
 *
 * This is mostly lifted from the LLVM SVN. 
 */
void
firtree_engine_create_standard_optimization_passes(
        llvm::PassManager *PM,
        guint OptimizationLevel,
        gboolean OptimizeSize,
        gboolean UnitAtATime,
        gboolean UnrollLoops,
        gboolean SimplifyLibCalls,
        gboolean HaveExceptions,
        llvm::Pass *InliningPass);

namespace Firtree {

/**
 * FunctionCallReplacementPass
 */
class FunctionCallReplacementPass : public llvm::BasicBlockPass {
    public:
        FunctionCallReplacementPass(char* ID) : llvm::BasicBlockPass(ID) { }
        virtual bool runOnBasicBlock(llvm::BasicBlock& bb) {
            llvm::BasicBlock::iterator inst_it = bb.begin();
            std::vector<llvm::CallInst*> insts_to_process;
            for( ; inst_it != bb.end(); ++inst_it) {
                llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(inst_it);
                if(call_inst) {
                    llvm::Function* called_function =
                        call_inst->getCalledFunction();
                    if(interestedInCallToFunction(called_function->getName())) {
                        insts_to_process.push_back(call_inst);
                    }
                }
            }

            bool modified = false;
            std::vector<llvm::CallInst*>::iterator cinst_it = insts_to_process.begin();
            for(; cinst_it != insts_to_process.end(); ++cinst_it) {
                llvm::CallInst* call_inst = *cinst_it;

                llvm::Value* replacement = getReplacementForCallInst(*call_inst);

                if(replacement && (replacement != call_inst)) {
                    call_inst->replaceAllUsesWith(replacement);
                    call_inst->eraseFromParent();
                    modified = true;
                }

            }

            return modified;
        }

    protected:
        /* Override these */
        virtual bool         interestedInCallToFunction(const std::string& name) = 0;
        virtual llvm::Value* getReplacementForCallInst(llvm::CallInst& instruction) = 0;
};

} /* namespace Firtree */

G_END_DECLS

#endif /* _FIRTREE_ENGINE_INTL */

/* vim:sw=4:ts=4:et:cindent
 */
