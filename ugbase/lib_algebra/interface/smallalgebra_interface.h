
#ifndef __H__UG__MARTIN_ALGEBRA__BLOCKS__
#define __H__UG__MARTIN_ALGEBRA__BLOCKS__

#include <ostream>

namespace ug{

template <typename t> struct block_traits;
template<typename value_type, typename vec_type> struct block_multiply_traits;


//////////////////////////////////////////////////////

template<typename TYPE>
inline double BlockNorm2(const TYPE &v);


template<typename TYPE>
inline double BlockNorm(const TYPE &v);


//////////////////////////////////////////////////////

// BlockRef: get/set vector
// since double does not support operator [], we need this function
// mostly it will be "assert(i==0); return vec;" or "return vec[i];"
template<typename T> inline double &BlockRef(T &vec, size_t i);
// const version
template<typename T> inline const double &BlockRef(const T &vec, size_t i);

// BlockRef: get/set matrix
// same thing for matrices
template<typename T> inline double &BlockRef(T &mat, size_t i, size_t j);
// const version
template<typename T> inline const double &BlockRef(const T &mat, size_t i, size_t j);
//////////////////////////////////////////////////////


//////////////////////////////////////////////////////
// SetSize(t, a, b) for matrices
template<typename T> inline void SetSize(T &t, size_t a, size_t b);

// SetSize(t, a) for vectors
template<typename T> inline void SetSize(T &t, size_t a);

// GetSize
template<typename T> inline size_t GetSize(const T &t);

template<typename T> inline size_t GetRows(const T &t);

template<typename T> inline size_t GetCols(const T &t);



///////////////////////////////////////////////////////////////////
// traits: information for numbers


template<typename T>
struct block_traits<T>
{
	typedef T1 vec_type;

	// inverse_type: specify this type so we know what type to use
	// INSTEAD of inverting the matrix (e.g. can be LU decomposition or Jacobi)
	typedef T2 inverse_type;

	// is_static is used in several functions. if so,
	// the type has static number of rows/cols and can be copied via memcpy
	// (that means no pointers used).
	enum { is_static = true};

	enum { static_num_rows = 1};
	enum { static_num_cols = 1};
	enum { static_size = 1 };
};

/*
 * used to determine the result of the muliplication of two values
 */
template<typename T>
struct block_multiply_traits<T1, T2>
{
	typedef T1 ReturnType;
};

/*
 * initializes the inverse inv of m. This function gets the inverse type of m
 * by using block_traits<T>::inverse_type. Please note that the inverse_type and
 * T need not be the same. Examples:
 * T = dense matrix, inverse_type = LU decompositions
 * T = small sparse matrix, inverse_type = couple of Jacobi steps.
 * T = double, inverse_type = double
 * T = complex, inverse_type = complex
 *
 * The inverse_type needs only to support the MatMult and MatMultAdd functions,
 * and you can also do all work there (like when you have a sparse matrix block and
 * want to use gauss-seidel on this block, then GetInverse just sets a pointer to
 * the sparse matrix).
 *
 */
template<typename T>
inline bool GetInverse(block_traits<T>::inverse_type &inv, const T &m);


/*
 * This functions really inverts m.
 * Use this function with care, because it is possible that some structures do not support it
 * (like sparse matrices).
 */
template<typename T>
inline bool Invert(T &m);

//! you can implement this function with GetInverse and MatMult
// dest = beta * mat^{-1} vec
template<typename TMat, typename TVec>
inline bool InverseMatMult(number &dest, const double &beta, const TMat &mat, const TVec &vec);


} // namespace ug


