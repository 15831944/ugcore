/*
 * grid_function_impl.h
 *
 *  Created on: 13.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__FUNCTION_SPACE__GRID_FUNCTION_IMPL__
#define __H__LIBDISCRETIZATION__FUNCTION_SPACE__GRID_FUNCTION_IMPL__

#include "grid_function.h"
#include "lib_discretization/local_finite_element/local_shape_function_set.h"
#include "lib_discretization/local_finite_element/local_dof_set.h"

namespace ug{

template <typename TDoFDistribution>
void
IGridFunction<TDoFDistribution>::
assign_dof_distribution(typename IGridFunction<TDoFDistribution>::dof_distribution_type& DoFDistr, bool adapt)
{
//	unregister from dof distribution iff already dd set
	if(m_pDD != NULL)
		m_pDD->unmanage_grid_function(*this);

//	remember new dof distribution
	m_pDD = &DoFDistr;

//	schedule for adaption
	if(adapt)
		m_pDD->manage_grid_function(*this);

//	resize the vector
	resize_values(num_indices());
}

////////////////////////////////////////////////////////////////////////////////
//	dof_positions
////////////////////////////////////////////////////////////////////////////////

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
template <typename TElem>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(TElem* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
//	reference element
	typedef typename reference_element_traits<TElem>::reference_element_type
			reference_element_type;

//	reference element dimension
	static const int refDim = reference_element_type::dim;

//	reference object id
	static const ReferenceObjectID roid = reference_element_type::REFERENCE_OBJECT_ID;

//	vector for the vertex positions
	std::vector<MathVector<dim> > vVertPos(reference_element_type::num_corners);

//	get the vertices
	CollectCornerCoordinates(vVertPos, *elem, get_domain(), true);

//	create a reference mapping
	ReferenceMapping<reference_element_type, dim> map(&(vVertPos[0]));

//	get finite element id
	LFEID lfeID = this->local_finite_element_id(fct);

//	get local shape function set
	const DimLocalShapeFunctionSet<dim>& lsfs
		= LocalShapeFunctionSetProvider::get<reference_element_type>(roid, lfeID);

//	typedef local position type
	typedef typename DimLocalShapeFunctionSet<dim>::position_type local_pos_type;

//	clear pos
	vPos.resize(lsfs.num_sh());

//	bool flag if position is exact, or no exact position available for shapes
	bool bExact = true;

//	loop all shape functions
	for(size_t sh = 0; sh < lsfs.num_sh(); ++sh)
	{
	//	get local position
		local_pos_type locPos;
		bExact &= lsfs.position(sh, locPos);

	//	map to global position
		map.local_to_global(vPos[sh], locPos);
	}

//	return if positions are given exactly
	return bExact;
};

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(GeometricObject* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->base_object_type_id())
	{
		case VERTEX: return dof_positions(static_cast<VertexBase*>(elem), fct, vPos);
		case EDGE:   return dof_positions(static_cast<EdgeBase*>(elem), fct, vPos);
		case FACE:   return dof_positions(static_cast<Face*>(elem), fct, vPos);
		case VOLUME: return dof_positions(static_cast<Volume*>(elem), fct, vPos);
	}
	throw(UGFatalError("Base Object type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(VertexBase* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSVRT_VERTEX: 		return dof_positions(static_cast<Vertex*>(elem), fct, vPos);
		case SPSVRT_HANGING_VERTEX: return dof_positions(static_cast<HangingVertex*>(elem), fct, vPos);
	}
	throw(UGFatalError("Vertex type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(EdgeBase* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSEDGE_EDGE: 			   return dof_positions(static_cast<Edge*>(elem), fct, vPos);
		case SPSEDGE_CONSTRAINED_EDGE: return dof_positions(static_cast<ConstrainedEdge*>(elem), fct, vPos);
		case SPSEDGE_CONSTRAINING_EDGE:return dof_positions(static_cast<ConstrainingEdge*>(elem), fct, vPos);
	}
	throw(UGFatalError("Edge type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(Face* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSFACE_TRIANGLE: return dof_positions(static_cast<Triangle*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINED_TRIANGLE: return dof_positions(static_cast<ConstrainedTriangle*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINING_TRIANGLE: return dof_positions(static_cast<ConstrainingTriangle*>(elem), fct, vPos);
		case SPSFACE_QUADRILATERAL: return dof_positions(static_cast<Quadrilateral*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINED_QUADRILATERAL: return dof_positions(static_cast<ConstrainedQuadrilateral*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINING_QUADRILATERAL: return dof_positions(static_cast<ConstrainingQuadrilateral*>(elem), fct, vPos);
	}
	throw(UGFatalError("Face type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
dof_positions(Volume* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSVOL_TETRAHEDRON: return dof_positions(static_cast<Tetrahedron*>(elem), fct, vPos);
		case SPSVOL_PYRAMID: return dof_positions(static_cast<Pyramid*>(elem), fct, vPos);
		case SPSVOL_PRISM: return dof_positions(static_cast<Prism*>(elem), fct, vPos);
		case SPSVOL_HEXAHEDRON: return dof_positions(static_cast<Hexahedron*>(elem), fct, vPos);
	}
	throw(UGFatalError("Volume type not found."));
}

////////////////////////////////////////////////////////////////////////////////
//	inner_dof_positions
////////////////////////////////////////////////////////////////////////////////

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
template <typename TElem>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(TElem* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
//	reference element
	typedef typename reference_element_traits<TElem>::reference_element_type
			reference_element_type;

//	reference element dimension
	static const int refDim = reference_element_type::dim;

//	reference object id
	static const ReferenceObjectID roid = reference_element_type::REFERENCE_OBJECT_ID;

//	vector for the vertex positions
	std::vector<MathVector<dim> > vVertPos(reference_element_type::num_corners);

//	get the vertices
	CollectCornerCoordinates(vVertPos, *elem, get_domain(), true);

//	create a reference mapping
	ReferenceMapping<reference_element_type, dim> map(&(vVertPos[0]));

//	get finite element id
	LFEID lfeID = this->local_finite_element_id(fct);

//	get local shape function set
	const DimLocalShapeFunctionSet<refDim>& lsfs
		= LocalShapeFunctionSetProvider::get<refDim>(roid, lfeID);

//	get local dof set
	const ILocalDoFSet& lds = LocalDoFSetProvider::get(roid, lfeID);

//	typedef local position type
	typedef typename DimLocalShapeFunctionSet<refDim>::position_type
		local_pos_type;

//	clear pos
	vPos.clear();

//	bool flag if position is exact, or no exact position available for shapes
	bool bExact = true;

//	loop all shape functions
	for(size_t sh = 0; sh < lsfs.num_sh(); ++sh)
	{
	//	check if dof in interior
		if(lds.local_dof(sh).dim() != refDim) continue;

	//	get local position
		local_pos_type locPos;
		bExact &= lsfs.position(sh, locPos);

	//	map to global position
		MathVector<dim> globPos;
		map.local_to_global(globPos, locPos);

	//	add
		vPos.push_back(globPos);
	}

//	return if positions are given exactly
	return bExact;
};

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(GeometricObject* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->base_object_type_id())
	{
		case VERTEX: return inner_dof_positions(static_cast<VertexBase*>(elem), fct, vPos);
		case EDGE:   return inner_dof_positions(static_cast<EdgeBase*>(elem), fct, vPos);
		case FACE:   return inner_dof_positions(static_cast<Face*>(elem), fct, vPos);
		case VOLUME: return inner_dof_positions(static_cast<Volume*>(elem), fct, vPos);
	}
	throw(UGFatalError("Base Object type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(VertexBase* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSVRT_VERTEX: 		return inner_dof_positions(static_cast<Vertex*>(elem), fct, vPos);
		case SPSVRT_HANGING_VERTEX: return inner_dof_positions(static_cast<HangingVertex*>(elem), fct, vPos);
	}
	throw(UGFatalError("Vertex type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(EdgeBase* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSEDGE_EDGE: 			   return inner_dof_positions(static_cast<Edge*>(elem), fct, vPos);
		case SPSEDGE_CONSTRAINED_EDGE: return inner_dof_positions(static_cast<ConstrainedEdge*>(elem), fct, vPos);
		case SPSEDGE_CONSTRAINING_EDGE:return inner_dof_positions(static_cast<ConstrainingEdge*>(elem), fct, vPos);
	}
	throw(UGFatalError("Edge type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(Face* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSFACE_TRIANGLE: return inner_dof_positions(static_cast<Triangle*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINED_TRIANGLE: return inner_dof_positions(static_cast<ConstrainedTriangle*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINING_TRIANGLE: return inner_dof_positions(static_cast<ConstrainingTriangle*>(elem), fct, vPos);
		case SPSFACE_QUADRILATERAL: return inner_dof_positions(static_cast<Quadrilateral*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINED_QUADRILATERAL: return inner_dof_positions(static_cast<ConstrainedQuadrilateral*>(elem), fct, vPos);
		case SPSFACE_CONSTRAINING_QUADRILATERAL: return inner_dof_positions(static_cast<ConstrainingQuadrilateral*>(elem), fct, vPos);
	}
	throw(UGFatalError("Face type not found."));
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool GridFunction<TDomain, TDoFDistribution, TAlgebra>::
inner_dof_positions(Volume* elem, size_t fct, std::vector<MathVector<dim> >& vPos) const
{
	switch(elem->shared_pipe_section())
	{
		case SPSVOL_TETRAHEDRON: return inner_dof_positions(static_cast<Tetrahedron*>(elem), fct, vPos);
		case SPSVOL_PYRAMID: return inner_dof_positions(static_cast<Pyramid*>(elem), fct, vPos);
		case SPSVOL_PRISM: return inner_dof_positions(static_cast<Prism*>(elem), fct, vPos);
		case SPSVOL_HEXAHEDRON: return inner_dof_positions(static_cast<Hexahedron*>(elem), fct, vPos);
	}
	throw(UGFatalError("Volume type not found."));
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
void
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
clone_pattern(const this_type& v)
{
// 	copy approximation space
	assign_approximation_space(*v.m_pApproxSpace);

//	assign dof distribution (resizes vector)
	assign_dof_distribution(*v.m_pDD);
};

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
void
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
resize_values(size_t s, number defaultValue)
{
//	remember old values
	const size_t oldSize = vector_type::size();

//	resize vector
	vector_type::resize(s);

//	set vector to zero-values
	for(size_t i = oldSize; i < s; ++i)
		this->operator[](i) = defaultValue;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
permute_values(const std::vector<size_t>& vIndNew)
{
//	check sizes
	if(vIndNew.size() != this->size())
	{
		UG_LOG("ERROR in GridFunction::swap_values: For a permutation the"
				" index set must have same cardinality as vector.\n");
		return false;
	}

// \todo: avoid tmp vector, only copy values into new vector and use that one
//	create tmp vector
	vector_type vecTmp; vecTmp.resize(this->size());

//	loop indices and copy values
	for(size_t i = 0; i < vIndNew.size(); ++i)
		vecTmp[vIndNew[i]] = this->operator[](i);

//	copy tmp vector into this vector
	if(!this->assign(vecTmp)) return false;

//	we're done
	return true;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
copy_values(const std::vector<std::pair<size_t, size_t> >& vIndexMap,bool bDisjunct)
{
//	disjunct case
	if(bDisjunct)
		for(size_t i = 0; i < vIndexMap.size(); ++i)
			this->operator[](vIndexMap[i].second)
				= this->operator[](vIndexMap[i].first);
//	other case not implemented
	else return false;

//	we're done
	return true;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
assign(const vector_type& v)
{
//	check size
	if(v.size() != vector_type::size())
	{
		UG_LOG("ERROR in GridFunction::assign:"
				"Assigned vector has incorrect size.\n");
		return false;
	}

//	assign vector
	*(dynamic_cast<vector_type*>(this)) = v;

//	we're done
	return true;
}

template <typename TDomain, typename TDoFDistribution, typename TAlgebra>
bool
GridFunction<TDomain, TDoFDistribution, TAlgebra>::
assign(const this_type& v)
{
// 	copy approximation space
	assign_approximation_space(*v.m_pApproxSpace);

//	assign dof distribution (resizes vector)
	assign_dof_distribution(*v.m_pDD);

//  copy values
	*(dynamic_cast<vector_type*>(this)) = *dynamic_cast<const vector_type*>(&v);

//	we're done
	return true;
}

} // end namespace ug

#endif /* __H__LIBDISCRETIZATION__FUNCTION_SPACE__GRID_FUNCTION_IMPL__ */
