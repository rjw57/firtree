/* clutter.hh */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
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

#ifndef FIRTREE_CPU_ENGINE_CLUTTER_H

#define FIRTREE_CPU_ENGINE_CLUTTER_H

#if FIRTREE_HAVE_CLUTTER

#include <cogl/cogl.h>
#include <firtree/firtree.h>
#include <firtree/firtree-cogl-texture-sampler.h>
#include <firtree/internal/firtree-engine-intl.hh>
#include <firtree/internal/firtree-cogl-texture-sampler-intl.hh>

/* This is a LLVM pass which replaces calls to sample_cogl_texture with
 * calls to sample_image_buffer_nn() or sample_image_buffer_lerp(). */
class CanonicaliseCoglCallsPass : public Firtree::FunctionCallReplacementPass {
    public:

    static char ID;

    CanonicaliseCoglCallsPass() 
        : Firtree::FunctionCallReplacementPass(&ID) 
    { }

    protected:

    virtual bool interestedInCallToFunction(const std::string& name) {
        return (name == "sample_cogl_texture");
    }

    virtual llvm::Value* getReplacementForCallInst(llvm::CallInst& instruction) {
        llvm::CallInst* call_inst = &instruction;
        llvm::Module* m = call_inst->getParent()->getParent()->getParent();

        /* this is a call we need to replace. */
        llvm::Value* tex_handle = call_inst->getOperand(1);
        llvm::Value* location = call_inst->getOperand(2);

        if(!llvm::isa<llvm::ConstantExpr>(tex_handle) || 
                !llvm::isa<llvm::PointerType>(tex_handle->getType())) {
            g_assert(false &&
                    "Texture handles passed to sample_cogl_texture() "
                    "must be constant pointers.");
        }

        llvm::ConstantExpr* const_expr = 
            llvm::cast<llvm::ConstantExpr>(tex_handle);
        g_assert(const_expr->isCast());

        llvm::ConstantInt* tex_handle_int_val =
            llvm::cast<llvm::ConstantInt>(const_expr->getOperand(0));

        FirtreeCoglTextureSampler* sampler = 
            (FirtreeCoglTextureSampler*)
            tex_handle_int_val->getZExtValue();
        g_assert(FIRTREE_IS_COGL_TEXTURE_SAMPLER(sampler));

        CoglHandle cogl_tex_handle = 
            firtree_cogl_texture_sampler_get_cogl_texture(sampler);

        guint width = cogl_texture_get_width(cogl_tex_handle);
        guint height = cogl_texture_get_height(cogl_tex_handle);

        guint stride = 0;
        FirtreeBufferFormat format = FIRTREE_FORMAT_LAST;
        guchar* data = NULL;

        guint data_size = firtree_cogl_texture_sampler_get_data(sampler,
                &data, &stride, &format);
        g_assert(format != FIRTREE_FORMAT_LAST);
        g_assert(data != NULL);
        g_assert(data_size != 0);

        gboolean do_interp =
            (cogl_texture_get_mag_filter(cogl_tex_handle) == CGL_LINEAR);

        llvm::Function* sample_buffer_func = 
            firtree_engine_create_sample_image_buffer_prototype(
                    m, do_interp);
        g_assert(sample_buffer_func);

        llvm::Value* llvm_width = llvm::ConstantInt::get(
                llvm::Type::Int32Ty, 
                (uint64_t)width, false);
        llvm::Value* llvm_height = llvm::ConstantInt::get(
                llvm::Type::Int32Ty,
                (uint64_t)height, false);
        llvm::Value* llvm_stride = llvm::ConstantInt::get(
                llvm::Type::Int32Ty,
                (uint64_t)stride, false);
        llvm::Value* llvm_format = llvm::ConstantInt::get(
                llvm::Type::Int32Ty,
                (uint64_t)format, false);

        /* This looks dirty but is apparently valid.
         *   See: http://www.nabble.com/Creating-Pointer-Constants-td22401381.html */
        llvm::Constant* llvm_data_int = llvm::ConstantInt::get(
                llvm::Type::Int64Ty, 
                (uint64_t)data, false);
        llvm::Value* llvm_data = llvm::ConstantExpr::getIntToPtr(
                llvm_data_int,
                llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); 

        std::vector<llvm::Value*> func_args;
        func_args.push_back(llvm_data);
        func_args.push_back(llvm_format);
        func_args.push_back(llvm_width);
        func_args.push_back(llvm_height);
        func_args.push_back(llvm_stride);
        func_args.push_back(location);

        llvm::Value* new_call = llvm::CallInst::Create(
                sample_buffer_func,
                func_args.begin(), func_args.end(), "sample", 
                call_inst);

        return new_call;
    }
};

char CanonicaliseCoglCallsPass::ID = 0;
llvm::RegisterPass<CanonicaliseCoglCallsPass> X("canonicalise-cogl-calls", 
        "Replace calls to sample_cogl_texture().");

#endif /* FIRTREE_HAVE_CLUTTER */


#endif /* end of include guard: FIRTREE_CPU_ENGINE_CLUTTER_H */


