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

#include <algorithm>

// For the Apple implementation of GetProcAddress...
#if defined(FIRTREE_APPLE)
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <float.h>
#include <string.h>
#include <assert.h>

#include <firtree/opengl.h>
#include <firtree/main.h>
#include <firtree/kernel.h>

#include <firtree/internal/image-int.h>
#include <firtree/internal/lru-cache.h>
#include <firtree/internal/pbuffer.h>

// HACK:
// X11 headers define this. We don't really
// want them to since it fscks up glsl.h.
#undef Bool

#include <compiler/include/compiler.h>
#include <compiler/backends/glsl/glsl.h>
#include <compiler/backends/irdump/irdump.h>

#include <algorithm>

extern "C" { 
#include <selog/selog.h>
}

namespace Firtree { 

namespace GLSL {

static OpenGLContext* _currentGLContext = NULL;
static GLRenderer* _currentGLRenderer = NULL;

}

//=============================================================================
OpenGLContext* GetCurrentGLContext()
{
    return GLSL::_currentGLContext;
}

//=============================================================================
GLRenderer* GetCurrentGLRenderer()
{
    return GLSL::_currentGLRenderer;
}

namespace GLSL {

//=============================================================================
#define CHECK_GL(ctx,call) do { \
    { ctx->call ; } \
    GLenum _err = ctx->glGetError(); \
    if(_err != GL_NO_ERROR) { \
        FIRTREE_ERROR("OpenGL error executing '%s': %s", \
            #call, gluErrorString(_err)); \
    } \
} while(0)

//=============================================================================
#define CHECK_GL_RV(rv, ctx,call) do { \
    { rv = ctx->call ; } \
    GLenum _err = ctx->glGetError(); \
    if(_err != GL_NO_ERROR) { \
        FIRTREE_ERROR("OpenGL error executing '%s': %s", \
            #call, gluErrorString(_err)); \
    } \
} while(0)

//=============================================================================
/// Pubffer backed context
class PBufferContext : public OpenGLContext
{
    protected:
        PBufferContext(uint32_t w, uint32_t h, bool s) 
            :   OpenGLContext()
            ,   m_PBuffer(NULL)
        {
            m_PBuffer = new Internal::Pbuffer();
            bool rv = m_PBuffer->CreateContext(w,h,
                    Internal::R8G8B8A8, 
                    s ? (Internal::Pbuffer::SoftwareOnly) : (Internal::Pbuffer::NoFlags));
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

        virtual void* GetProcAddress(const char* name) const
        {
            return m_PBuffer->GetProcAddress(name);
        }

    private:
        Internal::Pbuffer*  m_PBuffer;

        friend class OpenGLContext;
};

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
    }
}

//=============================================================================
CompiledGLSLKernel::CompiledGLSLKernel(const char* source)
    :   Firtree::ReferenceCounted()
    ,   m_IsCompiled(false)
    ,   m_CompileStatus(false)
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

    m_CompileStatus = Compile();
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
bool CompiledGLSLKernel::Compile() 
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
        // FIRTREE_ERROR("Error compiling kernel:\n%s", m_InfoLog.c_str());
        return false;
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

    return true;
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

    if(sampler != NULL)
    {
        FIRTREE_SAFE_RETAIN(sampler);
    }

    if((numeric == NULL) && (m_Parameters.count(key) > 0))
    {
        FIRTREE_SAFE_RELEASE(m_Parameters[key]);
        m_Parameters[key] = NULL;
    }

    if(sampler != NULL)
    {
        m_Parameters[key] = sampler;
    } else if(numeric != NULL) {
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
    ,   m_CachedVertexShaderObject(-1)
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
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedProgramObject));
        m_CachedProgramObject = -1;
        m_GLContext->End();
    }

    if(m_CachedFragmentShaderObject > 0)
    {
        m_GLContext->Begin();
        EnsureContext(m_GLContext);
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedFragmentShaderObject));
        m_CachedFragmentShaderObject = -1;
        m_GLContext->End();
    }

    if(m_CachedVertexShaderObject > 0)
    {
        m_GLContext->Begin();
        EnsureContext(m_GLContext);
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedVertexShaderObject));
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
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedProgramObject));
        m_CachedProgramObject = -1;
    }

    if(m_CachedFragmentShaderObject > 0)
    {
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedFragmentShaderObject));
        m_CachedFragmentShaderObject = -1;
    }

    if(m_CachedVertexShaderObject > 0)
    {
        CHECK_GL(m_GLContext, glDeleteObjectARB(m_CachedVertexShaderObject));
        m_CachedVertexShaderObject = -1;
    }

    FIRTREE_DEBUG("Performance hint: (re-)compiling a GLSL shader.");

    std::string shaderSource;
    LinkShader(shaderSource, this);

    CHECK_GL_RV(m_CachedVertexShaderObject, m_GLContext, glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    if(m_CachedVertexShaderObject == 0)
    {
        FIRTREE_ERROR("Error creating vertex shader object.");
        m_GLContext->End();
        return 0;
    }

    CHECK_GL_RV(m_CachedFragmentShaderObject, m_GLContext, glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    if(m_CachedFragmentShaderObject == 0)
    {
        FIRTREE_ERROR("Error creating fragment shader object.");
        m_GLContext->End();
        return 0;
    }

    const char* pSrc = shaderSource.c_str();
    // printf("%s", pSrc);
    CHECK_GL( m_GLContext, glShaderSourceARB(m_CachedFragmentShaderObject, 1, &pSrc, NULL) );
    CHECK_GL( m_GLContext, glCompileShaderARB(m_CachedFragmentShaderObject) );

    GLint status = 0;
    CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedFragmentShaderObject,
                GL_OBJECT_COMPILE_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedFragmentShaderObject,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK_GL( m_GLContext, glGetInfoLogARB(m_CachedFragmentShaderObject,
                    logLen, &logLen, log) );
        FIRTREE_ERROR("Error compiling fragment shader: %s\nSource: %s\n", 
                log, pSrc);
        free(log);
        m_GLContext->End();
        return 0;
    }

    const char* vertexSrc = 
                "void main(void) { gl_Position = gl_Vertex; gl_TexCoord[0] = gl_MultiTexCoord0; }";
    CHECK_GL( m_GLContext, glShaderSourceARB(m_CachedVertexShaderObject, 1, &vertexSrc, NULL) );
    CHECK_GL( m_GLContext, glCompileShaderARB(m_CachedVertexShaderObject) );

    CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedVertexShaderObject,
                GL_OBJECT_COMPILE_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedVertexShaderObject,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK_GL( m_GLContext, glGetInfoLogARB(m_CachedVertexShaderObject,
                    logLen, &logLen, log) );
        FIRTREE_ERROR("Error compiling vertex shader: %s\nSource: %s\n", 
                log, pSrc);
        free(log);
        m_GLContext->End();
        return 0;
    }

    CHECK_GL_RV( m_CachedProgramObject, m_GLContext, glCreateProgramObjectARB() );
    CHECK_GL( m_GLContext, glAttachObjectARB(m_CachedProgramObject, 
                m_CachedFragmentShaderObject) );
    CHECK_GL( m_GLContext, glAttachObjectARB(m_CachedProgramObject, 
                m_CachedVertexShaderObject) );
    CHECK_GL( m_GLContext, glLinkProgramARB(m_CachedProgramObject) );
    CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedProgramObject,
                GL_OBJECT_LINK_STATUS_ARB, &status) );

    if(status != GL_TRUE)
    {
        GLint logLen = 0;
        CHECK_GL( m_GLContext, glGetObjectParameterivARB(m_CachedProgramObject,
                    GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen) );
        char* log = (char*) malloc(logLen + 1);
        CHECK_GL( m_GLContext, glGetInfoLogARB(m_CachedProgramObject, logLen, &logLen, log) );
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
    FIRTREE_SAFE_RELEASE(oldTrans);
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
    ,   m_Kernel(NULL)
{
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);
    if(imImpl == NULL) { 
        FIRTREE_ERROR("Internal Error: Passed image is invalid (%p)", im);
        return; 
    }

    Firtree::Kernel* k = imImpl->GetKernel();
    if(k == NULL) { 
        FIRTREE_ERROR("Internal Error: Attempt to create kernel sampler from "
                "image (%p) with no kernel.", im);
        return; 
    }

    CompiledGLSLKernel* gk = k->GetWrappedGLSLKernel();
    if(gk == NULL) { 
        FIRTREE_ERROR("Internal Error: Attempt to create GLSL kernel sampler "
                "from kernel (%p) with no GLSL implementation.", k);
        return;
    }
    
    // NB underlyingTransform must be released
    AffineTransform* underlyingTransform = 
        imImpl->GetTransformFromUnderlyingImage();

    m_Kernel = gk;
    FIRTREE_SAFE_RETAIN(m_Kernel);

    AffineTransform* t = GetAndOwnTransform();
    t->AppendTransform(underlyingTransform);
    SetTransform(t);
    FIRTREE_SAFE_RELEASE(t);
    FIRTREE_SAFE_RELEASE(underlyingTransform);
}

//=============================================================================
KernelSamplerParameter::~KernelSamplerParameter()
{
    FIRTREE_SAFE_RELEASE(m_Kernel);
}

//=============================================================================
static void WriteSamplerFunctionsForKernel(std::string& dest,
        CompiledGLSLKernel* kernel)
{
    static char idxStr[255]; 
    std::string tempStr;

    dest += "vec4 __builtin_sample_";
    dest += kernel->GetCompiledKernelName();
    dest += "(int sampler, vec2 samplerCoord) {\n";
    dest += "  vec4 result = vec4(0,0,0,0);\n";
   
    const std::map<std::string, Parameter*>& params = kernel->GetParameters();
    
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
        AffineTransform* invTrans = pSP->GetAndOwnTransform();
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
        FIRTREE_SAFE_RELEASE(invTrans);
    } else {
        for(std::vector<SamplerParameter*>::const_iterator i = samplerParams.begin();
                i != samplerParams.end(); i++)
        {
            SamplerParameter *pSP = *i;
            GLSLSamplerParameter *pGSP =
                GLSLSamplerParameter::ExtractFrom(pSP);
            if(pGSP->GetSamplerIndex() != -1)
            {
                AffineTransform* invTrans = pSP->GetAndOwnTransform();
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
                FIRTREE_SAFE_RELEASE(invTrans);
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
    AffineTransform* transObj = GetAndOwnTransform();
    const AffineTransformStruct& trans = transObj->GetTransformStruct();
    SHA1Update(&shaCtx, (uint8_t*)(&trans), sizeof(AffineTransformStruct));
    FIRTREE_SAFE_RELEASE(transObj);

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
        "vec4 __builtin_premultiply(vec4 a) { return vec4(a.rgb*a.a, a.a); }\n"
        "vec4 __builtin_unpremultiply(vec4 a) { "
        "     vec4 rv = a;"
        "     if(a.a != 0.0) { rv = vec4(a.rgb/a.a, a.a); } else { return vec4(0,0,0,0); } "
        "     return rv;"
        "}\n"
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

    AffineTransform* invTrans = sampler->GetAndOwnTransform();
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

    FIRTREE_SAFE_RELEASE(invTrans);

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

    OpenGLContext* ctx = GetOpenGLContext();

    const std::map<std::string, Parameter*>& params = m_Kernel->GetParameters();

    std::string uniPrefix = "input_"; 
    uniPrefix += GetBlockPrefix();
    uniPrefix += "_";

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
        GLint uniformLoc;
        CHECK_GL_RV(uniformLoc, ctx, glGetUniformLocationARB(program, paramName.c_str()));

        GLenum err = ctx->glGetError();
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
                                CHECK_GL(ctx, glUniform1fARB(uniformLoc, vec[0]));
                                break;
                            case 2:
                                CHECK_GL(ctx, glUniform2fARB(uniformLoc, vec[0], vec[1]));
                                break;
                            case 3:
                                CHECK_GL(ctx, glUniform3fARB(uniformLoc, vec[0], vec[1], vec[2]));
                                break;
                            case 4:
                                CHECK_GL(ctx, glUniform4fARB(uniformLoc, vec[0], vec[1], vec[2], vec[3]));
                                break;
                            default:
                                FIRTREE_ERROR("Parameter %s has invalid size: %i",
                                        paramName.c_str(), cp->GetSize());
                        }

                        err = ctx->glGetError();
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
                                CHECK_GL(ctx, glUniform1iARB(uniformLoc, vec[0]));
                                break;
                            case 2:
                                CHECK_GL(ctx, glUniform2iARB(uniformLoc, vec[0], vec[1]));
                                break;
                            case 3:
                                CHECK_GL(ctx, glUniform3iARB(uniformLoc, vec[0], vec[1], vec[2]));
                                break;
                            case 4:
                                CHECK_GL(ctx, glUniform4iARB(uniformLoc, vec[0], vec[1], vec[2], vec[3]));
                                break;
                            default:
                                FIRTREE_ERROR("Parameter %s has invalid size: %i",
                                        paramName.c_str(), cp->GetSize());
                        }

                        err = ctx->glGetError();
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
            CHECK_GL(ctx, glUniform1iARB(uniformLoc, gsp->GetSamplerIndex()));
            err = ctx->glGetError();
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
    FIRTREE_SAFE_RETAIN(m_Image);

    m_Domain = Rect2D(0.f, 0.f, 1.f, 1.f);

    // NB underlyingTransform must be released.
    AffineTransform* underlyingTransform = 
        m_Image->GetTransformFromUnderlyingImage();
    SetTransform(underlyingTransform);
    FIRTREE_SAFE_RELEASE(underlyingTransform);
}

//=============================================================================
TextureSamplerParameter::~TextureSamplerParameter()
{
    FIRTREE_SAFE_RELEASE(m_Image);
}

//=============================================================================
AffineTransform* TextureSamplerParameter::GetAndOwnTransform() const
{
    AffineTransform* underlyingTransform = 
        m_Image->GetTransformFromUnderlyingImage();
    Size2D underlyingSize = m_Image->GetUnderlyingPixelSize();
    AffineTransform* t = AffineTransform::Identity();

    // Undo the flip if necessary.
    if(m_Image->GetIsFlipped())
    {
        t->ScaleBy(1.0, -1.0);
        t->TranslateBy(0.0, 1.0);
    }

    // The texture has co-ordinates in the range (0,1]. Re-scale
    // to be pixel-based co-ordinates.
    t->ScaleBy(underlyingSize.Width, underlyingSize.Height);
    t->AppendTransform(underlyingTransform);
    FIRTREE_SAFE_RELEASE(underlyingTransform);

    return t;
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
    AffineTransform* transObj = GetAndOwnTransform();
    const AffineTransformStruct& trans = transObj->GetTransformStruct();
    SHA1Update(&shaCtx, (uint8_t*)(&trans), sizeof(AffineTransformStruct));
    FIRTREE_SAFE_RELEASE(transObj);

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

    OpenGLContext* ctx = GetOpenGLContext();

    std::string paramName(GetBlockPrefix());
    paramName += "_texture";

    GLint uniformLoc;
    CHECK_GL_RV(uniformLoc, ctx, glGetUniformLocationARB(program, paramName.c_str()));
    GLenum err = ctx->glGetError();
    if(err != GL_NO_ERROR)
    {
        FIRTREE_ERROR("OpenGL error: %s", gluErrorString(err));
    }

    if(uniformLoc != -1)
    {
        CHECK_GL(ctx, glActiveTextureARB(GL_TEXTURE0_ARB + GetGLTextureUnit()));
        CHECK_GL(ctx, glBindTexture(GL_TEXTURE_2D, GetGLTextureObject()));

        CHECK_GL(ctx, glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
        CHECK_GL(ctx, glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ));
        CHECK_GL(ctx, glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ));
        CHECK_GL(ctx, glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ));

        CHECK_GL(ctx, glUniform1iARB(uniformLoc, GetGLTextureUnit()));
        err = ctx->glGetError();
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
    Internal::ImageImpl* imImpl = 
        dynamic_cast<Internal::ImageImpl*>(im);
    if(imImpl == NULL) { return NULL; }

    switch(imImpl->GetPreferredRepresentation())
    {
        case Internal::ImageImpl::Kernel:
            return CreateKernelSampler(im);
            break;
        default:
            return CreateTextureSampler(im);
            break;
    }
}

//=============================================================================
GLSLSamplerParameter* CreateTextureSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    GLSLSamplerParameter* rv = TextureSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetAndOwnTransform();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    FIRTREE_SAFE_RELEASE(tc);
    return rv;
}

//=============================================================================
GLSLSamplerParameter* CreateKernelSamplerWithTransform(
        Image* im, const AffineTransform* transform)
{
    GLSLSamplerParameter* rv = KernelSamplerParameter::Create(im);
    AffineTransform* tc = rv->GetAndOwnTransform();
    tc->AppendTransform(transform);
    rv->SetTransform(tc);
    FIRTREE_SAFE_RELEASE(tc);
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
        dynamic_cast<Internal::ImageImpl*>(image)->CreateBitmapImageRep();
    m_OpenGLContext->End();

    return rv;
}

//=============================================================================
bool GLRenderer::WriteImageToFile(Image* image, const char* pFileName)
{
    BitmapImageRep* bir = CreateBitmapImageRepFromImage(image);
    if(bir == NULL)
    {
        FIRTREE_WARNING("Failed to create bitmap of image %p.", image);
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

    CHECK_GL( m_OpenGLContext, glClearColor(r,g,b,a) );
    CHECK_GL( m_OpenGLContext, glClear(GL_COLOR_BUFFER_BIT) );

    if(m_OpenGLContext != NULL)
    {
        m_OpenGLContext->End();
    }
}

//=============================================================================
void GLRenderer::RenderInRect(Image* image, const Rect2D& destRect, 
        const Rect2D& srcRect)
{
#if 1
    FIRTREE_DEBUG("RenderInRect image %p:", image);
    FIRTREE_DEBUG(" dst rect: %f,%f+%f+%f.", destRect.Origin.X, destRect.Origin.Y,
            destRect.Size.Width, destRect.Size.Height);
    FIRTREE_DEBUG(" src rect: %f,%f+%f+%f.", srcRect.Origin.X, srcRect.Origin.Y,
            srcRect.Size.Width, srcRect.Size.Height);
#endif

    if(m_OpenGLContext == NULL)
    {
        FIRTREE_ERROR("Renderer told to use NULL context.");
        return;
    }

    m_OpenGLContext->Begin();

    float vp[4];
    CHECK_GL( m_OpenGLContext, glGetFloatv(GL_VIEWPORT, vp) );

    Rect2D viewportRect = Rect2D(vp[0], vp[1], vp[2], vp[3]);

    FIRTREE_DEBUG("  vp rect: %f,%f+%f+%f.", 
            viewportRect.Origin.X, viewportRect.Origin.Y,
            viewportRect.Size.Width, viewportRect.Size.Height);

    /*
    CHECK_GL( m_OpenGLContext, glMatrixMode(GL_PROJECTION) );
    CHECK_GL( m_OpenGLContext, glLoadIdentity() );
    CHECK_GL( m_OpenGLContext, glOrtho(vp[0],vp[0]+vp[2],vp[1],vp[1]+vp[3],-1.0,1.0) );

    CHECK_GL( m_OpenGLContext, glMatrixMode(GL_MODELVIEW) );
    CHECK_GL( m_OpenGLContext, glLoadIdentity() );

    CHECK_GL( m_OpenGLContext, glMatrixMode(GL_TEXTURE) );
    CHECK_GL( m_OpenGLContext, glLoadIdentity() );
    */

    GLRenderer* oldRenderer = GetCurrentGLRenderer();

    GLSL::_currentGLRenderer = this;

    CHECK_GL( m_OpenGLContext, glDisable(GL_DEPTH_TEST) );

    SamplerParameter* sampler;

    // Do we have a sampler for this image?
    if(!m_SamplerCache->Contains(image))
    {
        sampler = SamplerParameter::CreateFromImage(image);
        m_SamplerCache->SetEntryForKey(sampler, image);
        FIRTREE_SAFE_RELEASE(sampler);
    }

    sampler = dynamic_cast<SamplerParameter*>
        (m_SamplerCache->GetEntryForKey(image));

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

        FIRTREE_SAFE_RELEASE(srcToDestTrans);

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

    CHECK_GL( m_OpenGLContext, glDisable(GL_DEPTH_TEST) );
    CHECK_GL( m_OpenGLContext, glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) );      
    // This causes a crash on OSX. No idea why :(
    //  `-> CHECK_GL( glBlendEquation(GL_FUNC_ADD) );
    CHECK_GL( m_OpenGLContext, glEnable(GL_BLEND) );

    int program = glslSampler->GetShaderProgramObject();

    if(program > 0)
    {
        CHECK_GL( m_OpenGLContext, glUseProgramObjectARB(program) );
        if((err = m_OpenGLContext->glGetError()) != GL_NO_ERROR)
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
  
    CHECK_GL( m_OpenGLContext, glActiveTextureARB(GL_TEXTURE0) );

    FIRTREE_DEBUG(" rdr rect: %f,%f+%f+%f.", 
            renderRect.Origin.X, renderRect.Origin.Y,
            renderRect.Size.Width, renderRect.Size.Height);

#if 1
    
    // We don't use CHECK_GL here because calling glGetError() between
    // glBegin()/glEnd() itself is an error.
    m_OpenGLContext->glBegin(GL_QUADS);
    m_OpenGLContext->glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MinY());
    m_OpenGLContext->glVertex2f(-1.0 + 2.0*renderRect.MinX()/vp[2],
            -1.0 + 2.0*renderRect.MinY()/vp[3]);
    m_OpenGLContext->glTexCoord2f(clipSrcRect.MinX(), clipSrcRect.MaxY());
    m_OpenGLContext->glVertex2f(-1.0 + 2.0*renderRect.MinX()/vp[2],
            -1.0 + 2.0*renderRect.MaxY()/vp[3]);
    m_OpenGLContext->glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MaxY());
    m_OpenGLContext->glVertex2f(-1.0 + 2.0*renderRect.MaxX()/vp[2],
            -1.0 + 2.0*renderRect.MaxY()/vp[3]);
    m_OpenGLContext->glTexCoord2f(clipSrcRect.MaxX(), clipSrcRect.MinY());
    m_OpenGLContext->glVertex2f(-1.0 + 2.0*renderRect.MaxX()/vp[2],
            -1.0 + 2.0*renderRect.MinY()/vp[3]); 
    CHECK_GL( m_OpenGLContext, glEnd() );
#endif

    // Stop using our shader program.
    CHECK_GL( m_OpenGLContext, glUseProgramObjectARB(0) );

    // HACK: display render rectangle
#if 0
    //CHECK_GL( m_OpenGLContext, glColor3f(1,1,0) );
    m_OpenGLContext->glBegin(GL_LINE_STRIP);
    m_OpenGLContext->glVertex2f(renderRect.MinX(), renderRect.MinY());
    m_OpenGLContext->glVertex2f(renderRect.MinX(), renderRect.MaxY());
    m_OpenGLContext->glVertex2f(renderRect.MaxX(), renderRect.MaxY());
    m_OpenGLContext->glVertex2f(renderRect.MaxX(), renderRect.MinY());
    m_OpenGLContext->glVertex2f(renderRect.MinX(), renderRect.MinY());
    CHECK_GL( m_OpenGLContext, glEnd() );
#endif

    m_OpenGLContext->End();
    
    GLSL::_currentGLRenderer = oldRenderer;
}

//=============================================================================
OpenGLContext::OpenGLContext()
    :   ReferenceCounted()
    ,   m_BeginDepth(0)
    ,   m_FilledFunctionPointerTable(false)
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
void* OpenGLContext::GetProcAddress(const char* name) const
{
    // This is the default implementation. Perticular contexts may wish
    // to override this.
#if defined(FIRTREE_UNIX) && !defined(FIRTREE_APPLE)
    return (void*)(glXGetProcAddress(reinterpret_cast<const GLubyte*>(name)));
#elif defined(FIRTREE_APPLE)
    // Taken from the Apple document 'Obtaining a Function Pointer to an
    // Arbitrary OpenGL Entry Point'.
    NSSymbol symbol;
    char *symbolName;
    symbolName = (char*)malloc (strlen (name) + 2);
    strcpy(symbolName + 1, name);
    symbolName[0] = '_';
    symbol = NULL;
    if (NSIsSymbolNameDefined (symbolName))
        symbol = NSLookupAndBindSymbol (symbolName);
    free (symbolName);
    return symbol ? NSAddressOfSymbol (symbol) : NULL; 
#else
    #error No GetProcAddress implementation for this platform.
    return NULL
#endif
}

//=============================================================================
void OpenGLContext::FillFunctionPointerTable()
{
    if(m_FilledFunctionPointerTable)
        return;

#   define FILL_ENTRY(name, type) do { \
        this->name = reinterpret_cast<type>(GetProcAddress(#name)); \
        if(this->name == NULL) { FIRTREE_WARNING("No implementation for '%s'.", #name); } \
    } while(0)

    FILL_ENTRY(glBindTexture, PFNGLBINDTEXTUREPROC);
    FILL_ENTRY(glGetTexLevelParameteriv, PFNGLGETTEXLEVELPARAMETERIVPROC);
    FILL_ENTRY(glGetTexImage, PFNGLGETTEXIMAGEPROC);
    FILL_ENTRY(glTexImage2D, PFNGLTEXIMAGE2DPROC);
    FILL_ENTRY(glGetError, PFNGLGETERRORPROC);
    FILL_ENTRY(glGetIntegerv, PFNGLGETINTEGERVPROC);
    FILL_ENTRY(glGetFloatv, PFNGLGETFLOATVPROC);
    FILL_ENTRY(glViewport, PFNGLVIEWPORTPROC);
    FILL_ENTRY(glPopMatrix, PFNGLPOPMATRIXPROC);
    FILL_ENTRY(glPushMatrix, PFNGLPUSHMATRIXPROC);
    FILL_ENTRY(glMatrixMode, PFNGLMATRIXMODEPROC);
    FILL_ENTRY(glBegin, PFNGLBEGINPROC);
    FILL_ENTRY(glEnd, PFNGLENDPROC);
    FILL_ENTRY(glClear, PFNGLCLEARPROC);
    FILL_ENTRY(glLoadIdentity, PFNGLLOADIDENTITYPROC);
    FILL_ENTRY(glClearColor, PFNGLCLEARCOLORPROC);
    FILL_ENTRY(glEnable, PFNGLENABLEPROC);
    FILL_ENTRY(glDisable, PFNGLDISABLEPROC);
    FILL_ENTRY(glVertex2f, PFNGLVERTEX2FPROC);
    FILL_ENTRY(glTexCoord2f, PFNGLTEXCOORD2FPROC);
    FILL_ENTRY(glOrtho, PFNGLORTHOPROC);
    FILL_ENTRY(glBlendFunc, PFNGLBLENDFUNCPROC);
    FILL_ENTRY(glTexParameterf, PFNGLTEXPARAMETERFPROC);
    FILL_ENTRY(glTexParameteri, PFNGLTEXPARAMETERIPROC);
    FILL_ENTRY(glDrawBuffer, PFNGLDRAWBUFFERPROC);
    FILL_ENTRY(glGenTextures, PFNGLGENTEXTURESPROC);
    FILL_ENTRY(glDeleteTextures, PFNGLDELETETEXTURESPROC);

    FILL_ENTRY(glGenerateMipmapEXT, PFNGLGENERATEMIPMAPEXTPROC);
    FILL_ENTRY(glGenFramebuffersEXT, PFNGLGENFRAMEBUFFERSEXTPROC);
    FILL_ENTRY(glDeleteFramebuffersEXT, PFNGLDELETEFRAMEBUFFERSEXTPROC);
    FILL_ENTRY(glBindFramebufferEXT, PFNGLBINDFRAMEBUFFEREXTPROC);
    FILL_ENTRY(glFramebufferTexture2DEXT, PFNGLFRAMEBUFFERTEXTURE2DEXTPROC);
    FILL_ENTRY(glCheckFramebufferStatusEXT, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC);
    FILL_ENTRY(glGenRenderbuffersEXT, PFNGLGENRENDERBUFFERSEXTPROC);
    FILL_ENTRY(glBindRenderbufferEXT, PFNGLBINDRENDERBUFFEREXTPROC);
    FILL_ENTRY(glRenderbufferStorageEXT, PFNGLRENDERBUFFERSTORAGEEXTPROC);
    FILL_ENTRY(glFramebufferRenderbufferEXT, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC);

    FILL_ENTRY(glActiveTextureARB, PFNGLACTIVETEXTUREARBPROC);
    FILL_ENTRY(glUseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
    FILL_ENTRY(glGetInfoLogARB, PFNGLGETINFOLOGARBPROC);
    FILL_ENTRY(glGetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC);
    FILL_ENTRY(glShaderSourceARB, PFNGLSHADERSOURCEARBPROC);
    FILL_ENTRY(glCompileShaderARB, PFNGLCOMPILESHADERARBPROC);
    FILL_ENTRY(glCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC);
    FILL_ENTRY(glAttachObjectARB, PFNGLATTACHOBJECTARBPROC);
    FILL_ENTRY(glLinkProgramARB, PFNGLLINKPROGRAMARBPROC);
    FILL_ENTRY(glCreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC);
    FILL_ENTRY(glGetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC);
    FILL_ENTRY(glUniform1iARB, PFNGLUNIFORM1IARBPROC);
    FILL_ENTRY(glUniform2iARB, PFNGLUNIFORM2IARBPROC);
    FILL_ENTRY(glUniform3iARB, PFNGLUNIFORM3IARBPROC);
    FILL_ENTRY(glUniform4iARB, PFNGLUNIFORM4IARBPROC);
    FILL_ENTRY(glUniform1fARB, PFNGLUNIFORM1FARBPROC);
    FILL_ENTRY(glUniform2fARB, PFNGLUNIFORM2FARBPROC);
    FILL_ENTRY(glUniform3fARB, PFNGLUNIFORM3FARBPROC);
    FILL_ENTRY(glUniform4fARB, PFNGLUNIFORM4FARBPROC);
    FILL_ENTRY(glDeleteObjectARB, PFNGLDELETEOBJECTARBPROC);

    m_FilledFunctionPointerTable = true;
}

//=============================================================================
unsigned int OpenGLContext::GenTexture()
{
    unsigned int texName = 0;

    Begin();
    CHECK_GL(this, glGenTextures(1, reinterpret_cast<GLuint*>(&texName)));
    m_ActiveTextures.push_back(texName);
    End();

    FIRTREE_DEBUG("Performance hint: creating OpenGL texture.");

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
    CHECK_GL(this, glDeleteTextures(1, reinterpret_cast<GLuint*>(&texName)));
    End();
}

//=============================================================================
void OpenGLContext::Begin()
{
    m_PriorContexts.push(GLSL::_currentGLContext);
    GLSL::_currentGLContext = this;

    m_BeginDepth++;

    FillFunctionPointerTable();

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
OpenGLContext* OpenGLContext::CreateOffScreenContext(uint32_t w, uint32_t h, 
        bool software)
{
    return new GLSL::PBufferContext(w, h, software);
}

} // namespace Firtree

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
