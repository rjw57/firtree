
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

#include "glsl-runtime-priv.h"

#include <float.h>

#define FIRTREE_NO_GLX
#include <firtree/opengl.h>
#include <firtree/main.h>
#include <firtree/kernel.h>

#include <firtree/internal/image-int.h>

#include <compiler/include/compiler.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

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

        if(!GLEW_ARB_multitexture)
        {
            FIRTREE_ERROR("ARB_mulitexture support required.");
        }
    }
}

//=============================================================================
Kernel::Kernel()
    :   Firtree::Kernel()
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
Firtree::Kernel* Kernel::Create() { return new Kernel(); }

//=============================================================================
Firtree::Kernel* Kernel::Create(const char* source) { return new Kernel(source); }

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
                            dynamic_cast<NumericParameter*>
                            (NumericParameter::Create());
                        kp->SetSize(p.vectorSize);
                        kp->SetIsColor(p.isColor);

                        switch(p.basicType)
                        {
                            case GLSLBackend::Parameter::Int:
                                kp->SetBaseType(NumericParameter::TypeInteger);
                                break;
                            case GLSLBackend::Parameter::Float:
                                kp->SetBaseType(NumericParameter::TypeFloat);
                                break;
                            case GLSLBackend::Parameter::Bool:
                                kp->SetBaseType(NumericParameter::TypeBool);
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
void Kernel::SetValueForKey(Parameter* param, const char* key)
{
    GLSL::SamplerParameter* sampler = 
        dynamic_cast<GLSL::SamplerParameter*>(param);
    NumericParameter* numeric = 
        dynamic_cast<NumericParameter*>(param);

    if((sampler == NULL) && (numeric == NULL))
        return;

    if(sampler != NULL)
    {
        sampler->Retain();

        if(m_Parameters.count(key) > 0)
        {
            Parameter* p = m_Parameters[key];
            FIRTREE_SAFE_RELEASE(m_Parameters[key]);
        }

        m_Parameters[key] = sampler;
    } else if(numeric != NULL)
    {
        NumericParameter* p = 
            NumericParameterForKeyAndType(key, numeric->GetBaseType());
        if((p == NULL) || (p->GetSize() != numeric->GetSize()))
        {
            FIRTREE_WARNING("No such parameter '%s' of correct size and type "
                    "found in kernel", key);
            return;
        }

        p->AssignFrom(*numeric);
    }
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
            Parameter* p = (*i).second;
            FIRTREE_SAFE_RELEASE(p);
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

    NumericParameter* kcp = dynamic_cast<NumericParameter*>(kp);
    if(kcp == NULL) { return NULL; }

    if(kcp->GetBaseType() != type) { return NULL; }

    return kcp;
}

//=============================================================================
SamplerParameter* Kernel::SamplerParameterForKey(const char* key)
{
    Parameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    SamplerParameter* sp = dynamic_cast<SamplerParameter*>(kp);
    return sp;
}

//=============================================================================
SamplerParameter* KernelSamplerParameter::Create(Image* im)
{
    return new KernelSamplerParameter(im);
}

//=============================================================================
SamplerParameter::SamplerParameter()
    :   Firtree::SamplerParameter()
    ,   m_Transform(AffineTransform::Identity())
    ,   m_SamplerIndex(-1)
    ,   m_BlockPrefix("toplevel")
{
}

//=============================================================================
SamplerParameter::~SamplerParameter()
{
    m_Transform->Release();
}

//=============================================================================
void SamplerParameter::SetTransform(const AffineTransform* f)
{
    if(f == NULL)
        return;

    AffineTransform* oldTrans = m_Transform;
    m_Transform = f->Copy();
    if(oldTrans != NULL) { oldTrans->Release(); }
}

//=============================================================================
const Rect2D SamplerParameter::GetExtent() const 
{
    return Rect2D(-0.5f*FLT_MAX, -0.5f*FLT_MAX, FLT_MAX, FLT_MAX);
}

//=============================================================================
const Rect2D SamplerParameter::GetDomain() const 
{
    return Rect2D(-0.5f*FLT_MAX, -0.5f*FLT_MAX, FLT_MAX, FLT_MAX);
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(Image* im)
    :   Firtree::GLSL::SamplerParameter()
    ,   m_KernelCompileStatus(false)
{
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);
    if(imImpl == NULL) { return; }
    if(!(imImpl->HasKernel())) { return; }

    Firtree::Kernel* k = imImpl->GetKernel();
    GLSL::Kernel* gk = dynamic_cast<GLSL::Kernel*>(k);
    if(gk == NULL) { return; }
    
    AffineTransform* underlyingTransform = 
        imImpl->GetTransformFromUnderlyingImage();

    m_Kernel = gk;
    m_Kernel->Retain();

    AffineTransform* t = GetTransform()->Copy();
    t->AppendTransform(underlyingTransform);
    SetTransform(t);
    FIRTREE_SAFE_RELEASE(t);
    FIRTREE_SAFE_RELEASE(underlyingTransform);
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
   
    const std::map<std::string, Parameter*>& params = kernel->GetParameters();

    dest += "vec4 __builtin_sample_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec4 result = vec4(0,0,0,0);\n";
    
    // Create a vector os sampler parameters.
    std::vector<SamplerParameter*> samplerParams;
    for(std::map<std::string, Parameter*>::const_iterator i = params.begin();
            i != params.end(); i++)
    {
        Parameter *pKP = (*i).second;
        if(pKP != NULL)
        {
            SamplerParameter *pKSP = 
                dynamic_cast<SamplerParameter*>(pKP);
            if(pKSP != NULL)
            {
                samplerParams.push_back(pKSP);
            }
        }
    }

    // Special case for kernels with only one sampler.
    if(samplerParams.size() == 1)
    {
        SamplerParameter *pKSP = samplerParams.front();
        pKSP->BuildSampleGLSL(tempStr, "samplerCoord", "result");
        dest += tempStr;
    } else {
        for(std::vector<SamplerParameter*>::const_iterator i = samplerParams.begin();
                i != samplerParams.end(); i++)
        {
            SamplerParameter *pKSP = *i;
            snprintf(idxStr, 255, "%i", pKSP->GetSamplerIndex());
            dest += "if(sampler == ";
            dest += idxStr;
            dest += ") {";
            pKSP->BuildSampleGLSL(tempStr, "samplerCoord", "result");
            dest += tempStr;
            dest += "}\n";
        }
    }

    dest += "  return result;\n";
    dest += "}\n";
    
    dest += "vec2 __builtin_sampler_transform_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec3 row1 = vec3(1,0,0);\n";
    dest += "  vec3 row2 = vec3(0,1,0);\n";
    dest += "  vec3 result = vec3(samplerCoord, 1.0);\n";

    // Special case for kernels with only one sampler.
    if(samplerParams.size() == 1)
    {
        SamplerParameter *pSP = samplerParams.front();
        AffineTransform* invTrans = pSP->GetTransform()->Copy();
        invTrans->Invert();
        const AffineTransformStruct& transform =
            invTrans->GetTransformStruct();
        if(!invTrans->IsIdentity())
        {
            dest += "row1 = vec3(";
            snprintf(idxStr, 255, "%f,%f,%f",
                    transform.m11, transform.m12, transform.tX);
            dest += idxStr;
            dest += ");\n";
            dest += "row2 = vec3(";
            snprintf(idxStr, 255, "%f,%f,%f", 
                    transform.m21, transform.m22, transform.tY);
            dest += idxStr;
            dest += ");\n";
            dest += "result.xy = vec2(dot(row1, result), dot(row2, result));\n";
        }
        invTrans->Release();
    } else {
        for(std::vector<SamplerParameter*>::const_iterator i = samplerParams.begin();
                i != samplerParams.end(); i++)
        {
            SamplerParameter *pSP = *i;
            AffineTransform* invTrans = pSP->GetTransform()->Copy();
            invTrans->Invert();
            const AffineTransformStruct& transform =
                invTrans->GetTransformStruct();
            if(!invTrans->IsIdentity())
            {
                snprintf(idxStr, 255, "%i", pSP->GetSamplerIndex());
                dest += "if(sampler == ";
                dest += idxStr;
                dest += ") {\n";
                dest += "row1 = vec3(";
                snprintf(idxStr, 255, "%f,%f,%f",
                        transform.m11, transform.m12, transform.tX);
                dest += idxStr;
                dest += ");\n";
                dest += "row2 = vec3(";
                snprintf(idxStr, 255, "%f,%f,%f", 
                        transform.m21, transform.m22, transform.tY);
                dest += idxStr;
                dest += ");\n";
                dest += "result.xy = vec2(dot(row1, result), dot(row2, result));\n";
                dest += "}\n";
            }
            invTrans->Release();
        }
    }

    dest += "  return result.xy;\n";
    dest += "}\n";
 
    dest += "vec4 __builtin_sampler_extent_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler) {\n";
    dest += "  vec4 retVal = vec4(0,0,0,0);\n";

    if(samplerParams.size() == 1)
    {
        SamplerParameter *pSP = samplerParams.front();
        const Rect2D& extent = pSP->GetExtent();
        dest += "retVal = vec4(";
        snprintf(idxStr, 255, "%f,%f,%f,%f",
                extent.Origin.X, extent.Origin.Y,
                extent.Size.Width, extent.Size.Height);
        dest += idxStr;
        dest += ");\n";
    } else {
        for(std::vector<SamplerParameter*>::const_iterator i = samplerParams.begin();
                i != samplerParams.end(); i++)
        {
            SamplerParameter *pSP = *i;
            const Rect2D& extent = pSP->GetExtent();
            snprintf(idxStr, 255, "%i", pSP->GetSamplerIndex());
            dest += "if(sampler == ";
            dest += idxStr;
            dest += ") {\n";
            dest += "retVal = vec4(";
            snprintf(idxStr, 255, "%f,%f,%f,%f",
                    extent.Origin.X, extent.Origin.Y,
                    extent.Size.Width, extent.Size.Height);
            dest += idxStr;
            dest += ");\n";
            dest += "}\n";
        }
    }

    dest += "  return retVal;\n";
    dest += "}\n";
}

//=============================================================================
bool BuildGLSLShaderForSampler(std::string& dest, Firtree::SamplerParameter* s)
{
    GLSL::SamplerParameter* sampler = 
        dynamic_cast<GLSL::SamplerParameter*>(s);

    if(sampler == NULL)
        return false;

    std::string body, tempStr;
    static char countStr[255]; 

    sampler->BuildTopLevelGLSL(body);

    if(!sampler->IsValid())
        return false;

    // Add builtin functions
    dest += 
        "vec2 __builtin_sincos(float a) { return vec2(sin(a),cos(a)); }"
        "vec2 __builtin_cossin(float a) { return vec2(cos(a),sin(a)); }"
        ;

    std::vector<SamplerParameter*> children;
    KernelSamplerParameter* ksp = dynamic_cast<KernelSamplerParameter*>(s);
    if(ksp != NULL)
    {
        ksp->AddChildSamplersToVector(children);
    }

    if(children.size() > 0)
    {
        int samplerIdx = 0;
        int textureIdx = 0;
        for(int i=0; i<children.size(); i++)
        {
            SamplerParameter* child = 
                dynamic_cast<GLSL::SamplerParameter*>(children[i]);
            if(child != NULL)
            {
                if(child != NULL)
                {
                    child->SetSamplerIndex(samplerIdx);
                    samplerIdx++;
                }

                TextureSamplerParameter* tsp =
                    dynamic_cast<TextureSamplerParameter*>(child);
                if(tsp != NULL)
                {
                    tsp->SetGLTextureUnit(textureIdx);
                    textureIdx++;
                }
            }
        }
    }

    dest += body;

    if(children.size() > 0)
    {
        for(int i=0; i<children.size(); i++)
        {
            KernelSamplerParameter* child = 
                dynamic_cast<KernelSamplerParameter*>(children[i]);

            if(child != NULL)
            {
                // Each child gets it's own sampler function.
                WriteSamplerFunctionsForKernel(dest, child->GetKernel());
            }
        }

        WriteSamplerFunctionsForKernel(dest, 
                ((KernelSamplerParameter*)sampler)->GetKernel());
    }

    dest += "void main() {\n"
        "vec3 inCoord = vec3(gl_TexCoord[0].xy, 1.0);\n";

    AffineTransform* invTrans = sampler->GetTransform()->Copy();
    invTrans->Invert();
    const AffineTransformStruct& transform =
        invTrans->GetTransformStruct();
    snprintf(countStr, 255, "vec3 row1 = vec3(%f,%f,%f);\n", 
            transform.m11, transform.m12, transform.tX);
    dest += countStr;
    snprintf(countStr, 255, "vec3 row2 = vec3(%f,%f,%f);\n", 
            transform.m21, transform.m22, transform.tY);
    dest += countStr;
    invTrans->Release();

    dest += "vec2 destCoord = vec2(dot(inCoord, row1), dot(inCoord, row2));\n";
    sampler->BuildSampleGLSL(tempStr, "destCoord", "gl_FragColor");
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

    // Declare the sampling functions.
    dest += "vec4 __builtin_sample_";
    dest += GetKernel()->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord);\n";

    dest += "vec2 __builtin_sampler_transform_";
    dest += GetKernel()->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord);\n";

    dest += "vec4 __builtin_sampler_extent_";
    dest += GetKernel()->GetCompiledKernelName();
    dest += "(int sampler);\n";

    // Recurse down through kernel's sampler parameters.

    const std::map<std::string, Parameter*>& kernelParams = 
        m_Kernel->GetParameters();

    for(std::map<std::string, Parameter*>::const_iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        if((*i).second != NULL)
        {
            SamplerParameter* sp = 
                dynamic_cast<SamplerParameter*>((*i).second);
            if(sp != NULL)
            {
                // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
                std::string prefix(GetBlockPrefix());
                prefix += "_";
                prefix += (*i).first;

                sp->SetBlockPrefix(prefix.c_str());
                std::string samplerGLSL;
                sp->BuildTopLevelGLSL(samplerGLSL);
                dest += samplerGLSL;

                KernelSamplerParameter* ksp = 
                    dynamic_cast<KernelSamplerParameter*>(sp);
                if((ksp != NULL) && (!ksp->IsValid()))
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
        std::vector<GLSL::SamplerParameter*>& sampVec)
{
    const std::map<std::string, Parameter*>& kernelParams = 
        m_Kernel->GetParameters();

    for(std::map<std::string, Parameter*>::const_iterator i=kernelParams.begin();
            m_KernelCompileStatus && (i != kernelParams.end()); i++)
    {
        // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
        if((*i).second != NULL)
        {
            GLSL::SamplerParameter* sp = 
                dynamic_cast<GLSL::SamplerParameter*>((*i).second);
            if(sp == NULL)
                continue;

            KernelSamplerParameter* ksp = 
                dynamic_cast<KernelSamplerParameter*>(sp);
            if(ksp != NULL)
            {
                ksp->AddChildSamplersToVector(sampVec);
            }

            sampVec.push_back(sp);
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

    const std::map<std::string, Parameter*>& params = m_Kernel->GetParameters();

    std::string uniPrefix = GetBlockPrefix();
    uniPrefix += "_params.";

    // Setup any sampler parameters.
    std::vector<SamplerParameter*> children;
    AddChildSamplersToVector(children);
    for(int i=0; i<children.size(); i++)
    {
        SamplerParameter* child = children[i];

        // Set all the child's uniforms
        child->SetGLSLUniforms(program);
    }

    for(std::map<std::string, Parameter*>::const_iterator i = params.begin();
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

        NumericParameter* cp = dynamic_cast<NumericParameter*>(p);
        SamplerParameter* sp = 
                dynamic_cast<SamplerParameter*>(p);
        if(cp != NULL)
        {
            switch(cp->GetBaseType())
            {
                case NumericParameter::TypeFloat:
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
                case NumericParameter::TypeBool:
                case NumericParameter::TypeInteger:
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
        } else if(sp != NULL) 
        {
            glUniform1iARB(uniformLoc, sp->GetSamplerIndex());
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

//=============================================================================
TextureSamplerParameter::TextureSamplerParameter(Image* im)
    :   Firtree::GLSL::SamplerParameter()
    ,   m_TextureUnit(0)
{
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);

    if(imImpl == NULL) { return; }

    m_Image = imImpl;
    m_Image->Retain();

    Size2D underlyingSize = m_Image->GetUnderlyingPixelSize();
    AffineTransform* underlyingTransform = 
        m_Image->GetTransformFromUnderlyingImage();

    m_Domain = Rect2D(0.f, 0.f, 1.f, 1.f);

    AffineTransform* t = GetTransform()->Copy();
    t->ScaleBy(1, -1);
    t->TranslateBy(0, 1);
    t->ScaleBy(underlyingSize.Width, underlyingSize.Height);
    t->AppendTransform(underlyingTransform);
    SetTransform(t);
    FIRTREE_SAFE_RELEASE(t);
    FIRTREE_SAFE_RELEASE(underlyingTransform);
}

//=============================================================================
TextureSamplerParameter::~TextureSamplerParameter()
{
    FIRTREE_SAFE_RELEASE(m_Image);
}

//=============================================================================
const Rect2D TextureSamplerParameter::GetExtent() const 
{
    return RectTransform(GetDomain(), GetTransform());
}

//=============================================================================
SamplerParameter* TextureSamplerParameter::Create(Image* im)
{
    return new TextureSamplerParameter(im);
}

//=============================================================================
bool TextureSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    dest = "uniform sampler2D ";
    dest += GetBlockPrefix();
    dest += "_texture;\n";

    dest += "vec4 ";
    dest += GetBlockPrefix();
    dest += "_kernel(in vec2 destCoord) {\n";
    dest += "  return texture2D(";
    dest += GetBlockPrefix();
    dest += "_texture, destCoord);\n";
    dest += "}\n";

    return true;
}

//=============================================================================
void TextureSamplerParameter::BuildSampleGLSL(std::string& dest,
        const char* samplerCoordVar, const char* resultVar)
{
    dest = resultVar;
    dest += " = texture2D(";
    dest += GetBlockPrefix();
    dest += "_texture, ";
    dest += samplerCoordVar;
    dest += ");\n";
}

//=============================================================================
bool TextureSamplerParameter::IsValid() const 
{
    return (m_Image != NULL);
}

//=============================================================================
void TextureSamplerParameter::SetGLSLUniforms(unsigned int program)
{
    if(!IsValid())
        return;

    _KernelEnsureAPI();

    std::string paramName(GetBlockPrefix());
    paramName += "_texture";

    GLint uniformLoc = glGetUniformLocationARB(program, paramName.c_str());
    GLenum err = glGetError();
    if(err != GL_NO_ERROR)
    {
        FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
    }

    if(uniformLoc != -1)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + GetGLTextureUnit());
        glBindTexture(GL_TEXTURE_2D, GetGLTextureObject());

        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

        glUniform1iARB(uniformLoc, GetGLTextureUnit());
        err = glGetError();
        if(err != GL_NO_ERROR)
        {
            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
        }
    }
}

//=============================================================================
unsigned int TextureSamplerParameter::GetGLTextureObject() const
{
    if(m_Image == NULL)
    {
        return 0;
    }

    return m_Image->GetAsOpenGLTexture();
}

//=============================================================================
Firtree::SamplerParameter* CreateSampler(Image* im)
{
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);
    if(imImpl == NULL) { return NULL; }

    if(imImpl->HasKernel())
    {
        return CreateKernelSampler(im);
    }

    return CreateTextureSampler(im);
}

//=============================================================================
Firtree::SamplerParameter* CreateTextureSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    SamplerParameter* rv = TextureSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetTransform()->Copy();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    tc->Release();
    return rv;
}

//=============================================================================
Firtree::SamplerParameter* CreateKernelSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    SamplerParameter* rv = KernelSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetTransform()->Copy();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    tc->Release();
    return rv;
}

//=============================================================================
Firtree::SamplerParameter* CreateTextureSampler(Image* im)
{
    AffineTransform* t = AffineTransform::Identity();
    Firtree::SamplerParameter* rv =
        CreateTextureSamplerWithTransform(im, t);
    FIRTREE_SAFE_RELEASE(t);
    return rv;
}

//=============================================================================
Firtree::SamplerParameter* CreateKernelSampler(Image* im)
{
    return KernelSamplerParameter::Create(im);
}

//=============================================================================
Firtree::Kernel* CreateKernel(const char* source)
{
    return GLSL::Kernel::Create(source);
}

//=============================================================================
bool SetGLSLUniformsForSampler(Firtree::SamplerParameter* sampler, unsigned int prog)
{
    GLSL::SamplerParameter* s = 
        dynamic_cast<GLSL::SamplerParameter*>(sampler);
    if(s == NULL)
    {
        return false;
    }

    s->SetGLSLUniforms(prog);

    return true;
}

//=============================================================================
const char* GetInfoLogForSampler(Firtree::SamplerParameter* sampler)
{
    GLSL::KernelSamplerParameter* s = 
        dynamic_cast<GLSL::KernelSamplerParameter*>(sampler);
    if(s == NULL)
    {
        return NULL;
    }

    return s->GetKernel()->GetInfoLog();
}

//=============================================================================
struct RenderingContext {
    GLuint                   Program;
    GLuint                   FragShader;
    GLSL::SamplerParameter*  Sampler;
};

//=============================================================================
#define CHECK(a) do { \
    { do { (a); } while(0); } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) {  \
        FIRTREE_ERROR("%s:%i: OpenGL Error %s", __FILE__, __LINE__,\
                gluErrorString(_err)); return false; \
    } } while(0) 

//=============================================================================
RenderingContext* CreateRenderingContext(Firtree::SamplerParameter* topLevelSampler)
{
    _KernelEnsureAPI();

    GLSL::SamplerParameter* sampler = 
        dynamic_cast<GLSL::SamplerParameter*>(topLevelSampler);

    if(sampler == NULL)
    {
        return NULL;
    }

    RenderingContext* retVal = new RenderingContext();

    retVal->Sampler = sampler;
    retVal->Sampler->Retain();

    std::string shaderSource;
    bool success = GLSL::BuildGLSLShaderForSampler(shaderSource, retVal->Sampler);

    if(!success)
    {
        FIRTREE_ERROR("Error compiling kernel:\n %s",
                GLSL::GetInfoLogForSampler(sampler));
        delete retVal;
        return NULL;
    }

    retVal->FragShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if(retVal->FragShader == 0)
    {
        fprintf(stderr, "Error creating shader object.\n");
        delete retVal;
        return NULL;
    }

    const char* pSrc = shaderSource.c_str();
    // printf(pSrc);
    CHECK( glShaderSourceARB(retVal->FragShader, 1, &pSrc, NULL) );
    CHECK( glCompileShaderARB(retVal->FragShader) );

    GLint status = 0;
    CHECK( glGetObjectParameterivARB(retVal->FragShader, 
                GL_OBJECT_COMPILE_STATUS_ARB, &status) );
    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK( glGetObjectParameterivARB(retVal->FragShader,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetInfoLogARB(retVal->FragShader, logLen, &logLen, log) );
        FIRTREE_ERROR("Error compiling shader: %s\nSource: %s\n", 
                log, pSrc);
        free(log);
        delete retVal;
        return NULL;
    }

    CHECK( retVal->Program = glCreateProgramObjectARB() );
    CHECK( glAttachObjectARB(retVal->Program, retVal->FragShader) );
    CHECK( glLinkProgramARB(retVal->Program) );
    CHECK( glGetObjectParameterivARB(retVal->Program, 
                GL_OBJECT_LINK_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK( glGetObjectParameterivARB(retVal->Program,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK( glGetInfoLogARB(retVal->Program, logLen, &logLen, log) );
        FIRTREE_ERROR("Error linking shader: %s\n", log);
        free(log);
        delete retVal;
        return NULL;
    }

    return retVal;
}

//=============================================================================
void ReleaseRenderingContext(RenderingContext* c)
{
    if(c != NULL) { 
        FIRTREE_SAFE_RELEASE(c->Sampler);
        glDeleteObjectARB(c->FragShader);
        glDeleteObjectARB(c->Program);
        delete c; 
    }
}

//=============================================================================
void RenderAtPoint(RenderingContext* context, const Point2D& location,
        const Rect2D& srcRect)
{
    RenderInRect(context, Rect2D(location, srcRect.Size), srcRect);
}

//=============================================================================
void RenderInRect(RenderingContext* context, const Rect2D& destRect, 
        const Rect2D& srcRect)
{
    GLenum err;
    if(context == NULL) { return; }

#if 1
    AffineTransform* srcToDestTrans = RectComputeTransform(srcRect, destRect);

    // Firstly clip srcRect by extent
    Rect2D extent = context->Sampler->GetExtent();
    Rect2D clipSrcRect = RectIntersect(srcRect, extent);

    // Transform clipped source to destination
    Rect2D renderRect = RectTransform(clipSrcRect, srcToDestTrans);

    // Clip destination by viewport... TODO
    
    // Transform clipped destination rect back to source
    srcToDestTrans->Invert();
    clipSrcRect = RectTransform(renderRect, srcToDestTrans);

    srcToDestTrans->Release();

    // Do nothing if we've clipped everything away.
    if((clipSrcRect.Size.Width == 0.f) && (clipSrcRect.Size.Height == 0.f))
        return;
#else
    Rect2D clipSrcRect = srcRect;
    Rect2D renderRect = destRect;
#endif

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);      
    // This causes a crash on OSX. No idea why :(
    //  `-> glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_BLEND);

    glUseProgramObjectARB(context->Program);
    if((err = glGetError()) != GL_NO_ERROR)
    {
        FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
        return;
    }

    GLSL::SetGLSLUniformsForSampler(context->Sampler, context->Program);

#if 0
    printf("(%f,%f)->(%f,%f)\n",
            renderRect.MinX(), renderRect.MinY(),
            renderRect.MaxX(), renderRect.MaxY());
#endif
  
    glActiveTextureARB(GL_TEXTURE0);

    glBegin(GL_QUADS);
    glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MinY());
    glVertex2f(renderRect.MinX(), renderRect.MinY());
    glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MaxY());
    glVertex2f(renderRect.MinX(), renderRect.MaxY());
    glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MaxY());
    glVertex2f(renderRect.MaxX(), renderRect.MaxY());
    glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MinY());
    glVertex2f(renderRect.MaxX(), renderRect.MinY());
    glEnd();
}

} } // namespace Firtree::GLSL

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
