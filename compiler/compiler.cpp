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
// This file implements the FIRTREE compiler.
//=============================================================================

#include <public/include/main.h>

#include "glslang/Include/ShHandle.h"
#include "glslang/Public/ShaderLang.h"

#include "frontend/glsl-compiler.h"

#include <compiler/include/compiler.h>

namespace Firtree {

//=============================================================================
struct Compiler::PrivData 
{
    ShHandle        compiler;
};

//=============================================================================
Compiler::Compiler(Backend& backend)
    :   m_backend(backend)
{
    m_Priv = new Compiler::PrivData();

    ShInitialize();

    m_Priv->compiler = ShConstructCompiler(EShLangKernel, 0 /* no debug opts */ );

    // Check compiler was created.
    if(m_Priv->compiler == 0)
    {
        FIRTREE_ERROR("Error creating compiler.");
        return;
    }

    // Set the backend. A bit mucky....
    FirtreeInternal::FirtreeCompiler* pfc = 
        reinterpret_cast<FirtreeInternal::FirtreeCompiler*>(m_Priv->compiler);
    pfc->SetBackend(&m_backend);
}

//=============================================================================
Compiler::~Compiler()
{
    ShDestruct(m_Priv->compiler);
    m_Priv->compiler = 0;

    delete m_Priv;
}

//=============================================================================
bool Compiler::Compile(const char** sourceLines, unsigned int lineCount)
{
    // Assert we have a compiler.
    assert((m_Priv != NULL) && (m_Priv->compiler != 0));

    // Prepare an array of pointers to source lines to pass to
    // compiler.
    const char** lines = new const char*[lineCount+1];

    unsigned int i;
    for(i=0; i<lineCount; i++)
    {
        if(sourceLines[i] == NULL)
        {
            FIRTREE_WARNING("A source line is a NULL pointer.");
            delete[] lines;
            return false;
        }

        lines[i] = sourceLines[i];

        // printf("%i: %s\n", i, lines[i]);
    }
    lines[i] = NULL;

    // Call the GLSL compiler front end
    TBuiltInResource resources;
    int ret = ShCompile(m_Priv->compiler, lines, 1 /* multiple strings */,
            EShOptNone, &resources, 0 /* no debug opts */);

    // Delete line buffer.
    delete[] lines;

    return (ret == 1);
}

//=============================================================================
const char* Compiler::GetInfoLog()
{
    if((m_Priv == NULL) || (m_Priv->compiler == 0))
    {
        return NULL;
    }

    return ShGetInfoLog(m_Priv->compiler);
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
