/*
 * multi_index.h
 *
 *  Created on: 22.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIB_DISCRETIZATION__COMMON__MULTI_INDEX__
#define __H__LIB_DISCRETIZATION__COMMON__MULTI_INDEX__

#include <vector>
#include <iostream>
#include <assert.h>

#include "common/common.h"

namespace ug{

/**
 * A MultiIndex is just a vector of integers.
 */
template<int N, typename TSingleIndexType = size_t>
class MultiIndex
{
	public:
		typedef TSingleIndexType single_index_type;

	public:
		/// number of indices in multi index
		inline size_t size() const {return N;}

		/// access to index component
		inline single_index_type& operator[] (size_t i)
		{
			UG_ASSERT(i < N, "Index invalid");
			return m_indices[i];
		}

		/// const access to index component
		inline const single_index_type& operator[] (size_t i) const
		{
			UG_ASSERT(i < N, "Index invalid");
			return m_indices[i];
		}

		///	comparison operator
		bool operator==(const MultiIndex& o) const
		{
			for(size_t i=0; i < N; ++i)
				if(m_indices[i] != o[i]) return false;
			return true;
		}

		bool operator!=(const MultiIndex& o) const
		{
			return !(*this==o);
		}

	private:
		single_index_type m_indices[N];
};

// specialization of 1
template <>
class MultiIndex<1, size_t>
{
	public:
		typedef size_t single_index_type;

	public:
	///	Default constructor
		MultiIndex(){};

	///	Constructor with values
		MultiIndex(single_index_type a)
			: m_indices(a)
		{};

		/// number of indices in multi index
		inline size_t size() const {return 1;}

		/// access to index component
		inline single_index_type& operator[] (size_t i)
		{
			UG_ASSERT(i == 0, "Index invalid");
			return m_indices;
		}

		/// const access to index component
		inline const single_index_type& operator[] (size_t i) const
		{
			UG_ASSERT(i == 0, "Index invalid");
			return m_indices;
		}

		///	comparison operator
		bool operator==(const MultiIndex& o) const
		{
			return m_indices == o[0];
		}

		bool operator!=(const MultiIndex& o) const
		{
			return !(*this==o);
		}

	private:
		single_index_type m_indices;
};

// specialization of 2
template <>
class MultiIndex<2, size_t>
{
	public:
		typedef size_t single_index_type;

	public:
	///	Default constructor
		MultiIndex(){};

	///	Constructor with values
		MultiIndex(single_index_type a, single_index_type b)
			: i0(a), i1(b)
		{}

		/// number of indices in multi index
		inline size_t size() const {return 2;}

		/// access to index component
		inline single_index_type& operator[] (size_t i)
		{
			UG_ASSERT(i < 2, "Index invalid");
			return m_indices[i];
		}

		/// const access to index component
		inline const single_index_type& operator[] (size_t i) const
		{
			UG_ASSERT(i < 2, "Index invalid");
			return m_indices[i];
		}

		///	comparison operator
		bool operator==(const MultiIndex& o) const
		{
			return (m_indices[0] == o[0]) && (m_indices[1] == o[1]);
		}

		bool operator!=(const MultiIndex& o) const
		{
			return !(*this==o);
		}


	private:
		union
		{
			struct
			{
				single_index_type i0, i1;
			};
			single_index_type m_indices[2];
		};
};

// specialization of 3
template <>
class MultiIndex<3, size_t>
{
	public:
		typedef size_t single_index_type;

	public:
	///	Default constructor
		MultiIndex(){};

	///	Constructor with values
		MultiIndex(single_index_type a, single_index_type b, single_index_type c)
			: i0(a), i1(b), i2(c)
		{}

		/// number of indices in multi index
		inline size_t size() const {return 3;}

		/// access to index component
		inline single_index_type& operator[] (size_t i)
		{
			UG_ASSERT(i < 3, "Index invalid");
			return m_indices[i];
		}

		/// const access to index component
		inline const single_index_type& operator[] (size_t i) const
		{
			UG_ASSERT(i < 3, "Index invalid");
			return m_indices[i];
		}

		///	comparison operator
		bool operator==(const MultiIndex& o) const
		{
			return 	(m_indices[0] == o[0]) &&
					(m_indices[1] == o[1]) &&
					(m_indices[2] == o[2]);
		}

		bool operator!=(const MultiIndex& o) const
		{
			return !(*this==o);
		}

	private:
		union
		{
			struct
			{
				single_index_type i0, i1, i2;
			};
			single_index_type m_indices[3];
		};
};

template <int N>
std::ostream& operator<< (std::ostream& outStream, const ug::MultiIndex<N>& v)
{
	outStream << "[" ;
	for(size_t i = 0; i < N; ++i)
	{
		outStream << v[i];
		if(i != N-1) outStream << ",";
	}
	outStream << "]";
	return outStream;
}

}


#endif /* __H__LIB_DISCRETIZATION__COMMON__MULTI_INDEX__ */
