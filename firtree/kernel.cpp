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
#include <string.h>
#include <firtree/main.h>
#include <firtree/kernel.h>

// GLSL runtime.
#include <firtree/runtime/glsl/glsl-runtime-priv.h>

#include <compiler/include/compiler.h>

/// Kernels are implemented in Firtree by keeping an internal compiled 
/// representation for all possible runtimes. This has the advantage of
/// keeping all the kernel housekeeping within the kernel object itself
/// but is a little mucky as the kernel object needs to 'know' about all
/// Firtree runtimes.

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
Kernel::Kernel(const char* source)
    :   ReferenceCounted()
    ,   m_WrappedGLSLKernel(NULL)    
{
    // Create a GLSL-based kernel and keep a reference to it.
    m_WrappedGLSLKernel = GLSL::CompiledGLSLKernel::CreateFromSource(source);
}

//=============================================================================
Kernel::~Kernel()
{
    FIRTREE_SAFE_RELEASE(m_WrappedGLSLKernel);
}

//=============================================================================
Kernel* Kernel::CreateFromSource(const char* source)
{
    return new Kernel(source);
}

//=============================================================================
GLSL::CompiledGLSLKernel* Kernel::GetWrappedGLSLKernel() const
{
    return m_WrappedGLSLKernel;
}

//=============================================================================
const char* Kernel::GetSource() const
{
    return m_WrappedGLSLKernel->GetSource();
}

//=============================================================================
const std::map<std::string, Parameter*>& Kernel::GetParameters()
{
    return m_WrappedGLSLKernel->GetParameters();
}

//=============================================================================
void Kernel::SetValueForKey(Image* image, const char* key)
{
    SamplerParameter* sampler = 
        SamplerParameter::CreateFromImage(image);
    m_WrappedGLSLKernel->SetValueForKey(sampler, key);
    FIRTREE_SAFE_RELEASE(sampler);
}

//=============================================================================
void Kernel::SetValueForKey(Parameter* param, const char* key)
{
    m_WrappedGLSLKernel->SetValueForKey(param, key);
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
SamplerParameter::SamplerParameter(Image* srcImage)
    :   Parameter()
    ,   m_SourceImage(srcImage)
{
    FIRTREE_SAFE_RETAIN(m_SourceImage);
    
    m_WrappedGLSLSampler = GLSL::CreateSampler(m_SourceImage);
}

//=============================================================================
SamplerParameter::~SamplerParameter()
{
    FIRTREE_SAFE_RELEASE(m_WrappedGLSLSampler);
    FIRTREE_SAFE_RELEASE(m_SourceImage);
}

//=============================================================================
SamplerParameter* SamplerParameter::CreateFromImage(Image* im)
{
    return new SamplerParameter(im);
}

//=============================================================================
const Rect2D SamplerParameter::GetDomain() const
{
    return m_WrappedGLSLSampler->GetDomain();
}

//=============================================================================
const Rect2D SamplerParameter::GetExtent() const
{
    return m_WrappedGLSLSampler->GetExtent();
}

//=============================================================================
const AffineTransform* SamplerParameter::GetTransform() const
{
    return m_WrappedGLSLSampler->GetTransform();
}

//=============================================================================
GLSL::GLSLSamplerParameter* SamplerParameter::GetWrappedGLSLSampler() const
{
    return m_WrappedGLSLSampler;
}

} // namespace Firtree 

//=============================================================================
// vim:sw=4:ts=4:cindent:et
//
