/*
 * global_layout.cpp
 *
 *  Created on: 02.08.2011
 *      Author: mrupp
 */

#ifndef UG_PARALLEL
#error "This only works with a UG_PARALLEL define."
#endif

#include "global_layout.h"

namespace ug
{

void DeserializeAndAddGlobalInterface(BinaryBuffer &stream,	std::vector<AlgebraID> &interface)
{
	std::vector<AlgebraID> tmp;
	Deserialize(stream, tmp);
	interface.insert(interface.end(), tmp.begin(), tmp.end());
}

void DeserializeAndAddGlobalLayout(BinaryBuffer &stream, GlobalLayout &globalLayout)
{
	size_t size;
	Deserialize(stream, size);
	for(size_t i=0; i<size; i++)
	{
		int pid = Deserialize<int>(stream);
		DeserializeAndAddGlobalInterface(stream, globalLayout[pid]);
	}
}

void PrintGlobalLayout(const GlobalLayout &globalLayout, const char *name)
{
	UG_LOG("GlobalLayout ");
	if(name) UG_DLOG(LIB_ALG_AMG, 4, name);
	UG_LOG("\n");

	for(GlobalLayout::const_iterator iter = globalLayout.begin(); iter != globalLayout.end(); ++iter)
	{
		int pid = iter->first;
		const std::vector<AlgebraID> &v = iter->second;
		UG_LOG("to processor " << pid << "\n");
		for(size_t i=0; i<v.size(); i++)
		{
			UG_LOG(" " << v[i] << "\n");
		}
	}
}

void ReceiveGlobalLayout(pcl::ParallelCommunicator<IndexLayout> &comm, std::vector<int> &srcprocs,
		GlobalLayout &globalMasterLayout, GlobalLayout &globalSlaveLayout)
{
	typedef std::map<int, BinaryBuffer> BufferMap;
	BufferMap streams;

	for(size_t i=0; i<srcprocs.size(); i++)
		comm.receive_raw(srcprocs[i], streams[srcprocs[i]]);

	comm.communicate();
	for(size_t i=0; i<streams.size(); i++)
	{
		BinaryBuffer &stream = streams[srcprocs[i]];
		UG_DLOG(LIB_ALG_AMG, 4, "received " << stream.write_pos() << " bytes from " << srcprocs[i] << "\n");
		DeserializeAndAddGlobalLayout(stream, globalMasterLayout);
		DeserializeAndAddGlobalLayout(stream, globalSlaveLayout);
	}
}

void SerializeGlobalLayout(BinaryBuffer &stream, GlobalLayout &globalLayout)
{
	size_t size = globalLayout.size();
	Serialize(stream, size);
	UG_DLOG(LIB_ALG_AMG, 4, " size = " << size << "\n");
	for(GlobalLayout::iterator iter = globalLayout.begin(); iter != globalLayout.end(); ++iter)
	{
		int pid = iter->first;
		std::vector<AlgebraID> &v = iter->second;
		//UG_DLOG(LIB_ALG_AMG, 4, " " << pid << " - " << v << "\n");
		//if(v.size() == 0) continue;
		Serialize(stream, pid);
		Serialize(stream, v);
	}
}

void SendGlobalLayout(pcl::ParallelCommunicator<IndexLayout> &comm, GlobalLayout &globalMasterLayout, GlobalLayout &globalSlaveLayout, int pid)
{
	UG_DLOG(LIB_ALG_AMG, 4, "sending to " << pid << "\n");
	BinaryBuffer stream;
	UG_DLOG(LIB_ALG_AMG, 4, "serialize globalMasterLayout\n");
	SerializeGlobalLayout(stream, globalMasterLayout);
	UG_DLOG(LIB_ALG_AMG, 4, "serialize globalSlaveLayout\n");
	SerializeGlobalLayout(stream, globalSlaveLayout);

	UG_DLOG(LIB_ALG_AMG, 4, "size is " << stream.write_pos() << "\n");
	comm.send_raw(pid, stream.buffer(), stream.write_pos(), false);
	comm.communicate();
}


void MergeGlobalLayout(GlobalLayout &globalLayout, std::map<int, int> &merge)
{
	for(std::map<int, int>::iterator it = merge.begin(); it != merge.end(); ++it)
	{
		std::vector<AlgebraID> &a = globalLayout[it->first];

		if(it->second != pcl::GetProcRank())
		{
			std::vector<AlgebraID> &b = globalLayout[it->second];
			b.insert(b.end(), a.begin(), a.end());
		}
		globalLayout.erase(it->first);
	}
	// we don't want interfaces to ourselfs
	globalLayout.erase(pcl::GetProcRank());
}

}
