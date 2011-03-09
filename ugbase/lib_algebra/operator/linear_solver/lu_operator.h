/*
 * lu_operator.h
 *
 *  Created on: 16.06.2010
 *      Author: mrupp
 */

#ifndef __H__LIB_ALGEBRA__LAPACK_LU_OPERATOR__
#define __H__LIB_ALGEBRA__LAPACK_LU_OPERATOR__
#include <iostream>
#include <sstream>

#include "lib_algebra/operator/operator_inverse_interface.h"

#ifdef UG_PARALLEL
	#include "lib_algebra/parallelization/parallelization.h"
#endif

namespace ug{

template <typename TAlgebra>
class LUSolver : public IMatrixOperatorInverse<	typename TAlgebra::vector_type,
														typename TAlgebra::vector_type,
														typename TAlgebra::matrix_type>
{
	public:
	// 	Algebra type
		typedef TAlgebra algebra_type;

	// 	Vector type
		typedef typename TAlgebra::vector_type vector_type;

	// 	Matrix type
		typedef typename TAlgebra::matrix_type matrix_type;

	public:
		LUSolver() :
			m_pOperator(NULL), m_mat(), m_pConvCheck(NULL)
		{};

		virtual const char* name() const {return "LUSolver";}

		void set_convergence_check(IConvergenceCheck& convCheck)
		{
			m_pConvCheck = &convCheck;
			m_pConvCheck->set_offset(3);
			m_pConvCheck->set_symbol('%');
			m_pConvCheck->set_name("LU Solver");
		}
		IConvergenceCheck* get_convergence_check() {return m_pConvCheck;}

		bool init_lu(const matrix_type &A)
		{
			if(block_traits<typename vector_type::value_type>::is_static)
			{
				const size_t nrOfRows = block_traits<typename matrix_type::value_type>::static_num_rows;
				UG_ASSERT(nrOfRows == block_traits<typename matrix_type::value_type>::static_num_cols, "only square matrices supported");
				m_size = A.num_rows() * nrOfRows;

				m_mat.resize(m_size);

				for(size_t r=0; r<A.num_rows(); r++)
					for(typename matrix_type::const_row_iterator it = A.begin_row(r); it != A.end_row(r); ++it)
					{
						size_t rr = r*nrOfRows;
						size_t cc = (*it).iIndex*nrOfRows;
						for(size_t r2=0; r2<nrOfRows; r2++)
								for(size_t c2=0; c2<nrOfRows; c2++)
								  m_mat(rr + r2, cc + c2) = BlockRef((*it).dValue, r2, c2);
					}
			}
			else
			{
				// not tested yet
				m_size = 0;
				std::vector<size_t> blockbegin(A.num_rows()+1);

				for(size_t i=0; i<A.num_rows(); i++)
				{
					bool bFound;
					typename matrix_type::const_row_iterator it = A.get_connection(i,i, bFound);
					UG_ASSERT(bFound, "Matrix has to have entry A(" << i << ", " << i << ")");
					size_t s = GetRows((*it).dValue);
					UG_ASSERT(s == GetCols((*it).dValue), "diagonal elements have to be square");
					if(i == 0)
					blockbegin[i] = m_size;
					m_size += s;
				}
				blockbegin[A.num_rows()] = m_size;

				m_mat.resize(m_size);

				for(size_t r=0; r<A.num_rows(); r++)
					for(typename matrix_type::const_row_iterator it = A.begin_row(r); it != A.end_row(r); ++it)
					{
						size_t c = (*it).iIndex;
						const typename matrix_type::value_type &val = (*it).dValue;
						UG_ASSERT(blockbegin[r]+GetRows(val) == blockbegin[r+1], "blocksizes in matrix inconsistent");
						UG_ASSERT(blockbegin[c]+GetCols(val) == blockbegin[c+1], "blocksizes in matrix inconsistent");
						for(size_t r2=0; r2 < GetRows(val); r2++)
							for(size_t c2=0; c2 < GetCols(val); c2++)
								m_mat(blockbegin[r] + r2, blockbegin[c]+c2) = BlockRef(val, r2, c2);
					}

			}

			return m_mat.invert();
		}

		bool apply_lu(vector_type &x, const vector_type &b)
		{
#ifndef NDEBUG
			if(block_traits<typename vector_type::value_type>::is_static)
			{
				const size_t static_size = block_traits<typename vector_type::value_type>::static_size;
				UG_ASSERT(m_size == b.size() * static_size && m_size == x.size() * static_size,
						" wrong size! has to be " << m_size << ", but is " << b << " and " << x);
			}
			else
			{
				size_t b_size = 0;
				for(size_t i=0; i<b.size(); i++)
				{
					UG_ASSERT(GetSize(b[i]) == GetSize(x[i]), "wrong size! Sizes of b and x must be the same, but is "
							<< GetSize(b[i]) << " and " << GetSize(x[i]) << "!");
					b_size += GetSize(b[i]);
				}
				UG_ASSERT(m_size == b_size, " wrong size! has to be " << m_size << ", but is " << b_size << "!");
			}
#endif

			x = b;
			m_tmp.resize(m_size);
			for(size_t i=0, k=0; i<b.size(); i++)
			{
				for(size_t j=0; j<GetSize(b[i]); j++)
					m_tmp[k++] = BlockRef(b[i],j);
			}
			m_mat.apply(m_tmp);

			for(size_t i=0, k=0; i<b.size(); i++)
			{
				for(size_t j=0; j<GetSize(b[i]); j++)
					BlockRef(x[i],j) = m_tmp[k++];
			}

			return true;
		}

	//	set operator L, that will be inverted
		virtual bool init(IMatrixOperator<vector_type, vector_type, matrix_type>& Op)
		{
		// 	remember operator
			m_pOperator = &Op;

		//	get matrix of Operator
			m_pMatrix = &m_pOperator->get_matrix();

		//	check that matrix exist
			if(m_pMatrix == NULL)
				{UG_LOG("ERROR in LUOperator::init: No Matrix given,\n"); return false;}

		//	init LU operator
			if(!init_lu(*m_pMatrix))
				{UG_LOG("ERROR in LUOperator::init: Cannot init LU Decomposition.\n"); return false;}

		//	we're done
			return true;
		}

	// 	Compute u = L^{-1} * f
		virtual bool apply(vector_type& u, const vector_type& f)
		{
#ifdef UG_PARALLEL
			if(!f.has_storage_type(PST_ADDITIVE))
			{
				UG_LOG("ERROR: In 'LaplackLUSolver::apply':Inadequate storage format of Vector f.\n");
				return false;
			}
			if(!u.has_storage_type(PST_CONSISTENT))
			{
				UG_LOG("ERROR: In 'LaplackLUSolver::apply':Inadequate storage format of Vector u.\n");
				return false;
			}
#endif
			UG_ASSERT(f.size() == m_pMatrix->num_rows(),	"Vector and Row sizes have to match!");
			UG_ASSERT(u.size() == m_pMatrix->num_cols(), "Vector and Column sizes have to match!");
			UG_ASSERT(f.size() == u.size(), "Vector sizes have to match!");

			// TODO: This must be inverted
			if(!apply_lu(u, f))
				{UG_LOG("ERROR in LUOperator::apply: Cannot apply LU decomposition.\n"); return false;}

#ifdef UG_PARALLEL
			// todo: we set solution to consistent here, but that is only true for
			//			serial case. Handle parallel case.
			u.set_storage_type(PST_CONSISTENT);
#endif

		//	we're done
			return true;
		}

	// 	Compute u = L^{-1} * f AND return defect f := f - L*u
		virtual bool apply_return_defect(vector_type& u, vector_type& f)
		{
		//	solve u
			if(!apply(u, f)) return false;

		//	update defect
			if(!m_pMatrix->matmul_minus(f, u))
			{
				UG_LOG("ERROR in 'LUSolver::apply_return_defect': Cannot apply matmul_minus.\n");
				return false;
			}

		//	we're done
			return true;
		}

	// 	Destructor
		virtual ~LUSolver() {};

	protected:
		// Operator to invert
		IMatrixOperator<vector_type, vector_type, matrix_type>* m_pOperator;

		// matrix to invert
		matrix_type* m_pMatrix;

		// inverse
		DenseMatrixInverse<DenseMatrix<VariableArray2<double> > > m_mat;
		DenseVector<VariableArray1<double> > m_tmp;
		size_t m_size;

		// Convergence Check
		IConvergenceCheck* m_pConvCheck;
};

} // end namespace ug

#endif /* __H__LIB_ALGEBRA__LAPACK_LU_OPERATOR__ */
