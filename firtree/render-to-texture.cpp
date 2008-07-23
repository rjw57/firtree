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
/// \file render-to-texture.cpp Implementation of the render to texture support.
//=============================================================================

#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/image.h>
#include <firtree/opengl.h>
#include <firtree/internal/render-to-texture.h>

#include <assert.h>

namespace Firtree { namespace Internal {

//=============================================================================
#define CHECK_GL(a) do { \
    { a ; } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) { \
        FIRTREE_ERROR("OpenGL error executing '%s': %s", \
            #a, gluErrorString(_err)); \
    } \
} while(0)

//=============================================================================
static void EnsureContext(OpenGLContext* context) 
{
    if(context != NULL)
    {
        context->EnsureCurrent();
    } else {
        FIRTREE_WARNING("Attempt to render to texture with no parent context.");
        assert(false);
    }

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

        if(!GLEW_EXT_framebuffer_object)
        {
            FIRTREE_ERROR("OpenGL EXT_framebuffer_object support required.");
        }

        if(!GLEW_ARB_texture_float)
        {
            FIRTREE_ERROR("OpenGL ARB_texture_float support required.");
        }
    }
}

//=============================================================================
RenderTextureContext::RenderTextureContext(uint32_t width, uint32_t height,
        Firtree::OpenGLContext* parentContext)
    :   OpenGLContext()
    ,   m_ParentContext(parentContext)
    ,   m_Size(width, height)
    ,   m_OpenGLTextureName(0)
    ,   m_OpenGLFrameBufferName(0)
    ,   m_PreviousOpenGLFrameBufferName(0)
{   
    FIRTREE_SAFE_RETAIN(m_ParentContext);

    EnsureContext(m_ParentContext);
    CHECK_GL( glGenTextures(1, &m_OpenGLTextureName) );

    CHECK_GL( glBindTexture(GL_TEXTURE_2D, m_OpenGLTextureName) );
    CHECK_GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width,
                height, 0, GL_RGBA, GL_FLOAT, NULL) );

    CHECK_GL( glGenerateMipmapEXT(GL_TEXTURE_2D) );

    GLint prevFb = 0;
    CHECK_GL( glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &prevFb) );

    CHECK_GL( glGenFramebuffersEXT(1, &m_OpenGLFrameBufferName) );
    CHECK_GL( glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_OpenGLFrameBufferName) );
    CHECK_GL( glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
                GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
                m_OpenGLTextureName, 0) );
    GLenum framebufferStatus;
    CHECK_GL( framebufferStatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) );

    if(framebufferStatus != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        FIRTREE_ERROR("Could not create frame buffer for render to "
                "texture. Status is 0x%x.", framebufferStatus);
    }

    CHECK_GL( glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, prevFb) );
}

//=============================================================================
RenderTextureContext::~RenderTextureContext()
{
    if(m_OpenGLFrameBufferName != 0)
    {
        EnsureContext(m_ParentContext);
        CHECK_GL( glDeleteFramebuffersEXT(1, &m_OpenGLTextureName) );
        m_OpenGLFrameBufferName = 0;
    }

    if(m_OpenGLTextureName != 0)
    {
        EnsureContext(m_ParentContext);
        CHECK_GL( glDeleteTextures(1, &m_OpenGLTextureName) );
        m_OpenGLTextureName = 0;
    }

    FIRTREE_SAFE_RELEASE(m_ParentContext);
}

//=============================================================================
RenderTextureContext* RenderTextureContext::Create(uint32_t width,
        uint32_t height, Firtree::OpenGLContext* parentContext)
{
    return new RenderTextureContext(width, height, parentContext);
}

//=============================================================================
void RenderTextureContext::EnsureCurrent()
{
    GLint currentFb = 0;
    CHECK_GL( glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFb) );
    if(currentFb != m_OpenGLFrameBufferName)
    {
        CHECK_GL( glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFb) );
    }
}

//=============================================================================
void RenderTextureContext::Begin()
{
    GLint currentFb = 0;
    CHECK_GL( glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFb) );
    m_PreviousOpenGLFrameBufferName = currentFb;
    EnsureCurrent();
}

//=============================================================================
void RenderTextureContext::End()
{
    CHECK_GL( glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 
                m_PreviousOpenGLFrameBufferName) );
}

} } // namespace Firtree::Internal

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
