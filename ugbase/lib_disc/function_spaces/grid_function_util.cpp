/*
 * grid_function_util.cpp
 *
 *  Created on: 11.01.2012
 *      Author: andreasvogel
 */

#include "grid_function_util.h"
#include "grid_function_impl.h"
#include "approximation_space.h"
#include "lib_disc/domain.h"

namespace ug{

///	writes positions of vertex dofs into a std::vector
template <typename TDomain, typename TDD>
void ExtractPositionsVertex(ConstSmartPtr<TDomain> domain,
                            ConstSmartPtr<TDD> dd,
                            std::vector<MathVector<TDomain::dim> >& vPos)
{
//	get position accessor
	const typename TDomain::position_accessor_type& aaPos = domain->position_accessor();

//	iterator
	typename TDD::template traits<VertexBase>::const_iterator iter, iterEnd;

//	algebra indices vector
	std::vector<size_t> ind;

//	loop all subsets
	for(int si = 0; si < dd->num_subsets(); ++si)
	{
	//	get iterators
		iter = dd->template begin<VertexBase>(si);
		iterEnd = dd->template end<VertexBase>(si);

	//	loop all vertices
		for(;iter != iterEnd; ++iter)
		{
		//	get vertex
			VertexBase* v = *iter;

		//	load indices associated with vertex
			dd->inner_algebra_indices(v, ind);

		//	write position
			for(size_t i = 0; i < ind.size(); ++i)
			{
				const size_t index = ind[i];
				vPos[index] = aaPos[v];
			}
		}
	}
}

template <typename TDomain, typename TDD, typename TBaseElem>
void ExtractPositionsElem(ConstSmartPtr<TDomain> domain,
                          ConstSmartPtr<TDD> dd,
                          std::vector<MathVector<TDomain::dim> >& vPos)
{
	typename TDD::template traits<TBaseElem>::const_iterator iter, iterEnd;

//	vector for positions
	std::vector<MathVector<TDomain::dim> > vElemPos;

//	algebra indices vector
	std::vector<MultiIndex<2> > ind;

//	loop all subsets
	for(int si = 0; si < dd->num_subsets(); ++si)
	{
	//	get iterators
		iter = dd->template begin<TBaseElem>(si);
		iterEnd = dd->template end<TBaseElem>(si);

	//	loop all functions
		for(size_t fct = 0; fct < dd->num_fct(); ++fct)
		{
		//	skip non-used function
			if(!dd->is_def_in_subset(fct,si)) continue;

			if(dd->function_pattern().dim(fct) != TBaseElem::dim)
				continue;

		//	loop all elements
			for(;iter != iterEnd; ++iter)
			{
			//	get vertex
				TBaseElem* elem = *iter;

			//	load indices associated with element function
				dd->multi_indices(elem, fct, ind);

			//	load positions associated with element and function
				DoFPosition(vElemPos, elem, *(const_cast<TDomain*>(domain.get())),
				            dd->local_finite_element_id(fct), dd->dim(fct));

			//	check correct size
				UG_ASSERT(ind.size() == vElemPos.size(), "Num MultiIndex ("<<ind.size()
						  <<") and Num Position ("<<vElemPos.size()<<") must match."
						 "GeomObject dim="<<geometry_traits<TBaseElem>::BASE_OBJECT_ID);

			//	write position
				for(size_t sh = 0; sh < ind.size(); ++sh)
				{
					const size_t index = ind[sh][0];
					vPos[index] = vElemPos[sh];
				}
			}
		}
	}
}

template <typename TDomain, typename TDD>
void ExtractPositions(ConstSmartPtr<TDomain> domain,
                      ConstSmartPtr<TDD> dd,
                      std::vector<MathVector<TDomain::dim> >& vPos)
{
//	number of total dofs
	int nr = dd->num_indices();

//	resize positions
	vPos.resize(nr);

//	extract for all element types
	ExtractPositionsVertex<TDomain, TDD>(domain, dd, vPos);
	ExtractPositionsElem<TDomain, TDD, EdgeBase>(domain, dd, vPos);
	ExtractPositionsElem<TDomain, TDD, Face>(domain, dd, vPos);
	ExtractPositionsElem<TDomain, TDD, Volume>(domain, dd, vPos);
}

template void ExtractPositions(ConstSmartPtr<Domain1d> domain, ConstSmartPtr<LevelDoFDistribution> dd, std::vector<MathVector<Domain1d::dim> >& vPos);
template void ExtractPositions(ConstSmartPtr<Domain2d> domain, ConstSmartPtr<LevelDoFDistribution> dd, std::vector<MathVector<Domain2d::dim> >& vPos);
template void ExtractPositions(ConstSmartPtr<Domain3d> domain, ConstSmartPtr<LevelDoFDistribution> dd, std::vector<MathVector<Domain3d::dim> >& vPos);

template void ExtractPositions(ConstSmartPtr<Domain1d> domain, ConstSmartPtr<SurfaceDoFDistribution> dd, std::vector<MathVector<Domain1d::dim> >& vPos);
template void ExtractPositions(ConstSmartPtr<Domain2d> domain, ConstSmartPtr<SurfaceDoFDistribution> dd, std::vector<MathVector<Domain2d::dim> >& vPos);
template void ExtractPositions(ConstSmartPtr<Domain3d> domain, ConstSmartPtr<SurfaceDoFDistribution> dd, std::vector<MathVector<Domain3d::dim> >& vPos);


} // end namespace ug
