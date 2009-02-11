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
/// \file kernel.h The interface to a FIRTREE kernel.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_KERNEL_H
#define FIRTREE_KERNEL_H
//=============================================================================

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#include <firtree/main.h>
#include <firtree/math.h>

namespace Firtree { namespace GLSL { 
    class KernelSamplerParameter; 
    class CompiledGLSLKernel; 
    class GLSLSamplerParameter; 
} }

namespace Firtree { namespace LLVM {
    class CompiledKernel;
    class KernelFunction;
    class SamplerProvider;
    class KernelImageSamplerProvider;
} }

namespace llvm { class Module; }

namespace Firtree {

class Kernel;
class Image;
class Value;

//=============================================================================
/// \brief Abstract base class for extent providers.
///
/// Computing the non-transparent pixel extent for a kernel is non-trivial.
/// Consequently, kernels rely on an 'extent provider' to examine their
/// state and return their extent. A custom extent provider should inherit
/// from this class.
class ExtentProvider : public ReferenceCounted
{
    public:
        /// Examine the passed kernel and return an extent for it.
        virtual Rect2D ComputeExtentForKernel(Kernel* kernel) = 0;

        // ====================================================================
        /// \brief Construct a standard ExtentProvider. 
        ///
        /// The extent of many kernels can be computed by a standard algorithm.
        /// This method returns an instance of an ExtentProvider which 
        /// implements the following algorithm.
        ///
        /// Firstly the extent of the kernel's sampler parameter named
        /// 'samplerName' is retrieved. If no sampler is specified, the union
        /// of all the kernel's samplers' extents is found. Should the
        /// kernel use no samplers, an infinite extent is used.
        ///
        /// This extent is then inset by (deltaX, deltaY) and returned.
        ///
        static ExtentProvider* CreateStandardExtentProvider(
                const char* samplerName = NULL,
                float deltaX = 0.f, float deltaY = 0.f);
};

//=============================================================================
/// A kernel encapsulates a kernel function written in the FIRTREE kernel
/// language along with a set of values for various kernel parameters.
class Kernel : public ReferenceCounted
{
    protected:
        ///@{
        /// Protected constructors/destructors. Use the various Kernel
        /// factory functions instead.
        Kernel(const char* source);
        ///@}

    public:
        virtual ~Kernel();
        // ====================================================================
        // CONSTRUCTION METHODS

        /// Construct a new Firtree kernel from the specified kernel
        /// source.
        static Kernel* CreateFromSource(const char* source);

        // ====================================================================
        // CONST METHODS

        /// Get the compile status flag. Returns true if compilation succeeded
        /// and false otherwise.
        bool GetStatus() const;

        /// Get the compile log as a big string. The pointer is only valid
        /// for the lifetime of the class.
        const char* GetCompileLog() const;
        
        /// Retrieve a pointer to the source for this kernel expressed in
        /// the FIRTREE kernel language.
        const char* GetSource() const;
        
        /// Return the value object for the named parameter or NULL if
        /// this parameter is unset or there is no parameter with that name.
        const Value* GetValueForKey(const char* key) const;

        /// Return true if the kernel is 'valid', i.e. all static parameters
        /// have some value and the kernel source was successfully
        /// compiled.
        bool IsValid() const;

        // ====================================================================
        // MUTATING METHODS

        /// Convenience accessor for setting a kernel parameter value to
        /// a sampler which samples from the passed image.
        void SetValueForKey(Image* image, const char* key);

        /// Accessor for setting a kernel parameter value. The accessors below
        /// are convenience wrappers around this method.
        void SetValueForKey(const Value* val, const char* key);

        ///@{
        /// Accessors for the various scalar, vector and sampler parameters.
        void SetValueForKey(float value, const char* key);
        void SetValueForKey(float x, float y, const char* key);
        void SetValueForKey(float x, float y, float z, const char* key);
        void SetValueForKey(float x, float y, float z, float w, 
                const char* key);
        void SetValueForKey(const float* value, int count, 
                const char* key);
        void SetValueForKey(int value, const char* key);
        void SetValueForKey(bool value, const char* key);
        ///@}
        
    protected:
        /// Internal method to construct a llvm module for the
        /// kernel.
        llvm::Module* CreateSamplerModule(const std::string& prefix);

        void SetSamplerProviderForKey(LLVM::SamplerProvider* sampler_prov, 
                const char* key);
        const LLVM::SamplerProvider* GetSamplerProviderForKey(const char* key) const;

        friend class LLVM::SamplerProvider;
        friend class LLVM::KernelImageSamplerProvider;

    private:
        const LLVM::KernelFunction&   GetFunctionRecord() const;

        /// A flag indicating the compile staus.
        bool                        m_bCompileStatus;

        std::string                 m_CompileLog;
        std::string                 m_CompiledSource;

        typedef std::map<std::string, Value*> 
            ParameterValueMap;
        typedef std::map<std::string, LLVM::SamplerProvider*> 
            ParameterSamplerMap;

        LLVM::CompiledKernel*       m_CompiledKernel;
        ParameterValueMap           m_ParameterValues;
        ParameterSamplerMap         m_ParameterSamplers;
};

}

//=============================================================================
#endif // FIRTREE_KERNEL_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

