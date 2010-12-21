/**
 * \file math_matrix.h
 *
 * \author Andreas Vogel
 *
 * \date 08.07.2009
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include <cstddef>
#include <ostream>
#include <iomanip>
#include "../../types.h"

namespace ug
{

//template <std::size_t N, std::size_t M, typename T = number> class MathMatrix;

/**
 * \class MathMatrix
 *
 * \brief A class for fixed size, dense matrices.
 *
 *	A static memory NxM matrix
 */
template <std::size_t N, std::size_t M, typename T = number>
class MathMatrix
{
	public:
		typedef T value_type;
		typedef std::size_t size_type;
		static const std::size_t RowSize = N;
		static const std::size_t ColSize = M;

	public:
		MathMatrix() {}
		MathMatrix(const MathMatrix& v)	{assign(v);}

		/**
		 * \brief Assigns the elements of the given matrix to this one.
		 *
		 * \param v The matrix to be assigned.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator=  (const MathMatrix& v)
		{
			assign(v);
			return *this;
		}

		/**
		 * \brief Adds a matrix to 'this' one: \f$ A_{this} \leftarrow A_{this} + B\f$.
		 *
		 * \param B The matrix to be added.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator+= (const MathMatrix& B)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] += B(i,j);
				}
			}
			return *this;
		}

		/**
		 * \brief Subtracts a matrix from 'this' one: \f$ A_{this} \leftarrow A_{this} - B\f$.
		 *
		 * \param B The matrix to be subtracted.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator-= (const MathMatrix& B)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] -= B(i,j);
				}
			}
			return *this;
		}

		/**
		 * \brief Assigns the given value to all elements of the matrix.
		 *
		 * \param val The value to be assigned to the matrix.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator= (const value_type& val)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] = val;
				}
			}
			return *this;
		}

		/**
		 * \brief Adds the given value to all elements of the matrix.
		 *
		 * \param val The value to be added.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator+= (const value_type& val)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] += val;
				}
			}
			return *this;
		}

		/**
		 * \brief Subtracts the given value from all elements of the matrix.
		 *
		 * \param val The value to be subtracted.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator-= (const value_type& val)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] -= val;
				}
			}
			return *this;
		}

		/**
		 * \brief Divides all elements of the matrix by the given value.
		 *
		 * \param val The divisor.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator/= (const value_type& val)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] /= val;
				}
			}
			return *this;
		}

		/**
		 * \brief Multiplies all elements of the matrix with the given value.
		 *
		 * \param val The factor.
		 * \return A reference to this matrix.
		 */
		MathMatrix& operator*= (const value_type& val)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] *= val;
				}
			}
			return *this;
		}

		/**
		 * \brief Multiplies the matrix element-wise with another matrix and sums up the entries.
		 *
		 * \param v The Matrix.
		 * \return A scalar value of the element-wise summed up products
		 */
		value_type operator* (const MathMatrix& v) const
		{
			value_type res = 0.0;
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					res += m_data[i][j] * v.m_data[i][j];
				}
			}
			return res;
		}

		//inline std::size_t row_size() const {return N;}
		//inline std::size_t col_size() const {return M;}
	
		inline std::size_t num_rows() const {return N;}
		inline std::size_t num_cols() const {return M;}

		inline value_type& entry(std::size_t row, std::size_t col)				{return m_data[row][col];}
		inline const value_type& entry(std::size_t row, std::size_t col) const	{return m_data[row][col];}

		inline value_type* operator[](std::size_t index)				{return m_data[index];}
		inline const value_type* operator[](std::size_t index) const	{return m_data[index];}

		inline value_type& operator() (std::size_t row, std::size_t col)				{return m_data[row][col];}
		inline const value_type& operator() (std::size_t row, std::size_t col) const	{return m_data[row][col];}

	protected:
		value_type m_data[N][M];

		inline void assign(const MathMatrix& v)
		{
			for(std::size_t i = 0; i < N; ++i)
			{
				for(std::size_t j = 0; j < M; ++j)
				{
					m_data[i][j] = v.entry(i,j);
				}
			}
		}

};


/// Print MathMatrix<N,M> to standard output
template <std::size_t N, std::size_t M>
std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<N,M>& m)
{
	for(std::size_t i = 0; i < N; ++i)
	{
		for(std::size_t j = 0; j < M; ++j)
		{
			outStream << "[" << i << "][" << j << "]: " << std::scientific << std::setprecision(8) << std::setw(15) << m.entry(i, j) << std::endl;
		}
	}
	return outStream;
}

std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<2,2>& m);
std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<2,3>& m);
std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<3,2>& m);
std::ostream& operator<< (std::ostream& outStream, const ug::MathMatrix<3,3>& m);


} //end of namespace: lgmath


#endif /* MathMatrix_H_ */
