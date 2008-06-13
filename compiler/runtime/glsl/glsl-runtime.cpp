
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
// This file implements the FIRTREE compiler utility functions.
//=============================================================================

#include "glsl-runtime.h"

#include <compiler/include/kernel.h>
#include <compiler/include/compiler.h>
#include <compiler/include/main.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

#define FIRTREE_NO_GLX
#include <compiler/include/opengl.h>

static void* _KernelGetOpenGLProcAddress(const char* name);

namespace Firtree { namespace GLSL {

//=============================================================================
static void _KernelEnsureAPI() 
{
    static bool initialised = false;
    if(!initialised)
    {
        initialised = true;

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            FIRTREE_ERROR("Could not initialize GLEW: %s", 
                    glewGetErrorString(err));
        }

        if(!GLEW_ARB_shading_language_100 || !GLEW_ARB_shader_objects)
        {
            FIRTREE_ERROR("OpenGL shader language support required.");
        }
    }
}

//=============================================================================
Kernel::Kernel()
    :   ReferenceCounted()
    ,   m_IsCompiled(false)
{
}

//=============================================================================
Kernel::Kernel(const char* source)
{
    this->SetSource(source);
}

//=============================================================================
Kernel::~Kernel()
{
    ClearParameters();
}

//=============================================================================
Kernel* Kernel::NewKernel() { return new Kernel(); }

//=============================================================================
Kernel* Kernel::NewKernel(const char* source) { return new Kernel(source); }

//=============================================================================
void Kernel::SetSource(const char* source)
{
    m_IsCompiled = false;

    // Set the source cache
    m_Source = source;

    // Attempt to compile the kernel.

    m_CompiledGLSL.clear();
    m_InfoLog.clear();
    m_CompiledKernelName.clear();

    /*
    {
        IRDumpBackend irbe(stdout);
        Compiler irc(irbe);
        irc.Compile(&pSrc, 1);
    }
    */

    GLSLBackend be("$$BLOCK$$");
    Compiler c(be);
    bool rv = c.Compile(&source, 1);
    m_InfoLog = c.GetInfoLog();
    if(!rv)
    {
        return;
    }

    m_CompiledGLSL = be.GetOutput();
    m_CompiledKernelName = be.GetOutputKernelName();

    GLSLBackend::Parameters& params = be.GetInputParameters();
    for(GLSLBackend::Parameters::iterator i = params.begin();
            i != params.end(); i++)
    {
        GLSLBackend::Parameter& p = *i;

        m_UniformNameMap[p.humanName] = p.uniformName;
        if((m_Parameters.count(p.humanName) == 0) ||
                (m_Parameters[p.humanName] == NULL))
        {
            switch(p.basicType)
            {
                case GLSLBackend::Parameter::Int:
                case GLSLBackend::Parameter::Float:
                case GLSLBackend::Parameter::Bool:
                    {
                        NumericParameter* kp = 
                            NumericParameter::NewNumericParameter()->GetAsNumeric();
                        kp->SetSize(p.vectorSize);
                        kp->SetIsColor(p.isColor);

                        switch(p.basicType)
                        {
                            case GLSLBackend::Parameter::Int:
                                kp->SetBaseType(NumericParameter::Int);
                                break;
                            case GLSLBackend::Parameter::Float:
                                kp->SetBaseType(NumericParameter::Float);
                                break;
                            case GLSLBackend::Parameter::Bool:
                                kp->SetBaseType(NumericParameter::Bool);
                                break;
                        }

                        m_Parameters[p.humanName] = kp;
                    }
                    break;
                case GLSLBackend::Parameter::Sampler:
                    m_Parameters[p.humanName] = NULL; // To be set later
                    break;
                default:
                    FIRTREE_WARNING("Unhandled parameter type: %i", p.basicType);
                    break;
            }
        }
    }

    m_IsCompiled = true;

    UpdateBlockNameReplacedSourceCache();
}

//=============================================================================
const char* Kernel::GetCompiledGLSL() const {
    return m_BlockReplacedGLSL.c_str();
}

//=============================================================================
const char* Kernel::GetCompiledKernelName() const {
    return m_BlockReplacedKernelName.c_str();
}

//=============================================================================
void Kernel::SetBlockName(const char* blockName)
{
    // Set the block name.
    m_BlockName = blockName;

    if(!m_IsCompiled)
        return;

    UpdateBlockNameReplacedSourceCache();
}

//=============================================================================
void Kernel::UpdateBlockNameReplacedSourceCache()
{
    if(!m_IsCompiled)
        return;

    // Form the blockname replaced source
    std::string findWhat("$$BLOCK$$");

    int pos = 0;

    m_BlockReplacedGLSL = m_CompiledGLSL;
    while(1)
    {
        pos = m_BlockReplacedGLSL.find(findWhat, pos);
        if (pos==-1) break;
        m_BlockReplacedGLSL.replace(pos,findWhat.size(),m_BlockName);
    }

    pos = 0;
    m_BlockReplacedKernelName = m_CompiledKernelName;
    while(1)
    {
        pos = m_BlockReplacedKernelName.find(findWhat, pos);
        if (pos==-1) break;
        m_BlockReplacedKernelName.replace(pos,findWhat.size(),m_BlockName);
    }
}

//=============================================================================
void Kernel::SetValueForKey(float value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const float* value, int count, const char* key)
{
    NumericParameter* p = NumericParameterForKeyAndType(key, 
            NumericParameter::Float);

    if(p == NULL) {
        FIRTREE_ERROR("No parameter: %s.", key);
    }

    if(p->GetSize() != count)
    {
        FIRTREE_ERROR("Parameter %s soes not have size %s as expected.", key, count);
    }

    for(int i=0; i<count; i++)
    {
        p->SetFloatValue(value[i], i);
    }
}

//=============================================================================
void Kernel::SetValueForKey(int value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const int* value, int count, const char* key)
{
    NumericParameter* p = NumericParameterForKeyAndType(key, 
            NumericParameter::Int);

    if(p == NULL) {
        FIRTREE_ERROR("No parameter: %s.", key);
    }

    if(p->GetSize() != count)
    {
        FIRTREE_ERROR("Parameter %s soes not have size %s as expected.", key, count);
    }

    for(int i=0; i<count; i++)
    {
        p->SetIntValue(value[i], i);
    }
}

//=============================================================================
void Kernel::SetValueForKey(bool value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const bool* value, int count, const char* key)
{
    NumericParameter* p = NumericParameterForKeyAndType(key, 
            NumericParameter::Bool);

    if(p == NULL) {
        FIRTREE_ERROR("No parameter: %s.", key);
    }

    if(p->GetSize() != count)
    {
        FIRTREE_ERROR("Parameter %s soes not have size %s as expected.", key, count);
    }

    for(int i=0; i<count; i++)
    {
        p->SetBoolValue(value[i], i);
    }
}

//=============================================================================
void Kernel::SetValueForKey(KernelSamplerParameter* sampler, const char* key)
{
    if(sampler == NULL)
        return;

    if(m_Parameters.count(key) > 0)
    {
        Parameter* p = m_Parameters[key];
        if(p != NULL)
        {
            p->Release();
            m_Parameters[key] = NULL;
        }
    }

    sampler->Retain();
    m_Parameters[key] = sampler;
}

//=============================================================================
const char* Kernel::GetUniformNameForKey(const char* key)
{
    if(m_UniformNameMap.count(key) == 0)
    {
        return NULL;
    }

    return m_UniformNameMap[key].c_str();
}

//=============================================================================
void Kernel::ClearParameters()
{
    for(std::map<std::string, Parameter*>::iterator i = m_Parameters.begin();
            i != m_Parameters.end(); i++)
    {
        if((*i).second != NULL)
        {
            (*i).second->Release();
        }
    }

    m_Parameters.clear();
    m_UniformNameMap.clear();
}

//=============================================================================
Parameter* Kernel::ParameterForKey(const char* key)
{
    if(m_Parameters.count(std::string(key)) > 0)
    {
        return m_Parameters[key];
    }

    return NULL;
}

//=============================================================================
NumericParameter* Kernel::NumericParameterForKeyAndType(const char* key, 
        NumericParameter::BaseType type)
{
    Parameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    NumericParameter* kcp = kp->GetAsNumeric();
    if(kcp == NULL) { return NULL; }

    if(kcp->GetBaseType() != type) { return NULL; }

    return kcp;
}

//=============================================================================
KernelSamplerParameter* Kernel::SamplerParameterForKey(const char* key)
{
    Parameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    return (KernelSamplerParameter*)(kp->GetAsSampler());
}

//=============================================================================
Parameter* KernelSamplerParameter::Sampler(Kernel* kernel)
{
    return new KernelSamplerParameter(kernel);
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(Kernel* kernel)
    :   SamplerParameter()
    ,   m_Kernel(kernel)
    ,   m_KernelCompileStatus(false)
    ,   m_SamplerIndex(-1)
    ,   m_BlockPrefix("toplevel")
{
    m_Kernel->Retain();
}

//=============================================================================
KernelSamplerParameter::~KernelSamplerParameter()
{
    m_Kernel->Release();
}

//=============================================================================
static void WriteSamplerFunctionsForKernel(std::string& dest,
        Kernel* kernel)
{
    static char idxStr[255]; 
    std::string tempStr;
   
    std::map<std::string, Parameter*>& params = kernel->GetParameters();

    dest += "vec4 __builtin_sample_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec4 result = vec4(0,0,0,0);\n";

    for(std::map<std::string, Parameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        Parameter *pKP = (*i).second;
        if(pKP != NULL)
        {
            KernelSamplerParameter *pKSP = 
                (KernelSamplerParameter*)(pKP->GetAsSampler());
            if(pKSP != NULL)
            {
                snprintf(idxStr, 255, "%i", pKSP->GetSamplerIndex());
                dest += "if(sampler == ";
                dest += idxStr;
                dest += ") {";
                pKSP->BuildSampleGLSL(tempStr, "samplerCoord", "result");
                dest += tempStr;
                dest += "}\n";
            }
        }
    }

    dest += "  return result;\n";
    dest += "}\n";
    
    dest += "vec2 __builtin_sampler_transform_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec3 row1 = vec3(1,0,0);\n";
    dest += "  vec3 row2 = vec3(0,1,0);\n";

    for(std::map<std::string, Parameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        Parameter *pKP = (*i).second;
        if(pKP != NULL)
        {
            KernelSamplerParameter *pKSP = 
                (KernelSamplerParameter*)(pKP->GetAsSampler());
            if(pKSP != NULL)
            {
                const float* transform = pKSP->GetTransform();
                snprintf(idxStr, 255, "%i", pKSP->GetSamplerIndex());
                dest += "if(sampler == ";
                dest += idxStr;
                dest += ") {\n";
                dest += "row1 = vec3(";
                snprintf(idxStr, 255, "%f,%f,%f", transform[0], transform[1], transform[2]);
                dest += idxStr;
                dest += ");\n";
                dest += "row2 = vec3(";
                snprintf(idxStr, 255, "%f,%f,%f", transform[3], transform[4], transform[5]);
                dest += idxStr;
                dest += ");\n";
                dest += "}\n";
            }
        }
    }

    dest += "  vec3 inVal = vec3(samplerCoord, 1.0);\n";
    dest += "  vec2 result = vec2(dot(row1, inVal), dot(row2, inVal));\n";
    dest += "  return result;\n";
    dest += "}\n";
 
    dest += "vec4 __builtin_sampler_extent_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler) {\n";
    dest += "  vec4 retVal = vec4(0,0,0,0);\n";

    for(std::map<std::string, Parameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        Parameter *pKP = (*i).second;
        if(pKP != NULL)
        {
            KernelSamplerParameter *pKSP = 
                (KernelSamplerParameter*)(pKP->GetAsSampler());
            if(pKSP != NULL)
            {
                const float* extent = pKSP->GetExtent();
                snprintf(idxStr, 255, "%i", pKSP->GetSamplerIndex());
                dest += "if(sampler == ";
                dest += idxStr;
                dest += ") {\n";
                dest += "retVal = vec4(";
                snprintf(idxStr, 255, "%f,%f,%f,%f", extent[0], extent[1], extent[2], extent[3]); 
                dest += idxStr;
                dest += ");\n";
                dest += "}\n";
            }
        }
    }

    dest += "  return retVal;\n";
    dest += "}\n";
}

//=============================================================================
bool KernelSamplerParameter::BuildGLSL(std::string& dest)
{
    std::string body, tempStr;
    static char countStr[255]; 

    // Compile all the kernels...
    BuildTopLevelGLSL(body);

    if(!IsValid())
        return false;

    std::vector<KernelSamplerParameter*> children;
    AddChildSamplersToVector(children);

    if(children.size() > 0)
    {
        for(int i=0; i<children.size(); i++)
        {
            KernelSamplerParameter* child = children[i];
            children[i]->SetSamplerIndex(i);

            dest += "vec4 __builtin_sample_";
            dest += child->GetKernel()->GetCompiledKernelName();
            dest += "(int sampler, vec2 samplerCoord);\n";

            dest += "vec2 __builtin_sampler_transform_";
            dest += child->GetKernel()->GetCompiledKernelName();
            dest += "(int sampler, vec2 samplerCoord);\n";

            dest += "vec4 __builtin_sampler_extent_";
            dest += child->GetKernel()->GetCompiledKernelName();
            dest += "(int sampler);\n";
        }

        dest += "vec4 __builtin_sample_";
        dest += m_Kernel->GetCompiledKernelName();
        dest += "(int sampler, vec2 samplerCoord);\n";

        dest += "vec2 __builtin_sampler_transform_";
        dest += m_Kernel->GetCompiledKernelName();
        dest += "(int sampler, vec2 samplerCoord);\n";

        dest += "vec4 __builtin_sampler_extent_";
        dest += m_Kernel->GetCompiledKernelName();
        dest += "(int sampler);\n";
    }

    dest += body;

    if(children.size() > 0)
    {
        for(int i=0; i<children.size(); i++)
        {
            KernelSamplerParameter* child = children[i];

            // Each child gets it's own sampler function.
            WriteSamplerFunctionsForKernel(dest, child->GetKernel());
        }

        WriteSamplerFunctionsForKernel(dest, m_Kernel);
    }

    dest += "void main() {\n"
        "vec3 inCoord = vec3(gl_FragCoord.xy, 1.0);\n";

    const float *transform = this->GetTransform();
    snprintf(countStr, 255, "vec3 row1 = vec3(%f,%f,%f);\n", 
            transform[0], transform[1], transform[2]);
    dest += countStr;
    snprintf(countStr, 255, "vec3 row2 = vec3(%f,%f,%f);\n", 
            transform[3], transform[4], transform[5]);
    dest += countStr;

    dest += "vec2 destCoord = vec2(dot(inCoord, row1), dot(inCoord, row2));\n";
    BuildSampleGLSL(tempStr, "destCoord", "gl_FragColor");
    dest += tempStr;
    dest += "\n}\n";

    return true;
}

//=============================================================================
bool KernelSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    m_Kernel->SetBlockName(GetBlockPrefix());
    m_KernelCompileStatus = m_Kernel->GetIsCompiled();

    if(!IsValid())
        return false;

    dest = "";

    // Recurse down through kernel's sampler parameters.

    std::map<std::string, Parameter*>& kernelParams = 
        m_Kernel->GetParameters();

    for(std::map<std::string, Parameter*>::iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        if((*i).second != NULL)
        {
            KernelSamplerParameter* ksp = 
                (KernelSamplerParameter*)((*i).second->GetAsSampler());
            if(ksp != NULL)
            {
                // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
                std::string prefix(GetBlockPrefix());
                prefix += "_";
                prefix += (*i).first;

                ksp->SetBlockPrefix(prefix.c_str());
                std::string samplerGLSL;
                ksp->BuildTopLevelGLSL(samplerGLSL);
                dest += samplerGLSL;

                if(!ksp->IsValid())
                {
                    // HACK!
                    fprintf(stderr, "Compilation failed: %s\n", 
                            ksp->GetKernel()->GetInfoLog());
                    m_KernelCompileStatus = false;
                    return false;
                }
            }
        }
    }

    dest += m_Kernel->GetCompiledGLSL();

    return IsValid();
}

//=============================================================================
void KernelSamplerParameter::AddChildSamplersToVector(
        std::vector<KernelSamplerParameter*>& sampVec)
{
    std::map<std::string, Parameter*>& kernelParams = 
        m_Kernel->GetParameters();

    for(std::map<std::string, Parameter*>::iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
        if((*i).second != NULL)
        {
            KernelSamplerParameter* ksp = 
                (KernelSamplerParameter*)((*i).second->GetAsSampler());
            if(ksp != NULL)
            {
                ksp->AddChildSamplersToVector(sampVec);
                sampVec.push_back(ksp);
            }
        }
    }
}

//=============================================================================
void KernelSamplerParameter::BuildSampleGLSL(std::string& dest,
                const char* samplerCoordVar,
                const char* resultVar)
{
    std::string result = resultVar;
    result += " = ";
    result += m_Kernel->GetCompiledKernelName();
    result += "(";
    result += samplerCoordVar;
    result += ");";
    dest = result;
}

//=============================================================================
void KernelSamplerParameter::SetGLSLUniforms(unsigned int program)
{
    _KernelEnsureAPI();

    std::map<std::string, Parameter*>& params = m_Kernel->GetParameters();

    std::string uniPrefix = GetBlockPrefix();
    uniPrefix += "_params.";

    // Setup any sampler parameters.
    std::vector<KernelSamplerParameter*> children;
    AddChildSamplersToVector(children);
    for(int i=0; i<children.size(); i++)
    {
        KernelSamplerParameter* child = children[i];

        // Set all the child's uniforms
        child->SetGLSLUniforms(program);
    }

    for(std::map<std::string, Parameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        Parameter* p = (*i).second;

        if(p == NULL)
        {
            FIRTREE_WARNING("Uninitialised parameter: %s", (*i).first.c_str());
            continue;
        }

        const char* uniName = m_Kernel->GetUniformNameForKey((*i).first.c_str());
        if(uniName == NULL)
        {
            FIRTREE_WARNING("Unknown parameter: %s", (*i).first.c_str());
            continue;
        }
        std::string paramName = uniPrefix + uniName;

        // Find this parameter's uniform location
        GLint uniformLoc = glGetUniformLocationARB(program, paramName.c_str());

        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
        }

        if(uniformLoc == -1)
        {
            // The linker may have removed this uniform from the program if it
            // isn't used.
            
            /*
            FIRTREE_WARNING("Parameter '%s' could not be set. Is it used in the kernel?",
                    (*i).first.c_str());
                    */

            continue;
        }

        if(p->GetAsNumeric() != NULL)
        {
            NumericParameter* cp = p->GetAsNumeric();

            switch(cp->GetBaseType())
            {
                case NumericParameter::Float:
                    {
                        static float vec[4];
                        for(int j=0; j<cp->GetSize(); j++)
                        {
                            vec[j] = cp->GetFloatValue(j);
                        }

                        switch(cp->GetSize())
                        {
                            case 1:
                                glUniform1fARB(uniformLoc, vec[0]);
                                break;
                            case 2:
                                glUniform2fARB(uniformLoc, vec[0], vec[1]);
                                break;
                            case 3:
                                glUniform3fARB(uniformLoc, vec[0], vec[1], vec[2]);
                                break;
                            case 4:
                                glUniform4fARB(uniformLoc, vec[0], vec[1], vec[2], vec[3]);
                                break;
                            default:
                                FIRTREE_ERROR("Parameter %s has invalid size: %i",
                                        paramName.c_str(), cp->GetSize());
                        }

                        err = glGetError();
                        if(err != GL_NO_ERROR)
                        {
                            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
                        }
                    }
                    break;
                case NumericParameter::Bool:
                case NumericParameter::Int:
                    {
                        static int vec[4];
                        for(int j=0; j<cp->GetSize(); j++)
                        {
                            vec[j] = cp->GetIntValue(j);
                        }

                        switch(cp->GetSize())
                        {
                            case 1:
                                glUniform1iARB(uniformLoc, vec[0]);
                                break;
                            case 2:
                                glUniform2iARB(uniformLoc, vec[0], vec[1]);
                                break;
                            case 3:
                                glUniform3iARB(uniformLoc, vec[0], vec[1], vec[2]);
                                break;
                            case 4:
                                glUniform4iARB(uniformLoc, vec[0], vec[1], vec[2], vec[3]);
                                break;
                            default:
                                FIRTREE_ERROR("Parameter %s has invalid size: %i",
                                        paramName.c_str(), cp->GetSize());
                        }

                        err = glGetError();
                        if(err != GL_NO_ERROR)
                        {
                            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
                        }
                    }
                    break;
                default:
                    FIRTREE_WARNING("Numeric parameter setting implemented for this type: %s",
                            paramName.c_str());
                    break;
            }
        } else if(p->GetAsSampler() != NULL) 
        {
            KernelSamplerParameter* ksp = 
                (KernelSamplerParameter*)(p->GetAsSampler());
            glUniform1iARB(uniformLoc, ksp->GetSamplerIndex());
            err = glGetError();
            if(err != GL_NO_ERROR)
            {
                FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
            }
        } else {
            FIRTREE_ERROR("Unknown kernel parameter type.");
        }
    }
}

} } // namespace Firtree::GLSL

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
