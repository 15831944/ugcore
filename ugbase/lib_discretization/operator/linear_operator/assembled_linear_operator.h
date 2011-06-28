/*
 * assembled_linear_operator.h
 *
 *  Created on: ..
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__OPERATOR__LINEAR_OPERATOR__ASSEMBLED_LINEAR_OPERATOR__
#define __H__LIBDISCRETIZATION__OPERATOR__LINEAR_OPERATOR__ASSEMBLED_LINEAR_OPERATOR__

#include "lib_algebra/operator/operator_interface.h"

#ifdef UG_PARALLEL
#include "lib_discretization/parallelization/parallelization_util.h"
#endif

namespace ug{

///	matrix operator based on the assembling of a problem
/**
 * This operator implements the MatrixOperator interface, thus is basically a
 * matrix that can be applied to vectors. In addition the class allows to set
 * an IAssemble object and an IDoFDistribution. Invoking the init method the
 * matrix is created using the IAssemble routine on the given DoFDistribution.
 *
 * \tparam	TDoFDistribution	dof distribution type
 * \tparam	TAlgebra			algebra type
 */
template <typename TDoFDistribution, typename TAlgebra>
class AssembledLinearOperator :
	public virtual MatrixOperator<	typename TAlgebra::vector_type,
									typename TAlgebra::vector_type,
									typename TAlgebra::matrix_type>
{
	public:
	///	Type of Algebra
		typedef TAlgebra algebra_type;

	///	Type of Vector
		typedef typename TAlgebra::vector_type vector_type;

	///	Type of Matrix
		typedef typename TAlgebra::matrix_type matrix_type;

	///	Type of DoFDistribution
		typedef IDoFDistribution<TDoFDistribution> dof_distribution_type;

	///	Type of base class
		typedef MatrixOperator<vector_type,vector_type,matrix_type> base_type;

	public:
	///	Default Constructor
		AssembledLinearOperator() :
			m_pAss(NULL), m_pDoFDistribution(NULL)
			{};

	///	Constructor
		AssembledLinearOperator(IAssemble<TDoFDistribution, algebra_type>& ass)
			:	m_pAss(&ass), m_pDoFDistribution(NULL)
		{};

	///	sets the discretization to be used
		void set_discretization(IAssemble<TDoFDistribution, algebra_type>& ass)
			{m_pAss = &ass;}

	///	sets the dof distribution used for assembling
		void set_dof_distribution(const dof_distribution_type& dofDistr)
			{m_pDoFDistribution = &dofDistr;}

	///	returns the dof distribution
		const dof_distribution_type* get_dof_distribution()
			{return m_pDoFDistribution;}

	///	initializes the operator that may depend on the current solution
		virtual bool init(const vector_type& u);

	///	initialize the operator
		virtual bool init();

	///	initializes the operator and assembles the passed rhs vector
		bool init_op_and_rhs(vector_type& b);

	///	compute d = J(u)*c (here, J(u) is a Matrix)
		virtual bool apply(vector_type& d, const vector_type& c);

	///	Compute d := d - J(u)*c
		virtual bool apply_sub(vector_type& d, const vector_type& c);

	///	Set Dirichlet values
		bool set_dirichlet_values(vector_type& u);

	/// forces the disc to consider the grid as regular
		void force_regular_grid(bool bForce)
			{if(m_pAss != NULL) m_pAss->force_regular_grid(bForce);}

	///	Destructor
		virtual ~AssembledLinearOperator() {};

	protected:
	// 	assembling procedure
		IAssemble<TDoFDistribution, algebra_type>* m_pAss;

	// 	DoF Distribution used
		const dof_distribution_type* m_pDoFDistribution;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// help function to assemble a linear operator
/**
 * This function initializes the operator, sets the vector b to the computed rhs
 * and sets the dirichlet post processes for the vector u.
 *
 * \param[out]	op		Operator
 * \param[out]	u		Solution
 * \param[out]	b		Rigth-Hand side vector
 *
 * \tparam	TDoFDistribution	dof distribution type
 * \tparam	TAlgebra			algebra type
 */
template <typename TDoFDistribution, typename TAlgebra>
bool AssembleLinearOperatorRhsAndSolution
		(AssembledLinearOperator<TDoFDistribution, TAlgebra>& op,
		 typename TAlgebra::vector_type& u,
		 typename TAlgebra::vector_type& b);

} // namespace ug

// include implementation
#include "assembled_linear_operator_impl.h"

#endif /* __H__LIBDISCRETIZATION__OPERATOR__LINEAR_OPERATOR__ASSEMBLED_LINEAR_OPERATOR__ */
