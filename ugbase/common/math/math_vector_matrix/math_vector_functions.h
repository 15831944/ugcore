//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m12 d03

#ifndef __H__COMMON__MATH_VECTOR_FUNCTIONS__
#define __H__COMMON__MATH_VECTOR_FUNCTIONS__

#include "math_vector.h"

namespace ug
{
////////////////////////////////////////////////////////////////
// Addition of Vectors

///	adds two MathVector<N>s and stores the result in a third one
// vOut = v1 + v2
template <typename vector_t>
inline
void
VecAdd(vector_t& vOut, const vector_t& v1, const vector_t& v2);

///	adds two MathVector<N>s and stores the result in a third one
// vOut = v1 + v2 + v3
template <typename vector_t>
inline
void
VecAdd(vector_t& vOut, const vector_t& v1, const vector_t& v2,
							const vector_t& v3);

///	adds two MathVector<N>s and stores the result in a third one
// vOut = v1 + v2 + v3 + v4
template <typename vector_t>
inline
void
VecAdd(vector_t& vOut, const vector_t& v1, const vector_t& v2,
							const vector_t& v3, const vector_t& v4);

////////////////////////////////////////////////////////////////
// Subtraction of Vectors

///	subtracts v2 from v1 and stores the result in a vOut
// vOut = v1 - v2
template <typename vector_t>
inline
void
VecSubtract(vector_t& vOut, const vector_t& v1, const vector_t& v2);


////////////////////////////////////////////////////////////////
// Scaling of a Vector

///	scales a MathVector<N>
// vOut = s * v
template <typename vector_t>
inline
void
VecScale(vector_t& vOut, const vector_t& v, typename vector_t::value_type s);

////////////////////////////////////////////////////////////////
// Scaled Addition of Vectors

/// Scales two Vectors, adds them and returns the sum in a third vector
// vOut = s1*v1 + s2*v2
template <typename vector_t>
inline
void
VecScaleAdd(vector_t& vOut, typename vector_t::value_type s1, const vector_t& v1,
								 typename vector_t::value_type s2, const vector_t& v2);

/// Scales three Vectors, adds them and returns the sum in a fourth vector
// vOut = s1*v1 + s2*v2 + s3*v3
template <typename vector_t>
inline
void
VecScaleAdd(vector_t& vOut, typename vector_t::value_type s1, const vector_t& v1,
								 typename vector_t::value_type s2, const vector_t& v2,
								 typename vector_t::value_type s3, const vector_t& v3);

/// Scales four Vectors, adds them and returns the sum in a fifth vector
// vOut = s1*v1 + s2*v2 + s3*v3 + s4*v4
template <typename vector_t>
inline
void
VecScaleAdd(vector_t& vOut, typename vector_t::value_type s1, const vector_t& v1,
								 typename vector_t::value_type s2, const vector_t& v2,
								 typename vector_t::value_type s3, const vector_t& v3,
								 typename vector_t::value_type s4, const vector_t& v4);

////////////////////////////////////////////////////////////////
// Interpolation of Vectors

//	performs linear interpolation between two Vectors.
template <typename vector_t>
inline
void
VecInterpolateLinear(vector_t& vOut, const vector_t& v1, const vector_t& v2,
						typename vector_t::value_type interpAmount);

////////////////////////////////////////////////////////////////
// Length of Vectors

///	returns the squared length of v. Faster than VecLength.
template <typename vector_t>
inline
typename vector_t::value_type
VecLengthSq(const vector_t& v);

///	returns the length of v. Slower than VecLengthSq.
template <typename vector_t>
inline
typename vector_t::value_type
VecLength(const vector_t& v);

////////////////////////////////////////////////////////////////
// Distance of Vectors

///	returns the squared distance of two vector_ts.
template <typename vector_t>
inline
typename vector_t::value_type
VecDistanceSq(const vector_t& v1, const vector_t& v2);

///	returns the distance of two vector_ts.
template <typename vector_t>
inline
typename vector_t::value_type
VecDistance(const vector_t& v1, const vector_t& v2);


////////////////////////////////////////////////////////////////
// Dot Product
///	returns the dot-product of two vector_ts
template <typename vector_t>
inline
typename vector_t::value_type
VecDot(const vector_t& v1, const vector_t& v2);

////////////////////////////////////////////////////////////////
// Cross Product
///	calculates the cross product of two Vectors of dimension 3. It makes no sense to use VecCross for vector_ts that have not dimension 3.
template <typename vector_t>
inline
void
VecCross(vector_t& vOut, const vector_t& v1, const vector_t& v2);

////////////////////////////////////////////////////////////////
// Normalize a Vector
///	scales a vector_t to unit length
template <typename vector_t>
inline
void
VecNormalize(vector_t& vOut, const vector_t& v);

////////////////////////////////////////////////////////////////
// CalculateTriangleNormalNoNormalize
///	Calculates a triangle-normal in 3d (no normalization is performed).
//TODO: This method should not be part of the core-math methods.
//		Instead it should be moved to a seperate math-util file.
template <typename vector_t>
inline
void
CalculateTriangleNormalNoNormalize(vector_t& vOut, const vector_t& v1,
						const vector_t& v2, const vector_t& v3);

////////////////////////////////////////////////////////////////
// CalculateTriangleNormal
///	Calculates a triangle-normal in 3d (output has length 1).
//TODO: This method should not be part of the core-math methods.
//		Instead it should be moved to a seperate math-util file.
template <typename vector_t>
inline
void
CalculateTriangleNormal(vector_t& vOut, const vector_t& v1,
						const vector_t& v2, const vector_t& v3);


////////////////////////////////////////////////////////////////
// Operations with scalar

/// Set each vector component to scalar (componentwise)
template <typename vector_t>
inline
void
VecSet(vector_t& vInOut, typename vector_t::value_type s);

/// Add a scalar to a vector (componentwise)
template <typename vector_t>
inline
void
VecAdd(vector_t& vOut, const vector_t& v, typename vector_t::value_type s);

/// Subtract a scalar from a vector (componentwise)
template <typename vector_t>
inline
void
VecSubtract(vector_t& vOut, const vector_t& v, typename vector_t::value_type s);

/// Devide a vector by a scalar (componentwise)
template <typename vector_t>
inline
void
VecDevide(vector_t& vOut, const vector_t& v, typename vector_t::value_type s);

/// Multiply a vector by a scalar (componentwise)
template <typename vector_t>
inline
void
VecMultiply(vector_t& vOut, const vector_t& v, typename vector_t::value_type s);

////////////////////////////////////////////////////////////////
// Norms
template <typename vector_t>
inline
typename vector_t::value_type
VecTwoNorm(vector_t& v);

template <typename vector_t>
inline
typename vector_t::value_type
VecTwoNormSq(vector_t& v);

template <typename vector_t>
inline
typename vector_t::value_type
VecOneNorm(vector_t& v);

template <typename vector_t>
inline
typename vector_t::value_type
VecPNorm(vector_t& v, unsigned int p);

template <typename vector_t>
inline
typename vector_t::value_type
VecMaxNorm(vector_t& v);

template <typename vector_t>
inline
typename vector_t::value_type
VecInftyNorm(vector_t& v);



}//	end of namespace

////////////////////////////////////////////////////////////////////////
//	include a general, but not very fast implementation of the declared methods above.
#include "math_vector_functions_common_impl.hpp"

#endif /* __H__COMMON__MATH_MathVector_FUNCTIONS__ */
