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
/// \file render-to-texture.h Provide an OpenGL context capable of rendering
/// to a texture.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_RENDER_TO_TEXTURE_H
#define FIRTREE_RENDER_TO_TEXTURE_H
//=============================================================================

#include <firtree/main.h>
#include <firtree/glsl-runtime.h>

namespace Firtree { namespace Internal {

//=============================================================================
/// A render-to-texture OpenGL context.
class RenderTextureContext : public Firtree::OpenGLContext
{
    protected:
        ///@{
        /// Protected constructors/destructors. Use the various Image
        /// factory functions instead.
        RenderTextureContext(uint32_t width, uint32_t height,
                Firtree::OpenGLContext* parentContext);
        ///@}

    public:
        virtual ~RenderTextureContext();

        // ====================================================================
        // CONSTRUCTION METHODS

        /// Construct a render-to-texture context. The texture will always
        /// be a RGBA floating point texture.
        ///
        /// \param width The width of the texture (in pixels).
        /// \param height The height of the texture (in pixels).
        /// \param parentContext The context which should be made current 
        /// when attempting to create the render buffers.
        static RenderTextureContext* Create(uint32_t width, uint32_t height,
                Firtree::OpenGLContext* parentContext);

        // ====================================================================
        // CONST METHODS
        
        /// Return the size of the context (in pixels)
        Size2DU32 GetSize() const { return m_Size; }
         
        /// Return the OpenGL texture this cotnext wraps.
        unsigned int GetOpenGLTexture() const { return m_OpenGLTextureName; }

        // ====================================================================
        // MUTATING METHODS

        /// Overridden from OpenGLContext
        ///@{
        virtual void Begin();
        virtual void End();
        ///@}

    protected:
        Firtree::OpenGLContext*     m_ParentContext;
        Size2DU32                   m_Size;

        int32_t                    m_OpenGLTextureName;
        int32_t                    m_OpenGLFrameBufferName;

        int32_t                    m_PreviousOpenGLFrameBufferName;
};

} }

//=============================================================================
#endif // FIRTREE_RENDER_TO_TEXTURE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

