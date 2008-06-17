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

#include <public/include/main.h>
#include <public/include/math.h>

namespace Firtree {

class NumericParameter;
class SamplerParameter;
class Kernel;

//=============================================================================
class Parameter : public ReferenceCounted
{
    public:
        Parameter();
        Parameter(const Parameter& p);
        virtual ~Parameter();
    private:
};

//=============================================================================
class NumericParameter : public Parameter
{
    public:
        enum BaseType {
            TypeInteger, TypeFloat, TypeBool,
        };

    protected:
        NumericParameter();
        virtual ~NumericParameter();

    public:
        static Parameter* Create();

        float GetFloatValue(int idx) const { return m_Value[idx].f; }
        int GetIntValue(int idx) const { return m_Value[idx].i; }
        bool GetBoolValue(int idx) const { return m_Value[idx].i != 0; }

        void SetFloatValue(float f, int idx) { m_Value[idx].f = f; }
        void SetIntValue(int i, int idx) { m_Value[idx].i = i; }
        void SetBoolValue(bool b, int idx) { m_Value[idx].i = b ? 1 : 0; }

        BaseType GetBaseType() const { return m_BaseType; }
        void SetBaseType(BaseType bt) { m_BaseType = bt; }

        int GetSize() const { return m_Size; }
        void SetSize(int s) { m_Size = s; }

        bool IsColor() { return m_IsColor; }
        void SetIsColor(bool flag) { m_IsColor = flag; }

        void AssignFrom(const NumericParameter& param);

    private:
        BaseType    m_BaseType;
        int         m_Size;
        union {
            float f;
            int i;      /* 0/1 for bool */
        }           m_Value[4];
        bool        m_IsColor : 1;
};

//=============================================================================
class SamplerParameter : public Parameter
{
    protected:
        SamplerParameter();
        virtual ~SamplerParameter();

    public:
        virtual const Rect2D GetExtent() const = 0;
        virtual const AffineTransform* GetTransform() const = 0;
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
        Kernel();
        Kernel(const char* source);
        virtual ~Kernel();
        ///@}

    public:
        // ====================================================================
        // CONST METHODS

        /// Retrieve a pointer to the source for this kernel expressed in
        /// the FIRTREE kernel language.
        virtual const char* GetSource() const = 0;

        // ====================================================================
        // MUTATING METHODS

        /// Set the kernel's function expressed in the FIRTREE
        /// kernel language. The source should be copied by this method.
        virtual void SetSource(const char* source) = 0;

        /// Return a const reference to a map containing the parameter 
        /// name/value pairs.
        virtual const std::map<std::string, Parameter*>& GetParameters() = 0;

        /// Accessor for setting a kernel parameter value. The accessors below
        /// are convenience wrappers around this method.
        virtual void SetValueForKey(Parameter* parameter, const char* key) = 0;

        ///@{
        /// Accessors for the various scalar, vector and sampler parameters.
        void SetValueForKey(float value, const char* key);
        void SetValueForKey(const float* value, int count, 
                const char* key);
        void SetValueForKey(int value, const char* key);
        void SetValueForKey(const int* value, int count, 
                const char* key);
        void SetValueForKey(bool value, const char* key);
        void SetValueForKey(const bool* value, int count, 
                const char* key);
        ///@}
    private:
};

}

//=============================================================================
#endif // FIRTREE_KERNEL_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

