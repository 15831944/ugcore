/*
 *  fixed_array.h
 *
 *  Created by Martin Rupp on 21.07.10.
 *  Copyright 2010 . All rights reserved.
 *
 */

#ifndef __H__UG__COMMON__FIXED_ARRAY_H__
#define __H__UG__COMMON__FIXED_ARRAY_H__

#include "storage.h"
#include <iostream>
#include <cassert>

/////////////////////////////////////////////////////////////////////////////////////////////
//	FixedArray1
/**
 * FixedArray1 is a one-dimensional array which supports most of the interface of std::vector.
 * (some functions and iterators have to be added).
 * You can use FixedArray1 in DenseVector to get a fixed size mathematical Vector.
 * Use storage_traits<type>::is_static to distinguish between fixed and variable vectors. Functions like
 * resize(newN) assert N == newN. So FixedArray1<T, N>::size is always N.
 * \param T type of object in Array (i.e. double)
 * \param n number of elements in the fixed array (T values[n]; )
 */
namespace ug{

template<typename T, size_t n>
class FixedArray1
{
public:
	typedef T value_type;
	typedef size_t size_type;
	typedef static_type storage_type;

public:
	FixedArray1();
	FixedArray1(size_type n_);
	FixedArray1(const FixedArray1<T, n> &other);

//protected:
	// see Alexandrescu: non-virtual destructors should be protected
	~FixedArray1();

public:
	// capacity
	inline size_type
	size() const;

	inline size_type
	capacity() const;

	inline bool
	resize(size_type newN);

	inline bool
	reserve(size_type newN) const;

	// Element access
	inline const T &
	at(size_type i) const
	{
		return operator[](i);
	}

	inline T &
	at(size_type i)
	{
		return operator[](i);
	}

	inline const T &
	operator[](size_type i) const ;

	inline T &
	operator[](size_type i) ;

	// output
	template<typename _T, size_type _n>
	friend
	std::ostream &
	operator << (std::ostream &out, const FixedArray1<_T, _n> &arr);

protected:
	T values[n];
};

template<typename T, size_t N>
struct storage_traits1<FixedArray1<T, N> >
{
	enum {is_static = true};
	enum {static_size = N};
};


/////////////////////////////////////////////////////////////////////////////////////////////
//	FixedArray2
/**
 * FixedArray2 is a two-dimensional array which supports a similar interface like stl::vector.
 * You can use FixedArray2 in GeMatrix to get a fixed size Matrix.
 * Use is_static to distinguish between fixed and variable arrays.
  * \param T type of object in Array (i.e. double)
 * \param colsT fixed number of columns
 * \param rowsT fixed number of rows
 * \param T_ordering Ordering of columns/rows. Default is ColMajor. \sa eMatrixOrdering
 */
template<typename T, size_t rowsT, size_t colsT, eMatrixOrdering T_ordering=ColMajor>
class FixedArray2
{
public:
	typedef T value_type;
	typedef size_t size_type;
	static const eMatrixOrdering ordering = T_ordering;
	enum { is_static=true};
	enum { static_num_rows=rowsT};
	enum { static_num_cols=colsT};
	typedef static_type storage_type;

public:
	FixedArray2();
	FixedArray2(size_type rows, size_type cols);
	FixedArray2(const FixedArray2<T, rowsT, colsT, T_ordering> &other);

//protected:
	// see Alexandrescu: non-virtual destructors should be protected
	~FixedArray2();

public:
	// capacity
	inline size_type
	num_rows() const;

	inline size_type
	num_cols() const;

	inline bool
	resize(size_type newRows, size_type newCols);

	inline size_type
	capacity_num_rows() const { return rowsT; }

	inline size_type
	capacity_num_cols() const { return colsT; };

	inline bool
	empty() const;

	inline bool
	reserve(size_type nrRows, size_type nrCols) const 	{ return; }


	// Element access
	inline const T &
	at(size_type r, size_type c) const
	{
		// todo: if(r >= rowsT || c >= colsT) throw
		return operator()(r, c);
	}

	inline T &
	at(size_type r, size_type c)
	{
		// todo: if(r >= rowsT || c >= colsT) throw
		return operator()(r, c);
	}

	inline const T &
	operator()(size_type r, size_type c) const ;

	inline T &
	operator()(size_type r, size_type c) ;

	// output
	template<typename a, size_type b, size_type c, eMatrixOrdering d>
	friend
	std::ostream &
	operator << (std::ostream &out, const FixedArray2<a, b, c, d> &arr);

protected:
	T values[rowsT*colsT];
};


}
#include "fixed_array_impl.h"
// #include "fixed_array_specialization.h"

#endif // __H__UG__COMMON__FIXED_ARRAY_H__
