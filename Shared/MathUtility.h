/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

/// The value of the constant pi as a float.
float Pi()
{
    return 3.141592653589f;
}

/// The value of the pi divided by two.
float HalfPi()
{
    return Pi() / 2;
}

/// A small value, used for approximate comparisons
float Epsilon() 
{
    return 0.00000001f;
}

/// Returns a value just under 1.
float OneMinusEpsilon(float e = Epsilon()) 
{
    return 1.0f - e;
}

/// Compares two values to see if they are equal to some degree of error
template<typename T>
BOOL NearlyEquals(T x, T y, T e = Epsilon())
{
    return (x >= y - e) && (x <= y + e);
}

/// Compares a value to see if it is equal to +/- Epsilon
template<typename T>
BOOL NearZero(T x, T e = Epsilon())
{
    return fabs(x) <= e;
}

/// Returns the sign of a value, or zero if it is equal (or near) to zero
int Sign(float f, float e = Epsilon()) {
    return NearZero(f, e)
        ? 0 
        : ( f > 0.0f ? 1 : -1);
}

/// Compares two matrices. 
bool EqualMatrices(const Matrix3& m1,const Matrix3& m2) 
{
	return m1.GetRow(0)==m2.GetRow(0) && 
		     m1.GetRow(1)==m2.GetRow(1) && 
		     m1.GetRow(2)==m2.GetRow(2) && 
		     m1.GetRow(3)==m2.GetRow(3);
}

/* 
 * Checks that the two points cross the XY plane.  
 * If so, computes the crossing point.
 *
 * \param pt1 First point.
 * \param pt2 Second point.
 * \param[out] Point of intersection.
 * 
 * \return An integer indicating the kind of intersection. 
 * - 2  both points are on the plane (cross_pt is not changed)
 * - 1  cross_pt contains the one point of intersection
 * - 0  no points of intersection (cross_pt not changed)
 */
int CrossCheck(const Point3& pt1,const Point3& pt2,Point3* cross_pt)
{
	const float	z1 = pt1.z;
	const float	z2 = pt2.z;
	const int	sign_z1 = Sign(z1);
	const int	sign_z2 = Sign(z2);

	// check for corners straddling the plane
	if(sign_z1 == 0 && sign_z2 == 0)
		return 2;
	else if(sign_z1 != sign_z2)
	{
		// straddle!  compute intersection pt
		if(cross_pt)
		{
			float dd = z1 / (z1 - z2);
			cross_pt->x = pt1.x + dd * (pt2.x - pt1.x);
			cross_pt->y = pt1.y + dd * (pt2.y - pt1.y);
			cross_pt->z = 0.0f;
		}
		return 1;
	}
	return 0;
}


/// An approximate method of comparing Point3's.
bool EqualPoint3(const Point3& p1, const Point3& p2, float e = Epsilon())
{
    return NearlyEquals(p1.x, p2.x, e) 
        && NearlyEquals(p1.y, p2.y, e)
        && NearlyEquals(p1.z, p2.z, e);
}

/// Removes scaling from a matrix
void RemoveScaling(Matrix3 &tm) 
{
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
}

/// Creates a matrix from AffineParts
void ComposeFromAffine(const AffineParts &ap, Matrix3 &mat)
{
	Matrix3 tm;
	
	mat.IdentityMatrix();
	mat.SetTrans( ap.t );

	if ( ap.f != 1.0f ) {				// has f component
		tm.SetScale( Point3( ap.f, ap.f, ap.f ) );
		mat = tm * mat;
	}

	if ( !ap.q.IsIdentity() ) {			// has q rotation component
		ap.q.MakeMatrix( tm );
		mat = tm * mat;
	}
	
	if ( ap.k.x != 1.0f || ap.k.y != 1.0f || ap.k.z != 1.0f ) {		// has k scale component
		tm.SetScale( ap.k );
		if ( !ap.u.IsIdentity() ) {			// has u rotation component
			Matrix3 utm;
			ap.u.MakeMatrix( utm );
			mat = Inverse( utm ) * tm * utm * mat;
		} else {
			mat = tm * mat;
		}
	}
}

/// Computes the distance from a matrix representation of location to a point.
float GetDistanceTo(const Matrix3& m, const Point3& pt)
{
    return Length(m.GetTrans() - pt) / Length(m.GetRow(2));
}

/// Swaps two values
template<typename T>
void Swap(T& x, T& y)
{
    T temp = x;
    x = y;
    y = temp;
}

/// Swaps two values, if the first is greater than the second.
template<typename T>
void SwapIfGreater(T& x, T& y)
{
    if (x > y)
        Swap(x, y);
}

/// Returns TRUE if a value x is between the values a and b.
template<typename T>
BOOL IsBetween(T x, T a, T b) 
{
    SwapIfGreater(a, b);
    return ((x >= a) && (x <= b));
}

/// Interpolates a value m by applying a function represented by 
/// an array (ntab) depending on which interval it appears in in mtab
float InterpolateValues(float m, float *mtab, float *ntab, int n) 
{
	float frac;
	for (int i=1; i<n; i++) {
		if (IsBetween(m,mtab[i-1],mtab[i])) {
			frac = (m - mtab[i-1])/(mtab[i]-mtab[i-1]);
			return((1.0f-frac)*ntab[i-1] + frac*ntab[i]);
		}
	}
	return 0.0f;
}

/// Computes the projection of a point on a perpendicular place
Point3 ProjectionOnPerpPlane(const Point3& Vector_ProjectionOf, const Point3& Vector_OnPlanePerpTo)
{
	Point3 xAxis = CrossProd(Vector_OnPlanePerpTo, Vector_ProjectionOf);

	if (FLength(xAxis) < Epsilon())
    {
		int ix = Vector_OnPlanePerpTo.MinComponent();
		xAxis = Point3(0,0,0); 
		xAxis[ix] = 1;
		Point3 cp = CrossProd(xAxis, Vector_OnPlanePerpTo);
		xAxis = Normalize(CrossProd(Vector_OnPlanePerpTo, cp));
	}

	Point3 yAxis = CrossProd(xAxis, Vector_OnPlanePerpTo);
	return Normalize(yAxis);
}

/// Return the identity matrix
const Matrix3& Identity()
{
    static const Matrix3 r(1);
    return r;
}

/// Given a point and target, computes a quaternion that represents the rotation
/// to look towards the target.
Quat ComputeLookAtQuat(const Point3& source, const Point3& target)
{
	Point3 source_target_vector = target - source;
	Point3 source_target_unit_vector = Normalize(source_target_vector);		
    Point3 source_to_upnode_vector = Normalize(-source);
	
	Point3 zdir = source_target_unit_vector;
	Point3 ydir = -Normalize(ProjectionOnPerpPlane(source_to_upnode_vector, zdir));
	Point3 xdir = Normalize(CrossProd(ydir, zdir));

    Matrix3 mat(xdir, ydir, zdir, Point3::Origin);	
    Quat quat(mat);
	quat.MakeClosest(IdentQuat());
	quat.Normalize();

	return quat;
}

/// Converts degrees to radians.
float DegToRad(float deg)
{
    return (Pi() * deg) / 180.0f;
}

/// Returns x divided by y, unless y is nearly zero in which 
/// case it returns 0.
float SafeDivide(float x, float y, float e = Epsilon())
{
    return NearZero(y, e) ? 0.0f : x / y;
}