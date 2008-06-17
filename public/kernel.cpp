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
/// \file kernel.cpp This file implements the FIRTREE kernel interface.
//=============================================================================

#include <float.h>
#include <public/include/main.h>
#include <public/include/kernel.h>

#include <compiler/include/compiler.h>


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
    ,   m_BaseType(NumericParameter::TypeFloat)
    ,   m_Size(0)
{
}

//=============================================================================
NumericParameter::~NumericParameter()
{
}

//=============================================================================
void NumericParameter::AssignFrom(const NumericParameter& p)
{
    // Can just memcpy a numeric type.
    memcpy(this, &p, sizeof(NumericParameter));
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
void Kernel::SetValueForKey(float value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const float* value, int count, const char* key)
{
    NumericParameter* p = (NumericParameter*)(NumericParameter::Create());

    p->SetSize(count);
    p->SetBaseType(NumericParameter::TypeFloat);

    for(int i=0; i<count; i++) { p->SetFloatValue(value[i], i); }

    this->SetValueForKey(p, key);

    p->Release();
}

//=============================================================================
void Kernel::SetValueForKey(int value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const int* value, int count, const char* key)
{
    NumericParameter* p = (NumericParameter*)(NumericParameter::Create());

    p->SetSize(count);
    p->SetBaseType(NumericParameter::TypeInteger);

    for(int i=0; i<count; i++) { p->SetIntValue(value[i], i); }

    this->SetValueForKey(p, key);

    p->Release();
}

//=============================================================================
void Kernel::SetValueForKey(bool value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(const bool* value, int count, const char* key)
{
    NumericParameter* p = (NumericParameter*)(NumericParameter::Create());

    p->SetSize(count);
    p->SetBaseType(NumericParameter::TypeBool);

    for(int i=0; i<count; i++) { p->SetBoolValue(value[i], i); }

    this->SetValueForKey(p, key);

    p->Release();
}

//=============================================================================
SamplerParameter::SamplerParameter()
    :   Parameter()
    ,   m_Transform(AffineTransform::Identity())
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

const Rect2D SamplerParameter::GetExtent() const 
{
    return Rect2D(-0.5f*FLT_MAX, -0.5f*FLT_MAX, FLT_MAX, FLT_MAX);
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
