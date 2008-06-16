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

// ============================================================================
// Include OpenGL headers
// ============================================================================

// Not all platforms supported by FIRTREE have their OpenGL headers under
// GL/. This file includes gl.h, glext.h and glut.h for all supported
// platforms.

// ============================================================================
#ifndef FIRTREE_MATH_H
#define FIRTREE_MATH_H
// ============================================================================

#include <compiler/include/main.h>

// ============================================================================
namespace Firtree {
// ============================================================================

// ============================================================================
// 2D point
struct Point2D
{
    public:
	float X, Y;

	Point2D() : X(0.f), Y(0.f) { }
	Point2D(float x, float y) : X(x), Y(y) { }
	Point2D(const Point2D& p) : X(p.X), Y(p.Y) { }
};

// ============================================================================
// 2D size
struct Size2D
{
    public:
	float Width, Height;

	Size2D() : Width(0.f), Height(0.f) { }
	Size2D(float w, float h) : Width(w), Height(h) { }
	Size2D(const Size2D& p) : Width(p.Width), Height(p.Height) { }
};

// ============================================================================
// 2D rectangle
struct Rect2D
{
    public:
	Point2D Origin;
	Size2D Size;

	Rect2D() { };
	Rect2D(const Point2D& o, const Size2D& s) : Origin(o), Size(s) { }
	Rect2D(float x, float y, float w, float h) 
	    :	Origin(x,y), Size(w,h) { }
};

// ============================================================================
// Affine transformation

struct AffineTransformStruct
{
    float m11, m12;
    float m21, m22;
    float tX,  tY;
};

class AffineTransform : public ReferenceCounted
{
    protected:
	AffineTransform();
	AffineTransform(const AffineTransform&);
	virtual ~AffineTransform();

    public:
	static AffineTransform* Identity();
	static AffineTransform* FromTransformStruct(const AffineTransformStruct& s);
	static AffineTransform* Scaling(float xscale, float yscale);
	static AffineTransform* Scaling(float scale)
		{ return Scaling(scale, scale); }
	static AffineTransform* RotationByDegrees(float deg);
	static AffineTransform* RotationByRadians(float rad);
	static AffineTransform* Translation(float x, float y);
	AffineTransform* Copy() const;

	/// Append the transform pointed to by t to this. This
	/// changes this to be a transform which has the effect of
	/// the original transform followed by that pointed to by t.
	void AppendTransform(const AffineTransform* t);

	/// Inverts this matrix. If there is no inverse, returns
	/// false, otherwise returns true.
	bool Invert();

	/// Prepend the transform pointed to by t to this. This
	/// changes this to be a transform which has the effect of
	/// the transform pointed to t followed by the original transform.
	void PrependTransform(const AffineTransform* t);

	/// Prepends a rotation by deg degrees to this transform.
	void RotateByDegrees(float deg);

	/// Prepends a rotation by rad radians to this transform.
	void RotateByRadians(float rad);

	void ScaleBy(float xscale, float yscale);
	void ScaleBy(float scale) { ScaleBy(scale, scale); }

	/// Modifies this transform so that subsequent transforms
	/// result in points offset by (x, y). (i.e. appends a
	/// translation.
	void TranslateBy(float x, float y);

	Point2D TransformPoint(const Point2D& in) const;
	Size2D TransformSize(const Size2D& in) const;

	const AffineTransformStruct& GetTransformStruct() const 
		{ return m_Transform; }
	void AssignFromTransformStruct(const AffineTransformStruct& s);

    private:
	AffineTransformStruct		m_Transform;
};

// ============================================================================
} // namespace Firtree
// ============================================================================

// ============================================================================
#endif // FIRTREE_MATH_H
// ============================================================================
