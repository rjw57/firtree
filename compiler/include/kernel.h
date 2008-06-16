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
// This file defines the interface to the FIRTREE kernels and samplers.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_KERNEL_H
#define FIRTREE_KERNEL_H
//=============================================================================

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#include <compiler/include/main.h>
#include <compiler/include/math.h>

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

        virtual NumericParameter* GetAsNumeric() { return NULL; }
        virtual SamplerParameter* GetAsSampler() { return NULL; }
    private:
};

//=============================================================================
class NumericParameter : public Parameter
{
    public:
        enum BaseType {
            Int, Float, Bool,
        };

    protected:
        NumericParameter();
        virtual ~NumericParameter();

    public:
        static Parameter* Create();

        virtual NumericParameter* GetAsNumeric() { return this; }

        float GetFloatValue(int idx) { return m_Value[idx].f; }
        int GetIntValue(int idx) { return m_Value[idx].i; }
        bool GetBoolValue(int idx) { return m_Value[idx].i != 0; }

        void SetFloatValue(float f, int idx) { m_Value[idx].f = f; }
        void SetIntValue(int i, int idx) { m_Value[idx].i = i; }
        void SetBoolValue(bool b, int idx) { m_Value[idx].i = b ? 1 : 0; }

        BaseType GetBaseType() { return m_BaseType; }
        void SetBaseType(BaseType bt) { m_BaseType = bt; }

        int GetSize() { return m_Size; }
        void SetSize(int s) { m_Size = s; }

        bool IsColor() { return m_IsColor; }
        void SetIsColor(bool flag) { m_IsColor = flag; }

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
        virtual SamplerParameter* GetAsSampler() { return this; }

        virtual const Rect2D GetExtent() const;

        virtual void SetTransform(const AffineTransform* f);
        virtual const AffineTransform* GetTransform() const { return m_Transform; }

    private:
        /// Affine transformation to map from world co-ordinates
        /// to sampler co-ordinates.
        AffineTransform*    m_Transform;
};

//=============================================================================
class Kernel : public ReferenceCounted
{
    protected:
        Kernel();
        Kernel(const char* source);
        virtual ~Kernel();

    public:
        virtual void SetSource(const char* source) = 0;
        virtual const char* GetSource() const = 0;

        virtual void SetValueForKey(float value, const char* key) = 0;
        virtual void SetValueForKey(const float* value, int count, const char* key) = 0;
        virtual void SetValueForKey(int value, const char* key) = 0;
        virtual void SetValueForKey(const int* value, int count, const char* key) = 0;
        virtual void SetValueForKey(bool value, const char* key) = 0;
        virtual void SetValueForKey(const bool* value, int count, const char* key) = 0;
        virtual void SetValueForKey(SamplerParameter* sampler, const char* key) = 0;

        virtual std::map<std::string, Parameter*>& GetParameters() = 0;
    private:
};

}

//=============================================================================
#endif // FIRTREE_KERNEL_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

