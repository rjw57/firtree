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
/// \file main.h Main FIRTREE utility classes and types.
// ============================================================================

// ============================================================================
#ifndef FIRTREE_MATH_H
#define FIRTREE_MATH_H
// ============================================================================

#include <public/include/main.h>

// ============================================================================
namespace Firtree {
// ============================================================================

class AffineTransform;

// ============================================================================
/// Return the minimum of its arguments.
static const float Min(const float a, const float b) { return (a<b) ? a : b; }

// ============================================================================
/// Return the maximum of its arguments.
static const float Max(const float a, const float b) { return (a>b) ? a : b; }

// ============================================================================
/// A 2D point.
struct Point2D
{
    public:
	float X, Y;

	Point2D() : X(0.f), Y(0.f) { }
	Point2D(float x, float y) : X(x), Y(y) { }
	Point2D(const Point2D& p) : X(p.X), Y(p.Y) { }
};

// ============================================================================
/// A 2D size
struct Size2D
{
    public:
	float Width, Height;

	Size2D() : Width(0.f), Height(0.f) { }
	Size2D(float w, float h) : Width(w), Height(h) { }
	Size2D(const Size2D& p) : Width(p.Width), Height(p.Height) { }
};

// ============================================================================
/// A 2D rectangle
struct Rect2D
{
    public:
	Point2D Origin;
	Size2D Size;

	Rect2D() { };
	Rect2D(const Point2D& o, const Size2D& s) : Origin(o), Size(s) { }
	Rect2D(float x, float y, float w, float h) 
	    :	Origin(x,y), Size(w,h) { }

	float MinX() const { return Origin.X; }
	float MaxX() const { return Origin.X + Size.Width; }
	float MinY() const { return Origin.Y; }
	float MaxY() const { return Origin.Y + Size.Height; }
};

// ============================================================================
/// Return a Rect"D constructing from the minimum and maximum X and Y
/// co-oridinates. Should maxx<minx or maxy<miny, a zero-size rectangle is
/// returned.
Rect2D RectFromBounds(float minx, float miny, float maxx, float maxy);

// ============================================================================
/// Return the rectangle of intersection of two rectangles.
Rect2D RectIntersect(const Rect2D& a, const Rect2D& b);

// ============================================================================
/// Return the bounding rectangle of two rectangles.
Rect2D RectUnion(const Rect2D& a, const Rect2D& b);

// ============================================================================
/// Inset a rectangle by deltaX in the X-coordinate and deltaY in the 
/// Y-coordinate.
Rect2D RectInset(const Rect2D& a, const float deltaX, const float deltaY);

// ============================================================================
/// Return the bounding rectangle from the result of applying the transformation
/// t to the rectangle a.
Rect2D RectTransform(const Rect2D& a, const AffineTransform* t);

// ============================================================================
/// Structure representing an affine transformation matrix. Usually you wont
/// use these directly. Instead, you'll use the AffineTransform wrapper class.
struct AffineTransformStruct
{
    float m11, m12;
    float m21, m22;
    float tX,  tY;
};

// ============================================================================
/// A wrapper around the AffineTransformStruct structure allowing easy
/// creation and chaining of affine transformations.
class AffineTransform : public ReferenceCounted
{
    protected:
	AffineTransform();
	AffineTransform(const AffineTransform&);
	virtual ~AffineTransform();

    public:
        // ====================================================================
        // CONSTRUCTION METHODS

	/// Return a pointer to the identity transform.
	static AffineTransform* Identity();

	/// Return a pointer to a transform object representing the passed
	/// AffineTransformStruct. A copy is made of the data in this structure.
	static AffineTransform* FromTransformStruct(const AffineTransformStruct& s);

	/// Return a pointer to a transform object representing a non-uniform
	/// scale in the X- and Y-directions.
	static AffineTransform* Scaling(float xscale, float yscale);

	/// Return a pointer to a transform object representing an uniform
	/// scaling about the origin.
	static AffineTransform* Scaling(float scale)
		{ return Scaling(scale, scale); }

	/// Return a pointer to a transform object representing a rotation of
	/// 'deg' degrees anti-clockwise about the origin.
	static AffineTransform* RotationByDegrees(float deg);

	/// Return a pointer to a transform object representing a rotation of
	/// 'rad' radians anti-clockwise about the origin.
	static AffineTransform* RotationByRadians(float rad);

	/// Return a pointer to a transform object representing a translation
	/// of the origin to (x,y).
	static AffineTransform* Translation(float x, float y);

        // ====================================================================
        // CONST METHODS

	/// Return a deep copy of the transform. Should be Release()-d when
	/// you are finished with it.
	AffineTransform* Copy() const;

	/// Apply this transform to a point and return the transformed 
	/// point.
	Point2D TransformPoint(const Point2D& in) const;

	/// Apply this transform to a size and return the transformed size.
	/// This differs from TransformPoint() in that no translation is
	/// applied.
	Size2D TransformSize(const Size2D& in) const;

	/// Get a constant reference (valid for the lifetime of the object)
	/// to the underlying AffineTransformStruct this encapsulates.
	const AffineTransformStruct& GetTransformStruct() const 
		{ return m_Transform; }

        // ====================================================================
        // MUTATING METHODS

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

	/// Appends a rotation by deg degrees to this transform.
	void RotateByDegrees(float deg);

	/// Appends a rotation by rad radians to this transform.
	void RotateByRadians(float rad);

	/// Appends a scaling of xscale along the X-axis and
	/// yscale along the Y-axis.
	void ScaleBy(float xscale, float yscale);

	/// Appends a uniform scaling of 'scale' about the origin.
	void ScaleBy(float scale) { ScaleBy(scale, scale); }

	/// Modifies this transform so that subsequent transforms
	/// result in points offset by (x, y). (i.e. appends a
	/// translation.
	void TranslateBy(float x, float y);

	/// Perform a deep-copy of the AffineTransformStruct passed
	/// into this object.
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
