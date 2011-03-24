/*
 * \file operations_vec.h
 *
 * \author Martin Rupp
 *
 * \date 29.09.2010
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__UG__LIB_ALGEBRA__OPERATIONS_VEC__
#define __H__UG__LIB_ALGEBRA__OPERATIONS_VEC__

namespace ug
{

// operations for doubles
//-----------------------------------------------------------------------------
// todo: check if we might replace double with template<T>

// VecScale: These function calculate dest = sum_i alpha_i v_i

//! calculates dest = alpha1*v1. for doubles
inline void VecScaleAssign(double &dest, double alpha1, const double &v1)
{
	dest = alpha1*v1;
}

//! calculates dest = alpha1*v1 + alpha2*v2. for doubles
inline void VecScaleAdd(double &dest, double alpha1, const double &v1, double alpha2, const double &v2)
{
	dest = alpha1*v1 + alpha2*v2;
}

//! calculates dest = alpha1*v1 + alpha2*v2 + alpha3*v3. for doubles
inline void VecScaleAdd(double &dest, double alpha1, const double &v1, double alpha2, const double &v2, double alpha3, const double &v3)
{
	dest = alpha1*v1 + alpha2*v2 + alpha3*v3;
}


// VecProd

//! calculates s += scal<a, b>
inline void VecProdAdd(const double &a, const double &b, double &s)
{
	s += a*b;
}

template<typename vector_t>
inline void VecProdAdd(const vector_t &a, const vector_t &b, double &s)
{
	for(size_t i=0; i<a.size(); i++)
		VecProdAdd(a[i], b[i], s);
}


//! returns scal<a, b>
inline double VecProd(const double &a, const double &b, double &s)
{
	return a*b;
}


// VecNorm

//! returns norm_2^2(a)
inline double VecNormSquared(const double &a)
{
	return a*a;
}

//! calculates s += norm_2^2(a)
inline void VecNormSquaredAdd(const double &a, double &s)
{
	s += a*a;
}


// templated

// operations for vectors
//-----------------------------------------------------------------------------
// these functions execute vector operations by using the operations on the elements of the vector

// todo: change vector_t to TE_VEC<vector_t>


// VecScale: These function calculate dest = sum_i alpha_i v_i

//! calculates dest = alpha1*v1
template<typename vector_t>
inline void VecScaleAssign(vector_t &dest, double alpha1, const vector_t &v1)
{
	for(size_t i=0; i<dest.size(); i++)
		VecScaleAssign(dest[i], alpha1, v1[i]);
}

//! calculates dest = alpha1*v1 + alpha2*v2
template<typename vector_t>
inline void VecScaleAdd(vector_t &dest, double alpha1, const vector_t &v1, double alpha2, const vector_t &v2)
{
	for(size_t i=0; i<dest.size(); i++)
		VecScaleAdd(dest[i], alpha1, v1[i], alpha2, v2[i]);
}

//! calculates dest = alpha1*v1 + alpha2*v2 + alpha3*v3
template<typename vector_t>
inline void VecScaleAdd(vector_t &dest, double alpha1, const vector_t &v1, double alpha2, const vector_t &v2, double alpha3, const vector_t &v3)
{
	for(size_t i=0; i<dest.size(); i++)
		VecScaleAdd(dest[i], alpha1, v1[i], alpha2, v2[i], alpha3, v3[i]);
}


// VecProd

//! calculates s += scal<a, b>
template<typename vector_t>
inline void VecProd(const vector_t &a, const vector_t &b, double &sum)
{
	for(size_t i=0; i<a.size(); i++)
		VecProdAdd(a[i], b[i], sum);
}

//! returns scal<a, b>
template<typename vector_t>
inline double VecProd(const vector_t &a, const vector_t &b)
{
	double sum=0;
	VecProdAdd(a, b, sum);
	return sum;
}


//! calculates s += norm_2^2(a)
template<typename vector_t>
inline void VecNormSquaredAdd(const vector_t &a, const vector_t &b, double &sum)
{
	for(int i=0; i<a.size(); i++)
		VecNormSquaredAdd(a[i], sum);
}

//! returns norm_2^2(a)
template<typename vector_t>
inline double VecNormSquared(const vector_t &a, const vector_t &b)
{
	double sum=0;
	VecNormSquaredAdd(a, sum);
	return sum;
}


} // namespace ug

#endif /* __H__UG__LIB_ALGEBRA__OPERATIONS_VEC__ */
