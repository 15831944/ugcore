/*
 * prolongation_operator_impl.h
 *
 *  Created on: 04.12.2009
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR_IMPL__
#define __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR_IMPL__

#include "prolongation_operator.h"
#include "lib_disc/reference_element/reference_mapping_provider.h"
#include "lib_disc/local_finite_element/local_shape_function_set.h"

namespace ug{

template <typename TDD, typename TAlgebra>
void AssembleVertexProlongation(typename TAlgebra::matrix_type& mat,
                                const TDD& coarseDD, const TDD& fineDD,
								std::vector<bool>& vIsRestricted)
{
// 	allow only lagrange P1 functions
	for(size_t fct = 0; fct < fineDD.num_fct(); ++fct)
		if(fineDD.local_finite_element_id(fct) != LFEID(LFEID::LAGRANGE, 1))
			UG_THROW("AssembleVertexProlongation:"
				"Interpolation only implemented for Lagrange P1 functions.");

//  get subsethandler and grid
	const MultiGrid& grid = coarseDD.multi_grid();

//  get number of dofs on different levels
	const size_t numFineDoFs = fineDD.num_indices();
	const size_t numCoarseDoFs = coarseDD.num_indices();

//	check if grid distribution has dofs, otherwise skip creation since father
//	elements may not exist in parallel.
	if(numFineDoFs == 0 || numCoarseDoFs == 0) return;

//  resize matrix
	if(!mat.resize(numFineDoFs, numCoarseDoFs))
		UG_THROW("AssembleVertexProlongation:"
				"Cannot resize Interpolation Matrix.");

//	clear restricted vector
	vIsRestricted.clear(); vIsRestricted.resize(numCoarseDoFs, false);

	std::vector<MultiIndex<2> > coarseMultInd, fineMultInd;

//  iterators
	typedef typename TDD::template traits<VertexBase>::const_iterator const_iterator;
	const_iterator iter, iterBegin, iterEnd;

//  loop subsets on fine level
	for(int si = 0; si < fineDD.num_subsets(); ++si)
	{
		iterBegin = fineDD.template begin<VertexBase>(si);
		iterEnd = fineDD.template end<VertexBase>(si);

	//  loop vertices for fine level subset
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		//	get element
			VertexBase* fineVrt = *iter;

		//  get father
			GeometricObject* parent = grid.get_parent(fineVrt);

		//	type of father
			const ReferenceObjectID roid = parent->reference_object_id();

		//	loop all components
			for(size_t fct = 0; fct < fineDD.num_fct(); fct++)
			{
			//	check that fct defined on subset
				if(!fineDD.is_def_in_subset(fct, si)) continue;

			//  get global indices
				fineDD.inner_multi_indices(fineVrt, fct, fineMultInd);

			//	detect type of father
				switch(roid)
				{
					case ROID_VERTEX:
						{
							VertexBase* vrt = dynamic_cast<VertexBase*>(parent);
							coarseDD.inner_multi_indices(vrt, fct, coarseMultInd);
							DoFRef(mat, fineMultInd[0], coarseMultInd[0]) = 1.0;
							vIsRestricted[coarseMultInd[0][0]] = true;
						}
						break;
					case ROID_EDGE:
						for(int i = 0; i < 2; ++i)
						{
							EdgeBase* edge = dynamic_cast<EdgeBase*>(parent);
							coarseDD.inner_multi_indices(edge->vertex(i), fct, coarseMultInd);
							DoFRef(mat, fineMultInd[0], coarseMultInd[0]) = 0.5;
							vIsRestricted[coarseMultInd[0][0]] = true;
						}
						break;
					case ROID_QUADRILATERAL:
						for(int i = 0; i < 4; ++i)
						{
							Face* face = dynamic_cast<Face*>(parent);
							coarseDD.inner_multi_indices(face->vertex(i), fct, coarseMultInd);
							DoFRef(mat, fineMultInd[0], coarseMultInd[0]) = 0.25;
							vIsRestricted[coarseMultInd[0][0]] = true;
						}
						break;
					case ROID_HEXAHEDRON:
						for(int i = 0; i < 8; ++i)
						{
							Volume* hexaeder = dynamic_cast<Volume*>(parent);
							coarseDD.inner_multi_indices(hexaeder->vertex(i), fct, coarseMultInd);
							DoFRef(mat, fineMultInd[0], coarseMultInd[0]) = 0.125;
							vIsRestricted[coarseMultInd[0][0]] = true;
						}
						break;
					default: UG_THROW("AssembleVertexProlongation: Element Father"
									 "is of unsupported type "<<roid);
				}
			}
		}
	}
}


template <typename TDomain, typename TDD, typename TAlgebra>
void AssembleProlongationElementwise(typename TAlgebra::matrix_type& mat,
                                     const TDD& coarseDD, const TDD& fineDD,
                                     ConstSmartPtr<TDomain> spDomain,
                                     std::vector<bool>& vIsRestricted)
{
//	dimension
	const int dim = TDomain::dim;

//  get subsethandler and grid
	const MultiGrid& grid = coarseDD.multi_grid();

//  get number of dofs on different levels
	const size_t numFineDoFs = fineDD.num_indices();
	const size_t numCoarseDoFs = coarseDD.num_indices();

//	check if grid distribution has dofs, otherwise skip creation since father
//	elements may not exist in parallel.
	if(numFineDoFs == 0 || numCoarseDoFs == 0) return;

//  resize matrix
	if(!mat.resize(numFineDoFs, numCoarseDoFs))
		UG_THROW("AssembleVertexProlongation:"
				"Cannot resize Interpolation Matrix.");

//	clear restricted vector
	vIsRestricted.clear(); vIsRestricted.resize(numCoarseDoFs, false);

	std::vector<MultiIndex<2> > vCoarseMultInd, vFineMultInd;


//	vector of local finite element ids
	std::vector<LFEID> vLFEID(fineDD.num_fct());
	for(size_t fct = 0; fct < fineDD.num_fct(); ++fct)
		vLFEID[fct] = fineDD.local_finite_element_id(fct);

//  iterators
	typedef typename TDD::template dim_traits<dim>::const_iterator const_iterator;
	typedef typename TDD::template dim_traits<dim>::geometric_base_object Element;
	const_iterator iter, iterBegin, iterEnd;

//  loop subsets on fine level
	for(int si = 0; si < coarseDD.num_subsets(); ++si)
	{
		iterBegin = coarseDD.template begin<Element>(si);
		iterEnd = coarseDD.template end<Element>(si);

	//  loop vertices for fine level subset
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		//	get element
			Element* coarseElem = *iter;

		//  get children
			const size_t numChild = grid.num_children<Element, Element>(coarseElem);
			std::vector<Element*> vChild(numChild);
			for(size_t c = 0; c < vChild.size(); ++c)
				vChild[c] = grid.get_child<Element, Element>(coarseElem,  c);

		//	type of father
			const ReferenceObjectID roid = coarseElem->reference_object_id();

		//	loop all components
			for(size_t fct = 0; fct < coarseDD.num_fct(); fct++)
			{
			//	check that fct defined on subset
				if(!coarseDD.is_def_in_subset(fct, si)) continue;

			//  get global indices
				coarseDD.multi_indices(coarseElem, fct, vCoarseMultInd);

			//	get local finite element trial spaces
				const LocalShapeFunctionSet<dim>& lsfs = LocalShapeFunctionSetProvider::get<dim>(roid, vLFEID[fct]);

			//	get corner coordinates
				std::vector<MathVector<dim> > vCornerCoarse;
				CollectCornerCoordinates(vCornerCoarse, *coarseElem, *spDomain);

			//	get Reference Mapping
				DimReferenceMapping<dim, dim>& map = ReferenceMappingProvider::get<dim, dim>(roid, vCornerCoarse);

			//	loop children
				for(size_t c = 0; c < vChild.size(); ++c)
				{
					Element* child = vChild[c];

				//	fine dof indices
					fineDD.multi_indices(child, fct, vFineMultInd);

				//	global positions of fine dofs
					std::vector<MathVector<dim> > vDoFPos, vLocPos;
					DoFPosition(vDoFPos, child, *spDomain, vLFEID[fct], dim);

					UG_ASSERT(vDoFPos.size() == vFineMultInd.size(), "numDoFPos ("
					          <<vDoFPos.size()<<") != numDoFs ("<<vFineMultInd.size()<<").");

				//	get local position of DoF
					vLocPos.resize(vDoFPos.size());
					for(size_t ip = 0; ip < vLocPos.size(); ++ip) VecSet(vLocPos[ip], 0.0);
					map.global_to_local(vLocPos, vDoFPos);

				//	get all shape functions
					std::vector<std::vector<number> > vvShape;

				//	evaluate coarse shape fct at fine local point
					lsfs.shapes(vvShape, vLocPos);

					for(size_t ip = 0; ip < vvShape.size(); ++ip)
					{
						for(size_t sh = 0; sh < vvShape[ip].size(); ++sh)
						{
							DoFRef(mat, vFineMultInd[ip], vCoarseMultInd[sh]) = vvShape[ip][sh];
							vIsRestricted[vCoarseMultInd[sh][0]] = true;
						}
					}
				}

			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 	StdProlongation
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <typename TDomain, typename TAlgebra>
void StdProlongation<TDomain, TAlgebra>::
set_approximation_space(SmartPtr<ApproximationSpace<TDomain> > approxSpace)
{
	m_spApproxSpace = approxSpace;
}

template <typename TDomain, typename TAlgebra>
void StdProlongation<TDomain, TAlgebra>::set_levels(GridLevel coarseLevel, GridLevel fineLevel)
{
	m_fineLevel = fineLevel;
	m_coarseLevel = coarseLevel;

	if(m_fineLevel.level() - m_coarseLevel.level() != 1)
		UG_THROW("StdProlongation<TDomain, TAlgebra>::set_levels:"
				" Can only project between successive level.");

	if(m_fineLevel.type() != GridLevel::LEVEL ||
	   m_coarseLevel.type() != GridLevel::LEVEL)
		UG_THROW("StdProlongation<TDomain, TAlgebra>::set_levels:"
				" Can only project between level dof distributions, but fine="
				<<m_fineLevel<<", coarse="<<m_coarseLevel);
}

template <typename TDomain, typename TAlgebra>
void StdProlongation<TDomain, TAlgebra>::init()
{
	if(!m_spApproxSpace.valid())
		UG_THROW("StdProlongation<TDomain, TAlgebra>::init: "
				"Approximation Space not set. Cannot init Projection.");

	m_matrix.resize(0,0);

// 	check only lagrange P1 functions
	bool P1LagrangeOnly = true;
	for(size_t fct = 0; fct < m_spApproxSpace->function_pattern()->num_fct(); ++fct)
		if(m_spApproxSpace->function_pattern()->local_finite_element_id(fct) != LFEID(LFEID::LAGRANGE, 1))
			P1LagrangeOnly = false;

	try{
		if(P1LagrangeOnly)
		{
			AssembleVertexProlongation<LevelDoFDistribution, TAlgebra>
			(m_matrix,
			 *m_spApproxSpace->level_dof_distribution(m_coarseLevel.level()),
			 *m_spApproxSpace->level_dof_distribution(m_fineLevel.level()),
			 m_vIsRestricted);
		}
		else
		{
			AssembleProlongationElementwise<TDomain, LevelDoFDistribution, TAlgebra>
			(m_matrix,
			 *m_spApproxSpace->level_dof_distribution(m_coarseLevel.level()),
			 *m_spApproxSpace->level_dof_distribution(m_fineLevel.level()),
			 m_spApproxSpace->domain(),
			 m_vIsRestricted);
		}
	} UG_CATCH_THROW("StdProlongation<TDomain, TAlgebra>::init:"
				"Cannot assemble interpolation matrix.");

	#ifdef UG_PARALLEL
		m_matrix.set_storage_type(PST_CONSISTENT);
	#endif

	m_bInit = true;
}

template <typename TDomain, typename TAlgebra>
void StdProlongation<TDomain, TAlgebra>::apply(vector_type& uFineOut, const vector_type& uCoarseIn)
{
//	Check, that operator is initiallized
	if(!m_bInit)
		UG_THROW("StdProlongation<TDomain, TAlgebra>::apply:"
				" Operator not initialized.");

//	Some Assertions
	UG_ASSERT(uFineOut.size() == m_matrix.num_rows(),
				  "Vector and Row sizes have to match!");
	UG_ASSERT(uCoarseIn.size() == m_matrix.num_cols(),
				  "Vector and Column sizes have to match!");

//	Apply Matrix
	if(!m_matrix.apply(uFineOut, uCoarseIn))
	{
		std::stringstream ss;
		ss << "StdProlongation<TDomain, TAlgebra>::apply: Cannot apply matrix. ";
#ifdef UG_PARALLEL
		ss << "(Type uCoarse = " <<uCoarseIn.get_storage_mask();
#endif
		UG_THROW(ss.str().c_str());
	}

//	Set dirichlet nodes to zero again
//	todo: We could handle this by eliminating dirichlet rows as well
	try{
	for(size_t i = 0; i < m_vConstraint.size(); ++i)
		m_vConstraint[i]->adjust_defect(uFineOut, uFineOut, m_fineLevel);

	}UG_CATCH_THROW("StdProlongation<TDomain, TAlgebra>::apply: "
					"Error while setting dirichlet defect to zero.");
}

template <typename TDomain, typename TAlgebra>
void StdProlongation<TDomain, TAlgebra>::apply_transposed(vector_type& uCoarseOut, const vector_type& uFineIn)
{
//	Check, that operator is initiallized
	if(!m_bInit)
		UG_THROW("StdProlongation<TDomain, TAlgebra>::apply_transposed:"
				"Operator not initialized.");

	vector_type	uTmp; uTmp.resize(uCoarseOut.size());

//	Some Assertions
	UG_ASSERT(uFineIn.size() == m_matrix.num_rows(),
				  "Vector and Row sizes have to match!");
	UG_ASSERT(uCoarseOut.size() == m_matrix.num_cols(),
				  "Vector and Column sizes have to match!");

//	Apply transposed matrix
	if(!m_matrix.apply_transposed(uTmp, uFineIn))
		UG_THROW("StdProlongation<TDomain, TAlgebra>::apply_transposed:"
				" Cannot apply transposed matrix.");

	uTmp *= m_dampRes;

//	Copy only restricted values
//	This is needed in adaptive situations, where the defect that has been
//	projected from the surface should remain and only hidden (i.e.
//	indices with children) should be changed.
	for(size_t i = 0; i < uTmp.size(); ++i)
		if(m_vIsRestricted[i])
			uCoarseOut[i] = uTmp[i];

//	Set dirichlet nodes to zero again
//	todo: We could handle this by eliminating dirichlet columns as well
	try{
	for(size_t i = 0; i < m_vConstraint.size(); ++i)
		m_vConstraint[i]->adjust_defect(uCoarseOut, uCoarseOut, m_coarseLevel);
	} UG_CATCH_THROW("ProjectionOperator::apply_transposed: "
					"Error while setting dirichlet defect to zero.");
}

template <typename TDomain, typename TAlgebra>
SmartPtr<IProlongationOperator<TAlgebra> >
StdProlongation<TDomain, TAlgebra>::clone()
{
	SmartPtr<StdProlongation> op(new StdProlongation);
	op->set_approximation_space(m_spApproxSpace);
	for(size_t i = 0; i < m_vConstraint.size(); ++i)
		op->add_constraint(m_vConstraint[i]);
	op->set_restriction_damping(m_dampRes);
	return op;
}

template <typename TDomain, typename TAlgebra>
void
StdProlongation<TDomain, TAlgebra>::add_constraint(SmartPtr<IConstraint<algebra_type> > pp)
{
//	add only once
	if(std::find(m_vConstraint.begin(), m_vConstraint.end(), pp) !=
			m_vConstraint.end()) return;
	m_vConstraint.push_back(pp);
}

template <typename TDomain, typename TAlgebra>
void
StdProlongation<TDomain, TAlgebra>::remove_constraint(SmartPtr<IConstraint<algebra_type> > pp)
{
	m_vConstraint.erase(m_vConstraint.begin(),
	                     std::remove(m_vConstraint.begin(), m_vConstraint.end(), pp));
}


} // end namespace ug

#endif /* __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR_IMPL__ */
