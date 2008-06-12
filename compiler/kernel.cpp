
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

#include "backends/glsl/glutil.h"

#include "include/kernel.h"

#include <compiler/include/compiler.h>
#include <compiler/include/main.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

#define FIRTREE_NO_GLX
#include "include/opengl.h"

static void* _KernelGetOpenGLProcAddress(const char* name);

using namespace Firtree::GLSLInternal;

namespace Firtree {

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
KernelParameter::KernelParameter()
{
}

//=============================================================================
KernelParameter::KernelParameter(const KernelParameter& p)
{
    KernelParameter();
}

//=============================================================================
KernelParameter::~KernelParameter()
{
}

//=============================================================================
KernelConstParameter::KernelConstParameter()
    :   KernelParameter()
    ,   m_BaseType(KernelConstParameter::Float)
    ,   m_Size(0)
{
}

//=============================================================================
KernelConstParameter::~KernelConstParameter()
{
}

//=============================================================================
Kernel::Kernel()
{
}

//=============================================================================
Kernel::Kernel(const char* source)
{
    SetSource(source);
}

//=============================================================================
Kernel::~Kernel()
{
    ClearParameters();
}

//=============================================================================
bool Kernel::Compile(const char* blockName)
{
    const char* pSrc = m_Source.c_str();

    // ClearParameters();

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

    GLSLBackend be(blockName);
    Compiler c(be);
    bool rv = c.Compile(&pSrc, 1);
    m_InfoLog = c.GetInfoLog();
    if(!rv)
    {
        return false;
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
                        KernelConstParameter* kp = 
                            new KernelConstParameter();
                        kp->SetSize(p.vectorSize);
                        kp->SetIsColor(p.isColor);

                        switch(p.basicType)
                        {
                            case GLSLBackend::Parameter::Int:
                                kp->SetBaseType(KernelConstParameter::Int);
                                break;
                            case GLSLBackend::Parameter::Float:
                                kp->SetBaseType(KernelConstParameter::Float);
                                break;
                            case GLSLBackend::Parameter::Bool:
                                kp->SetBaseType(KernelConstParameter::Bool);
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

    return true;
}

//=============================================================================
void Kernel::SetValueForKey(float value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const float* value, int count, const char* key)
{
    KernelConstParameter* p = ConstParameterForKeyAndType(key, 
            KernelConstParameter::Float);

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
    KernelConstParameter* p = ConstParameterForKeyAndType(key, 
            KernelConstParameter::Int);

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
    KernelConstParameter* p = ConstParameterForKeyAndType(key, 
            KernelConstParameter::Bool);

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
void Kernel::SetValueForKey(const KernelSamplerParameter& sampler, const char* key)
{
    if(m_Parameters.count(key) > 0)
    {
        KernelParameter* p = m_Parameters[key];
        if(p != NULL)
        {
            delete p;
            m_Parameters[key] = NULL;
        }
    }

    KernelSamplerParameter* ks = new KernelSamplerParameter(sampler);
    m_Parameters[key] = ks;
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
    for(std::map<std::string, KernelParameter*>::iterator i = m_Parameters.begin();
            i != m_Parameters.end(); i++)
    {
        if((*i).second != NULL)
        {
            delete (*i).second;
        }
    }

    m_Parameters.clear();
    m_UniformNameMap.clear();
}

//=============================================================================
KernelParameter* Kernel::ParameterForKey(const char* key)
{
    if(m_Parameters.count(std::string(key)) > 0)
    {
        return m_Parameters[key];
    }

    return NULL;
}

//=============================================================================
KernelConstParameter* Kernel::ConstParameterForKeyAndType(const char* key, 
        KernelConstParameter::BaseType type)
{
    KernelParameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    KernelConstParameter* kcp = kp->GetAsConst();
    if(kcp == NULL) { return NULL; }

    if(kcp->GetBaseType() != type) { return NULL; }

    return kcp;
}

//=============================================================================
KernelSamplerParameter* Kernel::SamplerParameterForKey(const char* key)
{
    KernelParameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    return kp->GetAsSampler();
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(Kernel& kernel)
    :   KernelParameter()
    ,   m_Kernel(kernel)
    ,   m_KernelCompileStatus(false)
    ,   m_SamplerIndex(-1)
    ,   m_BlockPrefix("toplevel")
{
    float identityTransform[6] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    memcpy(m_Transform, identityTransform, 6*sizeof(float));

    m_Extent[0] = m_Extent[1] = 0.0f;
    m_Extent[2] = m_Extent[3] = 0.0f;
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(const KernelSamplerParameter& sampler)
    :   KernelParameter(sampler)
    ,   m_Kernel(sampler.GetKernel())
    ,   m_KernelCompileStatus(false)
    ,   m_SamplerIndex(-1)
    ,   m_BlockPrefix("toplevel")
{
    memcpy(m_Transform, sampler.m_Transform, 6*sizeof(float));
    memcpy(m_Extent, sampler.m_Extent, 4*sizeof(float));
}

//=============================================================================
KernelSamplerParameter::~KernelSamplerParameter()
{
}

//=============================================================================
bool KernelSamplerParameter::BuildGLSL(std::string& dest)
{
    std::string body, tempStr;

    // Compile all the kernels...
    BuildTopLevelGLSL(body);

    if(!IsValid())
        return false;

    std::vector<KernelSamplerParameter*> children;
    AddChildSamplersToVector(children);

    if(children.size() > 0)
    {
        dest += 
            "vec2 __builtin_sampler_transform(int s, vec2 v);\n"
            "vec4 __builtin_sampler_extent(int s);\n"
            "vec4 __builtin_sample(int s, vec2 dc);\n";
    }

    dest += "struct _mat23 { vec3 row1; vec3 row2; };\n";

    dest += body;

    if(children.size() > 0)
    {
        std::string samplerTableName = GetBlockPrefix();
        samplerTableName += "_samplerTable";

        dest += "struct sampler { vec4 extent; _mat23 transform; };\n";
        dest += "uniform sampler ";
        dest += samplerTableName;
        dest += "[";
        char countStr[255]; snprintf(countStr, 255, "%i", children.size());
        dest += countStr;
        dest += "];\n";

        dest += 
            "vec2 __builtin_sampler_transform(int s, vec2 v) { "
//            "  return v; }\n"
            "  vec3 a = vec3(v,1.0);"
            "  return vec2("
            "     dot(a, " + samplerTableName + "[s].transform.row1), "
            "     dot(a, " + samplerTableName + "[s].transform.row2) ); }\n"
            "vec4 __builtin_sampler_extent(int s) { "
            "  return " + samplerTableName + "[s].extent; }\n"
            "vec4 __builtin_sample(int s, vec2 dc) { \n"
            "  vec4 result = vec4(0,0,0,0);\n";

        for(int i=0; i<children.size(); i++)
        {
            KernelSamplerParameter* child = children[i];

            snprintf(countStr, 255, "%i", i);
            children[i]->SetSamplerIndex(i);
            dest += "if(s == ";
            dest += countStr;
            dest += ") {\n";
            child->BuildSampleGLSL(tempStr, "dc", "result");
            dest += tempStr;
            dest += "}\n";
        }

        dest +=
            "  return result; }\n"
            ;
    }

    dest += "void main() { vec2 destCoord = gl_FragCoord.xy;\n";
    /*
    dest += "mat2x3 transform = ";
    BuildSamplerTransformGLSL(tempStr);
    dest += tempStr;
    dest += ";\n";
    dest += "destCoord = vec3(destCoord, 1) * transform;\n";
    */
    BuildSampleGLSL(tempStr, "destCoord", "gl_FragColor");
    dest += tempStr;
    dest += "\n}\n";

    return true;
}

//=============================================================================
bool KernelSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    m_KernelCompileStatus = m_Kernel.Compile(GetBlockPrefix());

    if(!IsValid())
        return false;

    dest = "";

    // Recurse down through kernel's sampler parameters.

    std::map<std::string, KernelParameter*>& kernelParams = 
        m_Kernel.GetParameters();

    for(std::map<std::string, KernelParameter*>::iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        if((*i).second != NULL)
        {
            KernelSamplerParameter* ksp = (*i).second->GetAsSampler();
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
                    fprintf(stderr, "Compilation failed: %s\n", ksp->GetKernel().GetInfoLog());
                    m_KernelCompileStatus = false;
                    return false;
                }
            }
        }
    }

    dest += m_Kernel.GetCompiledGLSL();

    return IsValid();
}

//=============================================================================
void KernelSamplerParameter::AddChildSamplersToVector(
        std::vector<KernelSamplerParameter*>& sampVec)
{
    std::map<std::string, KernelParameter*>& kernelParams = 
        m_Kernel.GetParameters();

    for(std::map<std::string, KernelParameter*>::iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
        if((*i).second != NULL)
        {
            KernelSamplerParameter* ksp = (*i).second->GetAsSampler();
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
    result += m_Kernel.GetCompiledKernelName();
    result += "(";
    result += samplerCoordVar;
    result += ");";
    dest = result;
}

//=============================================================================
void KernelSamplerParameter::BuildSamplerExtentGLSL(std::string& dest)
{
    static char paramStr[255];
    snprintf(paramStr, 255, "%f,%f,%f,%f", m_Extent[0], m_Extent[1],
            m_Extent[2], m_Extent[3]);
    std::string result = "vec4(";
    result += paramStr;
    result += ")";
    dest = result;
}

//=============================================================================
void KernelSamplerParameter::BuildSamplerTransformGLSL(std::string& dest)
{
    static char floatStr[255];
    std::string result = "mat2x3(";

    for(int i=0; i<6; i++)
    {
        if(i != 0) { result += ", "; }
        snprintf(floatStr, 255, "%f", m_Transform[i]);
        result += floatStr;
    }
    result += ")";
    dest = result;
}

//=============================================================================
void KernelSamplerParameter::SetTransform(const float* f)
{
    memcpy(m_Transform, f, 6*sizeof(float));
}

//=============================================================================
void KernelSamplerParameter::SetGLSLUniforms(unsigned int program)
{
    _KernelEnsureAPI();

    std::map<std::string, KernelParameter*>& params = m_Kernel.GetParameters();

    std::string uniPrefix = GetBlockPrefix();
    uniPrefix += "_params.";

    // Setup any sampler parameters.
    std::vector<KernelSamplerParameter*> children;
    AddChildSamplersToVector(children);
    if(children.size() > 0)
    {
        std::string samplerTableName = GetBlockPrefix();
        samplerTableName += "_samplerTable";

        for(int i=0; i<children.size(); i++)
        {
            KernelSamplerParameter* child = children[i];

            // Set all the child's uniforms
            child->SetGLSLUniforms(program);

            std::string paramName = samplerTableName;
            char idxStr[255]; snprintf(idxStr, 255, "%i", i);
            paramName += "[";
            paramName += idxStr;
            paramName += "].";

            std::string extentName = paramName + "extent";
            GLint extentUniLoc = 
                glGetUniformLocationARB(program, extentName.c_str());
            std::string transformName1 = paramName + "transform.row1";
            GLint transformUniLoc1 = 
                glGetUniformLocationARB(program, transformName1.c_str());
            std::string transformName2 = paramName + "transform.row2";
            GLint transformUniLoc2 = 
                glGetUniformLocationARB(program, transformName2.c_str());
            GLenum err = glGetError();
            if(err != GL_NO_ERROR)
            {
                FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
            }

            /*
            if(extentUniLoc == -1)
            {
                FIRTREE_WARNING("Sample table extent location not found.");
            }

            if(transformUniLoc == -1)
            {
                FIRTREE_WARNING("Sample table transform location not found.");
            }
            */

            if(extentUniLoc != -1)
            {
                const float* extent = child->GetExtent();
                glUniform4fARB(extentUniLoc, extent[0], extent[1],
                        extent[2], extent[3]);
                err = glGetError();
                if(err != GL_NO_ERROR)
                {
                    FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
                }
            }
 
            const float* transform = child->GetTransform();
            if(transformUniLoc1 != -1)
            {
                glUniform3fvARB(transformUniLoc1, 1, transform);
            }

            if(transformUniLoc2 != - 1)
            {
                glUniform3fvARB(transformUniLoc2, 1, transform + 3);
            }

            err = glGetError();
            if(err != GL_NO_ERROR)
            {
                FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
            }
       }
    }

    for(std::map<std::string, KernelParameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        KernelParameter* p = (*i).second;

        if(p == NULL)
        {
            FIRTREE_WARNING("Uninitialised parameter: %s", (*i).first.c_str());
            continue;
        }

        const char* uniName = m_Kernel.GetUniformNameForKey((*i).first.c_str());
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

        if(p->GetAsConst() != NULL)
        {
            KernelConstParameter* cp = p->GetAsConst();

            switch(cp->GetBaseType())
            {
                case KernelConstParameter::Float:
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
                case KernelConstParameter::Bool:
                case KernelConstParameter::Int:
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
                    FIRTREE_WARNING("Const parameter setting implemented for this type: %s",
                            paramName.c_str());
                    break;
            }
        } else if(p->GetAsSampler() != NULL) 
        {
            glUniform1iARB(uniformLoc, p->GetAsSampler()->GetSamplerIndex());
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

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
