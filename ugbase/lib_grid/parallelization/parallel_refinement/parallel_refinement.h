// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 24.05.2011 (m,d,y)

#ifndef __H__UG__parallel_refinement__
#define __H__UG__parallel_refinement__

#include "parallel_adaptive_refiner_t.h"
#include "parallel_global_refiner_t.h"

#include "lib_grid/algorithms/refinement/global_multi_grid_refiner.h"
#include "lib_grid/algorithms/refinement/hanging_node_refiner_multi_grid.h"

namespace ug
{

/// \addtogroup lib_grid_parallelization_refinement
/// @{

///	Parallel global refinement for multi-grids
typedef TParallelGlobalRefiner<GlobalMultiGridRefiner>
		ParallelGlobalRefiner_MultiGrid;

///	Parallel adaptive hanging node refinement for multi-grids
typedef TParallelAdaptiveRefiner<HangingNodeRefiner_MultiGrid>
		ParallelHangingNodeRefiner_MultiGrid;

///	@}

}//	end of namespace

#endif
