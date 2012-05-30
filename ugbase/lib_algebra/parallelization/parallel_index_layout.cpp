/*
 * parallel_index_layout.cpp
 *
 *  Created on: May 30, 2012
 */

#include "common/common.h"
#include <vector>
#include <iomanip>
#include "pcl/pcl.h"
#include "parallel_index_layout.h"

namespace ug{

void LogIndexLayout(IndexLayout& layout, int depth)
{
	using namespace std;

	typedef IndexLayout::Interface Interface;
	typedef IndexLayout::iterator  InterfaceIter;

	UG_LOG("-- IndexLayout Informations: Proc "<< GetLogAssistant().get_output_process() << " --\n");

	UG_LOG(" interface | target proc id |   size    ");
	if(depth >= 1) UG_LOG(" | indices ")
	UG_LOG("\n");

	int i = 0;
	for(InterfaceIter iiter = layout.begin();
		iiter != layout.end(); ++iiter, ++i)
	{
		Interface& interface = layout.interface(iiter);
		UG_LOG(" " << std::setw(9) << i << " | " << std::setw(14) <<
		       layout.proc_id(iiter) << " | " << std::setw(9) << interface.size() << " ");
		if(depth >= 1)
		{
			UG_LOG(" | (");
			for(Interface::iterator indexIter = interface.begin();
					indexIter != interface.end(); ++indexIter)
			{
			//  get index
				const size_t index = interface.get_element(indexIter);

			//	add comma
				if(indexIter != interface.begin())
					UG_LOG(", ");

			//	log index
				UG_LOG(index);
			}
			UG_LOG(")");
		}
		UG_LOG("\n");
	}
	UG_LOG(endl);
}

void LogIndexLayoutOnAllProcs(IndexLayout& layout, int depth)
{
//	remember current outproc
	int outproc = GetLogAssistant().get_output_process();

//	loop all procs
	for(int p = 0; p < pcl::GetNumProcesses(); ++p)
	{
	//	synchronize, to prevent other procs to write before this one has finished.
		pcl::SynchronizeProcesses();

	//	write process p
		if(p == pcl::GetProcRank())
		{
		//	set output proc to proc p
			GetLogAssistant().set_output_process(p);

		//	write
			LogIndexLayout(layout, depth);
		}
	}
	pcl::SynchronizeProcesses();
	UG_LOG(std::flush);

//	reset output proc
	GetLogAssistant().set_output_process(outproc);
}

void ReplaceIndicesInLayout(IndexLayout& layout, const std::vector<int>& vMap)
{
//	interface iterators
	IndexLayout::iterator interfaceIter = layout.begin();
	IndexLayout::iterator interfaceEnd = layout.end();

//	iterate over interfaces
	for(; interfaceIter != interfaceEnd; ++interfaceIter)
	{
	//	get interface
		IndexLayout::Interface& interface = layout.interface(interfaceIter);

	//	loop over indices
		for(IndexLayout::Interface::iterator iter = interface.begin(); iter != interface.end();)
		{
		//  get index
			size_t& index = interface.get_element(iter);

		//	get new index
			const int newIndex = vMap[index];

		//	erase index if negative
			if(newIndex < 0)
				iter = interface.erase(iter);
		//	else replace index
			else
			{
				index = newIndex;
				 ++iter;
			}

		}
	}
}

}
