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
// This file specifies the interface between the 3d Labs GLSL frontend and 
// the FIRTREE compiler.
//=============================================================================

// Include the GLSL frontend interface.
#include "glslang/Include/Common.h"
#include "glslang/Include/ShHandle.h"

//=============================================================================
// COMPILATION
//=============================================================================

namespace Firtree { class Backend; }

namespace FirtreeInternal {

//=============================================================================
class FirtreeCompiler : public TCompiler 
{
    public:
        FirtreeCompiler() 
            : TCompiler(EShLangKernel, infoSink)
            , m_pBackend(NULL) { }
        virtual bool compile(TIntermNode* root);

        // FIRTREE interface
        Firtree::Backend* GetBackend() { return m_pBackend; }
        void SetBackend(Firtree::Backend* b) { m_pBackend = b; }

    protected:
        TInfoSink           infoSink;
        Firtree::Backend*   m_pBackend;
};

} // namespace FirtreeInternal

//=============================================================================
// vim:sw=4:ts=4:cindent:et
