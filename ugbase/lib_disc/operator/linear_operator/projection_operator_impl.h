/*
 * projection_operator_impl.h
 *
 *  Created on: 01.08.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROJECTION_OPERATOR_IMPL__
#define __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROJECTION_OPERATOR_IMPL__

#include "projection_operator.h"

namespace ug{

/**
 * This functions assembles the interpolation matrix between to
 * grid levels using only the Vertex degrees of freedom.
 *
 * \param[out]	mat 			Assembled interpolation matrix that interpolates u -> v
 * \param[in] 	approxSpace		Approximation Space
 * \param[in]	coarseLevel		Coarse Level index
 * \param[in]	fineLevel		Fine Level index
 */
template <typename TDD, typename TAlgebra>
void AssembleVertexProjection(typename TAlgebra::matrix_type& mat,
                              const TDD& coarseDD, const TDD& fineDD)
{
//  Allow only lagrange P1 functions
	for(size_t fct = 0; fct < fineDD.num_fct(); ++fct)
		if(fineDD.local_finite_element_id(fct) != LFEID(LFEID::LAGRANGE, 1))
			UG_THROW("AssembleVertexProjection: "
					"Interpolation only implemented for Lagrange P1 functions.");

// 	get MultiGrid
	const MultiGrid& grid = coarseDD.multi_grid();

// 	get number of dofs on different levels
	const size_t numFineDoFs = fineDD.num_indices();
	const size_t numCoarseDoFs = coarseDD.num_indices();

// 	resize matrix
	if(!mat.resize(numCoarseDoFs, numFineDoFs))
		UG_THROW("AssembleVertexProjection: "
				"Cannot resize Interpolation Matrix.");

	std::vector<size_t> coarseInd, fineInd;

// 	Vertex iterators
	typedef typename TDD::template traits<VertexBase>::const_iterator const_iterator;
	const_iterator iter, iterBegin, iterEnd;

// 	loop subsets of fine level
	for(int si = 0; si < fineDD.num_subsets(); ++si)
	{
		iterBegin = fineDD.template begin<Vertex>(si);
		iterEnd = fineDD.template end<Vertex>(si);

	// 	loop nodes of fine subset
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		// 	get father
			GeometricObject* geomObj = grid.get_parent(*iter);
			VertexBase* vert = dynamic_cast<VertexBase*>(geomObj);

		//	Check if father is Vertex
			if(vert != NULL)
			{
				// get global indices
				coarseDD.inner_algebra_indices(vert, coarseInd);
			}
			else continue;

		// 	get global indices
			fineDD.inner_algebra_indices(*iter, fineInd);

			for(size_t i = 0; i < coarseInd.size(); ++i)
				mat(coarseInd[i], fineInd[i]) = 1.0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 	InjectionTransfer
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
set_approximation_space(SmartPtr<ApproximationSpace<TDomain> > approxSpace)
{
	m_spApproxSpace = approxSpace;
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
set_levels(GridLevel coarseLevel, GridLevel fineLevel)
{
	m_fineLevel = fineLevel;
	m_coarseLevel = coarseLevel;

	if(m_fineLevel.level() - m_coarseLevel.level() != 1)
		UG_THROW("InjectionTransfer::set_levels:"
				" Can only project between successive level.");

	if(m_fineLevel.type() != GridLevel::LEVEL ||
	   m_coarseLevel.type() != GridLevel::LEVEL)
		UG_THROW("InjectionTransfer::set_levels:"
				" Can only project between level dof distributions.");
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::init()
{
	if(!m_spApproxSpace.valid())
		UG_THROW("InjectionTransfer::init: "
				"Approximation Space not set. Cannot init Projection.");

	m_matrix.resize(0,0);

	try{
	if(m_coarseLevel.type() == GridLevel::LEVEL)
		AssembleVertexProjection<LevelDoFDistribution, algebra_type>
		(m_matrix,
		 *m_spApproxSpace->level_dof_distribution(m_coarseLevel.level()),
		 *m_spApproxSpace->level_dof_distribution(m_fineLevel.level()));
	} UG_CATCH_THROW("TransferOperator::init():"
				" Cannot assemble interpolation matrix.");

	#ifdef UG_PARALLEL
		m_matrix.set_storage_type(PST_CONSISTENT);
	#endif

	m_bInit = true;
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
prolongate(vector_type& uFine, const vector_type& uCoarse)
{
//	Check, that operator is initiallized
	if(!m_bInit)
		UG_THROW("InjectionTransfer::apply:"
				" Operator not initialized.");

//	Some Assertions
	UG_ASSERT(uFine.size() == m_matrix.num_rows(),
			  "Vector [size= " << uFine.size() << "] and Rows [size= "
			  << m_matrix.num_rows() <<"] sizes have to match!");
	UG_ASSERT(uCoarse.size() == m_matrix.num_cols(),	"Vector [size= "
			  << uCoarse.size() << "] and Cols [size= " <<
			  m_matrix.num_cols() <<"] sizes have to match!");

//	Apply matrix
	if(!m_matrix.apply_transposed(uFine, uCoarse))
		UG_THROW("InjectionTransfer::apply: Cannot apply matrix.");
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
restrict(vector_type& uCoarse, const vector_type& uFine)
{
//	Check, that operator is initialized
	if(!m_bInit)
		UG_THROW("InjectionTransfer::apply_transposed:"
				"Operator not initialized.");

//	Some Assertions
	UG_ASSERT(uFine.size() == m_matrix.num_cols(),
			  "Vector [size= " << uFine.size() << "] and Cols [size= "
			  << m_matrix.num_cols() <<"] sizes have to match!");
	UG_ASSERT(uCoarse.size() == m_matrix.num_rows(),	"Vector [size= "
			  << uCoarse.size() << "] and Rows [size= " <<
			  m_matrix.num_rows() <<"] sizes have to match!");

//	Apply matrix
	if(!m_matrix.apply(uCoarse, uFine))
		UG_THROW("InjectionTransfer::apply_transposed:"
				" Cannot apply transposed matrix.");
}

template <typename TDomain, typename TAlgebra>
SmartPtr<ITransferOperator<TAlgebra> >
InjectionTransfer<TDomain, TAlgebra>::clone()
{
	SmartPtr<InjectionTransfer> op(new InjectionTransfer);
	op->set_approximation_space(m_spApproxSpace);
	for(size_t i = 0; i < m_vConstraint.size(); ++i)
		op->add_constraint(m_vConstraint[i]);
	return op;
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
add_constraint(SmartPtr<IConstraint<TAlgebra> > pp)
{
	UG_THROW("Not Implemented.");
//	add only once
	if(std::find(m_vConstraint.begin(), m_vConstraint.end(), pp) !=
			m_vConstraint.end()) return;
	m_vConstraint.push_back(pp);
}

template <typename TDomain, typename TAlgebra>
void InjectionTransfer<TDomain, TAlgebra>::
remove_constraint(SmartPtr<IConstraint<TAlgebra> > pp)
{
	UG_THROW("Not Implemented.");
	m_vConstraint.erase(m_vConstraint.begin(),
	                     std::remove(m_vConstraint.begin(), m_vConstraint.end(), pp));
}


} // end namespace ug

#endif /* __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROJECTION_OPERATOR_IMPL__ */
