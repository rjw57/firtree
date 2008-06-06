/* 
 * FIRTREE - A generic image processing system.
 * Copyright (C) 2008 Rich Wareham <srichwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//=============================================================================
// This file implements the interface between the 3d Labs GLSL frontend and 
// the FIRTREE compiler.
//=============================================================================

// Include the GLSL frontend interface.
#include "glsl-compiler.h"
#include "../include/compiler.h"

//=============================================================================
// COMPILATION
//=============================================================================

//=============================================================================
// Compiler construction
TCompiler* ConstructCompiler(EShLanguage language, int debugOptions)
{
    assert(language == EShLangKernel);
    return new FirtreeInternal::FirtreeCompiler();
}

//=============================================================================
// Compiler destruction
void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}

//=============================================================================
// Code generation.
bool FirtreeInternal::FirtreeCompiler::compile(TIntermNode *root)
{
    return m_pBackend->Generate(root);
}

//=============================================================================
// LINKING
//=============================================================================

namespace FirtreeInternal {

//=============================================================================
class FirtreeLinker : public TLinker 
{
    public:
        FirtreeLinker(EShExecutable e) : TLinker(e, infoSink) { }

        bool link(TCompilerList&, TUniformMap*) { return true; }
        void getAttributeBindings(ShBindingTable const **t) const { }

    protected:
        TInfoSink infoSink;
};

//=============================================================================
class FirtreeUniformMap : public TUniformMap 
{
    public:
        FirtreeUniformMap() : TUniformMap() { }
        virtual int getLocation(const char* name) { return 0; }
};

}

//=============================================================================
TShHandleBase* ConstructLinker(EShExecutable executable, int debugOptions)
{
    return new FirtreeInternal::FirtreeLinker(executable);
}

//=============================================================================
void DeleteLinker(TShHandleBase* linker)
{
    delete linker;
}

//=============================================================================
TUniformMap* ConstructUniformMap()
{
    return new FirtreeInternal::FirtreeUniformMap();
}

//=============================================================================
void DeleteUniformMap(TUniformMap* map)
{
    delete map;
}

//=============================================================================
TShHandleBase* ConstructBindings()
{
    return NULL;
}

//=============================================================================
void DeleteBindingList(TShHandleBase* bindingList)
{
    delete bindingList;
}

//=============================================================================
// vim:sw=4:ts=4:cindent:et
