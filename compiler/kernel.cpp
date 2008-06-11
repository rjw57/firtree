
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

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
static PFNGLUNIFORM1FPROC glUniform1f = NULL;
static PFNGLUNIFORM2FPROC glUniform2f = NULL;
static PFNGLUNIFORM3FPROC glUniform3f = NULL;
static PFNGLUNIFORM4FPROC glUniform4f = NULL;
static PFNGLUNIFORM1IPROC glUniform1i = NULL;
static PFNGLUNIFORM2IPROC glUniform2i = NULL;
static PFNGLUNIFORM3IPROC glUniform3i = NULL;
static PFNGLUNIFORM4IPROC glUniform4i = NULL;

static void* _KernelGetOpenGLProcAddress(const char* name);

using namespace Firtree::GLSLInternal;

namespace Firtree {

//=============================================================================
static void _KernelEnsureAPI() 
{
#   define ENSURE_API(name, type) do { \
    if(NULL == name) \
    { \
        name = (type)GetProcAddress(#name); \
        if(NULL == name) { \
            FIRTREE_ERROR(#name " not supported."); \
        } \
    } } while(0)

    ENSURE_API(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    ENSURE_API(glUniform1f, PFNGLUNIFORM1FPROC);
    ENSURE_API(glUniform2f, PFNGLUNIFORM2FPROC);
    ENSURE_API(glUniform3f, PFNGLUNIFORM3FPROC);
    ENSURE_API(glUniform4f, PFNGLUNIFORM4FPROC);
    ENSURE_API(glUniform1i, PFNGLUNIFORM1IPROC);
    ENSURE_API(glUniform2i, PFNGLUNIFORM2IPROC);
    ENSURE_API(glUniform3i, PFNGLUNIFORM3IPROC);
    ENSURE_API(glUniform4i, PFNGLUNIFORM4IPROC);
}

//=============================================================================
KernelParameter::KernelParameter(const char* name)
    :   m_Name(name)
{
}

//=============================================================================
KernelParameter::~KernelParameter()
{
}

//=============================================================================
KernelConstParameter::KernelConstParameter(const char* name)
    :   KernelParameter(name)
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

    ClearParameters();

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

        switch(p.basicType)
        {
            case GLSLBackend::Parameter::Int:
            case GLSLBackend::Parameter::Float:
            case GLSLBackend::Parameter::Bool:
                {
                    KernelConstParameter* kp = 
                        new KernelConstParameter(p.humanName.c_str());
                    kp->SetSize(p.vectorSize);
                    kp->SetGLSLName(p.uniformName);
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

                    m_Parameters.push_back(kp);
                }
                break;
            default:
                FIRTREE_WARNING("Unhandled parameter type: %i", p.basicType);
                break;
        }
    }

    return true;
}

//=============================================================================
void Kernel::SetValueForKey(float value, const char* key)
{
    KernelConstParameter* p = ConstParameterForKeyAndType(key, 
            KernelConstParameter::Float);

    if(p == NULL) {
        FIRTREE_ERROR("No parameter: %s.", key);
    }

    if(p->GetSize() != 1)
    {
        FIRTREE_ERROR("Parameter %s soes not have size 1 as expected.", key);
    }

    p->SetFloatValue(value, 0);
}

//=============================================================================
void Kernel::ClearParameters()
{
    for(std::vector<KernelParameter*>::iterator i = m_Parameters.begin();
            i != m_Parameters.end(); i++)
    {
        if(*i != NULL)
        {
            delete *i;
        }
    }

    m_Parameters.clear();
}

//=============================================================================
KernelParameter* Kernel::ParameterForKey(const char* key)
{
    for(std::vector<KernelParameter*>::iterator i = m_Parameters.begin();
            i != m_Parameters.end(); i++)
    {
        KernelParameter* param = *i;
        if(param->GetName() == key)
        {
            return param;
        }
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
KernelSamplerParameter::KernelSamplerParameter(const char* name, Kernel& kernel)
    :   KernelParameter(name)
    ,   m_Kernel(kernel)
    ,   m_KernelCompileStatus(false)
{
}

//=============================================================================
KernelSamplerParameter::~KernelSamplerParameter()
{
}

//=============================================================================
void KernelSamplerParameter::SetBlockPrefix(const char* p)
{
    KernelParameter::SetBlockPrefix(p);

    m_KernelCompileStatus = m_Kernel.Compile(GetBlockPrefix());
}

//=============================================================================
void KernelSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    dest = m_Kernel.GetCompiledGLSL();
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
void KernelSamplerParameter::SetGLSLUniforms(unsigned int program)
{
    _KernelEnsureAPI();

    std::vector<KernelParameter*>& params = m_Kernel.GetParameters();

    std::string uniPrefix = GetBlockPrefix();
    uniPrefix += "_params.";

    for(std::vector<KernelParameter*>::iterator i = params.begin();
            i != params.end(); i++)
    {
        KernelParameter* p = *i;
        std::string paramName = uniPrefix + p->GetGLSLName();

        // Find this parameter's uniform location
        GLint uniformLoc = glGetUniformLocation(program, paramName.c_str());
        if(uniformLoc == -1)
        {
            FIRTREE_ERROR("Uniform '%s' not found in program.", paramName.c_str());
        }
        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
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
                                glUniform1f(uniformLoc, vec[0]);
                                break;
                            case 2:
                                glUniform2f(uniformLoc, vec[0], vec[1]);
                                break;
                            case 3:
                                glUniform3f(uniformLoc, vec[0], vec[1], vec[2]);
                                break;
                            case 4:
                                glUniform4f(uniformLoc, vec[0], vec[1], vec[2], vec[3]);
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
                                glUniform1i(uniformLoc, vec[0]);
                                break;
                            case 2:
                                glUniform2i(uniformLoc, vec[0], vec[1]);
                                break;
                            case 3:
                                glUniform3i(uniformLoc, vec[0], vec[1], vec[2]);
                                break;
                            case 4:
                                glUniform4i(uniformLoc, vec[0], vec[1], vec[2], vec[3]);
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
        } else {
            FIRTREE_ERROR("Unknown kernel parameter type.");
        }
    }
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
