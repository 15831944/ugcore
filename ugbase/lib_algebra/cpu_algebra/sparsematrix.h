/**
 * \file sparsematrix.h
 *
 * \author Martin Rupp
 *
 * \date 26.11.2009
 *
 * Goethe-Center for Scientific Computing 2009-2010.
 */

#ifndef __H__UG__CPU_ALGEBRA__SPARSEMATRIX__
#define __H__UG__CPU_ALGEBRA__SPARSEMATRIX__

#include "math.h"


#include "../common/template_expressions.h"
#include "vector.h"


namespace ug{

/// \addtogroup lib_algebra
///	@{

template<typename value_type> class matrixrow;
template<typename vec_type> class Vector;

/** SparseMatrix
 *  \brief sparse matrix for big, variable sparse matrices.
 *
 *  matrix is stored independent row-wise
 *  When doing discretisation, use the add set and get methods
 *  for dealing with submatrices of A.
 *  For other things you can use the row iterators or
 *  operator()-methods.
 *
 * \sa matrixrow, CreateAsMultiplyOf
 * \param T blocktype
 */
template<typename T>
class SparseMatrix
{
public:
	typedef T value_type;
	enum {rows_sorted=true};

	typedef matrixrow<value_type> row_type;
	typedef matrixrow<value_type> matrixrow_type;


public:
	struct connection
	{
		size_t iIndex;		// index to
		value_type dValue; // smallmatrix value;

		void print(){std::cout << *this;}
		friend std::ostream &operator<<(std::ostream &output, const connection &c)
		{
			output << "(" << c.iIndex << "-> ";
			output << c.dValue;
			output << ")";
			return output;
		}

		void operator = (const connection &other)
		{
			iIndex = other.iIndex;
			dValue = other.dValue;
		}

		int operator < (const connection &c) const
		{
			return iIndex < c.iIndex;
		}
	};

public:
	// construction etc
	//----------------------

	// constructor for empty SparseMatrix
	SparseMatrix();
	// destructor
	~SparseMatrix ();


	bool resize(size_t newRows, size_t newCols);


	//! create this as a transpose of SparseMatrix B
	bool set_as_transpose_of(const SparseMatrix &B, double scale=1.0);

	//! create/recreate this as a copy of SparseMatrix B
	bool set_as_copy_of(const SparseMatrix<T> &B, double scale=1.0);
	SparseMatrix<T> &operator = (const SparseMatrix<T> &B)
	{
		set_as_copy_of(B);
		return *this;
	}


public:
	//! calculate dest = alpha1*v1 + beta1*A*w1 (A = this matrix)
	template<typename vector_t>
	bool axpy(vector_t &dest,
			const number &alpha1, const vector_t &v1,
			const number &beta1, const vector_t &w1) const;

	//! calculate dest = alpha1*v1 + beta1*A^T*w1 (A = this matrix)
	template<typename vector_t>
	bool axpy_transposed(vector_t &dest,
			const number &alpha1, const vector_t &v1,
			const number &beta1, const vector_t &w1) const;


	// DEPRECATED!
	//! calculate res = A x
		// apply is deprecated because of axpy(res, 0.0, res, 1.0, beta, w1)
		template<typename Vector_type>
		bool apply(Vector_type &res, const Vector_type &x) const
		{
			return axpy(res, 0.0, res, 1.0, x);
		}

		//! calculate res = A.T x
		// apply is deprecated because of axpy(res, 0.0, res, 1.0, beta, w1)
		template<typename Vector_type>
		bool apply_transposed(Vector_type &res, const Vector_type &x) const
		{
			return axpy_transposed(res, 0.0, res, 1.0, x);
		}

		// matmult_minus is deprecated because of axpy(res, 1.0, res, -1.0, x);
		//! calculate res -= A x
		template<typename Vector_type>
		bool matmul_minus(Vector_type &res, const Vector_type &x) const
		{
			return axpy(res, 1.0, res, -1.0, x);
		}


	//! isUnconnected: true if only A[i,i] != 0.0.
	inline bool is_isolated(size_t i) const;

	bool scale(double d);
	SparseMatrix<T> &operator *= (double d) { scale(d); return *this; }

	// submatrix set/get functions
	//-------------------------------

	/** Add a local matrix
	 *
	 * The local matrix type must declare the following members:
	 * - num_rows()
	 * - num_cols()
	 * - row_index(size_t i)
	 * - col_index(size_t j)
	 * - operator()(size_t i, size_t j)
	 * so that mat(i,j) will go to SparseMat(mat.row_index(i), mat.col_index(j))
	 * \param mat the whole local matrix type
	 */
	template<typename M>
	void add(const M &mat);
	template<typename M>
	//! set local matrix \sa add
	void set(const M &mat);
	//! get local matrix \sa add
	template<typename M>
	void get(M &mat) const;

	// finalizing functions
	//----------------------
	void defragment();


	//! set matrix to Id*a
	bool set(double a);

	/** operator() (size_t r, size_t c) const
	 * access connection (r, c)
	 * \param r row
	 * \param c column
	 * \note it is assert'ed that connection (r,c) is there
	 * use operator()(r,c,bConnectionFound) to check.
	 * \return SparseMat(r, c)
	 */
	const value_type &operator() (size_t r, size_t c) const;

	/** operator() (size_t r, size_t c) const
	 * access or create connection (r, c)
	 * \param r row
	 * \param c column
	 * \note (r,c) is added to sparsity pattern if not already there
	 * use operator()(r,c,bConnectionFound) to prevent
	 * \return SparseMat(r, c)=0.0 if connection created, otherwise SparseMat(r, c)
	 */
	value_type &operator() (size_t r, size_t c);

public:
	// row functions

	/** set a row of the matrix. all previous content in this row is destroyed (@sa add_matrix_row).
	 * \param row index of the row to set
	 * \param c pointer to a array of sorted connections of size nr
	 * \param nr number of connections in c
	 * \remark connections have to be sorted
	 */
	void set_matrix_row(size_t row, connection *c, size_t nr);

	/** add_matrix_row
	 *  add a row to a matrix row.
	 * \param row index of the row to set
	 * \param c pointer to a array of sorted connections of size nr
	 * \param nr number of connections in c
	 * \remark if we get new connections, matrix is definalized.
	 * \remark connections have to be sorted
	 */
	void add_matrix_row(size_t row, connection *c, size_t nr);

	//! returns number of connections of row row.
	inline size_t num_connections(size_t row) const;

	template<typename vector_t>
	inline void mat_mult_add_row(size_t row, typename vector_t::value_type &dest, double alpha, const vector_t &v) const;
public:
	// accessor functions
	//----------------------

	size_t num_rows() const { return rows; }
	size_t num_cols() const { return cols; }

	size_t total_num_connections() const { return iTotalNrOfConnections; }






public:

	// Iterators
	//---------------------------

	// const_row_iterator


	//typedef const connection * const_row_iterator;
	//typedef connection * const_row_iterator;
	/** const_row_iterator
	 * const iterator over a row
	 */

	// a row_iterator has to suppport
	// operator ++, operator +=, index() const, const value_type &value() const, value_type &value()
	// a const_row_iterator has to suppport
	// operator ++, operator +=, index() const, const value_type &value() const

	/** row_iterator
	 * iterator over a row
	 */

	class row_iterator
	{
	private:
		connection *p;

#ifdef UG_DEBUG
		connection *pEnd;
	public:
		inline row_iterator(connection *pC, connection *pE)
		: p(pC), pEnd(pE)
		  { }

		inline void check() const
		{
			UG_ASSERT(p != pEnd, "iterator is at end.");
		}
#else
	public:
		inline row_iterator(connection *pC, connection *pE)
			: p(pC)
			  { }
		inline void check() const {}
#endif

		inline connection &operator *() const {check(); return *p;}

		inline size_t index() const { check(); return p->iIndex; }
		inline value_type &value() const { check(); return p->dValue; }

		inline void operator ++() {	check(); ++p; }
		inline void operator += (int nr) { check(); p+=nr;}

		bool operator != (const row_iterator &other) const
		{
			return other.p != p;
		}
		bool operator == (const row_iterator &other) const
		{
			return other.p == p;
		}
	};

	class const_row_iterator
	{
	private:
		connection *p;

#ifdef UG_DEBUG
		connection *pEnd;
	public:
		inline const_row_iterator(connection *pC, connection *pE)
		: p(pC), pEnd(pE)
		  { }

		inline void check() const
		{
			UG_ASSERT(p != pEnd, "iterator is at end.");
		}
#else
	public:
		inline const_row_iterator(connection *pC, connection *pE)
			: p(pC)
			  { }
		inline void check() const {}
#endif

		inline const connection &operator *() const {check(); return *p;}

		inline size_t index() const { check(); return p->iIndex; }
		inline const value_type &value() const { check(); return p->dValue; }

		inline void operator ++() {	check(); ++p; }
		inline void operator += (int nr) { check(); p+=nr;}

		bool operator != (const const_row_iterator &other) const
		{
			return other.p != p;
		}
		bool operator == (const const_row_iterator &other) const
		{
			return other.p == p;
		}
	};


	const_row_iterator begin_row(size_t row) const
	{
		UG_ASSERT(row < num_rows(), "cannot access row " << row << " of " << num_rows());
		return const_row_iterator(pRowStart[row], pRowEnd[row]);
	}

	row_iterator begin_row(size_t row)
	{
		UG_ASSERT(row < num_rows(), "cannot access row " << row << " of " << num_rows());
		return row_iterator(pRowStart[row], pRowEnd[row]);
	}

	row_iterator end_row(size_t row)
	{
		UG_ASSERT(row < num_rows(), "cannot access row " << row << " of " << num_rows());
		return row_iterator(pRowEnd[row], pRowEnd[row]);
	}

	const_row_iterator end_row(size_t row) const
	{
		UG_ASSERT(row < num_rows(), "cannot access row " << row << " of " << num_rows());
		return const_row_iterator(pRowEnd[row], pRowEnd[row]);
	}

public:
	// connectivity functions
	//-------------------------
	const_row_iterator get_connection(size_t r, size_t c, bool &bFound) const;
	row_iterator get_connection(size_t r, size_t c, bool &bFound);

	const_row_iterator get_connection(size_t r, size_t c) const;
	row_iterator get_connection(size_t r, size_t c);

public:
	// output functions
	//----------------------

	void print(const char * const name = NULL) const;
	void printtype() const;

	void print_to_file(const char *filename) const;
	void printrow(size_t row) const;

	friend std::ostream &operator<<(std::ostream &out, const SparseMatrix &m)
	{
		out << "SparseMatrix " //<< m.name
		<< " [ " << m.rows << " x " << m.cols << " ]";
		return out;
	}


	void p() const { print(); } // for use in gdb
	void pr(size_t row) const {printrow(row); } // for use in gdb

private:
	// disallowed operations (not defined):
	//---------------------------------------
	SparseMatrix(SparseMatrix&); ///< disallow copy operator

private:
	bool create(size_t _rows, size_t _cols);
	bool destroy();

	void definalize();
	void finalize()
	{
		defragment();
	}
	bool is_finalized() const;

	// safe_set_connections
	/**
	 * "safe" way to set a connection, since when cons[row] is in the big consecutive consmem-array,
	 * you mustnt delete[] mem.
	 */
	void safe_set_connections(size_t row, connection *mem) const;
	//! returns true if the row is stored inside the consecutive mem.
	bool in_consmem(size_t row) const;




private:
	enum get_connection_nr_flag
	{
		// find the first (in terms of distance) which is
		LESS =1,
		LESS_EQUAL = 2,
		EQUAL = 3,
		GREATER_EQUAL = 4,
		GREATER = 5
		// than the connection (r,c)
	};

	bool get_connection_nr(size_t r, size_t c, size_t &nr, get_connection_nr_flag flag=EQUAL) const;

	template<get_connection_nr_flag flag>
	bool get_connection_nr_templ(size_t r, size_t c, size_t &nr) const;


public:
	//     data
	//----------------------

private:
	size_t rows;						//!< nr of rows
	size_t cols;						//!< nr of cols
	connection **pRowStart;				//< pointers to array of beginning of connections of each row
	connection **pRowEnd;				//< pointers to array of ends of connections of each row

	size_t iTotalNrOfConnections;		//!< number of connections ("non-zeros")
	size_t bandwidth;					//!< bandwidth (experimental)

	size_t estimatedRowSize;			//!< estimated length of each row
	size_t *iMaxNrOfConnections;		//!< max nr of connections for row [i]. TODO.


	connection *consmem;				//!< consecutive memory for the connections
	size_t consmemsize;					//!< size of the consecutive memory for connections
	size_t iFragmentedMem;				//!< size of connections memory not in consmem

	bool m_bIgnoreZeroes;				//!< if set, add and set wont set connections which are zero

	friend class matrixrow<value_type>;
};


template<typename T>
struct matrix_algebra_type_traits<SparseMatrix<T> >
{
	static const int type = MATRIX_USE_ROW_FUNCTIONS;
};

//! calculates dest = alpha1*v1 + beta1 * A1^T *w1;
template<typename vector_t, typename matrix_t>
inline void MatMultTransposedAdd(vector_t &dest,
		const number &alpha1, const vector_t &v1,
		const number &beta1, const SparseMatrix<matrix_t> &A1, const vector_t &w1)
{
	A1.axpy_transposed(dest, alpha1, v1, beta1, w1);
}

///	@}
} // namespace ug

//#include "matrixrow.h"
#include "sparsematrix_impl.h"
#include "sparsematrix_util.h"
#include "sparsematrix_print.h"

#endif
