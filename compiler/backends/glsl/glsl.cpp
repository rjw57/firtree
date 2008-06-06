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
// This file implements the FIRTREE GLSL backend.
//=============================================================================

#include "glsl.h"

#include "glslang/Include/ShHandle.h"
#include "glslang/Include/intermediate.h"
#include "glslang/Public/ShaderLang.h"

#include <map>

namespace Firtree {

//=============================================================================
struct GLSLBackend::Priv
{
    std::map<TString, TString>   symbolMap;
};

//=============================================================================
class GLSLTrav : public TIntermTraverser
{
    public:
        GLSLTrav(GLSLBackend& be) 
            :   TIntermTraverser()
            ,    m_Backend(be) 
            { }
        GLSLBackend& be() { return m_Backend; }

    protected:
        GLSLBackend&  m_Backend;
};

//=============================================================================
GLSLBackend::GLSLBackend(const char* prefix)
    :   m_Prefix(prefix)   
    ,   m_Output()
{
    m_Priv = new GLSLBackend::Priv;
}

//=============================================================================
GLSLBackend::~GLSLBackend()
{
    delete m_Priv;
}

//=============================================================================
bool GLSLBackend::Generate(TIntermNode* root)
{
    TIntermAggregate* rootAgg = root->getAsAggregate();
    if(rootAgg == NULL)
        return false;

    m_Priv->symbolMap.clear();

    return ProcessTopLevelAggregate(rootAgg);
}

//=============================================================================
bool GLSLBackend::ProcessTopLevelAggregate(TIntermAggregate* root)
{
    if(root->getOp() != EOpSequence)
    {
        FIRTREE_WARNING("Root node not sequence.");
        return false;
    }

    TIntermSequence& seq = root->getSequence();

    bool emittedKernel = false;

    for(TIntermSequence::iterator i = seq.begin(); i != seq.end(); i++)
    {
        // For each top-level node, check it is 
        //   a) an aggreagate,
        //   b) a function,
        //   c) the (one and only) kernel.
        
        TIntermAggregate* agg = (*i)->getAsAggregate();

        if(agg == NULL)
        {
            FIRTREE_WARNING("Top-level node not aggregate.");
            return false;
        }

        switch(agg->getOp())
        {
            case EOpFunction:
                ProcessFunctionDefinition(agg, false);
                break;

            case EOpKernel:
                if(emittedKernel)
                {
                    FIRTREE_WARNING("There must be only one kernel function per kernel.");
                    return false;
                }

                ProcessFunctionDefinition(agg, true);

                emittedKernel = true;
                break;

            default:
                FIRTREE_WARNING("Unknown top-level node type: %i\n", agg->getOp());
                return false;
        }
    }

    if(!emittedKernel)
    {
        FIRTREE_WARNING("No kernel definition found.");
        return false;
    }

    return true;
}

//=============================================================================
bool GLSLBackend::ProcessFunctionDefinition(TIntermAggregate* func, bool isKernel)
{
    AppendOutput("// %s definition (name: %s)\n", isKernel ? "Function" : "Kernel",
            func->getName().c_str());

    // Add this function to the symbol map.
    m_Priv->symbolMap[func->getName()] = TString("func_") + String(m_Priv->symbolMap.size());

    AppendGLSLType(func->getTypePointer()); 
    AppendOutput(" ");
    AppendOutput("%s_%s", m_Prefix.c_str(), m_Priv->symbolMap[func->getName()].c_str());
}

//=============================================================================
void GLSLBackend::AppendGLSLType(TType* t)
{
    if(t->isVector())
    {
        switch(t->getBasicType())
        {
            case EbtFloat:
                AppendOutput("vec%i", t->getNominalSize());
                break;
            case EbtInt:
                AppendOutput("ivec%i", t->getNominalSize());
                break;
            case EbtBool:
                AppendOutput("bvec%i", t->getNominalSize());
                break;
            default:
                FIRTREE_WARNING("Unknown basic type: %i", t->getBasicType());
                break;
        }
    } else {
        switch(t->getBasicType())
        {
            case EbtVoid:
                AppendOutput("void");
                break;
            case EbtFloat:
                AppendOutput("float");
                break;
            case EbtInt:
                AppendOutput("int");
                break;
            case EbtBool:
                AppendOutput("bool");
                break;
            default:
                FIRTREE_WARNING("Unknown basic type: %i", t->getBasicType());
                break;
        }
    }
}

//=============================================================================
void GLSLBackend::AppendOutput(const char* format, ...)
{
    va_list args;
   
    // Allocate space for the message
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    char* message = (char*)(malloc(size));
    if(message == NULL)
        return;

    va_start(args, format);
    vsnprintf(message, size, format, args);
    va_end(args);

    m_Output.append(message);

    free(message);
}

//=============================================================================
} // namespace Firtree


//=============================================================================
// vim:sw=4:ts=4:cindent:et
