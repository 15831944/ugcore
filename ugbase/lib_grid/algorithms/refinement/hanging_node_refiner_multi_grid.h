// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 11.01.2011 (m,d,y)

#ifndef __H__UG__HANGIN_NODE_REFINER_MULTI_GRID__
#define __H__UG__HANGIN_NODE_REFINER_MULTI_GRID__

#include "hanging_node_refiner_base.h"

namespace ug
{

///	\addtogroup lib_grid_algorithms_refinement
///	@{

///	Specialization of ug::HangingNodeRefiner for ug::MultiGrid
/**	This class should be used, if hanging node refinement shall be
 * applied on a hierarchical grid (ug::MultiGrid).
 *
 * New elements will be constructed one level above their parent elements.
 *
 * HangingNodeRefiner_MultiGrid supports coarsening. Please note that coarsening
 * presumably has not yet reached its final implementation and behavior may
 * thus change in future revisions.
 * In the current implementation, refinement marks are removed during coarsening
 * and coarsening marks are removed during refinement. In order to use
 * coarsening and refinement on thus should first mark all elements which shall
 * be coarsened, perform coarsening, then mark all elements will shall be
 * refined and perform refinement (or vice versa).
 *
 * Take a look at ug::HangingNodeRefinerBase for a more in-depth documentation.
 *
 * \todo: 	Avoid the removal of refinement marks during coarsening and of
 * 			coarsening marks during refinement.
 *
 * \sa ug::HangingNodeRefinerBase, ug::HangingNodeRefiner_Grid
 */
class HangingNodeRefiner_MultiGrid : public HangingNodeRefinerBase
{
	public:
		using HangingNodeRefinerBase::mark;

	public:
		HangingNodeRefiner_MultiGrid(IRefinementCallback* refCallback = NULL);
		HangingNodeRefiner_MultiGrid(MultiGrid& mg,
									IRefinementCallback* refCallback = NULL);

		virtual ~HangingNodeRefiner_MultiGrid();

		void assign_grid(MultiGrid& mg);
		virtual Grid* get_associated_grid()		{return m_pMG;}

	///	performs coarsening on the elements marked with RM_COARSEN.
	/**
	 * The grid's message hub is informed using a "GridAdaption" message,
	 * passing an instance of GridMessage_Adapation, with values
	 * GMAT_HNODE_COARSENING_BEGINS and GMAT_HNODE_COARSENING_ENDS.
	 * See lib_grid/lib_grid_messages.h for more details.
	 *
	 * automatically adjusts the selection so that only valid coarsening operations
	 * are executed. Not that refinement marks are removed in this process.
	 *
	 * During a coarsening step only elements from the surface layer are removed.
	 * If not all children of a sub-surface element are marked, then the marks are ignored.
	 *
	 * Note that coarsen in contrary to refine is conservative. While refine
	 * extends the selection to construct a valid grid, coarsen shrinks the
	 * selection. On could think of implementing an alternative non conservative
	 * coarsen approach. However, the conservative one is the one mostly used
	 * in adaptive multigrid methods and was thus chosen here.
	 */
		virtual bool coarsen();

	protected:
	///	a callback that allows to deny refinement of special vertices
		virtual bool refinement_is_allowed(VertexBase* elem);
	///	a callback that allows to deny refinement of special edges
		virtual bool refinement_is_allowed(EdgeBase* elem);
	///	a callback that allows to deny refinement of special faces
		virtual bool refinement_is_allowed(Face* elem);
	///	a callback that allows to deny refinement of special volumes
		virtual bool refinement_is_allowed(Volume* elem);

	///	performs registration and deregistration at a grid.
	/**	Initializes all grid related variables.
	 *  call set_grid(NULL) to unregister the observer from a grid.
	 *
	 * 	Please note that though the base grid features a set_grid method,
	 *  it is not declared virtual. This is because we want to call it
	 *  during construction and destruction.*/
		void set_grid(MultiGrid* mg);

	///	prepares selection and calls the base implementation
	/**	Makes sure that no elements with children are selected,
	 *  Additionally vertices are marked, which have no children but
	 *  associated elements which are marked for refinement.*/
		virtual void collect_objects_for_refine();

	///	creates required vertices in higher levels.
		virtual void pre_refine();

	/**	Calls the base implementation and passes the mg-child vertices as
	 *  newCornerVrts. If newCornerVrts were passed to this method, they
	 *  are ignored.
	 *  \{*/
		virtual void refine_edge_with_normal_vertex(EdgeBase* e,
											VertexBase** newCornerVrts = NULL);
		virtual void refine_edge_with_hanging_vertex(EdgeBase* e,
											VertexBase** newCornerVrts = NULL);

		virtual void refine_face_with_normal_vertex(Face* f,
											VertexBase** newCornerVrts = NULL);
		virtual void refine_face_with_hanging_vertex(Face* f,
											VertexBase** newCornerVrts = NULL);

		virtual void refine_volume_with_normal_vertex(Volume* v,
											VertexBase** newVolumeVrts = NULL);
	/*	\} */

	///	Returns the vertex associated with the edge
		virtual VertexBase* get_center_vertex(EdgeBase* e);

	///	Associates a vertex with the edge.
		virtual void set_center_vertex(EdgeBase* e, VertexBase* v);

	///	Returns the vertex associated with the face
		virtual VertexBase* get_center_vertex(Face* f);

	///	Associates a vertex with the face.
		virtual void set_center_vertex(Face* f, VertexBase* v);

	///	calls base implementation and replaces cge with a normal edge.
		virtual void refine_constraining_edge(ConstrainingEdge* cge);


	///	collects corner vertices and fills them into the associated vector
	/**	The size of cornersOut is automatically adjusted.
	 *  The i-th element of corners out will contain the child vertex of the
	 *  i-th vertex of elem.
	 */
		template <class TElem>
		void collect_child_corners(std::vector<VertexBase*>& cornersOut, TElem* elem)
		{
			cornersOut.resize(elem->num_vertices());
			for(size_t i = 0; i < elem->num_vertices(); ++i){
				//UG_ASSERT(m_pMG->get_child_vertex(elem->vertex(i)), "A child vertex has to exists!");
				cornersOut[i] = m_pMG->get_child_vertex(elem->vertex(i));
			}
		}


	///	Deselects all non-surface elements and all elements not marked with RM_COARSEN
		template <class TElem>
		void restrict_selection_to_surface_coarsen_elements();

	///	Only complete families (all siblings are selected) may be coarsened.
	/**	When calling this method, make sure that only surface elements are
	 * marked/selected and that only the mark RM_COARSEN is used.*/
		template <class TElem>
		void restrict_selection_to_coarsen_families();

	///	Applies marks like RM_COARSEN_CONSTRAINING or RM_COARSEN_UNCONSTRAIN
	/**	This method should first be called for Face, then for EdgeBase, then for
	 * VertexBase.*/
		template <class TElem>
		void adjust_coarsen_marks_on_side_elements();

	///deselect coarsen families, which are adjacent to unselected constraining elements
		template <class TElem>
		void deselect_invalid_coarsen_families();

	///	called be the coarsen method in order to adjust the selection to valid elements.
	/**	This method is responsible to mark all elements that shall be coarsened.
	 * Only sub-surface elements may be coarsened. If a sub-surface element has
	 * a side, which is not a sub-surface element, then the element may not be
	 * coarsened.*/
		virtual void collect_objects_for_coarsen();

	private:
		MultiGrid*	m_pMG;
};

/// @}	// end of add_to_group command

}//	end of namespace

#endif
