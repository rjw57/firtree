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

#define __STDC_LIMIT_MACROS
#include <limits.h>
#include <llvm/Module.h>

#include <float.h>
#include <string.h>
#include <firtree/main.h>
#include <firtree/kernel.h>
#include <firtree/value.h>

// GLSL runtime.
#include <firtree/runtime/glsl/glsl-runtime-priv.h>

#include <compiler/include/compiler.h>

// LLVM
#include <firtree/compiler/llvm_compiled_kernel.h>
#include <firtree/linker/sampler_provider.h>

#include <sstream>

/// Kernels are implemented in Firtree by keeping an internal compiled 
/// representation for all possible runtimes. This has the advantage of
/// keeping all the kernel housekeeping within the kernel object itself
/// but is a little mucky as the kernel object needs to 'know' about all
/// Firtree runtimes.

namespace Firtree {

//=============================================================================
/// An implementation of the ExtentProvider interface that provide a 'standard'
/// algorithm.
class StandardExtendProvider : public ExtentProvider
{
    public:
        StandardExtendProvider(const char* samplerName, float deltaX, 
                float deltaY)
            :   ExtentProvider()
            ,   m_Delta(deltaX, deltaY)
        {
            // If the passed sampler name is non-NULL, initialise
            // our copy of it.
            if(samplerName != NULL)
            {
                m_SamplerName = samplerName;
            }
        }

        virtual ~StandardExtendProvider()
        {
        }

        virtual Rect2D ComputeExtentForKernel(Kernel* kernel)
        {
            if(kernel == NULL)
            {
                FIRTREE_ERROR("Passed a null kernel.");
                return Rect2D::MakeInfinite();
            }

            //FIRTREE_DEBUG("%p -> compute extent.", kernel);

            const char* parameterName = NULL;
            if(!m_SamplerName.empty())
            {
                parameterName = m_SamplerName.c_str();
            }

            // If we don't have a parameter name, find each sampler the kernel
            // uses and form a union.
            if(parameterName == NULL)
            {
                //FIRTREE_DEBUG("No specified parameter, searching.");

                Rect2D extentRect = Rect2D::MakeInfinite();
                bool foundOneSampler = false;
                const std::vector<std::string>& paramNames = 
                    kernel->GetParameterNames();

                for(std::vector<std::string>::const_iterator i = paramNames.begin();
                        i != paramNames.end(); i++)
                {
                    SamplerParameter* sp = dynamic_cast<SamplerParameter*>
                        (kernel->GetParameterForKey((*i).c_str()));
                    if(sp != NULL)
                    {
                        Rect2D samplerExtent = sp->GetExtent();
                        /*
                        FIRTREE_DEBUG("Found: '%s' rect = %f,%f+%f+%f",
                                (*i).c_str(), 
                                samplerExtent.Origin.X, samplerExtent.Origin.Y,
                                samplerExtent.Size.Width, samplerExtent.Size.Height);
                                */
                        if(foundOneSampler) {
                            extentRect = Rect2D::Union(extentRect, samplerExtent);
                        } else {
                            extentRect = samplerExtent;
                        }
                        foundOneSampler = true;
                    }
                }

                return Rect2D::Inset(extentRect, m_Delta.Width, m_Delta.Height);
            }

            //FIRTREE_DEBUG("Using specified parameter: %s", m_SamplerName.c_str());

            if(parameterName == NULL)
            {
                FIRTREE_WARNING("Could not find valid sampler parameter in kernel from "
                         "which to use extent.");
                return Rect2D::MakeInfinite();
            }

            SamplerParameter* sp = dynamic_cast<SamplerParameter*>
                    (kernel->GetParameterForKey(parameterName));

            Rect2D samplerExtent = sp->GetExtent();

            return Rect2D::Inset(samplerExtent, m_Delta.Width, m_Delta.Height);
        }

    private:
        std::string     m_SamplerName;
        Size2D          m_Delta;
};

//=============================================================================
ExtentProvider* ExtentProvider::CreateStandardExtentProvider(const char* samplerName,
        float deltaX, float deltaY)
{
    return new StandardExtendProvider(samplerName, deltaX, deltaY);
}

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
    ,   m_WrappedLLVMKernel(NULL)
    ,   m_SamplerProvider(NULL)
    ,   m_SamplerLinker(NULL)
{
    // Create a GLSL-based kernel and keep a reference to it.
    m_WrappedGLSLKernel = GLSL::CompiledGLSLKernel::CreateFromSource(source);

    // Create a LLVM-based kernel and keep a reference.
    m_WrappedLLVMKernel = LLVM::CompiledKernel::Create();
    m_WrappedLLVMKernel->Compile(&source, 1);

    std::ostringstream log_string(std::ostringstream::out);
    const char *const * log = m_WrappedLLVMKernel->GetCompileLog();
    while(*log) {
        log_string << *log << "\n";
        ++log;
    }
    m_CompileLog = log_string.str();

    m_CompiledSource = source;

    m_SamplerProvider = LLVM::SamplerProvider::CreateFromCompiledKernel(
            m_WrappedLLVMKernel);

    m_SamplerLinker = new LLVM::SamplerLinker();
    m_SamplerLinker->SetDoOptimization(false);
}

//=============================================================================
Kernel::~Kernel()
{
    FIRTREE_SAFE_RELEASE(m_WrappedGLSLKernel);
    FIRTREE_SAFE_RELEASE(m_WrappedLLVMKernel);
    FIRTREE_SAFE_RELEASE(m_SamplerProvider);
    delete m_SamplerLinker;
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
bool Kernel::GetStatus() const
{
    return m_WrappedLLVMKernel->GetCompileStatus();
}

//=============================================================================
const char* Kernel::GetCompileLog() const
{
    return m_CompileLog.c_str();
}

//=============================================================================
const char* Kernel::GetSource() const
{
    return m_CompiledSource.c_str();
}

//=============================================================================
const std::vector<std::string>& Kernel::GetParameterNames() const
{
    return m_WrappedGLSLKernel->GetParameterNames();
}

//=============================================================================
const std::map<std::string, Parameter*>& Kernel::GetParameters() const
{
    return m_WrappedGLSLKernel->GetParameters();
}

//=============================================================================
const Value* Kernel::GetValueForKey(const char* key) const
{
    return m_SamplerProvider->GetParameterValue(key);
}

//=============================================================================
Parameter* Kernel::GetParameterForKey(const char* key) const
{
    return m_WrappedGLSLKernel->GetValueForKey(key);
}

//=============================================================================
void Kernel::SetValueForKey(Image* image, const char* key)
{
    // Firstly see if the value for this key is alreay this image.
    // If so, don't bother creating a new sampler.
    SamplerParameter* oldValue = 
        dynamic_cast<SamplerParameter*>(GetParameterForKey(key));
    if(oldValue != NULL)
    {
        if(oldValue->GetRepresentedImage() == image)
            return;

        FIRTREE_DEBUG("Performance hint: re-wiring pipeline is expensive.");
    }

    SamplerParameter* sampler = 
        SamplerParameter::CreateFromImage(image);
    m_WrappedGLSLKernel->SetValueForKey(sampler, key);
    FIRTREE_SAFE_RELEASE(sampler);

    LLVM::SamplerProvider* sampler_prov = LLVM::SamplerProvider::
        CreateFromImage(image);
    m_SamplerProvider->SetParameterSampler(key, sampler_prov);
    FIRTREE_SAFE_RELEASE(sampler_prov);

    // Re-wiring the pipeline *always* requires a re-link.
    ReLink();
}

//=============================================================================
void Kernel::SetValueForKey(Parameter* param, const char* key)
{
    m_WrappedGLSLKernel->SetValueForKey(param, key);
}

//=============================================================================
void Kernel::SetValueForKey(Value* value, const char* key)
{
    LLVM::SamplerProvider::const_iterator param =
        m_SamplerProvider->find(key);
    if(param != m_SamplerProvider->end()) {
        // Does setting this parameter cause a re-compile?
        if(m_SamplerProvider->SetParameterValue(param, value)) {
            ReLink();
        }
    }
}

//=============================================================================
void Kernel::SetValueForKey(float value, const char* key)
{
    SetValueForKey(&value, 1, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, const char* key)
{
    float v[] = {x, y};
    SetValueForKey(v, 2, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, float z, const char* key)
{
    float v[] = {x, y, z};
    SetValueForKey(v, 3, key);
}

//=============================================================================
void Kernel::SetValueForKey(float x, float y, float z, float w, 
        const char* key)
{
    float v[] = {x, y, z, w};
    SetValueForKey(v, 4, key);
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

    Value* v = Value::CreateVectorValue(value, count);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

//=============================================================================
void Kernel::SetValueForKey(int value, const char* key)
{
    NumericParameter* p = (NumericParameter*)(NumericParameter::Create());
    p->SetSize(1);
    p->SetBaseType(NumericParameter::TypeInteger);
    p->SetIntValue(value, 0);
    SetValueForKey(p, key);
    FIRTREE_SAFE_RELEASE(p);

    Value* v = Value::CreateIntValue(value);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

//=============================================================================
void Kernel::SetValueForKey(bool value, const char* key)
{
    NumericParameter* p = (NumericParameter*)(NumericParameter::Create());
    p->SetSize(1);
    p->SetBaseType(NumericParameter::TypeBool);
    p->SetBoolValue(value, 0);
    SetValueForKey(p, key);
    FIRTREE_SAFE_RELEASE(p);

    Value* v = Value::CreateBoolValue(value);
    SetValueForKey(v, key);
    FIRTREE_SAFE_RELEASE(v);
}

//=============================================================================
void Kernel::ReLink()
{
    if(m_SamplerLinker->CanLinkSampler(m_SamplerProvider)) {
        m_SamplerLinker->LinkSampler(m_SamplerProvider);
    }
}

//=============================================================================
void Kernel::Dump() {
    if(m_SamplerLinker->GetModule()) {
        m_SamplerLinker->GetModule()->dump();
    }
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
AffineTransform* SamplerParameter::GetAndOwnTransform() const
{
    return m_WrappedGLSLSampler->GetAndOwnTransform();
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
