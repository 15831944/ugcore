//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m03 d23

#ifndef __H__LIB_GRID__PARALLEL_MULTI_GRID_REFINER__
#define __H__LIB_GRID__PARALLEL_MULTI_GRID_REFINER__

#include <vector>
#include "lib_grid/lg_base.h"
#include "lib_grid/multi_grid.h"
#include "lib_grid/algorithms/refinement/multi_grid_refiner.h"
#include "../distributed_grid.h"

namespace ug
{

//	predeclarations
template <class TLayout>
class RefinementMarkDistributor;

class ParallelMultiGridRefiner : public MultiGridRefiner
{
	friend class RefinementMarkDistributor<VertexLayout>;
	friend class RefinementMarkDistributor<EdgeLayout>;
	friend class RefinementMarkDistributor<FaceLayout>;
	friend class RefinementMarkDistributor<VolumeLayout>;
	
	public:
		//ParallelMultiGridRefiner();
		ParallelMultiGridRefiner(DistributedGridManager& distGridMgr);
		~ParallelMultiGridRefiner();

	protected:
		virtual void collect_objects_for_refine();

		virtual void refinement_step_begins();
		virtual void refinement_step_ends();
		
		virtual void set_rule(VertexBase* e, RefinementMark mark);
		virtual void set_rule(EdgeBase* e, RefinementMark mark);
		virtual void set_rule(Face* e, RefinementMark mark);
		virtual void set_rule(Volume* e, RefinementMark mark);

	/**	Distributes marks for all elements that are stored in
	 *	m_vNewlyMarkedInterface...
	 *	you may optionally pass a vector to which elements will
	 *	be written that received a mark during communication.
	 *	Elements will be appended to the existing ones.*/
		template <class TDistributor, class TCommunicator>
		void
		exchange_data(TDistributor& distributor,
						TCommunicator& communicator,
						std::vector<typename TDistributor::Element>* pvReceivedElemsOut = NULL);
					
		template <class TMarkDistributor>
		void mark_received_elements(TMarkDistributor& distributor);
		
		void clear_newly_marked_element_buffers();
		
	///	adjust selection based on received elements
		void adjust_parallel_selection(const std::vector<VertexBase*>* pvVrts,
										const std::vector<EdgeBase*>* pvEdges,
										const std::vector<Face*>* pvFaces,
										const std::vector<Volume*>* pvVolumes);
		
	protected:
		DistributedGridManager& m_distGridMgr;
		
		std::vector<VertexBase*>	m_vNewlyMarkedInterfaceVrts;
		std::vector<EdgeBase*>		m_vNewlyMarkedInterfaceEdges;
		std::vector<Face*>			m_vNewlyMarkedInterfaceFaces;
		std::vector<Volume*>		m_vNewlyMarkedInterfaceVols;
};

}//	end of namespace

#endif
