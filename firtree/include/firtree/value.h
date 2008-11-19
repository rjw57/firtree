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
/// \file value.h The interface to a Firtree kernel language value.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_VALUE_H
#define FIRTREE_VALUE_H
//=============================================================================

#include <firtree/main.h>

namespace Firtree {
    
//=============================================================================
/// \brief A value which can be passed to a Firtree kernel.
class Value : public ReferenceCounted 
{
    protected:
        Value();
        virtual ~Value();

    public:
        /// Create an empty value. The value has invalid type until
        /// it is assigned.
        static Value* Create();

        /// Create a value with a specified floating point value.
        static Value* CreateFloatValue(float v);

        /// Create a value with a specified integer value.
        static Value* CreateIntValue(int v);

        /// Create a value with a specified boolean value value.
        static Value* CreateBoolValue(bool v);

        /// Create a 2D vector with specified components.
        static Value* CreateVectorValue(float x, float y);

        /// Create a 3D vector with specified components.
        static Value* CreateVectorValue(float x, float y, float z);

        /// Create a 4D vector with specified components.
        static Value* CreateVectorValue(float x, float y, float z, float w);

        /// Create a nD vector with specified components. 'v' points to
        /// an array of component values and 'num_components' gives
        /// its length. If num_components is outside of the range (2,4), 
        /// an error is thrown.
        static Value* CreateVectorValue(float* v, int num_components);

        /// Return the type of the value.
        inline KernelTypeSpecifier GetType() const { return m_Type; }

        /// Assign a specified floating point value.
        void SetFloatValue(float v);

        /// Assign a specified integer value.
        void SetIntValue(int v);

        /// Assign a specified boolean value.
        void SetBoolValue(bool v);

        /// Get the floating point value. If the value is a vector, this
        /// is the first component. If the value is integer, this is
        /// the integer cast to a float. If the value is boolean, this
        /// is 1.0 if true, 0.0 if false.
        inline float GetFloatValue() const {
            return m_FloatingPointComponents[0]; 
        }

        /// Get the integer value. If the value is a vector, this is the
        /// first component cast to an integer. If the value is a float, 
        /// this is the floating point value cast to an integer. If the value
        /// is a boolean, this is 1 for true and 0 for false.
        inline int GetIntValue() const { return m_IntegerValue; }

        /// Get the boolean value. Equivalent to (GetIntValue() != 0).
        inline bool GetBoolValue() const { return m_IntegerValue != 0; }

        /// Assign a 2D vector with given components to the value.
        void SetVectorValue(float x, float y);

        /// Assign a 3D vector with given components to the value.
        void SetVectorValue(float x, float y, float z);

        /// Assign a 4D vector with given components to the value.
        void SetVectorValue(float x, float y, float z, float w);

        /// Assign the value from an array of vector components. 
        /// 'num_components' must be in the range (2,4) or an error is
        /// thrown.
        void SetVectorValue(float* v, int num_components);

        /// Return the vector component values via the passed pointers.
        /// If pz and/or pw are NULL, the z and/or w components are not
        /// returned.
        void GetVectorValue(float* px, float* py,
                float* pz=NULL, float* pw=NULL) const;

        /// Write a copy of the vector into the array pointed to by
        /// dest. No more than 'expected_components' will be written
        /// and an error is thrown if 'expected_components' is 
        /// outside of the range (2,4).
        void GetVectorValue(float* dest, int expected_components) const;

        /// Return a pointer to the internal array of vector components.
        const float* GetVectorValue(uint32_t index) const;

    private:
        float               m_FloatingPointComponents[4];
        int                 m_IntegerValue;
        KernelTypeSpecifier       m_Type;
};

}

//=============================================================================
#endif // FIRTREE_VALUE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

