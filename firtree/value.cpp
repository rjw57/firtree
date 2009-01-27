// FIRTREE - A generic image processing library Copyright (C) 2007, 2008
// Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 2 as published
// by the Free Software Foundation.
//
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License version 2 for more details.
//
// You should have received a copy of the GNU General Public License version
// 2 along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include <firtree/value.h>
#include <string.h>

//===========================================================================
namespace Firtree { 
//===========================================================================

//===========================================================================
Value::Value() 
    :   ReferenceCounted()
    ,   m_IntegerValue(0)
    ,   m_Type(Firtree::TySpecInvalid)
{
}

//===========================================================================
Value::~Value()
{
}

//===========================================================================
void Value::SetFloatValue(float v)
{
    m_Type = Firtree::TySpecFloat;
    m_FloatingPointComponents[0] = v;
    m_IntegerValue = static_cast<int>(v);
}

//===========================================================================
void Value::SetIntValue(int v)
{
    m_Type = Firtree::TySpecInt;
    m_FloatingPointComponents[0] = static_cast<float>(v);
    m_IntegerValue = v;
}

//===========================================================================
void Value::SetBoolValue(bool v)
{
    m_Type = Firtree::TySpecBool;
    m_FloatingPointComponents[0] = static_cast<float>(v);
    m_IntegerValue = v;
}

//===========================================================================
void Value::SetVectorValue(float x, float y)
{
    SetVectorValue(x, y, 0.f, 0.f);
    m_Type = Firtree::TySpecVec2;
}

//===========================================================================
void Value::SetVectorValue(float x, float y, float z)
{
    SetVectorValue(x, y, z, 0.f);
    m_Type = Firtree::TySpecVec3;
}

//===========================================================================
void Value::SetVectorValue(float x, float y, float z, float w)
{
    m_FloatingPointComponents[0] = x;
    m_FloatingPointComponents[1] = y;
    m_FloatingPointComponents[2] = z;
    m_FloatingPointComponents[3] = w;
    m_Type = Firtree::TySpecVec4;
    m_IntegerValue = static_cast<int>(x);
}

//===========================================================================
void Value::SetVectorValue(float* v, int num_components)
{
    switch(num_components) {
        case 2:
            SetVectorValue(v[0], v[1]);
            break;
        case 3:
            SetVectorValue(v[0], v[1], v[2]);
            break;
        case 4:
            SetVectorValue(v[0], v[1], v[2], v[3]);
            break;
        default:
            FIRTREE_ERROR("Invalid number of components for vector (%i).",
                    num_components);
            break;
    }
}

//===========================================================================
unsigned Value::GetArity() const
{
    switch(m_Type) {
        case Firtree::TySpecVec2:
            return 2;
            break;
        case Firtree::TySpecVec3:
            return 3;
            break;
        case Firtree::TySpecVec4:
            return 4;
            break;
        default:
            break;
    }

    return 1;
}

//===========================================================================
void Value::GetVectorValue(float* px, float* py, float* pz, float* pw) const
{
    if(px) { *px = m_FloatingPointComponents[0]; }
    if(py) { *py = m_FloatingPointComponents[1]; }
    if(pz) { *pz = m_FloatingPointComponents[2]; }
    if(pw) { *pw = m_FloatingPointComponents[3]; }
}

//===========================================================================
void Value::GetVectorValue(float* dest, int expected_components) const
{
    if(dest == NULL) 
    {
        FIRTREE_ERROR("GetVectorValue() passed NULL destination.");
    }
    if((expected_components < 2) || (expected_components > 4)) 
    {
        FIRTREE_ERROR("Invalid number of components for vector (%i).",
                expected_components);
    }
    memcpy(dest, m_FloatingPointComponents,
            expected_components*sizeof(float));
}

//===========================================================================
const float Value::GetVectorValue(uint32_t index) const
{
    if(index > 4)
    {
        FIRTREE_ERROR("Invalid index for vector (%i).", index);
    }
    return m_FloatingPointComponents[index];
}

//===========================================================================
Value* Value::Create()
{
    return new Value();
}

//===========================================================================
Value* Value::CreateFloatValue(float v)
{
    Value* rv = Value::Create();
    rv->SetFloatValue(v);
    return rv;
}

//===========================================================================
Value* Value::CreateIntValue(int v)
{
    Value* rv = Value::Create();
    rv->SetIntValue(v);
    return rv;
}

//===========================================================================
Value* Value::CreateBoolValue(bool v)
{
    Value* rv = Value::Create();
    rv->SetBoolValue(v);
    return rv;
}

//===========================================================================
Value* Value::CreateVectorValue(float x, float y)
{
    Value* rv = Value::Create();
    rv->SetVectorValue(x,y);
    return rv;
}

//===========================================================================
Value* Value::CreateVectorValue(float x, float y, float z)
{
    Value* rv = Value::Create();
    rv->SetVectorValue(x,y,z);
    return rv;
}

//===========================================================================
Value* Value::CreateVectorValue(float x, float y, float z, float w)
{
    Value* rv = Value::Create();
    rv->SetVectorValue(x,y,z,w);
    return rv;
}

//===========================================================================
Value* Value::CreateVectorValue(float* v, int num_components)
{
    Value* rv = Value::Create();
    rv->SetVectorValue(v, num_components);
    return rv;
}

//===========================================================================
} // namespace Firtree
//===========================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
