// FIRTREE - A generic image processing library
// Copyright (C) 2008 Rich Wareham <richwareham@gmail.com>
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
// This file implements the FIRTREE math functions.
//=============================================================================

#include <firtree/math.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

// ===============================================================================
namespace Firtree {
// ===============================================================================

// ============================================================================
Rect2D RectFromBounds(float minx, float miny, float maxx, float maxy)
{
    if((minx>maxx) || (miny>maxy))
    {
        return Rect2D(maxx,maxy,0,0);
    }
    return Rect2D(minx,miny,maxx-minx,maxy-miny);
}

// ===============================================================================
Rect2D RectIntersect(const Rect2D& a, const Rect2D& b)
{
    float minx = Max(a.MinX(), b.MinX());
    float miny = Max(a.MinY(), b.MinY());
    float maxx = Min(a.MaxX(), b.MaxX());
    float maxy = Min(a.MaxY(), b.MaxY());

    return RectFromBounds(minx,miny,maxx,maxy);
}

// ===============================================================================
Rect2D RectUnion(const Rect2D& a, const Rect2D& b)
{
    float minx = Min(a.MinX(), b.MinX());
    float miny = Min(a.MinY(), b.MinY());
    float maxx = Max(a.MaxX(), b.MaxX());
    float maxy = Max(a.MaxY(), b.MaxY());

    return RectFromBounds(minx,miny,maxx,maxy);
}

// ===============================================================================
Rect2D RectTransform(const Rect2D& inRect, const AffineTransform* t)
{
    Point2D a(inRect.MinX(), inRect.MaxY());
    Point2D b(inRect.MaxX(), inRect.MaxY());
    Point2D c(inRect.MinX(), inRect.MinY());
    Point2D d(inRect.MaxX(), inRect.MinY());

    a = t->TransformPoint(a);
    b = t->TransformPoint(b);
    c = t->TransformPoint(c);
    d = t->TransformPoint(d);

    float minx = Min(Min(a.X, b.X), Min(c.X, d.X));
    float miny = Min(Min(a.Y, b.Y), Min(c.Y, d.Y));
    float maxx = Max(Max(a.X, b.X), Max(c.X, d.X));
    float maxy = Max(Max(a.Y, b.Y), Max(c.Y, d.Y));

    return RectFromBounds(minx,miny,maxx,maxy);
}

// ============================================================================
Rect2D RectInset(const Rect2D& a, const float deltaX, const float deltaY)
{
    return RectFromBounds(a.MinX()+deltaX, a.MinY()+deltaY, 
            a.MaxX()-deltaX, a.MaxY()-deltaY);
}

// ============================================================================
AffineTransform* RectComputeTransform(const Rect2D& whence, const Rect2D& hence)
{
    AffineTransform* trans = AffineTransform::Identity();

    // Firstly move the origins of the rectangle to co-incide.
    trans->TranslateBy(hence.Origin.X - whence.Origin.X,
            hence.Origin.Y - whence.Origin.Y);

    // Scale appropriately in X- and Y-
    trans->ScaleBy(hence.Size.Width / whence.Size.Width, 
            hence.Size.Height / whence.Size.Height);

    return trans;
}

static const float deg2rad = M_PI * 2.f / 360.f;
static const float rad2deg = 360.f / (M_PI * 2.f);

// ===============================================================================
// Computes result = a*b
static void MultiplyAffineTransforms(AffineTransformStruct& result,
        const AffineTransformStruct& a, const AffineTransformStruct& b)
{
    result.m11 = (a.m11*b.m11) + (a.m12*b.m21);
    result.m12 = (a.m11*b.m12) + (a.m12*b.m22);
    result.m21 = (a.m21*b.m11) + (a.m22*b.m21);
    result.m22 = (a.m21*b.m12) + (a.m22*b.m22);
    result.tX = (a.m11*b.tX) + (a.m12*b.tY) + a.tX;
    result.tY = (a.m21*b.tX) + (a.m22*b.tY) + a.tY;
}

// ===============================================================================
static float ComputeDeterminant(const AffineTransformStruct& t)
{
    float d = (t.m11*t.m22) - (t.m12*t.m21);
    return d;
}

// ===============================================================================
AffineTransform::AffineTransform()
    :   ReferenceCounted()
{
    m_Transform.m11 = 1.f; m_Transform.m12 = 0.f;
    m_Transform.m21 = 0.f; m_Transform.m22 = 1.f;
    m_Transform.tX = 0.f;
    m_Transform.tY = 0.f;
}

// ===============================================================================
AffineTransform::~AffineTransform()
{
}

// ===============================================================================
AffineTransform::AffineTransform(const AffineTransform& t)
{
    AssignFromTransformStruct(t.GetTransformStruct());
}

// ===============================================================================
AffineTransform* AffineTransform::Identity()
{
    return new AffineTransform();
}

// ===============================================================================
AffineTransform* AffineTransform::FromTransformStruct(const AffineTransformStruct& s)
{
    AffineTransform* rv = AffineTransform::Identity();
    rv->AssignFromTransformStruct(s);
    return rv;
}

// ===============================================================================
AffineTransform* AffineTransform::Scaling(float xscale, float yscale)
{
    AffineTransform* rv = AffineTransform::Identity();
    rv->m_Transform.m11 = xscale;
    rv->m_Transform.m22 = yscale;
    return rv;
}

// ===============================================================================
AffineTransform* AffineTransform::RotationByDegrees(float deg)
{
    return AffineTransform::RotationByRadians(deg2rad * deg);
}

// ===============================================================================
AffineTransform* AffineTransform::RotationByRadians(float rad)
{
    AffineTransform* rv = AffineTransform::Identity();
    float s = sin(rad);
    float c = cos(rad);
    rv->m_Transform.m11 = c; rv->m_Transform.m12 = -s; 
    rv->m_Transform.m21 = s; rv->m_Transform.m22 = c; 
    return rv;
}

// ===============================================================================
AffineTransform* AffineTransform::Translation(float x, float y)
{
    AffineTransform* rv = AffineTransform::Identity();
    rv->m_Transform.tX = x;
    rv->m_Transform.tY = y;
    return rv;
}

// ===============================================================================
bool AffineTransform::IsIdentity() const {
    bool idFlag = true;
    const AffineTransformStruct& t = GetTransformStruct();
    if(t.m11 != 1.0) { idFlag = false; }
    if(t.m12 != 0.0) { idFlag = false; }
    if(t.m21 != 0.0) { idFlag = false; }
    if(t.m22 != 1.0) { idFlag = false; }
    if(t.tX != 0.0) { idFlag = false; }
    if(t.tY != 0.0) { idFlag = false; }
    return idFlag;
}

// ===============================================================================
AffineTransform* AffineTransform::Copy() const {
    return new AffineTransform(*this);
}

// ===============================================================================
void AffineTransform::AppendTransform(const AffineTransform* t)
{
    if(t == NULL) { return; }

    const AffineTransformStruct& a = t->GetTransformStruct();
    const AffineTransformStruct& b = GetTransformStruct();

    AffineTransformStruct result;

    MultiplyAffineTransforms(result, a, b);

    AssignFromTransformStruct(result);
}

// ===============================================================================
bool AffineTransform::Invert()
{
    // Determinant is 1/determinant * adjoint matrix.
    const AffineTransformStruct& a = GetTransformStruct();
    float d = ComputeDeterminant(a);
    if((d == 0.f) || (!isfinite(d))) { return false; }
    float ood = 1.f / d;

    AffineTransformStruct result;

    result.m11 = ood * a.m22;
    result.m22 = ood * a.m11;
    result.m12 = ood * -a.m12;
    result.m21 = ood * -a.m21;

    result.tX = ood * (a.m12*a.tY - a.m22*a.tX);
    result.tY = ood * (a.m21*a.tX - a.m11*a.tY);

    /*
    AffineTransformStruct check;
    MultiplyAffineTransforms(check,result,a);
    printf("M*M^-1: %f,%f,%f;%f,%f,%f\n",
            check.m11, check.m12, check.tX,
            check.m21, check.m22, check.tY);
    MultiplyAffineTransforms(check,a,result);
    printf("M^-1*M: %f,%f,%f;%f,%f,%f\n",
            check.m11, check.m12, check.tX,
            check.m21, check.m22, check.tY);
            */

    AssignFromTransformStruct(result);

    return true;
}

// ===============================================================================
void AffineTransform::PrependTransform(const AffineTransform* t)
{
    if(t == NULL) { return; }

    const AffineTransformStruct& a = GetTransformStruct();
    const AffineTransformStruct& b = t->GetTransformStruct();

    AffineTransformStruct result;

    MultiplyAffineTransforms(result, a, b);

    AssignFromTransformStruct(result);
}

// ===============================================================================
void AffineTransform::RotateByDegrees(float deg)
{
    AffineTransform* rot = AffineTransform::RotationByDegrees(deg);
    AppendTransform(rot);
    rot->Release();
}

// ===============================================================================
void AffineTransform::RotateByRadians(float rad)
{
    AffineTransform* rot = AffineTransform::RotationByRadians(rad);
    AppendTransform(rot);
    rot->Release();
}

// ===============================================================================
void AffineTransform::ScaleBy(float xscale, float yscale)
{
    AffineTransform* s = AffineTransform::Scaling(xscale, yscale);
    AppendTransform(s);
    s->Release();
}

// ===============================================================================
void AffineTransform::TranslateBy(float x, float y)
{
    AffineTransform* t = AffineTransform::Translation(x, y);
    AppendTransform(t);
    t->Release();
}

// ===============================================================================
Point2D AffineTransform::TransformPoint(const Point2D& in) const
{
    Point2D outVal;
    const AffineTransformStruct& a = GetTransformStruct();

    outVal.X = a.m11*in.X + a.m12*in.Y + a.tX;
    outVal.Y = a.m21*in.X + a.m22*in.Y + a.tY;

    return outVal;
}

// ===============================================================================
Size2D AffineTransform::TransformSize(const Size2D& in) const
{
    Size2D outVal;
    const AffineTransformStruct& a = GetTransformStruct();

    outVal.Width = a.m11*in.Width + a.m12*in.Height;
    outVal.Height = a.m21*in.Width + a.m22*in.Height;

    return outVal;
}

// ===============================================================================
void AffineTransform::AssignFromTransformStruct(const AffineTransformStruct& s)
{
    memcpy(&m_Transform, &s, sizeof(AffineTransformStruct));
}

// ===============================================================================
} // namespace Firtree 
// ===============================================================================

// So that VIM does the "right thing"
// vim:cindent:sw=4:ts=4:et
