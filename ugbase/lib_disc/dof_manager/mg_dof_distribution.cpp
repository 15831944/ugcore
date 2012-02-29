/*
 * mg_dof_distribution.cpp
 *
 *  Created on: 06.12.2011
 *      Author: andreasvogel
 */

#include "common/log.h"
#include "mg_dof_distribution.h"
#include "lib_disc/domain.h"
#include "lib_disc/local_finite_element/local_dof_set.h"

using namespace std;

namespace ug{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//	ComputeOrientationOffset
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename TBaseElem>
void ComputeOrientationOffset(std::vector<size_t>& vOrientOffset, const size_t p,
                              const ReferenceElement& rRefElem,
                              const size_t nrEdge, const size_t numDoFsOnSub,
                              const std::vector<VertexBase*>& vCorner)
{}


/*
 * Orientation of an edge:
 * If DoFs are assigned to a lower-dimensional edge and we have a degree higher
 * than 2 (i.e. more than one DoF on the edge) orientation is required to
 * ensure continuity of the shape functions. This means, that each element
 * that has the edge as a subelement, must number the dofs on the edge equally
 * in global numbering.
 *
 * The idea is as follows: We induce a global ordering of dofs on the edge by
 * using the storage position of the vertices of the edge. By our own definition
 * we say, that dofs are always assigned in a line from the vertices with lower
 * storage position to the vertices with the higher one. Now, in the local ordering
 * of dofs on the reference element, the edge may have been a different numbering
 * for the corners.
 * Thus, we have to distinguish two case:
 * a) corner 0 in reference element numbering has the smaller storage position: in
 * 		this case we can simply use the usual offset numbering
 * b) corner 0 in reference element numbering has a bigger storage position: in
 * 		this case we have to use the reverse order as offset numbering
 */
template <>
void ComputeOrientationOffset<EdgeBase>
(std::vector<size_t>& vOrientOffset, const size_t p,
 const ReferenceElement& rRefElem,
 const size_t nrEdge, const size_t numDoFsOnSub,
 const std::vector<VertexBase*>& vCorner)
{
//	check
	UG_ASSERT(p-1 == numDoFsOnSub, "Wrong number of dofs on sub");
	UG_ASSERT(p > 2, "Orientation only needed for p > 2, but given p="<<p);

//	compare the two corners of the edge
	const size_t co0 = rRefElem.id(1, nrEdge, 0, 0);
	const size_t co1 = rRefElem.id(1, nrEdge, 0, 1);

//	resize the orientation array
	vOrientOffset.resize(numDoFsOnSub);

//	the standard orientation is from co0 -> co1.
//	we define to store the dofs that way if co0 has smaller address than
//	co1. If this is not the case now, we have to invert the order.
	if(vCorner[co0] < vCorner[co1])
	{
		for(size_t i = 0; i < numDoFsOnSub; ++i)
			vOrientOffset[i] = i;
	}
	else
	{
		for(size_t i = 0; i < numDoFsOnSub; ++i)
			vOrientOffset[i] = (p-2)-i;
	}
};




static void MapFace(size_t& map_i, size_t& map_j,
                    const size_t i, const size_t j,
                    const int numCo, const size_t p,
                    const int smallest, const int second)
{
	UG_ASSERT(i <= p, "Wrong index");
	UG_ASSERT(j <= p, "Wrong index");

//	handle rotation
	switch(numCo)
	{
		case 3:
		{
			switch(smallest)
			{
				case 0: map_i = i; map_j = j; break;
				case 1: map_i = j; map_j = p-i-j; break;
				case 2: map_i = p-i-j; map_j = i; break;
				default: UG_THROW_FATAL("Corner not found.");
			}
			break;
		}
		case 4:
		{
			switch(smallest)
			{
				case 0: map_i = i; map_j = j; break;
				case 1: map_i = j; map_j = p-i; break;
				case 2: map_i = p-i; map_j = p-j; break;
				case 3: map_i = p-j; map_j = i; break;
				default: UG_THROW_FATAL("Corner not found.");
			}
			break;
		}
		default: UG_THROW_FATAL("Num Corners not supported.");
	}

//	handle mirroring
	if(second == (smallest+numCo-1)%numCo)
	{
		const size_t h = map_i; map_i = map_j; map_j = h;
	}
};

/*
 * Orientation of a Face:
 * If DoFs are assigned to a lower-dimensional face and we have a degree higher
 * than 2 (i.e. more than one DoF on the face) orientation is required to
 * ensure continuity of the shape functions. This means, that each element
 * that has the face as a subelement, must number the dofs on the face equally
 * in global numbering.
 *
 * The idea of the ordering is as follows:
 * We define that DoFs are always assigned to the face in a prescribed order.
 * In ordr to do so, we find the vertex of the face with the smallest storage
 * position. Then we find the vertex, that is connected to the smallest vertex
 * by an edge, with the (second) smallest storage position. Thus, we have a
 * situation like this:
 *
 *  	*						*--------*			^ j
 *  	|  \					|		 |			|
 *  	|    \				    |	     |			|
 * 		|      \				|		 |			|-----> i
 *  	*------ *				*--------*
 *  smallest   second		smallest	second
 *
 * In this picture all rotations and mirroring can appear. We define that the
 * DoFs on the face are always numbered in x direction first, continuing in the
 * next row in y direction, numbering in x again and continuing in y, etc.
 * E.g. this gives for a p = 4 element (showing only inner dofs):
 *
 *  	*						*-------*			^ j
 *  	|5 \					| 6	7 8	|			|
 *  	|3 4 \					| 3 4 5	|			|
 * 		|0 1 2 \				| 0 1 2 |			|-----> i
 *  	*-------*				*-------*
 *  smallest   second		smallest	second
 *
 * Now, the face vertices of the given element have a local numbering vCo[] in
 * the reference element. The DoFs in the reference element meaning are ordered
 * as if vCo[0] was smallest and vCo[1] was second. Thus, now in real world,
 * these must not match and we have to find the orientation of the face. Smallest
 * and second is computed and than a mapping is set up.
 */
template <>
void ComputeOrientationOffset<Face>
(std::vector<size_t>& vOrientOffset, const size_t p,
 const ReferenceElement& rRefElem,
 const size_t nrFace, const size_t numDoFsOnSub,
 const std::vector<VertexBase*>& vCorner)
{
//	should only be called for p > 2, since else no orientation needed
	UG_ASSERT(p > 2, "Orientation only needed for p > 2, but given p="<<p);

//	resize array
	vOrientOffset.resize(numDoFsOnSub);

//	get corners of face
	const int numCo = rRefElem.num(2, nrFace, 0);
	std::vector<size_t> vCo(numCo);
	for(int i = 0; i < numCo; ++i)
		vCo[i] = rRefElem.id(2, nrFace, 0, i);

//	find smallest
	int smallest = 0;
	for(int i = 1; i < numCo; ++i)
		if(vCorner[ vCo[i] ] < vCorner[ vCo[smallest] ]) smallest = i;

//	find second smallest
	int second = (smallest+numCo-1)%numCo;
	const int next = (smallest+numCo+1)%numCo;
	if(vCorner[ vCo[next] ] < vCorner[ vCo[second] ]) second = next;

//	map the i,j
	size_t map_i, map_j;

//	in the inner, the number of dofs is as if it would be an element
//	of degree p-2. We cache this here
	const size_t pInner = p-2;

//	loop 'y'-direction
	size_t index = 0;
	for(size_t j = 0; j <= pInner; ++j)
	{
	//	for a quadrilateral we have a quadratic loop, but for a
	//	triangle we need to stop at the diagonal
		const size_t off = ((vCo.size() == 3) ? j : 0);

	//	loop 'x'-direction
		for(size_t i = 0; i <= pInner-off; ++i)
		{
		//	map the index-pair (i,j)
			MapFace(map_i, map_j, i, j, numCo, pInner, smallest, second);

		//	linearize index and mapped index
			const size_t mappedIndex = (p-1) * map_j + map_i;

		//	check
			UG_ASSERT(mappedIndex < numDoFsOnSub, "Wrong mapped index");
			UG_ASSERT(index < numDoFsOnSub, "Wrong index");

		//	set mapping
			vOrientOffset[index++] = mappedIndex;
		}
	}
};


////////////////////////////////////////////////////////////////////////////////
// MGDoFDistribution
////////////////////////////////////////////////////////////////////////////////

MGDoFDistribution::
MGDoFDistribution(SmartPtr<MGSubsetHandler> spMGSH, FunctionPattern& fctPatt)
	: m_bGrouped(false), m_spMGSH(spMGSH),
	  m_rFctPatt(fctPatt), m_rMultiGrid(*m_spMGSH->multi_grid())
{
	check_subsets();
	create_offsets();
	init_attachments();
	register_observer();
};

void MGDoFDistribution::create_offsets(ReferenceObjectID roid)
{
// 	loop subsets
	for(int si = 0; si < num_subsets(); ++si)
	{
	// 	counter
		size_t count = 0;

	//	get dimension of subset
		int dim = m_rFctPatt.dim_subset(si);

	//	reset
		m_vvNumDoFsOnROID[roid][si] = 0;

	//	loop functions
		for(size_t fct = 0; fct < m_rFctPatt.num_fct(); ++fct)
		{
		//	reset to not defined (initially)
			m_vvvOffsets[roid][si][fct] = NOT_DEF_ON_SUBSET;

		//	if function is not defined, we leave the offset as invalid.
			if(!is_def_in_subset(fct, si))	continue;

		//	get local shape function id
			LFEID lfeID = m_rFctPatt.local_finite_element_id(fct);

		//	get trial space
			const CommonLocalDoFSet& clds = LocalDoFSetProvider::get(dim, lfeID);

		//	get number of DoFs on the reference element need for the space
			const int numDoF = clds.num_dof(roid);

		//	overwrite max dim with dofs (if subset has that dimension)
			if(dim > ReferenceElementDimension((ReferenceObjectID)roid))
				m_vMaxDimToOrderDoFs[fct] = ReferenceElementDimension((ReferenceObjectID)roid);

		//	check that numDoFs specified by this roid
			if(numDoF == -1) continue;

		//	set offset for each function defined in the subset
			m_vvvOffsets[roid][si][fct] = count;

		//	increase number of DoFs per type on this subset
			m_vvNumDoFsOnROID[roid][si] += numDoF;

		//	increase the count
			count += numDoF;
		}
	}
}

void MGDoFDistribution::create_offsets()
{
//	function infos
	m_vMaxDimToOrderDoFs.resize(num_fct(), 0);
	m_vvFctDefInSubset.resize(num_fct());
	m_vNumDoFOnSubelem.resize(num_fct());
	m_vLFEID.resize(num_fct());
	for(size_t fct = 0; fct < num_fct(); ++fct)
	{
	//	get local shape function id
		m_vLFEID[fct] = m_rFctPatt.local_finite_element_id(fct);

		m_vvFctDefInSubset[fct].resize(num_subsets(), false);
		for(int si = 0; si < num_subsets(); ++si)
		{
			m_vvFctDefInSubset[fct][si] = m_rFctPatt.is_def_in_subset(fct,si);
		}

		for(int roid=ROID_VERTEX; roid < NUM_REFERENCE_OBJECTS; ++roid)
		{
			m_vLocalDoFSet[roid].resize(num_fct());

		//	remember local dof set
			m_vLocalDoFSet[roid][fct] = & LocalDoFSetProvider::get((ReferenceObjectID)roid, m_vLFEID[fct]);

			for(int subRoid=ROID_VERTEX; subRoid < NUM_REFERENCE_OBJECTS; ++subRoid)
			{
			//	get number of DoFs in this sub-geometric object
				m_vNumDoFOnSubelem[fct](roid,subRoid) = m_vLocalDoFSet[roid][fct]->num_dof((ReferenceObjectID)subRoid);
			}
		}
	}

//	loop all reference element to resize the arrays
	for(int roid=ROID_VERTEX; roid < NUM_REFERENCE_OBJECTS; ++roid)
	{
	//	clear offsets
		m_vvvOffsets[roid].clear();
		m_vvNumDoFsOnROID[roid].clear();

	//	resize for all subsets
		m_vvvOffsets[roid].resize(num_subsets());
		m_vvNumDoFsOnROID[roid].resize(num_subsets(), 0);

	//	resize for each function
		for(int si = 0; si < num_subsets(); ++si)
			m_vvvOffsets[roid][si].resize(m_rFctPatt.num_fct());
	}

//	loop all reference element, but not vertices (no disc there)
	for(int roid=ROID_VERTEX; roid < NUM_REFERENCE_OBJECTS; ++roid)
		create_offsets((ReferenceObjectID) roid);

//	reset dimension maximum
	for(size_t d=0; d < NUM_GEOMETRIC_BASE_OBJECTS; ++d)
	{
		m_vMaxDoFsInDim[d] = 0;
		m_vvMaxDoFsInDimPerSubset[d].resize(num_subsets(), 0);
	}

//	get max number of dofs per roid
	for(int roid=ROID_VERTEX; roid < NUM_REFERENCE_OBJECTS; ++roid)
	{
	//	lets find out the maximum number of DoFs for objects in dimension
		const int d = ReferenceElementDimension((ReferenceObjectID)roid);

	// 	lets find out the maximum number for a type on all subsets
		m_vMaxDoFsOnROID[roid] = 0;
		for(int si = 0; si < num_subsets(); ++si)
		{
			m_vMaxDoFsOnROID[roid] = std::max(m_vMaxDoFsOnROID[roid],
											  m_vvNumDoFsOnROID[roid][si]);

			m_vvMaxDoFsInDimPerSubset[d][si] = std::max(m_vvMaxDoFsInDimPerSubset[d][si],
			                                            m_vvNumDoFsOnROID[roid][si]);
		}

	//	compute maximum per dim objects
		m_vMaxDoFsInDim[d] = std::max(m_vMaxDoFsInDim[d],
		                              m_vMaxDoFsOnROID[roid]);
	}
}

void MGDoFDistribution::check_subsets()
{
//	check, that all geom objects are assigned to a subset
	if(	m_spMGSH->num<VertexBase>() != m_rMultiGrid.num<VertexBase>())
		UG_THROW_FATAL("All Vertices "
			   " must be assigned to a subset. The passed subset handler "
			   " contains non-assigned elements, thus the dof distribution"
			   " is not possible, aborting.");

	if(	m_spMGSH->num<EdgeBase>() != m_rMultiGrid.num<EdgeBase>())
		UG_THROW_FATAL("All Edges "
			   " must be assigned to a subset. The passed subset handler "
			   " contains non-assigned elements, thus the dof distribution"
			   " is not possible, aborting.");

	if(	m_spMGSH->num<Face>() != m_rMultiGrid.num<Face>())
		UG_THROW_FATAL("All Faces "
			   " must be assigned to a subset. The passed subset handler "
			   " contains non-assigned elements, thus the dof distribution"
			   " is not possible, aborting.");

	if(	m_spMGSH->num<Volume>() != m_rMultiGrid.num<Volume>())
		UG_THROW_FATAL("All Volumes "
			   " must be assigned to a subset. The passed subset handler "
			   " contains non-assigned elements, thus the dof distribution"
			   " is not possible, aborting.");
}

void MGDoFDistribution::print_local_dof_statistic(int verboseLev) const
{
//	write info for subset/fct -> localFEId info
	UG_LOG("\n\t\t\t Subsets\n");
	UG_LOG(" "<<setw(14)<<"Function"<<" |");
	for(int si = 0; si < num_subsets(); ++si)
		UG_LOG(setw(11)<<si<<setw(8)<<" "<<"|")
	UG_LOG("\n");
	for(size_t fct = 0; fct < num_fct(); ++fct)
	{
		UG_LOG(" "<<setw(14)<<m_rFctPatt.name(fct)<<" |");
		for(int si = 0; si < num_subsets(); ++si)
		{
			if(!is_def_in_subset(fct,si))
				 {UG_LOG(setw(8)<<"---"<<setw(8)<<" "<<"|");}
			else {UG_LOG(setw(16)<<m_rFctPatt.local_finite_element_id(fct)<<" |");}
		}
		UG_LOG("\n");
	}

//	write info about DoFs on ROID
	UG_LOG("\n");
	UG_LOG("                  | "<<"        "<<"  |  Subsets \n");
	UG_LOG(" ReferenceElement |");
	UG_LOG("   "<<setw(4)<<"max"<<"    |");
	for(int si = 0; si < num_subsets(); ++si)
		UG_LOG("   "<<setw(4)<<si<<"    |");
	UG_LOG("\n")
	UG_LOG("-------------------");
	for(int si = 0; si <= num_subsets(); ++si)
		UG_LOG("-------------");
	UG_LOG("\n")

	for(int i=ROID_VERTEX; i < NUM_REFERENCE_OBJECTS; ++i)
	{
		ReferenceObjectID roid = (ReferenceObjectID) i;

		UG_LOG(" " << setw(16) << roid << " |");
		UG_LOG("   "<<setw(4) << m_vMaxDoFsOnROID[roid] << "    |");
		for(int si = 0; si < num_subsets(); ++si)
			UG_LOG("   "<<setw(4) << m_vvNumDoFsOnROID[roid][si] << "    |");

		UG_LOG("\n");
	}
	for(int d = 0; d < NUM_GEOMETRIC_BASE_OBJECTS; ++d)
	{
		UG_LOG(setw(14) << " all " <<setw(2)<< d << "d |");
		UG_LOG("   "<<setw(4) << m_vMaxDoFsInDim[d] << "    |");
		UG_LOG("\n");
	}
	UG_LOG("\n");
}

////////////////////////////////////////////////////////////////////////////////
// MGDoFDistribution: Index Access

template <typename TBaseElem>
size_t MGDoFDistribution::
extract_inner_algebra_indices(TBaseElem* elem,
                              std::vector<size_t>& ind) const
{
//	get roid type and subset index
	const int si = m_spMGSH->get_subset_index(elem);
	static const ReferenceObjectID roid = elem->reference_object_id();

//	check if dofs present
	if(num_dofs(roid,si) > 0)
	{
	//	get index
		const size_t firstIndex = obj_index(elem);

		if(!m_bGrouped)
		{
			for(size_t fct = 0; fct < num_fct(); ++fct)
			{
			//	check that function is def on subset
				if(!is_def_in_subset(fct, si)) continue;

			//	get number of DoFs in this sub-geometric object
				const size_t numDoFsOnSub = num_dofs(fct,roid,roid);

			//	compute index
				const size_t index = firstIndex + offset(roid,si,fct);

			//	add dof to local indices
				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back(index+j);
			}
		}
		else
		{
		//	add dof to local indices
			ind.push_back(firstIndex);
		}
	}

//	return number of indices
	return ind.size();
}

template<typename TBaseElem>
void MGDoFDistribution::
extract_inner_algebra_indices(const std::vector<TBaseElem*>& vElem,
                              std::vector<size_t>& ind) const
{
//	loop passed elements
	for(size_t i = 0; i < vElem.size(); ++i)
		inner_algebra_indices(vElem[i], ind);
}

template<typename TBaseElem>
size_t MGDoFDistribution::inner_algebra_indices(TBaseElem* elem,
                                                std::vector<size_t>& ind,
                                                bool bClear) const
{
//	clear indices
	if(bClear) ind.clear();

//	return
	return extract_inner_algebra_indices(elem, ind);
}

template<typename TBaseElem>
size_t MGDoFDistribution::algebra_indices(TBaseElem* elem,
                                          std::vector<size_t>& ind,
                                          bool bClear) const
{
//	clear indices
	if(bClear) ind.clear();

//	reference dimension
	static const int dim = TBaseElem::dim;

//	get all sub-elements and add indices on those
	if(max_dofs(VERTEX) > 0)
	{
		std::vector<VertexBase*> vElem;
		CollectVertices(vElem, m_rMultiGrid, elem);
		extract_inner_algebra_indices<VertexBase>(vElem, ind);
	}
	if(dim >= EDGE && max_dofs(EDGE) > 0)
	{
		std::vector<EdgeBase*> vElem;
		CollectEdges(vElem, m_rMultiGrid, elem);
		extract_inner_algebra_indices<EdgeBase>(vElem, ind);
	}
	if(dim >= FACE && max_dofs(FACE) > 0)
	{
		std::vector<Face*> vElem;
		CollectFaces(vElem, m_rMultiGrid, elem);
		extract_inner_algebra_indices<Face>(vElem, ind);
	}
	if(dim >= VOLUME && max_dofs(VOLUME) > 0)
	{
		std::vector<Volume*> vElem;
		CollectVolumes(vElem, m_rMultiGrid, elem);
		extract_inner_algebra_indices<Volume>(vElem, ind);
	}

//	return number of indices
	return ind.size();
}

template<typename TBaseElem, typename TSubBaseElem>
void MGDoFDistribution::
multi_indices(TBaseElem* elem, const ReferenceObjectID roid,
              size_t fct, std::vector<multi_index_type>& ind,
              const std::vector<TSubBaseElem*>& vElem,
              const std::vector<VertexBase*>& vCorner, bool bHang) const
{
//	get dimension of subelement
	static const int d = TSubBaseElem::dim;

//	loop passed elements
	for(size_t i = 0; i < vElem.size(); ++i)
	{
	//	get subelement
		TSubBaseElem* subElem = vElem[i];

	//	get subset index
		const int si = m_spMGSH->get_subset_index(subElem);

	//	check if function is defined on the subset
		if(!is_def_in_subset(fct, si)) continue;

	//	get reference object id for subselement
		const ReferenceObjectID subRoid = subElem->reference_object_id();

	//	check if dof given
		if(num_dofs(subRoid,si) == 0) continue;

	//	get number of DoFs in this sub-geometric object
		const size_t numDoFsOnSub = num_dofs(fct, roid, subRoid);

	//	a) Orientation required
		if(d < m_vMaxDimToOrderDoFs[fct] && numDoFsOnSub > 1)
		{
			std::vector<size_t> vOrientOffset(numDoFsOnSub);

		//	get the orientation for this subelement
			ComputeOrientationOffset<TBaseElem>
				(vOrientOffset, m_vLFEID[fct].order(),
				 ReferenceElementProvider::get(roid),
				 	 i, numDoFsOnSub, vCorner);

			if(!m_bGrouped)
			{
			//	compute index
				const size_t index = obj_index(subElem) + offset(subRoid,si,fct);

				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back(multi_index_type(index+vOrientOffset[j],0));
			}
			else
			{
			//	compute index
				const size_t comp = offset(subRoid,si,fct);
				const size_t firstIndex = obj_index(subElem);

				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back(multi_index_type(firstIndex, comp+vOrientOffset[j]));
			}
		}
	//	b) No Orientation required
		else
		{
			if(!m_bGrouped)
			{
			//	compute index
				const size_t index = obj_index(subElem) + offset(subRoid,si,fct);

				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back(multi_index_type(index+j,0));
			}
			else
			{
			//	compute index
				const size_t comp = offset(subRoid,si,fct);
				const size_t firstIndex = obj_index(subElem);

				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back(multi_index_type(firstIndex, comp+j));
			}
		} // end switch "Orientation required"
	} // end loop sub elements

//	return number of indices
	return;
}

template<typename TBaseElem>
size_t MGDoFDistribution::inner_multi_indices(TBaseElem* elem, size_t fct,
                                              std::vector<multi_index_type>& ind,
                                              bool bClear) const
{
//	clear if requested
	if(bClear) ind.clear();

//	get dimension
	static const int d = TBaseElem::dim;

//	get subset index
	const int si = m_spMGSH->get_subset_index(elem);

//	check if function is defined on the subset
	if(!is_def_in_subset(fct, si)) return ind.size();

//	get roid type
	static const ReferenceObjectID roid = elem->reference_object_id();

//	check if dof given
	if(num_dofs(roid,si) == 0) return ind.size();

//	get number of DoFs in this sub-geometric object
	const size_t numDoFsOnSub = num_dofs(fct,roid,roid);

//	a) Orientation required:
	if(d < m_vMaxDimToOrderDoFs[fct] && numDoFsOnSub > 1)
	{
	//	get corners
		std::vector<VertexBase*> vCorner;
		CollectVertices(vCorner, m_rMultiGrid, elem);

	//	get the orientation for this
		std::vector<size_t> vOrientOffset(numDoFsOnSub);
		ComputeOrientationOffset<TBaseElem>
			(vOrientOffset, m_vLFEID[fct].order(),
			 ReferenceElementProvider::get(roid),
			 0, numDoFsOnSub, vCorner);

		if(!m_bGrouped)
		{
		//	compute index
			const size_t index = obj_index(elem) + offset(roid,si,fct);

			for(size_t j = 0; j < numDoFsOnSub; ++j)
				ind.push_back(multi_index_type(index+vOrientOffset[j],0));
		}
		else
		{
		//	compute index
			const size_t comp = offset(roid,si,fct);
			const size_t firstIndex = obj_index(elem);

			for(size_t j = 0; j < numDoFsOnSub; ++j)
				ind.push_back(multi_index_type(firstIndex, comp+vOrientOffset[j]));
		}
	}
//	b) No orientation needed
	else
	{
		if(!m_bGrouped)
		{
		//	compute index
			const size_t index = obj_index(elem) + offset(roid,si,fct);

			for(size_t j = 0; j < numDoFsOnSub; ++j)
				ind.push_back(multi_index_type(index+j,0));
		}
		else
		{
		//	compute index
			const size_t comp = offset(roid,si,fct);
			const size_t firstIndex = obj_index(elem);

			for(size_t j = 0; j < numDoFsOnSub; ++j)
				ind.push_back(multi_index_type(firstIndex, comp+j));
		}
	}

//	done
	return ind.size();
}

size_t MGDoFDistribution::multi_indices(GeometricObject* elem, size_t fct,
                                        std::vector<multi_index_type>& ind,
                                        bool bHang, bool bClear) const
{
	switch(elem->base_object_type_id())
	{
		case VERTEX: return multi_indices(static_cast<VertexBase*>(elem), fct, ind, bHang, bClear);
		case EDGE: return multi_indices(static_cast<EdgeBase*>(elem), fct, ind, bHang, bClear);
		case FACE: return multi_indices(static_cast<Face*>(elem), fct, ind, bHang, bClear);
		case VOLUME: return multi_indices(static_cast<Volume*>(elem), fct, ind, bHang, bClear);
		default: UG_THROW_FATAL("Geometric Base element not found.");
	}
}

template<typename TBaseElem>
size_t MGDoFDistribution::multi_indices(TBaseElem* elem, size_t fct,
                                        std::vector<multi_index_type>& ind,
                                        bool bHang, bool bClear) const
{
//	clear indices
	if(bClear) ind.clear();

//	reference dimension
	static const int dim = TBaseElem::dim;

//	reference object id
	const ReferenceObjectID roid = elem->reference_object_id();

//	get all sub-elements and add indices on those
	std::vector<VertexBase*> vCorner;
	CollectVertices(vCorner, m_rMultiGrid, elem);

	if(max_dofs(VERTEX) > 0)
	{
		multi_indices<TBaseElem, VertexBase>(elem, roid, fct, ind, vCorner, vCorner, bHang);
	}
	if(dim >= EDGE && max_dofs(EDGE) > 0)
	{
		std::vector<EdgeBase*> vElem;
		CollectEdgesSorted(vElem, m_rMultiGrid, elem);
		multi_indices<TBaseElem, EdgeBase>(elem, roid, fct, ind, vElem, vCorner, bHang);
	}
	if(dim >= FACE && max_dofs(FACE) > 0)
	{
		std::vector<Face*> vElem;
		CollectFacesSorted(vElem, m_rMultiGrid, elem);
		multi_indices<TBaseElem, Face>(elem, roid, fct, ind, vElem, vCorner, bHang);
	}
	if(dim >= VOLUME && max_dofs(VOLUME) > 0)
	{
		std::vector<Volume*> vElem;
		CollectVolumes(vElem, m_rMultiGrid, elem);
		multi_indices<TBaseElem, Volume>(elem, roid, fct, ind, vElem, vCorner, bHang);
	}

//	todo: add hanging nodes
//	If no hanging dofs are required, we're done
	if(!bHang) return ind.size();

	UG_THROW_FATAL("Hanging DoFs are currently not supported by this DoFManager.");

//	return number of indices
	return ind.size();
}

template<typename TBaseElem>
void MGDoFDistribution::indices_on_vertex(TBaseElem* elem, const ReferenceObjectID roid,
                                          LocalIndices& ind,
                                          const std::vector<VertexBase*>& vElem) const
{
//	get reference object id for subelement
	static const ReferenceObjectID subRoid = ROID_VERTEX;

//	add normal dofs
	for(size_t i = 0; i < vElem.size(); ++i)
	{
	//	get subset index
		const int si = m_spMGSH->get_subset_index(vElem[i]);

	//	loop all functions
		for(size_t fct = 0; fct < num_fct(); ++fct)
		{
		//	check if function is defined on the subset
			if(!is_def_in_subset(fct, si)) continue;

		//	get number of DoFs in this sub-geometric object
			const size_t numDoFsOnSub = num_dofs(fct,roid,subRoid);

		//	Always no orientation needed
			if(!m_bGrouped)
			{
			//	compute index
				const size_t index = obj_index(vElem[i]) + offset(subRoid,si,fct);

			//	add dof to local indices
				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back_index(fct, index+j);
			}
			else
			{
			//	compute index
				const size_t index = obj_index(vElem[i]);
				const size_t comp = offset(subRoid,si,fct);

			//	add dof to local indices
				for(size_t j = 0; j < numDoFsOnSub; ++j)
					ind.push_back_multi_index(fct, index, comp+j);
			}
		} // end loop functions
	} // end loop subelement

}

template<typename TBaseElem, typename TSubBaseElem>
void MGDoFDistribution::indices(TBaseElem* elem, const ReferenceObjectID roid,
                                LocalIndices& ind,
                                const std::vector<TSubBaseElem*>& vElem,
                                const std::vector<VertexBase*>& vCorner) const
{
//	dimension of subelement
	static const int d = TSubBaseElem::dim;

//	add normal dofs
	for(size_t i = 0; i < vElem.size(); ++i)
	{
	//	get subelement
		TSubBaseElem* subElem = vElem[i];

	//	get subset index
		const int si = m_spMGSH->get_subset_index(subElem);

	//	get reference object id for subselement
		const ReferenceObjectID subRoid = subElem->reference_object_id();

	//	loop all functions
		for(size_t fct = 0; fct < num_fct(); ++fct)
		{
		//	check if function is defined on the subset
			if(!is_def_in_subset(fct, si)) continue;

		//	get number of DoFs in this sub-geometric object
			const size_t numDoFsOnSub = num_dofs(fct,roid,subRoid);

		//	a)	Orientation is required: Thus, we compute the offsets, that are
		//		no longer in the usual order [0, 1, 2, ...]. Orientation is
		//		required if there are more than 1 dof on a subelement of a
		//		finite element and thus, when gluing two elements together,
		//		also the dofs on the subelements have to fit in order to
		//		guarantee continuity. This is not needed for Vertices, since there
		//		no distinction can be made when all dofs are at the same position.
		//		This is also not needed for the highest dimension of a finite
		//		element, since the dofs on this geometric object must not be
		//		identified with other dofs.
			if(d < m_vMaxDimToOrderDoFs[fct] && numDoFsOnSub > 1)
			{
			//	vector storing the computed offsets. If in correct order,
			//	this would be: [0, 1, 2, ...]. But this is usually not the
			// 	case and the numbers 0 to numDoFsOnSub-1 are permuted
				std::vector<size_t> vOrientOffset(numDoFsOnSub);

				ComputeOrientationOffset<TBaseElem>
					(vOrientOffset, m_vLFEID[fct].order(),
					 ReferenceElementProvider::get(roid),
					 i, numDoFsOnSub, vCorner);

				if(!m_bGrouped)
				{
					const size_t index = obj_index(subElem) + offset(subRoid,si,fct);
					for(size_t j = 0; j < numDoFsOnSub; ++j)
						ind.push_back_index(fct, index+vOrientOffset[j]);
				}
				else
				{
				//	compute index
					const size_t index = obj_index(subElem);
					const size_t comp = offset(subRoid,si,fct);

					for(size_t j = 0; j < numDoFsOnSub; ++j)
						ind.push_back_multi_index(fct, index, comp+vOrientOffset[j]);
				}
			}
		//	b)	No orientation needed
			else
			{
				if(!m_bGrouped)
				{
				//	compute index
					const size_t index = obj_index(subElem) + offset(subRoid,si,fct);

				//	add dof to local indices
					for(size_t j = 0; j < numDoFsOnSub; ++j)
						ind.push_back_index(fct, index+j);
				}
				else
				{
				//	compute index
					const size_t index = obj_index(subElem);
					const size_t comp = offset(subRoid,si,fct);

				//	add dof to local indices
					for(size_t j = 0; j < numDoFsOnSub; ++j)
						ind.push_back_multi_index(fct, index, comp+j);
				}
			} // end switch "Orientation needed"
		} // end loop functions
	} // end loop subelement

}

template <typename TConstraining, typename TConstrained, typename TBaseElem>
void MGDoFDistribution::
constrained_indices(LocalIndices& ind,
                    const std::vector<TBaseElem*>& vSubElem) const
{
//	loop all edges
	for(size_t i = 0; i < vSubElem.size(); ++i)
	{
	//	only constraining objects are of interest
		TConstraining* constrainingObj = dynamic_cast<TConstraining*>(vSubElem[i]);
		if(constrainingObj == NULL) continue;

	//	loop constraining vertices
		for(size_t i = 0; i != constrainingObj->num_constrained_vertices(); ++i)
		{
		//	get vertex
			TConstrained* vrt = constrainingObj->constrained_vertex(i);

		//	get roid
			const ReferenceObjectID subRoid = vrt->reference_object_id();

		//	get subset index
			int si = m_spMGSH->get_subset_index(vrt);

		//	loop functions
			for(size_t fct = 0; fct < num_fct(); ++fct)
			{
			//	check that function is defined on subset
				if(!is_def_in_subset(fct, si)) continue;

				if(!m_bGrouped)
				{
				//	compute index
					const size_t index = obj_index(vrt) + offset(subRoid,si,fct);

				//	add dof to local indices
					ind.push_back_index(fct, index);
				}
				else
				{
				//	compute index
					const size_t index = obj_index(vrt);
					const size_t comp = offset(subRoid,si,fct);

				//	add dof to local indices
					ind.push_back_multi_index(fct, index, comp);
				}
			}
		}
	}
}

template<typename TBaseElem>
void MGDoFDistribution::indices(TBaseElem* elem, LocalIndices& ind, bool bHang) const
{
//	reference dimension
	static const int dim = TBaseElem::dim;

//	resize the number of functions
	ind.resize_fct(num_fct());
	for(size_t fct = 0; fct < num_fct(); ++fct) ind.clear_dof(fct);

//	get all sub-elements and add indices on those
	std::vector<VertexBase*> vCorner;
	CollectVertices(vCorner, m_rMultiGrid, elem);

//	memory for (maybe needed) subelements
	std::vector<EdgeBase*> vEdge;
	std::vector<Face*> vFace;
	std::vector<Volume*> vVol;

//	collect elements, if needed
	if(dim >= EDGE)
		if(max_dofs(EDGE) > 0 || bHang) CollectEdgesSorted(vEdge, m_rMultiGrid, elem);
	if(dim >= FACE)
		if(max_dofs(FACE) > 0 || bHang) CollectFacesSorted(vFace, m_rMultiGrid, elem);
	if(dim >= VOLUME)
		if(max_dofs(VOLUME) > 0 || bHang) CollectVolumes(vVol, m_rMultiGrid, elem);

//	get reference object id
	const ReferenceObjectID roid = elem->reference_object_id();

//	get regular dofs on all subelements and the element itself
//	use specialized function for vertices (since only one position and one reference object)
	if(max_dofs(VERTEX) > 0) 				  indices_on_vertex<TBaseElem>(elem, roid, ind, vCorner);
	if(dim >= EDGE && max_dofs(EDGE) > 0) 	  indices<TBaseElem, EdgeBase>(elem, roid, ind, vEdge, vCorner);
	if(dim >= FACE && max_dofs(FACE) > 0) 	  indices<TBaseElem, Face>(elem, roid, ind, vFace, vCorner);
	if(dim >= VOLUME && max_dofs(VOLUME) > 0) indices<TBaseElem, Volume>(elem, roid, ind, vVol, vCorner);

//	If no hanging dofs are required, we're done
	if(!bHang) return;

//	get dofs on hanging vertices
	if(max_dofs(VERTEX > 0))
	{
		if(dim >= EDGE) constrained_indices<ConstrainingEdge, VertexBase, EdgeBase>(ind, vEdge);
		if(dim >= FACE) constrained_indices<ConstrainingQuadrilateral, VertexBase, Face>(ind, vFace);
	}

//	todo: allow also constrained dofs on other elements
	if(max_dofs(EDGE) || max_dofs(FACE) || max_dofs(VOLUME))
		UG_THROW_FATAL("Hanging DoFs are only implemented for P1 by this DoFManager.");

//	we're done
	return;
}



template <typename TBaseElem>
void MGDoFDistribution::
changable_indices(std::vector<size_t>& vIndex,
                  const std::vector<TBaseElem*>& vElem) const
{
//	Get connected indices
	for(size_t i = 0; i < vElem.size(); ++i)
	{
	//	Get Vertices of adjacent edges
		TBaseElem* elem = vElem[i];
		UG_ASSERT(m_spMGSH->get_subset_index(elem) >= 0, "Must have subset");

	//	get adjacent index
		const size_t adjInd = obj_index(elem);

	//	add to index list
		vIndex.push_back(adjInd);
	}
}


////////////////////////////////////////////////////////////////////////////////
// MGDoFDistribution: DoF Handling

template <typename TBaseObject>
void MGDoFDistribution::
add(TBaseObject* obj, const ReferenceObjectID roid, const int si,
    LevInfo& li)
{
//	if no dofs on this subset for the roid, do nothing
	if(m_vvNumDoFsOnROID[roid][si] == 0) return;

//	compute the number of indices needed on the Geometric object
	size_t numNewIndex = 1;
	if(!m_bGrouped) numNewIndex = m_vvNumDoFsOnROID[roid][si];

//	a) 	if no holes are in the index set,
	if(li.vFreeIndex.empty())
	{
	// 	set first available index to the object. The first available index is the
	//	first managed index plus the size of the index set. (If holes are in the
	//	index set, this is not treated here, wholes remain)
		obj_index(obj) = li.sizeIndexSet;

	//	the size of the index set has changed. adjust counter
		li.sizeIndexSet += numNewIndex;
	}
	else
	{
	//	get a free index (a hole) and use it
		obj_index(obj) = li.vFreeIndex.back();
		li.vFreeIndex.pop_back();
	}

//	number of managed indices and the number of managed indices on the subset has
//	changed. Thus, increase the counters.
	li.numIndex += numNewIndex;
	li.vNumIndexOnSubset[si] += numNewIndex;
}

template <typename TBaseObject>
void MGDoFDistribution::
defragment(TBaseObject* obj, const ReferenceObjectID roid, const int si,
           LevInfo& li, std::vector<std::pair<size_t, size_t> >& vReplaced)
{
//	get old (current) index
	const size_t oldIndex = obj_index(obj);

// 	check if index must be replaced by lower one
	if(oldIndex < li.numIndex) return;

//	must have holes in the index set
	UG_ASSERT(li.vFreeIndex.empty(), "Hole in index set, but no free index.");

//	get new index from stack
	while(!li.vFreeIndex.empty())
	{
	//	get a free index (a hole) and use it
		const size_t newIndex = li.vFreeIndex.back(); li.vFreeIndex.pop_back();

	//	check that index is admissible
		if(newIndex < li.numIndex)
		{
		//	set new index
			obj_index(obj) = newIndex;

		//	remember replacement
			vReplaced.push_back(std::pair<size_t,size_t>(oldIndex,newIndex));

		//	done
			break;
		}
	//	else try next
	}

//	compute the number of indices needed on the Geometric object
	size_t numNewIndex = 1;
	if(!m_bGrouped) numNewIndex = m_vvNumDoFsOnROID[roid][si];

//	number of Indices stays the same, but size of index set is changed.
	li.sizeIndexSet -= numNewIndex;
}


template <typename TBaseObject>
void MGDoFDistribution::
erase(TBaseObject* obj, const ReferenceObjectID roid, const int si,
      LevInfo& li)
{
//	if no indices needed, we do nothing
	if(m_vvNumDoFsOnROID[roid][si] == 0) return;

//	store the index of the object, that will be erased as a available hole of the
//	index set
	li.vFreeIndex.push_back(obj_index(obj));

//	compute number of indices on the geometric object
	size_t numNewIndex = 1;
	if(!m_bGrouped) numNewIndex = m_vvNumDoFsOnROID[roid][si];

//	number of managed indices has changed, thus decrease counter. Note, that the
//	size of the index set remains unchanged.
	li.numIndex -= numNewIndex;
	li.vNumIndexOnSubset[si] -= numNewIndex;
}

void MGDoFDistribution::
copy(GeometricObject* objNew, GeometricObject* objOld)
{
//	check subsets
	UG_ASSERT(m_spMGSH->get_subset_index(objNew) ==
			  m_spMGSH->get_subset_index(objOld),
			  "Subset index "<<m_spMGSH->get_subset_index(objNew)<<
			  "of replacing obj must match the one of replaced obj "
			  <<m_spMGSH->get_subset_index(objOld));

//	simply copy the index
	obj_index(objNew) = obj_index(objOld);
}

void MGDoFDistribution::init_attachments()
{
//	attach DoFs to vertices
	if(m_vMaxDoFsInDim[VERTEX] > 0) {
		m_rMultiGrid.attach_to<VertexBase>(m_aIndex);
		m_aaIndexVRT.access(m_rMultiGrid, m_aIndex);
	}
	if(m_vMaxDoFsInDim[EDGE] > 0) {
		m_rMultiGrid.attach_to<EdgeBase>(m_aIndex);
		m_aaIndexEDGE.access(m_rMultiGrid, m_aIndex);
	}
	if(m_vMaxDoFsInDim[FACE] > 0) {
		m_rMultiGrid.attach_to<Face>(m_aIndex);
		m_aaIndexFACE.access(m_rMultiGrid, m_aIndex);
	}
	if(m_vMaxDoFsInDim[VOLUME] > 0) {
		m_rMultiGrid.attach_to<Volume>(m_aIndex);
		m_aaIndexVOL.access(m_rMultiGrid, m_aIndex);
	}
}

void MGDoFDistribution::clear_attachments()
{
//	detach DoFs
	if(m_aaIndexVRT.valid()) m_rMultiGrid.detach_from<VertexBase>(m_aIndex);
	if(m_aaIndexEDGE.valid()) m_rMultiGrid.detach_from<EdgeBase>(m_aIndex);
	if(m_aaIndexFACE.valid()) m_rMultiGrid.detach_from<Face>(m_aIndex);
	if(m_aaIndexVOL.valid()) m_rMultiGrid.detach_from<Volume>(m_aIndex);

	m_aaIndexVRT.invalidate();
	m_aaIndexEDGE.invalidate();
	m_aaIndexFACE.invalidate();
	m_aaIndexVOL.invalidate();
}

size_t& MGDoFDistribution::obj_index(GeometricObject* obj)
{
	switch(obj->base_object_type_id())
	{
		case VERTEX: return obj_index(static_cast<VertexBase*>(obj));
		case EDGE:   return obj_index(static_cast<EdgeBase*>(obj));
		case FACE:   return obj_index(static_cast<Face*>(obj));
		case VOLUME: return obj_index(static_cast<Volume*>(obj));
		default: UG_THROW_FATAL("Base Object type not found.");
	}
}

const size_t& MGDoFDistribution::obj_index(GeometricObject* obj) const
{
	switch(obj->base_object_type_id())
	{
		case VERTEX: return obj_index(static_cast<VertexBase*>(obj));
		case EDGE:   return obj_index(static_cast<EdgeBase*>(obj));
		case FACE:   return obj_index(static_cast<Face*>(obj));
		case VOLUME: return obj_index(static_cast<Volume*>(obj));
		default: UG_THROW_FATAL("Base Object type not found.");
	}
};

void MGDoFDistribution::register_observer()
{
	int type = OT_GRID_OBSERVER;

	if(max_dofs(VERTEX)) type |= OT_VERTEX_OBSERVER;
	if(max_dofs(EDGE)) type |= OT_EDGE_OBSERVER;
	if(max_dofs(FACE)) type |= OT_FACE_OBSERVER;
	if(max_dofs(VOLUME)) type |= OT_VOLUME_OBSERVER;

	m_rMultiGrid.register_observer(this, type);
}

void MGDoFDistribution::unregister_observer()
{
	m_rMultiGrid.unregister_observer(this);
}

///////////////////////////////////////////////////////////////////////////////
// template instantiations
///////////////////////////////////////////////////////////////////////////////
template void MGDoFDistribution::indices<VertexBase>(VertexBase*, LocalIndices&, bool) const;
template void MGDoFDistribution::indices<EdgeBase>(EdgeBase*, LocalIndices&, bool) const;
template void MGDoFDistribution::indices<Face>(Face*, LocalIndices&, bool) const;
template void MGDoFDistribution::indices<Volume>(Volume*, LocalIndices&, bool) const;

template size_t MGDoFDistribution::multi_indices<VertexBase>(VertexBase*, size_t, std::vector<multi_index_type>&, bool, bool) const;
template size_t MGDoFDistribution::multi_indices<EdgeBase>(EdgeBase*, size_t, std::vector<multi_index_type>&, bool, bool) const;
template size_t MGDoFDistribution::multi_indices<Face>(Face*, size_t, std::vector<multi_index_type>&, bool, bool) const;
template size_t MGDoFDistribution::multi_indices<Volume>(Volume*, size_t, std::vector<multi_index_type>&, bool, bool) const;

template size_t MGDoFDistribution::inner_multi_indices<VertexBase>(VertexBase*, size_t,std::vector<multi_index_type>&, bool) const;
template size_t MGDoFDistribution::inner_multi_indices<EdgeBase>(EdgeBase*, size_t,std::vector<multi_index_type>&, bool) const;
template size_t MGDoFDistribution::inner_multi_indices<Face>(Face*, size_t,std::vector<multi_index_type>&, bool) const;
template size_t MGDoFDistribution::inner_multi_indices<Volume>(Volume*, size_t,std::vector<multi_index_type>&, bool) const;

template size_t MGDoFDistribution::algebra_indices<VertexBase>(VertexBase*,	std::vector<size_t>&, bool) const;
template size_t MGDoFDistribution::algebra_indices<EdgeBase>(EdgeBase*,	std::vector<size_t>&, bool) const;
template size_t MGDoFDistribution::algebra_indices<Face>(Face*,	std::vector<size_t>&, bool) const;
template size_t MGDoFDistribution::algebra_indices<Volume>(Volume*,	std::vector<size_t>&, bool) const;

template size_t MGDoFDistribution::inner_algebra_indices<VertexBase>(VertexBase*, std::vector<size_t>& , bool) const;
template size_t MGDoFDistribution::inner_algebra_indices<EdgeBase>(EdgeBase*, std::vector<size_t>& , bool) const;
template size_t MGDoFDistribution::inner_algebra_indices<Face>(Face*, std::vector<size_t>& , bool) const;
template size_t MGDoFDistribution::inner_algebra_indices<Volume>(Volume*, std::vector<size_t>& , bool) const;

template void MGDoFDistribution::changable_indices<VertexBase>(std::vector<size_t>& vIndex, const std::vector<VertexBase*>& vElem) const;
template void MGDoFDistribution::changable_indices<EdgeBase>(std::vector<size_t>& vIndex, const std::vector<EdgeBase*>& vElem) const;
template void MGDoFDistribution::changable_indices<Face>(std::vector<size_t>& vIndex, const std::vector<Face*>& vElem) const;
template void MGDoFDistribution::changable_indices<Volume>(std::vector<size_t>& vIndex, const std::vector<Volume*>& vElem) const;

template void MGDoFDistribution::add<VertexBase>(VertexBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::add<EdgeBase>(EdgeBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::add<Face>(Face* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::add<Volume>(Volume* obj, const ReferenceObjectID roid, const int si, LevInfo& li);

template void MGDoFDistribution::erase<VertexBase>(VertexBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::erase<EdgeBase>(EdgeBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::erase<Face>(Face* obj, const ReferenceObjectID roid, const int si, LevInfo& li);
template void MGDoFDistribution::erase<Volume>(Volume* obj, const ReferenceObjectID roid, const int si, LevInfo& li);

template void MGDoFDistribution::defragment<VertexBase>(VertexBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li, std::vector<std::pair<size_t,size_t> >& vReplaced);
template void MGDoFDistribution::defragment<EdgeBase>(EdgeBase* obj, const ReferenceObjectID roid, const int si, LevInfo& li, std::vector<std::pair<size_t,size_t> >& vReplaced);
template void MGDoFDistribution::defragment<Face>(Face* obj, const ReferenceObjectID roid, const int si, LevInfo& li, std::vector<std::pair<size_t,size_t> >& vReplaced);
template void MGDoFDistribution::defragment<Volume>(Volume* obj, const ReferenceObjectID roid, const int si, LevInfo& li, std::vector<std::pair<size_t,size_t> >& vReplaced);

} // end namespace ug
