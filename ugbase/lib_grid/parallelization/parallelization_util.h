//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m06 d29

#ifndef __H__LIB_GRID__PARALLELIZATION_UTIL__
#define __H__LIB_GRID__PARALLELIZATION_UTIL__

#include "lib_grid/lg_base.h"
#include "distributed_grid.h"
#include <boost/function.hpp>

namespace ug
{
////////////////////////////////////////////////////////////////////////
// declarations of callback functions
///	allows the user to adjust a grid or the associated subset-handler
/**	A FuncAdjustGrid could be a simple function pointer to a function like:
 *	void SomeFunc(MultiGrid& mg, ISubsetHandler& sh).
 *	Alternativley you could use an instance of a class that features
 *	a method:
 *	void operator()(MultiGrid& mg, ISubsetHandler& sh).
 */
typedef boost::function<void (MultiGrid& mg, ISubsetHandler& sh)>
	FuncAdjustGrid;

///	allows the user to perform grid partitioning for load-balancing
/**	Note that it is sufficient to specifiy a partition for the
 *	top-level elements.
 *	A FuncPartitionGrid could be a simple function pointer to a function like:
 *	void SomeFunc(...).
 *	Alternativley you could use an instance of a class that features
 *	a method:
 *	void operator()(...).
 */
typedef boost::function<bool (SubsetHandler& partitionOut,
							  MultiGrid& mg, ISubsetHandler& sh,
							  size_t numProcs)>
	FuncPartitionGrid;
	

////////////////////////////////////////////////////////////////////////
//	default implementations of adjust grid methods
///	this method leaves everything as it is.
inline void DefaultAdjustGrid(MultiGrid&, ISubsetHandler&)	{}

///	this class can be used to refine the grid before it is distributed
class AdjustGrid_GlobalRefinement
{
	public:
		AdjustGrid_GlobalRefinement(size_t numRefinements);
		void operator()(MultiGrid& mg, ISubsetHandler& sh);
		
	protected:
		size_t	m_numRefinements;
};

///	this class can be used to auto-assign elements to subsets
/**	Unassigned elements are assigned to subsets depending on
 *	whether they are inner or boundary elements.
 *	if innerSubsetIndex or boundarySubsetIndex < -1, nothing will
 *	be assigned.*/
class AdjustGrid_AutoAssignSubsets
{
	public:
		AdjustGrid_AutoAssignSubsets(int innerSubsetIndex,
									 int boundarySubsetIndex);
		void operator()(MultiGrid& mg, ISubsetHandler& sh);
		
	protected:
		int m_innerSubsetIndex;
		int m_boundarySubsetIndex;
};

///	this class auto-assigns subsets and refines globally
/**	\sa ug::AdjustGrid_AutoAssignSubsets, ug::AdjustGrid_GlobalRefinement*/
class AdjustGrid_AutoAssignSubsetsAndRefine
{
	public:
		AdjustGrid_AutoAssignSubsetsAndRefine(int innerSubsetIndex,
											  int outerSubsetIndex,
											  size_t numRefinements);
		void operator()(MultiGrid& mg, ISubsetHandler& sh);
		
	protected:
		AdjustGrid_AutoAssignSubsets	m_assignSubsets;
		AdjustGrid_GlobalRefinement		m_globalRefinement;
};

////////////////////////////////////////////////////////////////////////
//	default implementations of grid-partitioning methods
///	partitions the grid by repeated bisection
/**	Volume grids are partitioned in 3 dimensions, face grids are
 *	partitioned in 2 dimensions and edge grids are partitioned in
 *	one dimension.
 */
bool PartitionGrid_Bisection(SubsetHandler& partitionOut,
							  MultiGrid& mg, ISubsetHandler& sh,
							  size_t numProcs);



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
///	loads and distributes a grid in a parallel environment
/**	Each process calls this method with the same parameters.
 *	Only process 0 will actually load the grid. It will then
 *	partition and distribute it to the other processes. If
 *	keepSrcGrid is set to true, process 0 will keep the
 *	complete source-grid. vertical interfaces are created in this case.
 *
 *	In order to adjust a grid before it is distributed, you may specify a
 *	callback function object. By default the grid is not changed after load.
 *
 *	In order to partition the grid you may also specify a grid-partitioning
 *	function object. By default binary bisection is performed by
 *	PartitionGrid_Bisection.
 */
bool LoadAndDistributeGrid(DistributedGridManager& distGridMgrOut,
						   ISubsetHandler& shOut, int numProcs,
						   const char* filename,
						   bool keepSrcGrid = false,
						   FuncAdjustGrid funcAdjustGrid = DefaultAdjustGrid,
						   FuncPartitionGrid funcPartitionGrid = PartitionGrid_Bisection);

}//	end of namespace

#endif
