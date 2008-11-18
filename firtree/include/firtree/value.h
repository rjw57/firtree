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
/// \brief The possible types in the Firtree kernel language.
///
/// Note that during code generation, the '__color' type is aliased to
/// vec4 and the sampler type is aliased to const int.
enum TypeSpecifier {
    TySpecFloat,          ///< A 32-bit floating point.
    TySpecInt,            ///< A 32-bit signed integet.
    TySpecBool,           ///< A 1-bit boolean.
    TySpecVec2,           ///< A 2 component floating point vector.
    TySpecVec3,           ///< A 3 component floating point vector.
    TySpecVec4,           ///< A 4 component floating point vector.
    TySpecSampler,        ///< An image sampler.
    TySpecColor,          ///< A colour.
    TySpecVoid,           ///< A 'void' type.
    TySpecInvalid = -1,   ///< An 'invalid' type.
};
    
//=============================================================================
/// \brief A value which can be passed to a Firtree kernel.
class Value : public ReferenceCounted 
{
    protected:
        Value();
        virtual ~Value();

    public:
        static Value* Create();
        static Value* CreateFloatValue(float v);
        static Value* CreateIntValue(int v);
        static Value* CreateBoolValue(bool v);
        static Value* CreateVectorValue(float x, float y);
        static Value* CreateVectorValue(float x, float y, float z);
        static Value* CreateVectorValue(float x, float y, float z, float w);
        static Value* CreateVectorValue(float* v, int num_components);

        inline TypeSpecifier GetType() const { return m_Type; }

        void SetFloatValue(float v);
        void SetIntValue(int v);
        void SetBoolValue(bool v);

        inline float GetFloatValue() const {
            return m_FloatingPointComponents[0]; 
        }
        inline int GetIntValue() const { return m_IntegerValue; }
        inline bool GetBoolValue() const { return m_IntegerValue != 0; }

        void SetVectorValue(float x, float y);
        void SetVectorValue(float x, float y, float z);
        void SetVectorValue(float x, float y, float z, float w);
        void SetVectorValue(float* v, int num_components);

        void GetVectorValue(float* px, float* py,
                float* pz=NULL, float* pw=NULL) const;
        void GetVectorValue(float* dest, int expected_components) const;
        const float* GetVectorValue(uint32_t index) const;

    private:
        float               m_FloatingPointComponents[4];
        int                 m_IntegerValue;
        TypeSpecifier       m_Type;
};

}

//=============================================================================
#endif // FIRTREE_VALUE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

