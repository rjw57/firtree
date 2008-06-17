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
/// \file image-int.cpp The implementation of the internal FIRTREE image.
//=============================================================================

#include <firtree/include/main.h>
#include <firtree/include/image.h>
#include <firtree/internal/image-int.h>

#include <firtree/include/opengl.h>

#include <assert.h>

namespace Firtree { namespace Internal {

#define CHECK_GL(a) do { \
    { a ; } \
    GLenum _err = glGetError(); \
    if(_err != GL_NO_ERROR) { \
        FIRTREE_ERROR("OpenGL error executing '%s': %s", \
            #a, gluErrorString(_err)); \
    } \
} while(0)

//=============================================================================
ImageImpl::ImageImpl()
    :   Image()
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
{
}

//=============================================================================
ImageImpl::ImageImpl(const Image& inim)
    :   Image(inim)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
{
    const ImageImpl& im = dynamic_cast<const ImageImpl&>(inim);

    if((im.m_BitmapRep != NULL) && (im.m_BitmapRep->ImageBlob != NULL))
    {
        BitmapImageRep* oldRep = m_BitmapRep;

        // Shallow copy.
        m_BitmapRep = new BitmapImageRep(*(im.m_BitmapRep), false);

        if(oldRep != NULL) { delete oldRep; }
    } else {
        m_BitmapRep = NULL;
    }

    m_GLTexture = im.m_GLTexture;
}

//=============================================================================
ImageImpl::ImageImpl(const BitmapImageRep& imageRep, bool copy)
    :   Image(imageRep, copy)
    ,   m_BitmapRep(NULL)
    ,   m_GLTexture(0)
{
    if(imageRep.ImageBlob == NULL) { return; }
    if(imageRep.Stride < imageRep.Width) { return; }
    if(imageRep.Stride*imageRep.Height > imageRep.ImageBlob->GetLength()) { return; }

    m_BitmapRep = new BitmapImageRep(imageRep, copy);
}

//=============================================================================
ImageImpl::~ImageImpl()
{
    if(m_GLTexture != 0)
    {
        CHECK_GL( glDeleteTextures(1, &m_GLTexture) );
        m_GLTexture = 0;
    }

    if(m_BitmapRep != NULL)
    {
        delete m_BitmapRep;
        m_BitmapRep = NULL;
    }
}

//=============================================================================
Size2D ImageImpl::GetUnderlyingPixelSize() const
{
    // Do we have a bitmap representation?
    if((m_BitmapRep != NULL) && (m_BitmapRep->ImageBlob != NULL) &&
            (m_BitmapRep->ImageBlob->GetLength() > 0))
    {
        return Size2D(m_BitmapRep->Width, m_BitmapRep->Height);
    }

    // Do we have an OpenGL texture representation?
    if(m_GLTexture != 0)
    {
        GLint w, h;
        glBindTexture(GL_TEXTURE_2D, m_GLTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

        return Size2D(w,h);
    }

    return Size2D(0,0);
}

//=============================================================================
unsigned int ImageImpl::GetAsOpenGLTexture()
{
    // Trivial case: we already have a GL representation.
    if(m_GLTexture != 0)
    {
        return m_GLTexture;
    }

    // We have to construct one. Try the binary rep first.
    if((m_BitmapRep != NULL) && (m_BitmapRep->ImageBlob != NULL) &&
            (m_BitmapRep->ImageBlob->GetLength() > 0))
    {
        CHECK_GL( glGenTextures(1, &m_GLTexture) );
        CHECK_GL( glBindTexture(GL_TEXTURE_2D, m_GLTexture) );

        assert(m_BitmapRep->Stride == m_BitmapRep->Width*4);
        CHECK_GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                    m_BitmapRep->Width, m_BitmapRep->Height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    m_BitmapRep->ImageBlob->GetBytes()) );

        return m_GLTexture;
    }
    
    // If we get here, we have no way of constructing the texture.
    // Return 0 to signal this.
    return 0;
}

} } // namespace Firtree::Internal

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
