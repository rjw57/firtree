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

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <llvm/Module.h>

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

#include <algorithm>

extern "C" { 
#include <selog/selog.h>
}

namespace Firtree { 

namespace GLSL {

static OpenGLContext* _currentGLContext = NULL;

}

//=============================================================================
OpenGLContext* GetCurrentGLContext()
{
    return GLSL::_currentGLContext;
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
