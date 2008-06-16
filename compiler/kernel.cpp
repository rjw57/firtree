
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

#include <float.h>

namespace Firtree {

//=============================================================================
Parameter::Parameter()
    :   ReferenceCounted()
{
}

//=============================================================================
Parameter::Parameter(const Parameter& p)
{
    Parameter();
}

//=============================================================================
Parameter::~Parameter()
{
}

//=============================================================================
NumericParameter::NumericParameter()
    :   Parameter()
    ,   m_BaseType(NumericParameter::Float)
    ,   m_Size(0)
{
}

//=============================================================================
NumericParameter::~NumericParameter()
{
}

//=============================================================================
Parameter* NumericParameter::Create()
{
    return new NumericParameter();
}

//=============================================================================
Kernel::Kernel()
    :   ReferenceCounted()
{
}

//=============================================================================
Kernel::Kernel(const char* source)
{
}

//=============================================================================
Kernel::~Kernel()
{
}

//=============================================================================
SamplerParameter::SamplerParameter()
    :   Parameter()
    ,   m_Transform(AffineTransform::Identity())
{
    m_Extent[0] = m_Extent[1] = -FLT_MAX;
    m_Extent[2] = m_Extent[3] = FLT_MAX;
}

//=============================================================================
SamplerParameter::~SamplerParameter()
{
    m_Transform->Release();
}

//=============================================================================
void SamplerParameter::SetExtent(const float* e)
{
    memcpy(m_Extent, e, 4*sizeof(float));
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

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
