// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 25.05.2011 (m,d,y)

#ifndef __H__UG__partition_map__
#define __H__UG__partition_map__

#include "lib_grid/lg_base.h"

namespace ug
{

///	Used to describe how a domain shall be distributed in a parallel environment.
/**	A partition map holds a subset handler and a map, which specifies the
 * target process of each subset. Make sure to assign a grid before performing
 * the partitioning.
 *
 * \todo	a partition map should feature a constructor which takes a grid.
 */
class PartitionMap{
	public:
		void clear();

		void assign_grid(Grid& grid);

		SubsetHandler& get_partition_handler();

		void add_target_proc(int targetProcRank);

		void add_target_procs(int first, int num);

		size_t num_target_procs();

		int get_target_proc(size_t index);

		int* get_target_procs();

		std::vector<int>& get_target_proc_vec();

	///	changes an existing target process. Make sure that index < num_target_procs
		bool change_target_proc(size_t index, int newRank);

	///	returns the index at which the given process lies. -1 if it doesn't exist.
		int find_target_proc(int procRank);

	///	adds the given offset to all target-proc-ranks
		void shift_target_procs(int offset);

	private:
		SubsetHandler		m_shPartitions;
		std::vector<int>	m_targetProcs;
};

///	Save the partition map to a file.
/**	The resulting file will contain the grid on which the partition-map operates,
 * together with subsets, each representing the process on which the subset will
 * be sent.
 *
 * \todo	currently only the .ugx format is supported.
 */
template <class TAPos>
bool SavePartitionMapToFile(PartitionMap& pm, const char* filename,
							TAPos& aPos)
{
	SubsetHandler& partsh = pm.get_partition_handler();

//	make sure that a grid exists
	if(!partsh.get_assigned_grid()){
		UG_LOG("WARNING IN SavePartitionMapToFile: a grid has to be assigned "
				"to the PartitionMap. Aborting.\n");
		return false;
	}

	Grid& grid = *partsh.get_assigned_grid();

//	we need a subset-handler, which will have a 1-1 subset-process relation.
	SubsetHandler sh(grid);

//	add all partitions to the handler
	for(int si = 0; si < partsh.num_subsets(); ++si){
		int newSI = pm.get_target_proc(si);
		sh.assign_subset(partsh.begin<VertexBase>(si),
						 partsh.end<VertexBase>(si), newSI);
		sh.assign_subset(partsh.begin<EdgeBase>(si),
						 partsh.end<EdgeBase>(si), newSI);
		sh.assign_subset(partsh.begin<Face>(si),
						 partsh.end<Face>(si), newSI);
		sh.assign_subset(partsh.begin<Volume>(si),
						 partsh.end<Volume>(si), newSI);
	}

//	now save the grid to file
	return SaveGridToUGX(grid, sh, filename, aPos);
}

}//	end of namespace

#endif
