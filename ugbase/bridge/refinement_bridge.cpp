// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 14.07.2011 (m,d,y)
 
#include <vector>
#include <string>
#include <sstream>
#include "bridge.h"
#include "lib_disc/domain.h"
#include "lib_grid/lib_grid.h"

using namespace std;

namespace ug{

////////////////////////////////////////////////////////////////////////////////
///	Creates a global domain refiner.
/**	Automatically chooses whether a parallel refiner is required.*/
template <typename TDomain>
static IRefiner* GlobalDomainRefiner(TDomain* dom)
{
//todo: support normal grids, too!
	#ifdef UG_PARALLEL
		if(pcl::GetNumProcesses() > 1){
			IRefiner* refiner = new ParallelGlobalRefiner_MultiGrid(*dom->get_distributed_grid_manager());
			refiner->set_message_hub(dom->get_message_hub());
			return refiner;
		}
	#endif

	IRefiner* refiner = new GlobalMultiGridRefiner(dom->get_grid());
	refiner->set_message_hub(dom->get_message_hub());
	return refiner;
}

////////////////////////////////////////////////////////////////////////////////
///	Creates an adaptive hanging node domain refiner.
/**	Automatically chooses whether a parallel refiner is required.*/
template <typename TDomain>
static IRefiner* HangingNodeDomainRefiner(TDomain* dom)
{
//todo: support normal grids, too!
	#ifdef UG_PARALLEL
		if(pcl::GetNumProcesses() > 1){
			IRefiner* refiner = new ParallelHangingNodeRefiner_MultiGrid(*dom->get_distributed_grid_manager());
			refiner->set_message_hub(dom->get_message_hub());
			return refiner;
		}
	#endif

	IRefiner* refiner = new HangingNodeRefiner_MultiGrid(dom->get_grid());
	refiner->set_message_hub(dom->get_message_hub());
	return refiner;
}

////////////////////////////////////////////////////////////////////////////////
///	Marks all elements from refinement.
/**	If used in a parallel environment only elements on the calling procs
 * are marked.
 */
static void MarkForRefinement_All(IRefiner& ref)
{
	Grid* g = ref.get_associated_grid();
	if(!g){
		UG_LOG("Refiner is not registered at a grid. Aborting.\n");
		return;
	}
	ref.mark(g->vertices_begin(), g->vertices_end());
	ref.mark(g->edges_begin(), g->edges_end());
	ref.mark(g->faces_begin(), g->faces_end());
	ref.mark(g->volumes_begin(), g->volumes_end());
}

////////////////////////////////////////////////////////////////////////////////
///	Marks all vertices in the given d-dimensional sphere.
template <class TDomain>
void MarkForRefinement_VerticesInSphere(TDomain& dom, IRefiner& refiner,
									const typename TDomain::position_type& center,
									number radius)
{
	typedef typename TDomain::position_type 			position_type;
	typedef typename TDomain::position_accessor_type	position_accessor_type;

//	make sure that the refiner was created for the given domain
	if(refiner.get_associated_grid() != &dom.get_grid()){
		throw(UGError("ERROR in MarkForRefinement_VerticesInSphere: "
					"Refiner was not created for the specified domain. Aborting."));
	}

	Grid& grid = *refiner.get_associated_grid();
	position_accessor_type& aaPos = dom.get_position_accessor();

//	we'll compare against the square radius.
	number radiusSq = radius * radius;

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all vertices of the grid. If a vertex is inside the given sphere,
//	then we'll mark all associated elements.
	for(VertexBaseIterator iter = grid.begin<VertexBase>();
		iter != grid.end<VertexBase>(); ++iter)
	{
		if(VecDistanceSq(center, aaPos[*iter]) <= radiusSq){
			CollectAssociated(vEdges, grid, *iter);
			CollectAssociated(vFaces, grid, *iter);
			CollectAssociated(vVols, grid, *iter);

			refiner.mark(vEdges.begin(), vEdges.end());
			refiner.mark(vFaces.begin(), vFaces.end());
			refiner.mark(vVols.begin(), vVols.end());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
///	Marks all elements which lie completely in the given d-dimensional sphere.
/**	Valid parameters for TElem are EdgeBase, Face, Volume.*/
template <class TDomain, class TElem>
void MarkForRefinement_ElementsInSphere(TDomain& dom, IRefiner& refiner,
									const typename TDomain::position_type& center,
									number radius)
{
	typedef typename TDomain::position_type 			position_type;
	typedef typename TDomain::position_accessor_type	position_accessor_type;
	typedef typename geometry_traits<TElem>::iterator	ElemIter;

//	make sure that the refiner was created for the given domain
	if(refiner.get_associated_grid() != &dom.get_grid()){
		throw(UGError("ERROR in MarkForRefinement_VerticesInCube: "
					"Refiner was not created for the specified domain. Aborting."));
	}

	Grid& grid = *refiner.get_associated_grid();
	position_accessor_type& aaPos = dom.get_position_accessor();

//	we'll compare against the square radius.
	number radiusSq = radius * radius;

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all elements of the grid. If all vertices of an element are inside
//	the given sphere, then we'll mark those elements.
	for(ElemIter iter = grid.begin<TElem>();
		iter != grid.end<TElem>(); ++iter)
	{
	//	get element
		TElem* elem = *iter;

	//	bool flag to check whether all vertices are in the sphere
		bool bInSphere = true;

	//	loop all vertices
		for(size_t i = 0; i < elem->num_vertices(); ++i)
		{
		//	check if vertex is in sphere
			if(VecDistanceSq(center, aaPos[elem->vertex(i)]) > radiusSq)
				bInSphere = false;
		}

	//	mark the element
		if(bInSphere)
			refiner.mark(elem);
	}
}

////////////////////////////////////////////////////////////////////////////////
///	Marks all elements which have vertices in the given d-dimensional cube.
/**	Make sure that TAPos is an attachment of vector_t position types.*/
template <class TDomain>
void MarkForRefinement_VerticesInCube(TDomain& dom, IRefiner& refiner,
									const typename TDomain::position_type& min,
									const typename TDomain::position_type& max)
{
	typedef typename TDomain::position_type 			position_type;
	typedef typename TDomain::position_accessor_type	position_accessor_type;

//	make sure that the refiner was created for the given domain
	if(refiner.get_associated_grid() != &dom.get_grid()){
		throw(UGError("ERROR in MarkForRefinement_VerticesInCube: "
					"Refiner was not created for the specified domain. Aborting."));
	}

	Grid& grid = *refiner.get_associated_grid();
	position_accessor_type& aaPos = dom.get_position_accessor();

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all vertices of the grid. If a vertex is inside the given cube,
//	then we'll mark all associated elements.
	for(VertexBaseIterator iter = grid.begin<VertexBase>();
		iter != grid.end<VertexBase>(); ++iter)
	{
	//	Position
		position_type& pos = aaPos[*iter];

	//	check flag
		bool bRefine = true;

	//	check node
		for(size_t d = 0; d < pos.size(); ++d)
			if(pos[d] < min[d] || max[d] < pos[d])
				bRefine = false;

		if(bRefine)
		{
			CollectAssociated(vEdges, grid, *iter);
			CollectAssociated(vFaces, grid, *iter);
			CollectAssociated(vVols, grid, *iter);

			refiner.mark(vEdges.begin(), vEdges.end());
			refiner.mark(vFaces.begin(), vFaces.end());
			refiner.mark(vVols.begin(), vVols.end());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
///	Marks the long edges in anisotropic faces and faces with a big area in anisotropic volumes.
/**
 * THE NOT HIGHLY SUCCESSFUL IMPLEMENTATION OF THIS METHOD LEAD TO NEW INSIGHT.
 * A NEW ANISOTROPIC REFINEMENT STRATEGY WILL BE IMPLEMENTED, WHICH WILL ALSO
 * LEAD TO A REIMPLEMENTATION OF THIS METHOD. BEST NOT TO USE IT UNTIL THEN.
 *
 * The sizeRatio determines Whether a face or a volume is considered anisotropic.
 * Make sure that the sizeRatio is in the interval [0, 1]. If the
 * ratio of the shortest edge to an other edge falls below the given threshold,
 * then the associated face is considered anisotropic and the longer edge will be
 * marked. The face itself will then be marked for anisotropic refinement.
 * The same technique is applied to volumes, this time however the ratio between
 * face-areas is considered when judging whether a volume is anisotropic.
 *
 * VOLUME MARKS ARE CURRENTLY DISABLED
 *
 * Note that this algorithm only really works for a serial environment.
 * \todo	improve it so that it works in parallel, too. A specialization of
 * 			HangingNodeRefiner may be required for this.
 *
 * \todo	activate and improve volume marks
 **/
template <class TDomain>
void MarkForRefinement_AnisotropicElements(TDomain& dom, IRefiner& refiner,
											number sizeRatio)
{
	typedef typename TDomain::position_type 			position_type;
	typedef typename TDomain::position_accessor_type	position_accessor_type;

//	make sure that the refiner was created for the given domain
	if(refiner.get_associated_grid() != &dom.get_grid()){
		throw(UGError("ERROR in MarkForRefinement_VerticesInCube: "
					"Refiner was not created for the specified domain. Aborting."));
	}

//	access the grid and the position attachment
	Grid& grid = *refiner.get_associated_grid();
	position_accessor_type& aaPos = dom.get_position_accessor();

//	If the grid is a multigrid, we want to avoid marking of elements, which do
//	have children
	MultiGrid* pmg = dynamic_cast<MultiGrid*>(&grid);

//	make sure that the grid automatically generates sides for each element
	if(!grid.option_is_enabled(GRIDOPT_AUTOGENERATE_SIDES)){
		UG_LOG("WARNING in MarkForRefinement_AnisotropicElements: "
				"Enabling GRIDOPT_AUTOGENERATE_SIDES.\n");
		grid.enable_options(GRIDOPT_AUTOGENERATE_SIDES);
	}

//	we'll store associated edges and faces in those containers
	vector<EdgeBase*> edges;
	vector<Face*> faces;

//	iterate over all faces of the grid.
	for(FaceIterator iter = grid.begin<Face>();
		iter != grid.end<Face>(); ++iter)
	{
		Face* f = *iter;

	//	ignore faces with children
		if(pmg && pmg->has_children(f))
			continue;

	//	collect associated edges
		CollectAssociated(edges, grid, f);

	//	find the shortest edge
		EdgeBase* minEdge = FindShortestEdge(edges.begin(), edges.end(), aaPos);
		UG_ASSERT(minEdge, "Associated edges of each face have to exist at this point.");
		number minLen = EdgeLength(minEdge, aaPos);

	//	compare all associated edges of f against minEdge (even minEdge itself,
	//	if somebody sets edgeRatio to 1 or higher)
		for(size_t i_edge = 0; i_edge < edges.size(); ++i_edge){
			EdgeBase* e = edges[i_edge];
			number len = EdgeLength(e, aaPos);
		//	to avoid division by 0, we only consider edges with a length > 0
			if(len > 0){
				if(minLen / len <= sizeRatio){
				//	the edge will be refined
					refiner.mark(e);

				//	we'll also mark the current face, or else just a hanging
				//	node would be inserted.
				//	We do not mark other associated objects here since this would
				//	cause creation of a closure and would also behave differently
				//	in a parallel environment, compared to a serial environment.
				//	By using RM_ANISOTROPIC, we avoid that associated edges of
				//	the marked face will automatically be marked, too.
					refiner.mark(f, RM_ANISOTROPIC);
				}
			}
		}
	}

//	iterate over all faces again. We have to make sure that faces which have
//	a marked short edge are refined regular.
//	first push all marked faces into a queue
//	we're using grid::mark to make sure that each face lies only once on the queue.
//	Grid::mark has nothing to do with refinement. It is just a helper for us.
	grid.begin_marking();

	queue<Face*> queFaces;
	for(FaceIterator iter = grid.begin<Face>(); iter != grid.end<Face>(); ++iter){
		queFaces.push(*iter);
		grid.mark(*iter);
	}

	while(!queFaces.empty()){
		Face* f = queFaces.front();
		queFaces.pop();

		if(pmg && pmg->has_children(f)){
			grid.unmark(f);
			continue;
		}

	//	collect associated edges
		CollectAssociated(edges, grid, f);
/*
		if(refiner.get_mark(f) == RM_NONE){
			bool gotOne = false;
			for(size_t i_edge = 0; i_edge < edges.size(); ++i_edge){
				EdgeBase* e = edges[i_edge];
				if(refiner.get_mark(e) != RM_NONE){
					gotOne = true;
					break;
				}
			}

			if(gotOne){
				for(size_t i_edge = 0; i_edge < edges.size(); ++i_edge){
					EdgeBase* e = edges[i_edge];
					if(refiner.get_mark(e) == RM_NONE){
						refiner.mark(e);
						CollectFaces(faces, grid, e);
						for(size_t i_face = 0; i_face < faces.size(); ++i_face){
							Face* nbr = faces[i_face];
							if(!grid.is_marked(nbr)
							   && (refiner.get_mark(nbr) == RM_ANISOTROPIC))
							{
								grid.mark(nbr);
								queFaces.push(nbr);
							}
						}
					}
				}
				refiner.mark(f);
			}
		}
		else */if(refiner.get_mark(f) == RM_ANISOTROPIC){
		//	find the shortest edge
			EdgeBase* minEdge = FindShortestEdge(edges.begin(), edges.end(), aaPos);
			UG_ASSERT(minEdge, "Associated edges of each face have to exist at this point.");
			number minLen = EdgeLength(minEdge, aaPos);

		//	check if a short edge and a long edge is selected
			bool longEdgeSelected = false;
			bool shortEdgeSelected = false;

			for(size_t i_edge = 0; i_edge < edges.size(); ++i_edge){
				EdgeBase* e = edges[i_edge];
				if(refiner.get_mark(e) == RM_NONE)
					continue;

				number len = EdgeLength(e, aaPos);
			//	to avoid division by 0, we only consider edges with a length > 0
				if(len > 0){
					if(minLen / len <= sizeRatio){
						longEdgeSelected = true;
					}
					else{
						shortEdgeSelected = true;
					}
				}
			}

		//	if a short edge and a long edge was selected, we'll have to mark all
		//	edges and push associated faces with anisotropic mark to the queue
			if(longEdgeSelected && shortEdgeSelected){
				for(size_t i_edge = 0; i_edge < edges.size(); ++i_edge){
					EdgeBase* e = edges[i_edge];
					if(refiner.get_mark(e) == RM_NONE){
					//	mark it and push associated anisotropic faces to the queue
						refiner.mark(e);
	//!!!

	if(ConstrainingEdge::type_match(e)){
		UG_LOG("CONSTRAINING EDGE MARKED (2)\n");
	}

						CollectFaces(faces, grid, e);
						for(size_t i_face = 0; i_face < faces.size(); ++i_face){
							Face* nbr = faces[i_face];
							if(!grid.is_marked(nbr)
							   && (refiner.get_mark(nbr) == RM_ANISOTROPIC))
							{
								grid.mark(nbr);
								queFaces.push(nbr);
							}
						}
					}
				}
			}
		}
	//	now unmark the face
		grid.unmark(f);
	}

	grid.end_marking();

//	mark unmarked neighbors of marked edges for regular refinement
/*
	for(FaceIterator iter = grid.begin<Face>();
		iter != grid.end<Face>(); ++iter)
	{
		Face* f = *iter;

	//	if it is already marked, leave it as it is
		if(refiner.get_mark(f) != RM_NONE)
			continue;

	//	ignore faces with children
		if(pmg && pmg->has_children(f))
			continue;

		for(size_t i = 0; i < f->num_edges(); ++i){
			if(refiner.get_mark(grid.get_side(f, i)) != RM_NONE)
				refiner.mark(f);
		}
	}*/

/*
//	now that all faces are marked, we can process volumes. We consider a
//	volume which has an anisotropic side as an anisotropic volume
	for(VolumeIterator iter = grid.begin<Volume>();
		iter != grid.end<Volume>(); ++iter)
	{
		Volume* v = *iter;

	//	collect associated faces
		CollectAssociated(faces, grid, v);

	//	find the smallest face
		Face* minFace = FindSmallestFace(faces.begin(), faces.end(), aaPos);
		UG_ASSERT(minFace, "Associated faces of each volume have to exist at this point.");
		number minArea = FaceArea(minFace, aaPos);

	//	compare all associated faces of v against minArea
		for(size_t i_face = 0; i_face < faces.size(); ++i_face){
			Face* f = faces[i_face];
			number area = FaceArea(f, aaPos);
		//	avoid division by 0
			if(area > 0){
				if(minArea / area <= sizeRatio){
				//	the face will be refined. If it is already marked, we'll
				//	leave it at that, to keep the anisotropy.
				//	If it wasn't marked, we'll mark it for full refinement
				//	(all anisotropic faces have already been marked).
					if(refiner.get_mark(f) == RM_NONE)
						refiner.mark(f);

				//	the volume itself now has to be marked, too.
					refiner.mark(v, RM_ANISOTROPIC);
				}
			}
		}
	}*/
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace bridge{

static bool RegisterRefinementBridge_DomIndep(Registry& reg, string parentGroup)
{
	try
	{
	//	get group string
		stringstream groupString; groupString << parentGroup << "/Refinement";
		string grp = groupString.str();

	//	register domain independent mark methods
		reg.add_function("MarkForRefinement_All", &MarkForRefinement_All, grp);
	}
	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterRefinementBridge_DomIndep: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
template <class TDomain>
static bool RegisterRefinementBridge_DomDep(Registry& reg, string parentGroup)
{
	typedef TDomain 							domain_type;
	typedef typename TDomain::position_type		pos_type;

	try
	{
	//	get group string
		stringstream groupString; groupString << parentGroup << "/Refinement";
		string grp = groupString.str();

	//	refiner factory-method registration
	//	Note that the refiners themselfs have already been registered in lib_grid_bridge.
		reg.add_function("GlobalDomainRefiner",
						 &GlobalDomainRefiner<domain_type>, grp);
		reg.add_function("HangingNodeDomainRefiner",
						 &HangingNodeDomainRefiner<domain_type>, grp);

	//	register domain dependent mark methods
		reg.add_function("MarkForRefinement_VerticesInSphere",
					&MarkForRefinement_VerticesInSphere<domain_type>, grp)
			.add_function("MarkForRefinement_EdgesInSphere",
					&MarkForRefinement_ElementsInSphere<domain_type, EdgeBase>, grp)
			.add_function("MarkForRefinement_FacesInSphere",
					&MarkForRefinement_ElementsInSphere<domain_type, Face>, grp)
			.add_function("MarkForRefinement_VolumesInSphere",
					&MarkForRefinement_ElementsInSphere<domain_type, Volume>, grp)
			.add_function("MarkForRefinement_VerticesInCube",
					&MarkForRefinement_VerticesInCube<domain_type>, grp)
			.add_function("MarkForRefinement_AnisotropicElements",
					&MarkForRefinement_AnisotropicElements<domain_type>, grp);
	}
	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterRefinementBridge_DomDep: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RegisterRefinementBridge(Registry& reg, string parentGroup)
{
	bool bSuccess = true;

	bSuccess &= RegisterRefinementBridge_DomIndep(reg, parentGroup);

#ifdef UG_DIM_1
	bSuccess &= RegisterRefinementBridge_DomDep<Domain<1, MultiGrid, MGSubsetHandler> >(reg, parentGroup);
#endif
#ifdef UG_DIM_2
	bSuccess &= RegisterRefinementBridge_DomDep<Domain<2, MultiGrid, MGSubsetHandler> >(reg, parentGroup);
#endif
#ifdef UG_DIM_3
	bSuccess &= RegisterRefinementBridge_DomDep<Domain<3, MultiGrid, MGSubsetHandler> >(reg, parentGroup);
#endif
	return bSuccess;
}

}// end of namespace
}// end of namespace
