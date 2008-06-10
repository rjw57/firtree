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

#include "../utils.h"

#include <map>
#include <stack>

namespace Firtree {

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
        std::string right(PopTemporary());
        std::string left(PopTemporary());
        
        const char* tmp = NULL;
        switch(n->getOp())
        {
            case EOpAssign:
            case EOpAddAssign:
            case EOpSubAssign:
            case EOpMulAssign:
            case EOpDivAssign:
                /* These do not create temporaries. */
                break;

            default:
                tmp = AddTemporary();
                PushTemporary(tmp);
        }

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
                AppendOutput("%s = %s;\n", left.c_str(), right.c_str());
                PushTemporary(left.c_str());
                break;
            case EOpAddAssign:
                AppendOutput("%s += %s;\n", left.c_str(), right.c_str());
                PushTemporary(left.c_str());
                break;
            case EOpSubAssign:
                AppendOutput("%s -= %s;\n", left.c_str(), right.c_str());
                PushTemporary(left.c_str());
                break;
            case EOpMulAssign:
                AppendOutput("%s *= %s;\n", left.c_str(), right.c_str());
                PushTemporary(left.c_str());
                break;
            case EOpDivAssign:
                AppendOutput("%s /= %s;\n", left.c_str(), right.c_str());
                PushTemporary(left.c_str());
                break;
            case EOpIndexDirect:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s[%s];\n", tmp, left.c_str(), right.c_str());
                break;
            default:
                AppendOutput("/* ");
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = %s ?binop %s? %s*/\n",
                        tmp,
                        left.c_str(),
                        OperatorCodeToDescription(n->getOp()),
                        right.c_str());
                FAIL_RET("Unknown unary op: %s", OperatorCodeToDescription(n->getOp()));
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
            case EOpSamplerCoord:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_sampler_coord(%s);\n", tmp, operand.c_str());
                break;
            case EOpSamplerExtent:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_sampler_extent(%s);\n", tmp, operand.c_str());
                break;
            case EOpSamplerOrigin:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_sampler_origin(%s);\n", tmp, operand.c_str());
                break;
            case EOpSamplerSize:
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = __builtin_sampler_size(%s);\n", tmp, operand.c_str());
                break;
            default:
                AppendOutput("/* ");
                AppendGLSLType(n->getTypePointer());
                AppendOutput(" %s = ?unop %s? (%s) */\n", tmp,
                        OperatorCodeToDescription(n->getOp()), operand.c_str());
                FAIL_RET("Unknown unary op: %s", OperatorCodeToDescription(n->getOp()));
                break;
        }
    }

    return true;
}

//=============================================================================
bool GLSLBackend::VisitSelection(bool preVisit, TIntermSelection* n)
{
    FAIL_RET("Selection op not handled");
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

                    m_InputParameters.clear();
                    
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
                            TType* t = ns->getTypePointer();
                            AddSymbol(ns->getId(), "param_");
                            AppendOutput("  ");
                            AppendGLSLType(t);
                            AppendOutput(" %s; ", 
                                    GetSymbol(ns->getId()));
                            AppendOutput("/* orig: %s */\n", ns->getSymbol().c_str());

                            Parameter paramStruct;
                            paramStruct.humanName =
                                std::string(ns->getSymbol().c_str());
                            paramStruct.uniformName =
                                std::string(GetSymbol(ns->getId()));
                            paramStruct.vectorSize = t->getNominalSize();
                            paramStruct.isColor = t->isColor();

                            switch(t->getBasicType())
                            {
                                case EbtFloat:
                                    paramStruct.basicType = Parameter::Float;
                                    break;
                                case EbtInt:
                                    paramStruct.basicType = Parameter::Int;
                                    break;
                                case EbtBool:
                                    paramStruct.basicType = Parameter::Bool;
                                    break;
                                case EbtSampler:
                                    paramStruct.basicType = Parameter::Sampler;
                                    break;
                                default:
                                    FAIL_RET("Unknown parameter type to kernel.");
                            }

                            m_InputParameters.push_back(paramStruct);
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
                if(n->getOp() == EOpFunction)
                {
                    AppendOutput("%s", AddFunction(n->getName().c_str()));
                } else {
                    m_OutputKernelName = m_Prefix;
                    m_OutputKernelName += "_kernel";
                    AppendOutput(m_OutputKernelName.c_str());
                }

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
                // AppendOutput("/* destcoord */\n");
                PushTemporary("destCoord");
            }
            break;
        case EOpFunctionCall:
        case EOpCompare:
        case EOpClamp:
        case EOpSmoothStep:
        case EOpMod:
        case EOpMin:
        case EOpMax:
        case EOpStep:
        case EOpDot:
        case EOpCross:
        case EOpSample:
        case EOpSamplerTransform:
        case EOpConstructInt:
        case EOpConstructBool:
        case EOpConstructFloat:
        case EOpConstructVec2:
        case EOpConstructVec3:
        case EOpConstructVec4:
        case EOpConstructBVec2:
        case EOpConstructBVec3:
        case EOpConstructBVec4:
        case EOpConstructIVec2:
        case EOpConstructIVec3:
        case EOpConstructIVec4:
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
                    case EOpMod:
                        funcname = "mod";
                        break;
                    case EOpMin:
                        funcname = "min";
                        break;
                    case EOpMax:
                        funcname = "max";
                        break;
                    case EOpStep:
                        funcname = "step";
                        break;
                    case EOpDot:
                        funcname = "dot";
                        break;
                    case EOpCross:
                        funcname = "cross";
                        break;
                    case EOpSmoothStep:
                        funcname = "smoothstep";
                        break;
                    case EOpSample:
                        funcname = "__builtin_sample";
                        break;
                    case EOpSamplerTransform:
                        funcname = "__builtin_sampler_transform";
                        break;
                    case EOpConstructInt:
                        funcname = "int";
                        break;
                    case EOpConstructBool:
                        funcname = "bool";
                        break;
                    case EOpConstructFloat:
                        funcname = "float";
                        break;
                    case EOpConstructVec2:
                        funcname = "vec2";
                        break;
                    case EOpConstructVec3:
                        funcname = "vec3";
                        break;
                    case EOpConstructVec4:
                        funcname = "vec4";
                        break;
                    case EOpConstructBVec2:
                        funcname = "bvec2";
                        break;
                    case EOpConstructBVec3:
                        funcname = "bvec3";
                        break;
                    case EOpConstructBVec4:
                        funcname = "bvec4";
                        break;
                    case EOpConstructIVec2:
                        funcname = "ivec2";
                        break;
                    case EOpConstructIVec3:
                        funcname = "ivec3";
                        break;
                    case EOpConstructIVec4:
                        funcname = "ivec4";
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
                            OperatorCodeToDescription(n->getOp()));

                    FAIL_RET("Unknown aggregate op: %s", OperatorCodeToDescription(n->getOp()));
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
                        OperatorCodeToDescription(n->getFlowOp()));
                FAIL_RET("Unknown branch op: %s", OperatorCodeToDescription(n->getFlowOp()));
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
                FAIL("Unknown basic type: %i", t->getBasicType());
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
                FAIL("Unknown basic type: %i", t->getBasicType());
                break;
        }
    }

    if(t->isColor())
    {
        AppendOutput(" /*__color*/");
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
    // printf("Pushing: %s\n", t);
    m_Priv->temporaryStack.push(TString(t));
}

//=============================================================================
const char* GLSLBackend::PopTemporary()
{
    static TString lastTop;
    lastTop = m_Priv->temporaryStack.top().c_str();
    // printf("Popping: %s\n", lastTop.c_str());
    m_Priv->temporaryStack.pop();
    return lastTop.c_str();
}

//=============================================================================
} // namespace Firtree


//=============================================================================
// vim:sw=4:ts=4:cindent:et
