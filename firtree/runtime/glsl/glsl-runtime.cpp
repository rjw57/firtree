// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
// This file implements the FIRTREE compiler utility functions.
//=============================================================================

#include "glsl-runtime-priv.h"
#include "sha1.h"

#include <float.h>
#include <string.h>
#include <assert.h>

#define FIRTREE_NO_GLX
#include <firtree/opengl.h>
#include <firtree/main.h>
#include <firtree/kernel.h>

#include <firtree/internal/image-int.h>
#include <firtree/internal/lru-cache.h>
#include <firtree/internal/pbuffer.h>

#include <compiler/include/compiler.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

extern "C" { 
#include <selog/selog.h>
}

namespace Firtree { 

namespace GLSL {

static OpenGLContext* _currentGLContext = NULL;
static GLRenderer* _currentGLRenderer = NULL;

//=============================================================================
/// Pubffer backed context
class PBufferContext : public OpenGLContext
{
    protected:
        PBufferContext(uint32_t w, uint32_t h) 
            :   OpenGLContext()
            ,   m_PBuffer(NULL)
        {
            m_PBuffer = new Internal::Pbuffer();
            bool rv = m_PBuffer->CreateContext(w,h,
                    Internal::R8G8B8A8);
            if(!rv)
            {
                FIRTREE_ERROR("Error creating off-screen pbuffer.");
                return;
            }
        }

    public:
        virtual ~PBufferContext()
        {
            delete m_PBuffer;
        }

        virtual void Begin()
        {
            OpenGLContext::Begin();
            if(GetBeginDepth() == 1) { 
                m_PBuffer->StartRendering(); 
            }
        }

        virtual void End()
        {
            if(GetBeginDepth() == 1) { 
                m_PBuffer->SwapBuffers();
                m_PBuffer->EndRendering(); 
            }
            OpenGLContext::End();
        }

    private:
        Internal::Pbuffer*  m_PBuffer;

        friend class OpenGLContext;
};

//=============================================================================
OpenGLContext* GetCurrentGLContext()
{
    return _currentGLContext;
}

//=============================================================================
GLRenderer* GetCurrentGLRenderer()
{
    return _currentGLRenderer;
}

//=============================================================================
void LinkShader(std::string& dest, GLSLSamplerParameter* sampler);

//=============================================================================
bool SetGLSLUniformsForSampler(GLSLSamplerParameter* sampler, 
        unsigned int program);

//=============================================================================
const char* GetInfoLogForSampler(GLSLSamplerParameter* sampler);

//=============================================================================
static void EnsureContext(OpenGLContext* context) 
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
CompiledGLSLKernel::CompiledGLSLKernel(const char* source)
    :   Firtree::ReferenceCounted()
    ,   m_IsCompiled(false)
{
    this->SetSource(source);
}

//=============================================================================
CompiledGLSLKernel::~CompiledGLSLKernel()
{
    ClearParameters();
}

//=============================================================================
CompiledGLSLKernel* CompiledGLSLKernel::CreateFromSource(
        const char* source)
{
    return new CompiledGLSLKernel(source); 
}

//=============================================================================
void CompiledGLSLKernel::SetSource(const char* source)
{
    // Set the source cache
    m_Source = source;
    m_IsCompiled = false;

    Compile();
}

//=============================================================================
Parameter* CompiledGLSLKernel::GetValueForKey(const char* key) const
{
    if(m_Parameters.count(key) == 0)
    {
        return NULL;
    }

    return m_Parameters.find(key)->second;
}

//=============================================================================
void CompiledGLSLKernel::Compile() 
{
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

    const char* source = m_Source.c_str();

    bool rv = c.Compile(&source, 1);
    m_InfoLog = c.GetInfoLog();
    if(!rv)
    {
        return;
    }

    m_CompiledGLSL = be.GetOutput();
    m_CompiledKernelName = be.GetOutputKernelName();

    m_ParameterNames.clear();

    GLSLBackend::Parameters& params = be.GetInputParameters();
    for(GLSLBackend::Parameters::iterator i = params.begin();
            i != params.end(); i++)
    {
        GLSLBackend::Parameter& p = *i;

        m_ParameterNames.push_back(p.humanName);

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
                            default:
                                FIRTREE_ERROR("Internal error. Unexpected parameter type.");
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
const char* CompiledGLSLKernel::GetCompiledGLSL() const {
    return m_BlockReplacedGLSL.c_str();
}

//=============================================================================
const char* CompiledGLSLKernel::GetCompiledKernelName() const {
    return m_BlockReplacedKernelName.c_str();
}

//=============================================================================
void CompiledGLSLKernel::SetBlockName(const char* blockName)
{
    // Set the block name.
    m_BlockName = blockName;

    if(!m_IsCompiled)
        return;

    UpdateBlockNameReplacedSourceCache();
}

//=============================================================================
void CompiledGLSLKernel::UpdateBlockNameReplacedSourceCache()
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

    // Update SHA1 digest
    SHA1_CTX shaCtx;
    SHA1Init(&shaCtx);
    SHA1Update(&shaCtx, (unsigned char*)(m_BlockReplacedGLSL.c_str()), 
            m_BlockReplacedGLSL.length());
    SHA1Final(m_GLSLDigest, &shaCtx);
}

//=============================================================================
void CompiledGLSLKernel::SetValueForKey(Parameter* param, const char* key)
{
    SamplerParameter* sampler = 
        dynamic_cast<SamplerParameter*>(param);
    NumericParameter* numeric = 
        dynamic_cast<NumericParameter*>(param);

    if((sampler == NULL) && (numeric == NULL))
        return;

    if(sampler != NULL)
    {
        sampler->Retain();

        if(m_Parameters.count(key) > 0)
        {
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
const char* CompiledGLSLKernel::GetUniformNameForKey(const char* key)
{
    if(m_UniformNameMap.count(key) == 0)
    {
        return NULL;
    }

    return m_UniformNameMap[key].c_str();
}

//=============================================================================
void CompiledGLSLKernel::ClearParameters()
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
Parameter* CompiledGLSLKernel::ParameterForKey(const char* key)
{
    if(m_Parameters.count(std::string(key)) > 0)
    {
        return m_Parameters[key];
    }

    return NULL;
}

//=============================================================================
NumericParameter* CompiledGLSLKernel::NumericParameterForKeyAndType(const char* key, 
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
SamplerParameter* CompiledGLSLKernel::SamplerParameterForKey(const char* key)
{
    Parameter* kp = ParameterForKey(key);

    if(kp == NULL) { return NULL; }

    SamplerParameter* sp = dynamic_cast<SamplerParameter*>(kp);
    if(sp == NULL) { return NULL; }

    return sp;
}

//=============================================================================
GLSLSamplerParameter* KernelSamplerParameter::Create(Image* im)
{
    return new KernelSamplerParameter(im);
}

//=============================================================================
GLSLSamplerParameter::GLSLSamplerParameter(Image* im)
    :   Firtree::ReferenceCounted()
    ,   m_Transform(AffineTransform::Identity())
    ,   m_SamplerIndex(-1)
    ,   m_BlockPrefix("toplevel")
    ,   m_CachedFragmentShaderObject(-1)
    ,   m_CachedProgramObject(-1)
    ,   m_RepresentedImage(im)
    ,   m_GLContext(NULL)
{
    FIRTREE_SAFE_RETAIN(m_RepresentedImage);
}

//=============================================================================
GLSLSamplerParameter::~GLSLSamplerParameter()
{
    // Clear up any cached shader programs
    if(m_CachedProgramObject > 0)
    {
        m_GLContext->Begin();
        EnsureContext(m_GLContext);
        glDeleteObjectARB(m_CachedProgramObject);
        m_CachedProgramObject = -1;
        m_GLContext->End();
    }

    if(m_CachedFragmentShaderObject > 0)
    {
        m_GLContext->Begin();
        EnsureContext(m_GLContext);
        glDeleteObjectARB(m_CachedFragmentShaderObject);
        m_CachedFragmentShaderObject = -1;
        m_GLContext->End();
    }

    FIRTREE_SAFE_RELEASE(m_RepresentedImage);
    FIRTREE_SAFE_RELEASE(m_Transform);
    FIRTREE_SAFE_RELEASE(m_GLContext);
}

//=============================================================================
void GLSLSamplerParameter::SetOpenGLContext(OpenGLContext* glContext)
{
    OpenGLContext* oldContext = m_GLContext;

    m_GLContext = glContext;
    FIRTREE_SAFE_RETAIN(m_GLContext);

    // FIXME: Should we delete the old programs when changing context?

    FIRTREE_SAFE_RELEASE(oldContext);
}

//=============================================================================
#define CHECK_GL(a) do { \
    { do { (a); } while(0); } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) {  \
        FIRTREE_ERROR("%s:%i: OpenGL Error %s", __FILE__, __LINE__,\
                gluErrorString(_err)); \
    } } while(0) 

//=============================================================================
int GLSLSamplerParameter::GetShaderProgramObject()
{
    m_GLContext->Begin();
    EnsureContext(m_GLContext);
    
    // Compute the digest of this shader
    uint8_t digest[20];
    ComputeDigest(digest);

    // Discover if we have a cached program object and, if so,
    // whether it if for a shader which matches our digest.
    bool cachedProgramValid = true;
    if(m_CachedProgramObject > 0)
    {
        if(memcmp(m_CachedShaderDigest, digest, 20) != 0)
        {
            cachedProgramValid = false;
        }
    } else {
        cachedProgramValid = false;
    }

    // If our cached progrm is valid, return it.
    if(cachedProgramValid)
    {
        m_GLContext->End();
        return m_CachedProgramObject;
    }

    // We need to (re-)create the program. Firstly release any cached 
    // programs we already have.
    if(m_CachedProgramObject > 0)
    {
        glDeleteObjectARB(m_CachedProgramObject);
        m_CachedProgramObject = -1;
    }

    if(m_CachedFragmentShaderObject > 0)
    {
        glDeleteObjectARB(m_CachedFragmentShaderObject);
        m_CachedFragmentShaderObject = -1;
    }

    std::string shaderSource;
    LinkShader(shaderSource, this);

    m_CachedFragmentShaderObject = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if(m_CachedFragmentShaderObject == 0)
    {
        fprintf(stderr, "Error creating shader object.\n");
        m_GLContext->End();
        return 0;
    }

    const char* pSrc = shaderSource.c_str();
    // printf("%s", pSrc);
    CHECK_GL( glShaderSourceARB(m_CachedFragmentShaderObject, 1, &pSrc, NULL) );
    CHECK_GL( glCompileShaderARB(m_CachedFragmentShaderObject) );

    GLint status = 0;
    CHECK_GL( glGetObjectParameterivARB(m_CachedFragmentShaderObject,
                GL_OBJECT_COMPILE_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK_GL( glGetObjectParameterivARB(m_CachedFragmentShaderObject,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK_GL( glGetInfoLogARB(m_CachedFragmentShaderObject,
                    logLen, &logLen, log) );
        FIRTREE_ERROR("Error compiling shader: %s\nSource: %s\n", 
                log, pSrc);
        free(log);
        m_GLContext->End();
        return 0;
    }

    CHECK_GL( m_CachedProgramObject = glCreateProgramObjectARB() );
    CHECK_GL( glAttachObjectARB(m_CachedProgramObject, 
                m_CachedFragmentShaderObject) );
    CHECK_GL( glLinkProgramARB(m_CachedProgramObject) );
    CHECK_GL( glGetObjectParameterivARB(m_CachedProgramObject,
                GL_OBJECT_LINK_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK_GL( glGetObjectParameterivARB(m_CachedProgramObject,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK_GL( glGetInfoLogARB(m_CachedProgramObject, logLen, &logLen, log) );
        FIRTREE_ERROR("Error linking shader: %s\n", log);
        free(log);
        m_GLContext->End();
        return 0;
    }

    // Copy the shader's digest into our cache
    memcpy(m_CachedShaderDigest, digest, 20);

    m_GLContext->End();

    return m_CachedProgramObject;
}

//=============================================================================
void GLSLSamplerParameter::SetTransform(const AffineTransform* f)
{
    if(f == NULL)
        return;

    AffineTransform* oldTrans = m_Transform;
    m_Transform = f->Copy();
    if(oldTrans != NULL) { oldTrans->Release(); }
}

//=============================================================================
const Rect2D GLSLSamplerParameter::GetExtent() const 
{
    if(m_RepresentedImage == NULL)
        return Rect2D::MakeInfinite();

    return m_RepresentedImage->GetExtent();
}

//=============================================================================
const Rect2D GLSLSamplerParameter::GetDomain() const 
{
    return Rect2D::MakeInfinite();
}

//=============================================================================
KernelSamplerParameter::KernelSamplerParameter(Image* im)
    :   GLSLSamplerParameter(im)
{
    Internal::KernelImageImpl* imImpl = 
        dynamic_cast<Internal::KernelImageImpl*>(im);
    if(imImpl == NULL) { return; }

    Firtree::Kernel* k = imImpl->GetKernel();
    CompiledGLSLKernel* gk = k->GetWrappedGLSLKernel();
    if(gk == NULL) { return; }
    
    // NB underlyingTransform must be released
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
        CompiledGLSLKernel* kernel)
{
    static char idxStr[255]; 
    std::string tempStr;
   
    const std::map<std::string, Parameter*>& params = kernel->GetParameters();

    dest += "vec4 __builtin_sample_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec4 result = vec4(0,0,0,0);\n";
    
    // Create a vector of the kernel sampler parameters.
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
        SamplerParameter *pSP = samplerParams.front();
        GLSLSamplerParameter *pGSP = 
            GLSLSamplerParameter::ExtractFrom(pSP);
        pGSP->BuildSampleGLSL(tempStr, "samplerCoord", "result");
        dest += tempStr;
    } else {
        for(std::vector<SamplerParameter*>::const_iterator i = samplerParams.begin();
                i != samplerParams.end(); i++)
        {
            SamplerParameter *pSP = *i;
            GLSLSamplerParameter *pGSP = 
                GLSLSamplerParameter::ExtractFrom(pSP);
            if(pGSP->GetSamplerIndex() != -1)
            {
                snprintf(idxStr, 255, "%i", pGSP->GetSamplerIndex());
                dest += "if(sampler == ";
                dest += idxStr;
                dest += ") {";
                pGSP->BuildSampleGLSL(tempStr, "samplerCoord", "result");
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
            GLSLSamplerParameter *pGSP =
                GLSLSamplerParameter::ExtractFrom(pSP);
            if(pGSP->GetSamplerIndex() != -1)
            {
                AffineTransform* invTrans = pSP->GetTransform()->Copy();
                invTrans->Invert();
                const AffineTransformStruct& transform =
                    invTrans->GetTransformStruct();
                if(!invTrans->IsIdentity())
                {
                    snprintf(idxStr, 255, "%i", pGSP->GetSamplerIndex());
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
        const Rect2D& samplerExtent = pSP->GetExtent();

        Rect2D extent = samplerExtent;
        if(Rect2D::IsInfinite(samplerExtent))
        {
            extent = Rect2D(-0.5*FLT_MAX,-0.5*FLT_MAX,FLT_MAX,FLT_MAX);
        }

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
            GLSLSamplerParameter *pGSP = 
                GLSLSamplerParameter::ExtractFrom(pSP);
            if(pGSP->GetSamplerIndex() != -1)
            {
                const Rect2D& samplerExtent = pSP->GetExtent();

                Rect2D extent = samplerExtent;
                if(Rect2D::IsInfinite(samplerExtent))
                {
                    extent = Rect2D(-0.5*FLT_MAX,-0.5*FLT_MAX,FLT_MAX,FLT_MAX);
                }

                snprintf(idxStr, 255, "%i", pGSP->GetSamplerIndex());
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
    }

    dest += "  return retVal;\n";
    dest += "}\n";
}

//=============================================================================
void KernelSamplerParameter::ComputeDigest(uint8_t digest[20])
{
    // Initialise the SHA-1 digest generation.
    SHA1_CTX shaCtx;
    SHA1Init(&shaCtx);

    // Add the 'this' pointer to the SHA. Used as a measure
    // of uniqueness between two shader objects. Also means that
    // digests are unique even if the sampler is not valid.
    void* self = (void*)(this);
    SHA1Update(&shaCtx, (uint8_t*)(&self), sizeof(void*));

    // If the sampler is invalid, stop here
    if(!IsValid())
    {
        SHA1Final(digest, &shaCtx);
        return;
    }

    // Add in the digest of the kernel. 
    SHA1Update(&shaCtx, (uint8_t*)(m_Kernel->GetCompiledGLSLDigest()), 20);

    // For each parameter, add in it's digest if it is a sampler.
    const std::map<std::string, Parameter*>& kParams = m_Kernel->GetParameters();
    for(std::map<std::string, Parameter*>::const_iterator i = kParams.begin();
            i != kParams.end(); i++)
    {
        Parameter* pParam = (*i).second;
        SamplerParameter* pSampParam = dynamic_cast<SamplerParameter*>(pParam);

        // Is this a sampler?
        if(pSampParam == NULL) 
            continue;

        // Extract the GLSLSamplerParameter.
        GLSLSamplerParameter* pGLSLSampParam = 
            GLSLSamplerParameter::ExtractFrom(pSampParam);
        if(pGLSLSampParam == NULL)
            continue;

        uint8_t samplerDigest[20];

        // Add in this sampler's digest.
        pGLSLSampParam->ComputeDigest(samplerDigest);
        SHA1Update(&shaCtx, samplerDigest, 20);
    }

    // Add in the current transform and extent.
    Rect2D extent = GetExtent();
    SHA1Update(&shaCtx, (uint8_t*)(&extent), sizeof(Rect2D));
    const AffineTransformStruct& trans = GetTransform()->GetTransformStruct();
    SHA1Update(&shaCtx, (uint8_t*)(&trans), sizeof(AffineTransformStruct));

    // Finalise the SHA-1 digest.
    SHA1Final(digest, &shaCtx);
}

//=============================================================================
void LinkShader(std::string& dest, GLSLSamplerParameter* sampler)
{
    dest = "";

    if(sampler == NULL)
    {
        return;
    }

    // Form a vector of all child samplers for the target.
    std::vector<SamplerParameter*> children;
    KernelSamplerParameter* ksp = dynamic_cast<KernelSamplerParameter*>(sampler);
    if(ksp != NULL)
    {
        ksp->AddChildSamplersToVector(children);
    }

    // For each child sampler, assign a sampler index and
    // (if necessary) a GL texture unit.
    int samplerIdx = 0;
    int textureIdx = 0;
    for(unsigned int i=0; i<children.size(); i++)
    {
        GLSLSamplerParameter* child = 
            GLSLSamplerParameter::ExtractFrom(children[i]);
        if(child != NULL)
        {
            child->SetSamplerIndex(samplerIdx);
            samplerIdx++;

            // Assign a GL texture unit as well, should this
            // sampler be a texture sampler.
            TextureSamplerParameter* tsp =
                dynamic_cast<TextureSamplerParameter*>(child);
            if(tsp != NULL)
            {
                tsp->SetGLTextureUnit(textureIdx);
                textureIdx++;
            }
        }
    }

    // We can now start building the GLSL source.
    // Firstly, add the top-level builtin functions to the GLSL.
    dest += 
        "vec2 __builtin_sincos(float a) { return vec2(sin(a),cos(a)); }\n"
        "vec2 __builtin_cossin(float a) { return vec2(cos(a),sin(a)); }\n"
        // FIXME: Add rest of functions.
        ;

    // Get the main GLSL body of the target sampler. 
    std::string mainBody;
    sampler->BuildTopLevelGLSL(mainBody);

    // Append the shader body it to the output.
    dest += mainBody;

    // Write the sampler functions for each child
    for(unsigned int i=0; i<children.size(); i++)
    {
        GLSLSamplerParameter* gsp =
            GLSLSamplerParameter::ExtractFrom(children[i]);
        KernelSamplerParameter* child = 
            dynamic_cast<KernelSamplerParameter*>(gsp);

        if(child != NULL)
        {
            // Each child gets it's own sampler function.
            WriteSamplerFunctionsForKernel(dest, child->GetKernel());
        }
    }

    // Should the target be a sampler, write the sample functions for
    // it.
    if(ksp != NULL)
    {
        WriteSamplerFunctionsForKernel(dest, ksp->GetKernel());
    }
    
    // Now build the main() function.
    dest += "void main() {\n"
        "vec3 inCoord = vec3(gl_TexCoord[0].xy, 1.0);\n";

    AffineTransform* invTrans = sampler->GetTransform()->Copy();
    invTrans->Invert();
    if(!invTrans->IsIdentity())
    {
        static char countStr[255]; 
        const AffineTransformStruct& transform =
            invTrans->GetTransformStruct();
        snprintf(countStr, 255, "vec3 row1 = vec3(%f,%f,%f);\n", 
                transform.m11, transform.m12, transform.tX);
        dest += countStr;
        snprintf(countStr, 255, "vec3 row2 = vec3(%f,%f,%f);\n", 
                transform.m21, transform.m22, transform.tY);
        dest += countStr;
        dest += "vec2 destCoord = vec2(dot(inCoord, row1), dot(inCoord, row2));\n";
    } else {
        dest += "vec2 destCoord = inCoord.xy;\n";
    }

    invTrans->Release();

    std::string tempStr;
    sampler->BuildSampleGLSL(tempStr, "destCoord", "gl_FragColor");
    dest += tempStr;
    dest += "\n}\n";
}

//=============================================================================
bool KernelSamplerParameter::BuildTopLevelGLSL(std::string& dest)
{
    m_Kernel->SetBlockName(GetBlockPrefix());

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
            i != kernelParams.end(); i++)
    {
        if((*i).second != NULL)
        {
            SamplerParameter* sp = 
                dynamic_cast<SamplerParameter*>((*i).second);
            if(sp != NULL)
            {
                GLSLSamplerParameter* gsp =
                    GLSLSamplerParameter::ExtractFrom(sp);

                std::string prefix(GetBlockPrefix());
                prefix += "_";
                prefix += (*i).first;

                gsp->SetBlockPrefix(prefix.c_str());
                std::string samplerGLSL;
                gsp->BuildTopLevelGLSL(samplerGLSL);
                dest += samplerGLSL;

                KernelSamplerParameter* ksp = 
                    dynamic_cast<KernelSamplerParameter*>(gsp);
                if((ksp != NULL) && (!ksp->IsValid()))
                {
                    // HACK!
                    fprintf(stderr, "Compilation failed: %s\n", 
                            ksp->GetKernel()->GetInfoLog());
                    return false;
                }
            }
        }
    }

    dest += m_Kernel->GetCompiledGLSL();

    return IsValid();
}

//=============================================================================
bool KernelSamplerParameter::IsValid() const
{
    return (m_Kernel != NULL) && (m_Kernel->GetIsCompiled()); 
}

//=============================================================================
void KernelSamplerParameter::AddChildSamplersToVector(
        std::vector<SamplerParameter*>& sampVec)
{
    if(!IsValid())
        return;

    const std::map<std::string, Parameter*>& kernelParams = 
        m_Kernel->GetParameters();

    for(std::map<std::string, Parameter*>::const_iterator i=kernelParams.begin();
            i != kernelParams.end(); i++)
    {
        // FIRTREE_DEBUG("Parameter: %s = %p", (*i).first.c_str(), (*i).second);
        if((*i).second != NULL)
        {
            SamplerParameter* sp = 
                dynamic_cast<SamplerParameter*>((*i).second);
            if(sp == NULL)
                continue;

            GLSLSamplerParameter* glslsp = 
                GLSLSamplerParameter::ExtractFrom(sp);

            KernelSamplerParameter* ksp = 
                dynamic_cast<KernelSamplerParameter*>(glslsp);
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
    GetOpenGLContext()->Begin();
    EnsureContext(GetOpenGLContext());

    const std::map<std::string, Parameter*>& params = m_Kernel->GetParameters();

    std::string uniPrefix = GetBlockPrefix();
    uniPrefix += "_params.";

    // Setup any sampler parameters.
    std::vector<SamplerParameter*> children;
    AddChildSamplersToVector(children);
    for(unsigned int i=0; i<children.size(); i++)
    {
        SamplerParameter* child = children[i];
        GLSLSamplerParameter* glslChild =
            GLSLSamplerParameter::ExtractFrom(child);

        // Set all the child's uniforms and GL context.
        glslChild->SetOpenGLContext(GetOpenGLContext());
        glslChild->SetGLSLUniforms(program);
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
            GLSLSamplerParameter* gsp = 
                GLSLSamplerParameter::ExtractFrom(sp);
            glUniform1iARB(uniformLoc, gsp->GetSamplerIndex());
            err = glGetError();
            if(err != GL_NO_ERROR)
            {
                FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
            }
        } else {
            FIRTREE_ERROR("Unknown kernel parameter type.");
        }
    }

    GetOpenGLContext()->End();
}

//=============================================================================
TextureSamplerParameter::TextureSamplerParameter(Image* im)
    :   GLSLSamplerParameter(im)
    ,   m_TextureUnit(0)
{
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);

    if(imImpl == NULL) { return; }

    m_Image = imImpl;
    m_Image->Retain();

    Size2D underlyingSize = m_Image->GetUnderlyingPixelSize();

    // NB underlyingTransform must be released.
    AffineTransform* underlyingTransform = 
        m_Image->GetTransformFromUnderlyingImage();

    m_Domain = Rect2D(0.f, 0.f, 1.f, 1.f);

    // The texture has co-ordinates in the rannge (0,1]. Re-scale
    // to be pixel-based co-ordinates.
    AffineTransform* t = GetTransform()->Copy();
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
void TextureSamplerParameter::ComputeDigest(uint8_t digest[20])
{
    // Initialise the SHA-1 digest generation.
    SHA1_CTX shaCtx;
    SHA1Init(&shaCtx);

    // Add the 'this' pointer to the SHA. Used as a measure
    // of uniqueness between two shader objects. Also means that
    // digests are unique even if the sampler is not valid.
    void* self = (void*)(this);
    SHA1Update(&shaCtx, (uint8_t*)(&self), sizeof(void*));

    // Add in the current transform and extent.
    Rect2D extent = GetExtent();
    SHA1Update(&shaCtx, (uint8_t*)(&extent), sizeof(Rect2D));
    const AffineTransformStruct& trans = GetTransform()->GetTransformStruct();
    SHA1Update(&shaCtx, (uint8_t*)(&trans), sizeof(AffineTransformStruct));

    // Finalise the SHA-1 digest.
    SHA1Final(digest, &shaCtx);
    
#if 0
    printf("%p: ", this);
    for(int i=0; i<20; i++)
    {
        printf("%02X", digest[i]);
    }
    printf("\n");
#endif
}

//=============================================================================
GLSLSamplerParameter* TextureSamplerParameter::Create(Image* im)
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

    GetOpenGLContext()->Begin();
    EnsureContext(GetOpenGLContext());

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
    GetOpenGLContext()->End();
}

//=============================================================================
unsigned int TextureSamplerParameter::GetGLTextureObject() const
{
    if(m_Image == NULL)
    {
        return 0;
    }

    return m_Image->GetAsOpenGLTexture(GetOpenGLContext());
}

//=============================================================================
GLSLSamplerParameter* CreateSampler(Image* im)
{
    Internal::KernelImageImpl* imKernelImpl = 
        dynamic_cast<Internal::KernelImageImpl*>(im);
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);
    if(imImpl == NULL) { return NULL; }

    if(imKernelImpl != NULL)
    {
        return CreateKernelSampler(imKernelImpl);
    }

    return CreateTextureSampler(im);
}

//=============================================================================
GLSLSamplerParameter* CreateTextureSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    GLSLSamplerParameter* rv = TextureSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetTransform()->Copy();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    tc->Release();
    return rv;
}

//=============================================================================
GLSLSamplerParameter* CreateKernelSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    GLSLSamplerParameter* rv = KernelSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetTransform()->Copy();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    tc->Release();
    return rv;
}

//=============================================================================
GLSLSamplerParameter* CreateTextureSampler(Image* im)
{
    AffineTransform* t = AffineTransform::Identity();
    GLSLSamplerParameter* rv =
        CreateTextureSamplerWithTransform(im, t);
    FIRTREE_SAFE_RELEASE(t);
    return rv;
}

//=============================================================================
GLSLSamplerParameter* CreateKernelSampler(Image* im)
{
    return KernelSamplerParameter::Create(im);
}

//=============================================================================
const char* GetInfoLogForSampler(GLSLSamplerParameter* sampler)
{
    GLSL::KernelSamplerParameter* s = 
        dynamic_cast<GLSL::KernelSamplerParameter*>(sampler);
    if(s == NULL)
    {
        return NULL;
    }

    return s->GetKernel()->GetInfoLog();
}

} } // namespace Firtree::GLSL

//=============================================================================
// Public-facing parts of runtime.

namespace Firtree {

//=============================================================================
GLRenderer::GLRenderer(OpenGLContext* glContext)
    :   ReferenceCounted()
    ,   m_OpenGLContext(glContext)
{
    m_SamplerCache = new Internal::LRUCache<Image*>(8, 500);
    FIRTREE_SAFE_RETAIN(m_OpenGLContext);
}

//=============================================================================
GLRenderer::~GLRenderer()
{
    CollectGarbage();
    delete m_SamplerCache;
    FIRTREE_SAFE_RELEASE(m_OpenGLContext);
}

//=============================================================================
GLRenderer* GLRenderer::Create(OpenGLContext* glContext)
{
    if(glContext == NULL)
    {
        glContext = OpenGLContext::CreateNullContext();
        GLRenderer* rv = new GLRenderer(glContext);
        FIRTREE_SAFE_RELEASE(glContext);
        return rv;
    } 

    return new GLRenderer(glContext);
}

//=============================================================================
void GLRenderer::CollectGarbage()
{
    m_SamplerCache->Purge();
}

//=============================================================================
void GLRenderer::RenderWithOrigin(Image* image, 
        const Point2D& origin)
{
    // Firstly, extract the image size and make sure it isn't infinite
    // in extent.
    Rect2D extent = image->GetExtent();

    if(Rect2D::IsInfinite(extent))
    {
        FIRTREE_ERROR("An infinite extent image has no origin.");
        return;
    }

    Point2D renderPoint = Point2D(origin.X + extent.Origin.X,
            origin.Y + extent.Origin.Y);
    RenderAtPoint(image, renderPoint, extent);
}

//=============================================================================
void GLRenderer::RenderAtPoint(Image* image, const Point2D& location,
        const Rect2D& srcRect)
{
    RenderInRect(image, Rect2D(location, srcRect.Size), srcRect);
}

//=============================================================================
BitmapImageRep* GLRenderer::CreateBitmapImageRepFromImage(Image* image)
{
    m_OpenGLContext->Begin();
    BitmapImageRep* rv = 
        dynamic_cast<Internal::ImageImpl*>(image)->GetAsBitmapImageRep();
    m_OpenGLContext->End();

    FIRTREE_SAFE_RETAIN(rv);
    return rv;
}

//=============================================================================
bool GLRenderer::WriteImageToFile(Image* image, const char* pFileName)
{
    BitmapImageRep* bir = CreateBitmapImageRepFromImage(image);
    if(bir == NULL)
    {
        return false;
    }

    bool rv = bir->WriteToFile(pFileName);
    FIRTREE_SAFE_RELEASE(bir);

    return rv;
}

//=============================================================================
void GLRenderer::Clear(float r, float g, float b, float a)
{
    if(m_OpenGLContext != NULL)
    {
        m_OpenGLContext->Begin();
    }

    CHECK_GL( glClearColor(r,g,b,a) );
    CHECK_GL( glClear(GL_COLOR_BUFFER_BIT) );

    if(m_OpenGLContext != NULL)
    {
        m_OpenGLContext->End();
    }
}

//=============================================================================
void GLRenderer::RenderInRect(Image* image, const Rect2D& destRect, 
        const Rect2D& srcRect)
{
#if 0
    FIRTREE_DEBUG("RenderInRect:");
    FIRTREE_DEBUG(" dst: %f,%f+%f+%f.", destRect.Origin.X, destRect.Origin.Y,
            destRect.Size.Width, destRect.Size.Height);
    FIRTREE_DEBUG(" src: %f,%f+%f+%f.", srcRect.Origin.X, srcRect.Origin.Y,
            srcRect.Size.Width, srcRect.Size.Height);
#endif

    if(m_OpenGLContext == NULL)
    {
        FIRTREE_ERROR("Renderer told to use NULL context.");
        return;
    }

    m_OpenGLContext->Begin();

    GLRenderer* oldRenderer = GLSL::GetCurrentGLRenderer();

    GLSL::_currentGLRenderer = this;

    CHECK_GL( glDisable(GL_DEPTH_TEST) );

    SamplerParameter* sampler;

    // Do we have a sampler for this image?
    if(m_SamplerCache->Contains(image))
    {
        sampler = dynamic_cast<SamplerParameter*>
            (m_SamplerCache->GetEntryForKey(image));
    } else {
        sampler = SamplerParameter::CreateFromImage(image);
        m_SamplerCache->SetEntryForKey(sampler, image);
        sampler->Release();
    }

    GLenum err;
    if(sampler == NULL) { 
        m_OpenGLContext->End();
        GLSL::_currentGLRenderer = oldRenderer;
        return; 
    }

    GLSL::GLSLSamplerParameter* glslSampler =
        GLSL::GLSLSamplerParameter::ExtractFrom(sampler);
    if(glslSampler == NULL) { 
        m_OpenGLContext->End();
        GLSL::_currentGLRenderer = oldRenderer;
        return; 
    }

    glslSampler->SetOpenGLContext(m_OpenGLContext);

    float vp[4];
    CHECK_GL( glGetFloatv(GL_VIEWPORT, vp) );

    Rect2D viewportRect = Rect2D(vp[0], vp[1], vp[2], vp[3]);

#if 0
    uint8_t digest[20];
    glslSampler->ComputeDigest(digest);
    for(int i=0; i<20; i++)
    {
        printf("%02X", digest[i]);
    }
    printf("\n");
#endif

    Rect2D clipSrcRect = srcRect;
    Rect2D renderRect = destRect;

    // Firstly clip srcRect by extent
    Rect2D extent = sampler->GetExtent();

    // if extent is non-infinite, clip
    if(!Rect2D::IsInfinite(extent))
    {
        AffineTransform* srcToDestTrans = Rect2D::ComputeTransform(srcRect, destRect);

        clipSrcRect = Rect2D::Intersect(srcRect, extent);

        // Transform clipped source to destination
        renderRect = Rect2D::Transform(clipSrcRect, srcToDestTrans);

        // Clip the render rectangle by the viewport
        renderRect = Rect2D::Intersect(renderRect, viewportRect);

        // Transform clipped destination rect back to source
        srcToDestTrans->Invert();
        clipSrcRect = Rect2D::Transform(renderRect, srcToDestTrans);

        srcToDestTrans->Release();

        // Do nothing if we've clipped everything away.
        if(Rect2D::IsZero(renderRect))
        {
            m_OpenGLContext->End();
            GLSL::_currentGLRenderer = oldRenderer;
            return;
        }
    }

#if 0
    FIRTREE_DEBUG(" clip dst: %f,%f+%f+%f.", renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);
    FIRTREE_DEBUG(" clip src: %f,%f+%f+%f.", clipSrcRect.Origin.X, clipSrcRect.Origin.Y,
            clipSrcRect.Size.Width, clipSrcRect.Size.Height);
#endif

    CHECK_GL( glDisable(GL_DEPTH_TEST) );
    CHECK_GL( glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) );      
    // This causes a crash on OSX. No idea why :(
    //  `-> CHECK_GL( glBlendEquation(GL_FUNC_ADD) );
    CHECK_GL( glEnable(GL_BLEND) );

    int program = glslSampler->GetShaderProgramObject();

    if(program > 0)
    {
        CHECK_GL( glUseProgramObjectARB(program) );
        if((err = glGetError()) != GL_NO_ERROR)
        {
            m_OpenGLContext->End();
            GLSL::_currentGLRenderer = oldRenderer;
            FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
            return;
        }
    }

    glslSampler->SetGLSLUniforms(program);

#if 0
    printf("(%f,%f)->(%f,%f)\n",
            renderRect.MinX(), renderRect.MinY(),
            renderRect.MaxX(), renderRect.MaxY());
#endif
  
    CHECK_GL( glActiveTextureARB(GL_TEXTURE0) );

    CHECK_GL( glMatrixMode(GL_PROJECTION) );
    CHECK_GL( glPushMatrix() );
    CHECK_GL( glLoadIdentity() );
    CHECK_GL( glOrtho(vp[0],vp[0]+vp[2],vp[1],vp[1]+vp[3],-1.0,1.0) );

    CHECK_GL( glMatrixMode(GL_MODELVIEW) );
    CHECK_GL( glPushMatrix() );
    CHECK_GL( glLoadIdentity() );

#if 1
    glBegin(GL_QUADS);
    glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MinY());
    glVertex2f(renderRect.MinX(), renderRect.MinY());
    glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MaxY());
    glVertex2f(renderRect.MinX(), renderRect.MaxY());
    glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MaxY());
    glVertex2f(renderRect.MaxX(), renderRect.MaxY());
    glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MinY());
    glVertex2f(renderRect.MaxX(), renderRect.MinY()); 
    CHECK_GL( glEnd() );
#endif

    // HACK: display render rectangle
#if 0
    CHECK_GL( glUseProgramObjectARB(0) );
    CHECK_GL( glColor3f(1,1,1) );
    glBegin(GL_LINE_STRIP);
    glVertex2f(renderRect.MinX(), renderRect.MinY());
    glVertex2f(renderRect.MinX(), renderRect.MaxY());
    glVertex2f(renderRect.MaxX(), renderRect.MaxY());
    glVertex2f(renderRect.MaxX(), renderRect.MinY());
    glVertex2f(renderRect.MinX(), renderRect.MinY());
    CHECK_GL( glEnd() );
#endif

    CHECK_GL( glMatrixMode(GL_MODELVIEW) );
    CHECK_GL( glPopMatrix() );

    CHECK_GL( glMatrixMode(GL_PROJECTION) );
    CHECK_GL( glPopMatrix() );

    m_OpenGLContext->End();
    
    GLSL::_currentGLRenderer = oldRenderer;
}

//=============================================================================
OpenGLContext::OpenGLContext()
    :   ReferenceCounted()
    ,   m_BeginDepth(0)
{
}

//=============================================================================
OpenGLContext::~OpenGLContext()
{
    if(m_BeginDepth > 0)
    {
        FIRTREE_WARNING("OpenGLContext destroyed with unbalanced calls to "
                "Begin()/End().");
    }

    while(m_ActiveTextures.size() > 0)
    {
        FIRTREE_WARNING("Deleting orphaned texture: %i.", 
                m_ActiveTextures.front());
        DeleteTexture(m_ActiveTextures.front());
    }
}

//=============================================================================
unsigned int OpenGLContext::GenTexture()
{
    unsigned int texName = 0;

    Begin();
    glGenTextures(1, reinterpret_cast<GLuint*>(&texName));
    m_ActiveTextures.push_back(texName);
    End();

    FIRTREE_TRACE(
            "OpenGLContext: 0x%x, created texture %i.", this, texName);

    return texName;
}

//=============================================================================
void OpenGLContext::DeleteTexture(unsigned int texName)
{
    FIRTREE_TRACE(
            "OpenGLContext: 0x%x, asked to delete texture %i.", this, texName);

    std::vector<unsigned int>::iterator i =
        find(m_ActiveTextures.begin(), m_ActiveTextures.end(), texName);
    if(i == m_ActiveTextures.end())
    {
        FIRTREE_WARNING("Attempt to delete texture not created by this context.");
    } else {
        m_ActiveTextures.erase(i);
    }

    Begin();
    glDeleteTextures(1, reinterpret_cast<GLuint*>(&texName));
    End();
}

//=============================================================================
void OpenGLContext::Begin()
{
    m_PriorContexts.push(GLSL::_currentGLContext);
    GLSL::_currentGLContext = this;

    m_BeginDepth++;

    //FIRTREE_TRACE("OpenGLContext: 0x%x: Begin()", this);
}

//=============================================================================
void OpenGLContext::End()
{
    //FIRTREE_TRACE("OpenGLContext: 0x%x: End()", this);

    if(m_BeginDepth == 0)
    {
        FIRTREE_WARNING("Too many OpenGLContext::End() calls.");
    } else {
        m_BeginDepth--;
    }

    GLSL::_currentGLContext = m_PriorContexts.top();
    m_PriorContexts.pop();
}

//=============================================================================
OpenGLContext* OpenGLContext::CreateNullContext()
{
    return new OpenGLContext();
}

//=============================================================================
OpenGLContext* OpenGLContext::CreateOffScreenContext(uint32_t w, uint32_t h)
{
    return new GLSL::PBufferContext(w, h);
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
