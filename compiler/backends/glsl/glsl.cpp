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
            { 
                visitSymbol = VisitSymbol;
                visitConstantUnion = VisitConstantUnion;
                visitBinary = VisitBinary;
                visitUnary = VisitUnary;
                visitSelection = VisitSelection;
                visitAggregate = VisitAggregate;
                visitLoop = VisitLoop;
                visitBranch = VisitBranch;
                preVisit = true;
                postVisit = true;
            }
        GLSLBackend& be() { return m_Backend; }

        static void VisitSymbol(TIntermSymbol* n, TIntermTraverser* t) {
            (reinterpret_cast<GLSLTrav*>(t))->be().VisitSymbol(n);
        }
        static void VisitConstantUnion(TIntermConstantUnion* n, TIntermTraverser* t) {
            (reinterpret_cast<GLSLTrav*>(t))->be().VisitConstantUnion(n);
        }
        static bool VisitBinary(bool p, TIntermBinary* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitBinary(p,n);
        }
        static bool VisitUnary(bool p, TIntermUnary* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitUnary(p,n);
        }
        static bool VisitSelection(bool p, TIntermSelection* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitSelection(p,n);
        }
        static bool VisitAggregate(bool p, TIntermAggregate* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitAggregate(p,n);
        }
        static bool VisitLoop(bool p, TIntermLoop* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitLoop(p,n);
        }
        static bool VisitBranch(bool p, TIntermBranch* n, TIntermTraverser* t) {
            return (reinterpret_cast<GLSLTrav*>(t))->be().VisitBranch(p,n);
        }

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
    
    m_SuccessFlag = true;

    m_InParams = false;
    m_InFunction = false;

    GLSLTrav trav(*this);
    root->traverse(&trav);

    return m_SuccessFlag;
}

//=============================================================================
#define FAIL_RET(...) do {FIRTREE_WARNING(__VA_ARGS__); m_SuccessFlag = false; return false;} while(0)
#define FAIL(...) do {FIRTREE_WARNING(__VA_ARGS__); m_SuccessFlag = false;} while(0)

//=============================================================================
void GLSLBackend::VisitSymbol(TIntermSymbol* n)
{
    if(m_InParams)
    {
        // Skip samplers.
        if(n->getTypePointer()->getBasicType() == EbtSampler)
            return;

        if(m_ProcessedOneParam)
        {
            AppendOutput(", ");
        } else {
            m_ProcessedOneParam = true;
        }

        switch(n->getQualifier())
        {
            case EvqIn:
                AppendOutput("in ");
                break;
            case EvqOut:
                AppendOutput("out ");
                break;
            case EvqInOut:
                AppendOutput("inout ");
                break;
        }

        AppendGLSLType(n->getTypePointer()); AppendOutput(" ");
        AddSymbol(n->getId(), "param_");
        AppendOutput(GetSymbol(n->getId()));
    } else {
        AddSymbol(n->getId(), "tmp_");
        AppendOutput("/* %s */", GetSymbol(n->getId()));
    }
}

//=============================================================================
void GLSLBackend::VisitConstantUnion(TIntermConstantUnion* n)
{
}

//=============================================================================
bool GLSLBackend::VisitBinary(bool preVisit, TIntermBinary* n)
{
    return true;
}

//=============================================================================
bool GLSLBackend::VisitUnary(bool preVisit, TIntermUnary* n)
{
    return true;
}

//=============================================================================
bool GLSLBackend::VisitSelection(bool preVisit, TIntermSelection* n)
{
    return true;
}

//=============================================================================
bool GLSLBackend::VisitAggregate(bool preVisit, TIntermAggregate* n)
{
    switch(n->getOp())
    {
        case EOpFunction:
        case EOpKernel:
            if(preVisit) 
            { 
                AppendOutput("// %s definition (name: %s)\n", 
                        (n->getOp() == EOpFunction) ? "Function" : "Kernel",
                        n->getName().c_str());

                // Add this ntion to the symbol map.
                AddSymbol(n->getName().c_str(), "n_");

                AppendGLSLType(n->getTypePointer()); 
                AppendOutput(" ");
                AppendOutput("%s_%s", m_Prefix.c_str(), 
                        GetSymbol(n->getName().c_str()));

                m_InFunction = true;
            } else {
                m_InFunction = false;

                AppendOutput("}\n\n");
            }
            break;
        case EOpParameters:
            if(preVisit) 
            {
                AppendOutput("(");
                m_InParams = true;
                m_ProcessedOneParam = false;
            } else {
                m_InParams = false;
                AppendOutput(")\n{\n");
            }
            break;
        default:
            FIRTREE_WARNING("Unhandled aggregate operator: %i", n->getOp());
    }

    return true;
}

//=============================================================================
bool GLSLBackend::VisitLoop(bool preVisit, TIntermLoop* n)
{
    FAIL_RET("Loops not supported yet.");
}

//=============================================================================
bool GLSLBackend::VisitBranch(bool preVisit, TIntermBranch* n)
{
    return true;
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
void GLSLBackend::AddSymbol(int id, const char* typePrefix)
{
    m_Priv->symbolMap["I" + String(id)] = 
        TString(typePrefix) + String(m_Priv->symbolMap.size());
}

//=============================================================================
void GLSLBackend::AddSymbol(const char* name, const char* typePrefix)
{
    m_Priv->symbolMap["N" + TString(name)] = 
        TString(typePrefix) + String(m_Priv->symbolMap.size());
}

//=============================================================================
const char* GLSLBackend::GetSymbol(int id)
{
    return m_Priv->symbolMap["I" + String(id)].c_str();
}

//=============================================================================
const char* GLSLBackend::GetSymbol(const char* name)
{
    return m_Priv->symbolMap["N" + TString(name)].c_str();
}

//=============================================================================
} // namespace Firtree


//=============================================================================
// vim:sw=4:ts=4:cindent:et
