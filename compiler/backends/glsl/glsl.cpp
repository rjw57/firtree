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
#include <stack>

namespace Firtree {

extern const char* _OperatorNames[];

//=============================================================================
struct GLSLBackend::Priv
{
    public:
        Priv() : temporaryId(0), funccounter(0) { }
        ~Priv() { }

        std::map<int, std::string>  symbolMap;
        std::map<std::string, std::string>  funcMap;
        std::stack<TString>         temporaryStack;
        unsigned int                temporaryId;
        unsigned int                funccounter;
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
    m_InKernel = false;

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
    if(m_InParams && !m_InKernel)
    {
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

        AddSymbol(n->getId(), "param_");
        AppendGLSLType(n->getTypePointer()); AppendOutput(" ");
        AppendOutput(GetSymbol(n->getId()));
    } else {
        const char* symname = GetSymbol(n->getId());
        if(symname == NULL)
        {
            AddSymbol(n->getId(), "sym_");
            AppendGLSLType(n->getTypePointer()); 
            symname = GetSymbol(n->getId());
            AppendOutput(" %s;\n", symname);
        }

        if(m_InKernel && (strncmp(symname, "param", 5) == 0))
        {
            static char fullname[255];
            snprintf(fullname, 255, "%s_params.%s",
                    m_Prefix.c_str(), symname);
            PushTemporary(fullname);
        } else {
            PushTemporary(symname);
        }
    }
}

//=============================================================================
void GLSLBackend::VisitConstantUnion(TIntermConstantUnion* n)
{
    const char* tmp = AddTemporary();
    PushTemporary(tmp);

    constUnion* array = n->getUnionArrayPointer();
    TType* t = n->getTypePointer();

    AppendGLSLType(n->getTypePointer());
    AppendOutput(" %s = ", tmp);

    if(t->isVector())
    {
        AppendGLSLType(n->getTypePointer());
        AppendOutput("(");

        for(int i=0; i<t->getNominalSize(); i++)
        {
            switch(t->getBasicType())
            {
                case EbtFloat:
                    AppendOutput("%f", array[i].getFConst());
                    break;
                case EbtInt:
                    AppendOutput("%i", array[i].getIConst());
                    break;
                case EbtBool:
                    AppendOutput("%s", array[i].getBConst() ? "true" : "false");
                    break;
                default:
                    AppendOutput("/* ??? */");
                    break;
            }
            if(i+1 < t->getNominalSize())
            {
                AppendOutput(",");
            }
        }

        AppendOutput(");\n");
    } else {
        switch(t->getBasicType())
        {
            case EbtFloat:
                AppendOutput("%f;\n", array[0].getFConst());
                break;
            case EbtInt:
                AppendOutput("%i;\n", array[0].getIConst());
                break;
            case EbtBool:
                AppendOutput("%s;\n", array[0].getBConst() ? "true" : "false");
                break;
            default:
                AppendOutput("/* ??? */");
                break;
        }
    }
}

//=============================================================================
bool GLSLBackend::VisitBinary(bool preVisit, TIntermBinary* n)
{
    if(!preVisit)
    {
        TString right(PopTemporary());
        TString left(PopTemporary());
        
        const char* tmp = AddTemporary();
        PushTemporary(tmp);

        switch(n->getOp())
        {
            case EOpAdd:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s + %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpSub:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s - %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpMul:
            case EOpVectorTimesScalar:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s * %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpDiv:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s / %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpEqual:
            case EOpVectorEqual:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s == %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpNotEqual:
            case EOpVectorNotEqual:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s != %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpLessThan:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s < %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpGreaterThan:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s > %s;\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpAssign:
                PopTemporary(); // Not needed.
                AppendOutput("%s = %s;\n", left.c_str(), right.c_str());
                break;
            case EOpAddAssign:
                PopTemporary(); // Not needed.
                AppendOutput("%s += %s;\n", left.c_str(), right.c_str());
                break;
            case EOpSubAssign:
                PopTemporary(); // Not needed.
                AppendOutput("%s -= %s;\n", left.c_str(), right.c_str());
                break;
            case EOpMulAssign:
                PopTemporary(); // Not needed.
                AppendOutput("%s *= %s;\n", left.c_str(), right.c_str());
                break;
            case EOpDivAssign:
                PopTemporary(); // Not needed.
                AppendOutput("%s /= %s;\n", left.c_str(), right.c_str());
                break;
            case EOpIndexDirect:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s[%s];\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpMin:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = min(%s,%s);\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpMax:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = max(%s,%s);\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpStep:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = step(%s,%s);\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpDot:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = dot(%s,%s);\n", tmp, left.c_str(), right.c_str());
                break;
            case EOpCross:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = cross(%s,%s);\n", tmp, left.c_str(), right.c_str());
                break;
            default:
                AppendOutput("/* ");
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s ?binop %s? %s*/\n",
                        tmp,
                        left.c_str(),
                        _OperatorNames[n->getOp()],
                        right.c_str());
                break;
        }
    }

    return true;
}

//=============================================================================
bool GLSLBackend::VisitUnary(bool preVisit, TIntermUnary* n)
{
    if(!preVisit)
    {
        TString operand(PopTemporary());
        const char* tmp = AddTemporary();
        PushTemporary(tmp);

        switch(n->getOp())
        {
            case EOpNegative:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = -%s;\n", tmp, operand.c_str());
                break;
            case EOpPostIncrement:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s;\n", tmp, operand.c_str());
                AppendOutput(" ++%s;\n", operand.c_str());
                break;
            case EOpPostDecrement:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s;\n", tmp, operand.c_str());
                AppendOutput(" --%s;\n", operand.c_str());
                break;
            case EOpPreIncrement:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = ++%s;\n", tmp, operand.c_str());
                break;
            case EOpPreDecrement:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = --%s\n", tmp, operand.c_str());
                break;
            case EOpRadians:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = radians(%s);\n", tmp, operand.c_str());
                break;
            case EOpDegrees:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = degrees(%s);\n", tmp, operand.c_str());
                break;
            case EOpSin:
            case EOpSinRange:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = sin(%s);\n", tmp, operand.c_str());
                break;
            case EOpCos:
            case EOpCosRange:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = cos(%s);\n", tmp, operand.c_str());
                break;
            case EOpTan:
            case EOpTanRange:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = tan(%s);\n", tmp, operand.c_str());
                break;
            case EOpAsin:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = asin(%s);\n", tmp, operand.c_str());
                break;
            case EOpAcos:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = acos(%s);\n", tmp, operand.c_str());
                break;
            case EOpAtan:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = atan(%s);\n", tmp, operand.c_str());
                break;
            case EOpPow:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = pow(%s);\n", tmp, operand.c_str());
                break;
            case EOpExp:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = exp(%s);\n", tmp, operand.c_str());
                break;
            case EOpLog:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = log(%s);\n", tmp, operand.c_str());
                break;
            case EOpExp2:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = exp2(%s);\n", tmp, operand.c_str());
                break;
            case EOpLog2:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = log2(%s);\n", tmp, operand.c_str());
                break;
            case EOpSqrt:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = sqrt(%s);\n", tmp, operand.c_str());
                break;
            case EOpInverseSqrt:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = inversesqrt(%s);\n", tmp, operand.c_str());
                break;
            case EOpAbs:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = abs(%s);\n", tmp, operand.c_str());
                break;
            case EOpSign:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = sign(%s);\n", tmp, operand.c_str());
                break;
            case EOpFloor:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = floor(%s);\n", tmp, operand.c_str());
                break;
            case EOpCeil:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = ceil(%s);\n", tmp, operand.c_str());
                break;
            case EOpFract:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = fract(%s);\n", tmp, operand.c_str());
                break;
            case EOpLength:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = length(%s);\n", tmp, operand.c_str());
                break;
            case EOpNormalize:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = normalize(%s);\n", tmp, operand.c_str());
                break;
            case EOpSinCos:
            case EOpSinCosRange:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_sincos(%s);\n", tmp, operand.c_str());
                break;
            case EOpCosSin:
            case EOpCosSinRange:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_cossin(%s);\n", tmp, operand.c_str());
                break;
            case EOpPremultiply:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_premultiply(%s);\n", tmp, operand.c_str());
                break;
            case EOpUnPremultiply:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_unpremultiply(%s);\n", tmp, operand.c_str());
                break;
            default:
                AppendOutput("/* ");
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = ?unop %s? (%s) */\n", tmp,
                        _OperatorNames[n->getOp()], operand.c_str());
                break;
        }
    }

    return true;
}

//=============================================================================
bool GLSLBackend::VisitSelection(bool preVisit, TIntermSelection* n)
{
    FIRTREE_WARNING("Selection not handled.\n");
    return false;
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
                if(n->getOp() == EOpKernel)
                {
                    TIntermSequence& paramSequence =
                        n->getSequence()[0]->getAsAggregate()->getSequence();
                    
                    if(paramSequence.size() > 0)
                    {
                        // Kernels have their 'parameters' passed via
                        // a uniform structure.
                        AppendOutput("struct %s_uniforms { \n", 
                                m_Prefix.c_str());

                        for(TIntermSequence::iterator i = paramSequence.begin();
                                i != paramSequence.end(); i++)
                        {
                            TIntermSymbol* ns = (*i)->getAsSymbolNode();
                            AddSymbol(ns->getId(), "param_");
                            AppendOutput("  ");
                            AppendGLSLType(ns->getTypePointer()); 
                            AppendOutput(" %s;\n", 
                                    GetSymbol(ns->getId()));
                        }

                        AppendOutput("};\n\n");
                        AppendOutput("uniform %s_uniforms %s_params;\n\n",
                                m_Prefix.c_str(), m_Prefix.c_str());
                    }
                }

                AppendOutput("// %s definition (name: %s)\n", 
                        (n->getOp() == EOpFunction) ? "Function" : "Kernel",
                        n->getName().c_str());

                AppendGLSLType(n->getTypePointer()); 
                AppendOutput(" ");
                AppendOutput("%s", AddFunction(n->getName().c_str()));

                m_InFunction = true;
                if(n->getOp() == EOpKernel)
                {
                    m_InKernel = true;
                }
            } else {
                m_InFunction = false;
                m_InKernel = false;

                AppendOutput("}\n\n");
            }
            break;
        case EOpParameters:
            if(preVisit) 
            {
                AppendOutput("(");

                m_InParams = true;
                m_ProcessedOneParam = false;

                // If inside a kernel, add the kernel parameters
                if(m_InKernel)
                {
                    AppendOutput("in vec2 destCoord");
                    m_ProcessedOneParam = true;
                }
            } else {
                m_InParams = false;
                AppendOutput(")\n{\n");
            }
            break;
        case EOpSequence:
            /* We can pretty much ignore these. */
            break;
        case EOpDestCoord:
            if(!preVisit) 
            {
                PushTemporary("destCoord");
            }
            break;
        case EOpFunctionCall:
        case EOpCompare:
        case EOpClamp:
        case EOpSmoothStep:
            if(!preVisit) 
            {
                // Pop the arguments.
                std::vector<std::string> params;
                for(int i=0; i<n->getSequence().size(); i++)
                {
                    params.push_back(std::string(PopTemporary()));
                }

                const char* tmp = AddTemporary();
                PushTemporary(tmp);

                AppendGLSLType(n->getTypePointer()); 

                const char* funcname = "UNTITLED_FUNC";
                switch(n->getOp())
                {
                    case EOpFunctionCall:
                        funcname = GetFunction(n->getName().c_str());
                        break;
                    case EOpCompare:
                        funcname = "__builtin_compare";
                        break;
                    case EOpClamp:
                        funcname = "clamp";
                        break;
                    case EOpSmoothStep:
                        funcname = "smoothstep";
                        break;
                }

                AppendOutput(" %s = %s(", tmp, funcname);

                bool first = true;
                for(std::vector<std::string>::reverse_iterator i=params.rbegin();
                        i != params.rend(); i++)
                {
                    if(!first)
                    {
                        AppendOutput(", ");
                    }

                    AppendOutput("%s", (*i).c_str());
                    first = false;
                }

                AppendOutput(");\n");
            }
            break;
        default:
            {
                if(!preVisit) 
                {
                    // Pop the arguments.
                    std::vector<std::string> params;
                    for(int i=0; i<n->getSequence().size(); i++)
                    {
                        params.push_back(std::string(PopTemporary()));
                    }
                    const char* tmp = AddTemporary();
                    PushTemporary(tmp);
                    AppendGLSLType(n->getTypePointer()); 
                    AppendOutput(" %s;", tmp);
                    AppendOutput(" /* = ?aggregate %s */\n", 
                            _OperatorNames[n->getOp()]);
                }
            }
            break;
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
    if(!preVisit) 
    { 
        switch(n->getFlowOp())
        {
            case EOpReturn:
                {
                    const char* tmp = PopTemporary();
                    AppendOutput("return %s;\n", tmp);
                    break;
                }
            default:
                AppendOutput("/* ?branch %s */\n",
                        _OperatorNames[n->getFlowOp()]);
                break;
        }
    }

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
            case EbtSampler:
                AppendOutput("int /*sampler*/"); // Samplers are basically indicies.
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
    static char valstr[255];
    snprintf(valstr, 255, "%s%i", typePrefix, m_Priv->symbolMap.size());
    m_Priv->symbolMap[id] = std::string(valstr);
}

//=============================================================================
const char* GLSLBackend::GetSymbol(int id)
{
    std::string& val = m_Priv->symbolMap[id];
    if(val.empty())
    {
        return NULL;
    }
    return val.c_str();
}

//=============================================================================
const char* GLSLBackend::AddFunction(const char* name)
{
    static char valstr[255];
    snprintf(valstr, 255, "%s_func_%i", 
            m_Prefix.c_str(), m_Priv->funccounter++);
    m_Priv->funcMap[std::string(name)] = std::string(valstr);

    return GetFunction(name);
}

//=============================================================================
const char* GLSLBackend::GetFunction(const char* name)
{
    return m_Priv->funcMap[std::string(name)].c_str();
}

//=============================================================================
const char* GLSLBackend::AddTemporary()
{
    
    TString tempName = "tmp_" + String(m_Priv->temporaryId++);
    return tempName.c_str();
}

//=============================================================================
void GLSLBackend::PushTemporary(const char* t)
{
    m_Priv->temporaryStack.push(TString(t));
}

//=============================================================================
const char* GLSLBackend::PopTemporary()
{
    static TString lastTop;
    lastTop = m_Priv->temporaryStack.top().c_str();
    m_Priv->temporaryStack.pop();
    return lastTop.c_str();
}

//=============================================================================
const char* _OperatorNames[] = {
    "EOpNull",            // if in a node", should only mean a node is still being built
    "EOpSequence",        // denotes a list of statements, or parameters", etc.
    "EOpFunctionCall",    
    "EOpFunction",        // For function definition
    "EOpKernel",          // For kernel definition. FIRTREE only
    "EOpParameters",      // an aggregate listing the parameters to a function

    //
    // Unary operators
    //
    
    "EOpNegative",
    "EOpLogicalNot",
    "EOpVectorLogicalNot",
    "EOpBitwiseNot",

    "EOpPostIncrement",
    "EOpPostDecrement",
    "EOpPreIncrement",
    "EOpPreDecrement",

    "EOpConvIntToBool",
    "EOpConvFloatToBool",
    "EOpConvBoolToFloat",
    "EOpConvIntToFloat",
    "EOpConvFloatToInt",
    "EOpConvBoolToInt",

    //
    // binary operations
    //

    "EOpAdd",
    "EOpSub",
    "EOpMul",
    "EOpDiv",
    "EOpMod",
    "EOpRightShift",
    "EOpLeftShift",
    "EOpAnd",
    "EOpInclusiveOr",
    "EOpExclusiveOr",
    "EOpEqual",
    "EOpNotEqual",
    "EOpVectorEqual",
    "EOpVectorNotEqual",
    "EOpLessThan",
    "EOpGreaterThan",
    "EOpLessThanEqual",
    "EOpGreaterThanEqual",
    "EOpComma",

    "EOpVectorTimesScalar",
    "EOpVectorTimesMatrix",
    "EOpMatrixTimesVector",
    "EOpMatrixTimesScalar",

    "EOpLogicalOr",
    "EOpLogicalXor",
    "EOpLogicalAnd",

    "EOpIndexDirect",
    "EOpIndexIndirect",
    "EOpIndexDirectStruct",

    "EOpVectorSwizzle",

    //
    // Built-in functions potentially mapped to operators
    //

    "EOpRadians",
    "EOpDegrees",
    "EOpSin",
    "EOpCos",
    "EOpTan",
    "EOpAsin",
    "EOpAcos",
    "EOpAtan",

    //
    // FIRTREE only
    // 
    "EOpSinRange",
    "EOpCosRange",
    "EOpTanRange",
    "EOpSinCos",
    "EOpCosSin",
    "EOpSinCosRange",
    "EOpCosSinRange",

    "EOpPow",
    "EOpExp",
    "EOpLog",
    "EOpExp2",
    "EOpLog2",
    "EOpSqrt",
    "EOpInverseSqrt",

    "EOpAbs",
    "EOpSign",
    "EOpFloor",
    "EOpCeil",
    "EOpFract",
    "EOpMin",
    "EOpMax",
    "EOpClamp",
    "EOpMix",
    "EOpStep",
    "EOpSmoothStep",

    "EOpLength",
    "EOpDistance",
    "EOpDot",
    "EOpCross",
    "EOpNormalize",
    "EOpFaceForward",
    "EOpReflect",
    "EOpRefract",

    "EOpDPdx",            // Fragment only
    "EOpDPdy",            // Fragment only
    "EOpFwidth",          // Fragment only

    "EOpMatrixTimesMatrix",

    "EOpAny",
    "EOpAll",
    
    "EOpItof",         // pack/unpack only
    "EOpFtoi",         // pack/unpack only    
    "EOpSkipPixels",   // pack/unpack only
    "EOpReadInput",    // unpack only
    "EOpWritePixel",   // unpack only
    "EOpBitmapLsb",    // unpack only
    "EOpBitmapMsb",    // unpack only
    "EOpWriteOutput",  // pack only
    "EOpReadPixel",    // pack only

    "EOpDestCoord",    // FIRTREE only
    "EOpCompare",    // FIRTREE only
    "EOpPremultiply",    // FIRTREE only
    "EOpUnPremultiply",    // FIRTREE only
    "EOpSample",    // FIRTREE only
    "EOpSamplerCoord",    // FIRTREE only
    "EOpSamplerExtent",    // FIRTREE only
    "EOpSamplerOrigin",    // FIRTREE only
    "EOpSamplerSize",    // FIRTREE only
    "EOpSamplerTransform",    // FIRTREE only

    //
    // Branch
    //

    "EOpKill",            // Fragment only
    "EOpReturn",
    "EOpBreak",
    "EOpContinue",

    //
    // Constructors
    //

    "EOpConstructInt",
    "EOpConstructBool",
    "EOpConstructFloat",
    "EOpConstructVec2",
    "EOpConstructVec3",
    "EOpConstructVec4",
    "EOpConstructBVec2",
    "EOpConstructBVec3",
    "EOpConstructBVec4",
    "EOpConstructIVec2",
    "EOpConstructIVec3",
    "EOpConstructIVec4",
    "EOpConstructMat2",
    "EOpConstructMat3",
    "EOpConstructMat4",
    "EOpConstructStruct",

    //
    // moves
    //
    
    "EOpAssign",
    "EOpAddAssign",
    "EOpSubAssign",
    "EOpMulAssign",
    "EOpVectorTimesMatrixAssign",
    "EOpVectorTimesScalarAssign",
    "EOpMatrixTimesScalarAssign",
    "EOpMatrixTimesMatrixAssign",
    "EOpDivAssign",
    "EOpModAssign",
    "EOpAndAssign",
    "EOpInclusiveOrAssign",
    "EOpExclusiveOrAssign",
    "EOpLeftShiftAssign",
    "EOpRightShiftAssign",

    //
    // Array operators
    //

    "EOpArrayLength",
};

//=============================================================================
} // namespace Firtree


//=============================================================================
// vim:sw=4:ts=4:cindent:et
