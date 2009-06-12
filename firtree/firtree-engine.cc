/* firtree-engine.cc */

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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Constants.h>
#include <llvm/Linker.h>

#include <common/uuid.h>

#include "internal/firtree-engine-intl.hh"

llvm::Function*
firtree_engine_create_sample_image_buffer_prototype(llvm::Module* module, 
        gboolean interp)
{
    static const char* nn_function_name = "sample_image_buffer_nn";
    static const char* interp_function_name = "sample_image_buffer_lerp";
    const char* function_name = interp ? interp_function_name : nn_function_name;

    g_assert(module);
    g_assert(module->getFunction(function_name) == NULL);

    std::vector<const llvm::Type*> params;
    params.push_back(llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); /* buffer */
    params.push_back(llvm::Type::Int32Ty); /* format */
    params.push_back(llvm::Type::Int32Ty); /* width */
    params.push_back(llvm::Type::Int32Ty); /* height */
    params.push_back(llvm::Type::Int32Ty); /* stride */
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            function_name, module);

    g_assert(f);

    return f;
}

llvm::Function*
firtree_engine_create_sample_cogl_texture_prototype(llvm::Module* module)
{
    static const char* function_name = "sample_cogl_texture";

    g_assert(module);
    g_assert(module->getFunction(function_name) == NULL);
    
    std::vector<const llvm::Type*> params;
    params.push_back(llvm::PointerType::getUnqual(llvm::Type::Int8Ty)); /* handle */
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            function_name, module);

    g_assert(f);

    return f;
}

llvm::Function*
firtree_engine_create_sample_function_prototype(llvm::Module* module)
{
    /* work out the function name. */
    std::string func_name("sampler_");
    gchar uuid[37];
    generate_random_uuid(uuid, '_');
    func_name += uuid;

    g_assert(module);
    g_assert(module->getFunction(func_name.c_str()) == NULL);

    std::vector<const llvm::Type*> params;
    params.push_back(llvm::VectorType::get(llvm::Type::FloatTy, 2)); /* location */
    llvm::FunctionType* ft = llvm::FunctionType::get(
            llvm::VectorType::get(llvm::Type::FloatTy, 4), /* ret. type */
            params, false);
    llvm::Function* f = llvm::Function::Create( 
            ft, llvm::Function::ExternalLinkage,
            func_name.c_str(), module);

    g_assert(f);

    return f;
}

/* vim:sw=4:ts=4:et:cindent
 */
