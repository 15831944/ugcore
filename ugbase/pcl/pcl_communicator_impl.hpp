//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m12 d05

#ifndef __H__PCL__PCL_COMMUNICATOR_IMPL__
#define __H__PCL__PCL_COMMUNICATOR_IMPL__

#include <cassert>
#include "mpi.h"
#include "pcl_methods.h"
#include "pcl_communication_structs.h"
#include "pcl_communicator.h"
#include "pcl_profiling.h"
#include "pcl_util.h"
#include "common/log.h"

namespace pcl
{

template <class TLayout>
ParallelCommunicator<TLayout>::
ParallelCommunicator() :
	m_bDebugCommunication(false),
	m_bSendBuffersFixed(true)
{
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
send_raw(int targetProc, void* pBuff, int bufferSize,
	     bool bSizeKnownAtTarget)
{
	assert(targetProc == -1 || targetProc >= 0 && targetProc < pcl::GetNumProcesses());

	ug::BinaryBuffer& buffer = m_bufMapOut[targetProc];
	m_curOutProcs.insert(targetProc);

	if(!bSizeKnownAtTarget)
		buffer.write((char*)&bufferSize, sizeof(int));
		
	buffer.write((char*)pBuff, bufferSize);
	m_bSendBuffersFixed = m_bSendBuffersFixed
						&& bSizeKnownAtTarget;
}
			   
////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
send_data(int targetProc, Interface& interface,
			  ICommunicationPolicy<TLayout>& commPol)
{
	if(!interface.empty()){
		assert(targetProc == -1 || targetProc >= 0 && targetProc < pcl::GetNumProcesses());

		ug::BinaryBuffer& buffer = m_bufMapOut[targetProc];
		m_curOutProcs.insert(targetProc);

		commPol.collect(buffer, interface);
		m_bSendBuffersFixed = m_bSendBuffersFixed
							&& (commPol.get_required_buffer_size(interface) >= 0);
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
send_data(Layout& layout, ICommunicationPolicy<TLayout>& commPol)
{
PCL_PROFILE(pcl_IntCom_send_layout_data);
//	through the the category_tag we're able to find the correct send method.
	send_data(layout, commPol, typename TLayout::category_tag());
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
send_data(Layout& layout,
		  ICommunicationPolicy<TLayout>& commPol,
		  const layout_tags::single_level_layout_tag&)
{
	if(layout.has_interface_elements()){
		typename Layout::iterator iter = layout.begin();
		typename Layout::iterator end = layout.end();

		commPol.begin_layout_collection(&layout);

		for(; iter != end; ++iter)
		{
			if(!layout.interface(iter).empty()){
				ug::BinaryBuffer& buffer = m_bufMapOut[layout.proc_id(iter)];
				m_curOutProcs.insert(layout.proc_id(iter));

				commPol.collect(buffer, layout.interface(iter));
				m_bSendBuffersFixed = m_bSendBuffersFixed
					&& (commPol.get_required_buffer_size(layout.interface(iter)) >= 0);
			}
		}

		commPol.end_layout_collection();
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
send_data(Layout& layout,
		  ICommunicationPolicy<TLayout>& commPol,
		  const layout_tags::multi_level_layout_tag&)
{
	if(layout.has_interface_elements()){
		commPol.begin_layout_collection(&layout);
		
		for(size_t i = 0; i < layout.num_levels(); ++i)
		{
			typename Layout::iterator iter = layout.begin(i);
			typename Layout::iterator end = layout.end(i);

			for(; iter != end; ++iter)
			{
				if(!layout.interface(iter).empty()){
					ug::BinaryBuffer& buffer = m_bufMapOut[layout.proc_id(iter)];
					m_curOutProcs.insert(layout.proc_id(iter));

					commPol.collect(buffer, layout.interface(iter));
					m_bSendBuffersFixed = m_bSendBuffersFixed
						&& (commPol.get_required_buffer_size(layout.interface(iter)) >= 0);
				}
			}
		}

		commPol.end_layout_collection();
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
receive_raw(int srcProc, ug::BinaryBuffer& bufOut, int bufSize)
{
	m_extractorInfos.push_back(ExtractorInfo(srcProc, NULL,
											 NULL, NULL,
											 NULL, &bufOut,
											 bufSize));
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
receive_raw(int srcProc, void* bufOut, int bufSize)
{
	m_extractorInfos.push_back(ExtractorInfo(srcProc, NULL,
											 NULL, NULL,
											 bufOut, NULL,
											 bufSize));
}
			
////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
receive_data(int srcProc, Interface& interface,
			ICommunicationPolicy<TLayout>& commPol)
{
	if(!interface.empty()){
		m_extractorInfos.push_back(ExtractorInfo(srcProc, &commPol
												 &interface, NULL,
												 NULL, NULL, 0));
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
receive_data(Layout& layout, ICommunicationPolicy<TLayout>& commPol)
{
	if(layout.has_interface_elements()){
		m_extractorInfos.push_back(ExtractorInfo(-1, &commPol,
												 NULL, &layout,
												 NULL, NULL, 0));
	}
}

template <class TLayout>
template <class TLayoutMap>
void ParallelCommunicator<TLayout>::
exchange_data(TLayoutMap& layoutMap,
				const typename TLayoutMap::Key& keyFrom,
				const typename TLayoutMap::Key& keyTo,
				ICommunicationPolicy<TLayout>& commPol)
{
	if(layoutMap.template has_layout<Type>(keyFrom)){
		send_data(layoutMap.template get_layout<Type>(keyFrom), commPol);
	}
		
	if(layoutMap.template has_layout<Type>(keyTo)){
		receive_data(layoutMap.template get_layout<Type>(keyTo), commPol);
	}
}
							
////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
prepare_receiver_buffer_map(BufferMap& bufMap,
							std::set<int>& curProcs,
							TLayout& layout)
{
	prepare_receiver_buffer_map(bufMap, curProcs, layout,
								typename TLayout::category_tag());
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
prepare_receiver_buffer_map(BufferMap& bufMap,
							std::set<int>& curProcs,
							TLayout& layout,
							const layout_tags::single_level_layout_tag&)
{
//	simply 'touch' the buffer to make sure it's in the map.
	for(typename TLayout::iterator li = layout.begin();
		li != layout.end(); ++li)
	{
		if(!layout.interface(li).empty()){
			bufMap[layout.proc_id(li)];
			curProcs.insert(layout.proc_id(li));
		}
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
prepare_receiver_buffer_map(BufferMap& bufMap,
							std::set<int>& curProcs,
							TLayout& layout,
							const layout_tags::multi_level_layout_tag&)
{
//	simply 'touch' the buffer to make sure it's in the map.
	for(size_t i = 0; i < layout.num_levels(); ++i)
	{
		for(typename TLayout::iterator li = layout.begin(i);
			li != layout.end(i); ++li)
		{
			if(!layout.interface(li).empty()){
				bufMap[layout.proc_id(li)];
				curProcs.insert(layout.proc_id(li));
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
bool ParallelCommunicator<TLayout>::
collect_layout_buffer_sizes(TLayout& layout,
							ICommunicationPolicy<TLayout>& commPol,
							std::map<int, int>* pMapBuffSizesOut,
							const layout_tags::single_level_layout_tag&)
{
	for(typename TLayout::iterator li = layout.begin();
		li != layout.end(); ++li)
	{
		if(!layout.interface(li).empty()){
		//	get the buffer size
			int buffSize = commPol.get_required_buffer_size(
									layout.interface(li));
			if(buffSize < 0){
			//	buffer sizes can't be determined
				return false;
			}
			else if(pMapBuffSizesOut){
			//	find the entry in the map
				std::map<int, int>::iterator iter = pMapBuffSizesOut->find(layout.proc_id(li));
				if(iter != pMapBuffSizesOut->end())
					iter->second += buffSize;
				else
					(*pMapBuffSizesOut)[layout.proc_id(li)] = buffSize;
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
bool ParallelCommunicator<TLayout>::
collect_layout_buffer_sizes(TLayout& layout,
							ICommunicationPolicy<TLayout>& commPol,
							std::map<int, int>* pMapBuffSizesOut,
							const layout_tags::multi_level_layout_tag&)
{
PCL_PROFILE_FUNC();
//	iterate through all interfaces
	for(size_t i = 0; i < layout.num_levels(); ++i){
		for(typename TLayout::iterator li = layout.begin(i);
			li != layout.end(i); ++li)
		{
			if(!layout.interface(li).empty()){
			//	get the buffer size
				int buffSize = commPol.get_required_buffer_size(
										layout.interface(li));
				if(buffSize < 0){
				//	buffer sizes can't be determined
					return false;
				}
				else if(pMapBuffSizesOut){
				//	find the entry in the map
					std::map<int, int>::iterator iter = pMapBuffSizesOut->find(layout.proc_id(li));
					if(iter != pMapBuffSizesOut->end())
						iter->second += buffSize;
					else
						(*pMapBuffSizesOut)[layout.proc_id(li)] = buffSize;
				}
			}
		}
	}
	return true;
}
										
////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
extract_data(TLayout& layout, BufferMap& bufMap, CommPol& extractor)
{
PCL_PROFILE_FUNC();
	extract_data(layout, bufMap,
				extractor,
				typename TLayout::category_tag());
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
extract_data(TLayout& layout, BufferMap& bufMap, CommPol& extractor,
				const layout_tags::single_level_layout_tag&)
{
	extractor.begin_level_extraction(0);
//	extract data for the layouts interfaces
	for(typename Layout::iterator li = layout.begin();
		li != layout.end(); ++li)
	{
		if(!layout.interface(li).empty()){
			extractor.extract(bufMap[layout.proc_id(li)],
							layout.interface(li));
		}
	}
}

////////////////////////////////////////////////////////////////////////
template <class TLayout>
void ParallelCommunicator<TLayout>::
extract_data(TLayout& layout, BufferMap& bufMap, CommPol& extractor,
				const layout_tags::multi_level_layout_tag&)
{
//	extract data for the layouts interfaces
	for(size_t i = 0; i < layout.num_levels(); ++i)
	{
		extractor.begin_level_extraction(i);
		for(typename Layout::iterator li = layout.begin(i);
			li != layout.end(i); ++li)
		{
			if(!layout.interface(li).empty()){
				extractor.extract(bufMap[layout.proc_id(li)],
								layout.interface(li));
			}
		}
	}
}
				
////////////////////////////////////////////////////////////////////////
template <class TLayout>
bool ParallelCommunicator<TLayout>::
communicate()
{
	PCL_PROFILE(pcl_IntCom_communicate);

	m_curInProcs.clear();

//	note that we won't free the memory in the stream-packs.
//	we will only reset their write and read pointers.
	for(BufferMap::iterator iter = m_bufMapIn.begin();
		iter != m_bufMapIn.end(); ++iter)
	{
		iter->second.clear();
	}


//	iterate through all registered extractors and create entries for
//	the source-processes in the map (by simply 'touching' the entry).
	for(typename ExtractorInfoList::iterator iter = m_extractorInfos.begin();
		iter != m_extractorInfos.end(); ++iter)
	{
		ExtractorInfo& info = *iter;
		if(info.m_srcProc > -1){
			m_bufMapIn[info.m_srcProc];
			m_curInProcs.insert(info.m_srcProc);
		}
		else
		{
			prepare_receiver_buffer_map(m_bufMapIn, m_curInProcs, *info.m_layout);
		}
	}

//	if communication_debugging is enabled, then we check whether scheduled
//	sends and receives match.
//todo: use m_curInProcs / m_curOutProcs instead.
/*
	if(communication_debugging_enabled()){
		if(!StreamPacksMatch(m_streamPackIn, m_streamPackOut, m_debugProcComm)){
			UG_LOG("ERROR in ParallelCommunicator::communicate(): send / receive mismatch. Aborting.\n");
			return false;
		}
	}
*/

//	number of in and out-streams.
	size_t	numOutStreams = m_curOutProcs.size();
	size_t	numInStreams = m_curInProcs.size();

//	used for mpi-communication.
	std::vector<MPI_Request> vSendRequests(numOutStreams);
	std::vector<MPI_Request> vReceiveRequests(numInStreams);
	
//	wait until data has been received
	std::vector<MPI_Status> vReceiveStates(numInStreams);//TODO: fix spelling!
	std::vector<MPI_Status> vSendStates(numOutStreams);//TODO: fix spelling!


////////////////////////////////////////////////
//	determine buffer sizes and communicate them if required.
	std::vector<int> vBufferSizesIn(numInStreams);
	bool allBufferSizesFixed = m_bSendBuffersFixed;
	
	if(allBufferSizesFixed)
	{
	//	a map with <procId, Size>. Will be used to collect stream-sizes
		std::map<int, int> mapBuffSizes;
	//	initialise all sizes with 0
		for(std::set<int>::iterator iter = m_curInProcs.begin();
			iter != m_curInProcs.end(); ++iter)
		{
			mapBuffSizes[*iter] = 0;
		}
		
	//	iterate over all extractors and collect the buffer sizes
		for(typename ExtractorInfoList::iterator iter = m_extractorInfos.begin();
			iter != m_extractorInfos.end(); ++iter)
		{
			ExtractorInfo& info = *iter;
			if(info.m_srcProc > -1){
			//	the extractor only has a single interface.
				int buffSize = -1;
				if(info.m_extractor)
					buffSize = info.m_extractor->get_required_buffer_size(*info.m_interface);
				else
					buffSize = info.m_rawSize;

				if(buffSize < 0){
					allBufferSizesFixed = false;
					break;
				}
				else
					mapBuffSizes[info.m_srcProc] += buffSize;
			}
			else{
				if(!collect_layout_buffer_sizes(*info.m_layout,
												*info.m_extractor,
												&mapBuffSizes,
												typename TLayout::category_tag()))
				{
					allBufferSizesFixed = false;
					break;
				}
			}
		}
		
	//	if all buffer sizes are fixed, we'll copy them to vBufferSizes.
	//	to reduce the amount of possible errors, we'll iterate over
	//	m_streamPackIn...
		if(allBufferSizesFixed){
			int counter = 0;
			for(std::set<int>::iterator iter = m_curInProcs.begin();
				iter != m_curInProcs.end(); ++iter, ++counter)
			{
				vBufferSizesIn[counter] = mapBuffSizes[*iter];
			}
		}
	}
	
//	if the buffer size could not be determined, we have to communicate it.
	if(!allBufferSizesFixed)
	{
		PCL_PROFILE(pcl_IntCom_communicateBufSizes);

		int sizeTag = 189345;//	an arbitrary number
		int counter;

	//	shedule receives first
		counter = 0;
		for(std::set<int>::iterator iter = m_curInProcs.begin();
			iter != m_curInProcs.end(); ++iter, ++counter)
		{
			MPI_Irecv(&vBufferSizesIn[counter], sizeof(int), MPI_UNSIGNED_CHAR,	
					*iter, sizeTag, MPI_COMM_WORLD, &vReceiveRequests[counter]);
		}

	//	send buffer sizes
		counter = 0;
		for(std::set<int>::iterator iter = m_curOutProcs.begin();
			iter != m_curOutProcs.end(); ++iter, ++counter)
		{
			int streamSize = (int)m_bufMapOut[*iter].write_pos();

			MPI_Isend(&streamSize, sizeof(int), MPI_UNSIGNED_CHAR,
					*iter, sizeTag, MPI_COMM_WORLD, &vSendRequests[counter]);
		}

	//	TODO: this can be improved:
	//		instead of waiting for all, one could wait until one has finished and directly
	//		start copying the data to the local receive buffer. Afterwards on could continue
	//		by waiting for the next one etc...
		MPI_Waitall(numInStreams, &vReceiveRequests[0], &vReceiveStates[0]);
		MPI_Waitall(numOutStreams, &vSendRequests[0], &vSendStates[0]);
//		PROFILE_END();
	}

//	we can now resize the receive buffers to their final sizes
	{
		PCL_PROFILE(pcl_IntCom_communicate_resizeRecvBufs);
		size_t counter = 0;
		for(std::set<int>::iterator iter = m_curInProcs.begin();
			iter != m_curInProcs.end(); ++iter, ++counter)
		{
			ug::BinaryBuffer& buf = m_bufMapIn[*iter];
			buf.reserve(vBufferSizesIn[counter]);
		//	since we will write the data to the associated buffer
		//	using a raw copy, we will also set the write-position
		//	at this point (the write position will not be used during raw copy).
			buf.set_write_pos(vBufferSizesIn[counter]);
		}
	}
	
//	if communication_debugging is enabled, we can now check whether associated
//	send / receive buffers have the same size.
//todo: operate on the sets m_curInProcs and m_curOutProcs.
/*
	if(communication_debugging_enabled()){
		if(!StreamPackBuffersMatch(m_streamPackIn, m_streamPackOut, m_debugProcComm)){
			UG_LOG("ERROR in ParallelCommunicator::communicate(): "
					"send / receive buffer size mismatch. Aborting.\n");
			return false;
		}
	}
*/
	
////////////////////////////////////////////////
//	communicate data.
	PCL_PROFILE(pcl_IntCom_communicateData);
	int dataTag = 749345;//	an arbitrary number

	UG_DLOG(LIB_PCL, 1, "receiving from procs:");

//	first shedule receives
	int counter = 0;
	for(std::set<int>::iterator iter = m_curInProcs.begin();
		iter != m_curInProcs.end(); ++iter, ++counter)
	{
		UG_DLOG(LIB_PCL, 1, " " << *iter
				<< "(" << vBufferSizesIn[counter] << ")");
		
		ug::BinaryBuffer& binBuf = m_bufMapIn[*iter];
	//	receive the data
		MPI_Irecv(binBuf.buffer(), vBufferSizesIn[counter], MPI_UNSIGNED_CHAR,
				*iter, dataTag, MPI_COMM_WORLD, &vReceiveRequests[counter]);
	}

	UG_DLOG(LIB_PCL, 1, "\nsending to procs:");

//	now send data
	counter = 0;
	for(std::set<int>::iterator iter = m_curOutProcs.begin();
		iter != m_curOutProcs.end(); ++iter, ++counter)
	{
		ug::BinaryBuffer& binBuf = m_bufMapOut[*iter];

		UG_DLOG(LIB_PCL, 1, " " << *iter
				<< "(" << binBuf.write_pos() << ")");

		MPI_Isend(binBuf.buffer(), binBuf.write_pos(), MPI_UNSIGNED_CHAR,
				*iter, dataTag, MPI_COMM_WORLD, &vSendRequests[counter]);
	}
	UG_DLOG(LIB_PCL, 1, "\n");

//	TODO: this can be improved:
//		instead of waiting for all, one could wait until one has finished and directly
//		start copying the data to the local receive buffer. Afterwards on could continue
//		by waiting for the next one etc...
	PCL_PROFILE(pcl_IntCom_MPIWait);
	MPI_Waitall(numInStreams, &vReceiveRequests[0], &vReceiveStates[0]);
	MPI_Waitall(numOutStreams, &vSendRequests[0], &vSendStates[0]);
	PCL_PROFILE_END();
	PCL_PROFILE_END();
	

//	call the extractors with the received data
	for(typename ExtractorInfoList::iterator iter = m_extractorInfos.begin();
		iter != m_extractorInfos.end(); ++iter)
	{
		ExtractorInfo& info = *iter;
		if(info.m_srcProc > -1)
		{
		//	extract the data for single proc
			ug::BinaryBuffer& binBuf = m_bufMapIn[info.m_srcProc];
		//	this can be either an interface, a void* buffer or a
		//	binary-stream.
			if(info.m_interface){
				info.m_extractor->extract(binBuf, *info.m_interface);
			}
			else if(info.m_buffer){
				binBuf.read((char*)info.m_buffer, info.m_rawSize);
			}
			else{
				assert(info.m_binBuffer && "ERROR in ParallelCommunicator::communicate: No valid receiver specified.");
				
				int rawSize = info.m_rawSize;
				if(rawSize < 0){
				//	the raw size is encoded in the buffer in this case.
					binBuf.read((char*)&rawSize, sizeof(int));
				}
				
				info.m_binBuffer->clear();
				info.m_binBuffer->reserve(rawSize);
				binBuf.read((char*)info.m_binBuffer->buffer(), rawSize);
			}
		}
		else
		{
		//	extract the data for a layout
		//	notify the extractor that extraction for a layout begins.
			info.m_extractor->begin_layout_extraction(info.m_layout);
			
		//	extract the data
			extract_data(*info.m_layout,
						m_bufMapIn,
						*info.m_extractor);
			
		//	notify the extractor that extraction is complete.
			info.m_extractor->end_layout_extraction();
		}
	}

//	clean up
	for(BufferMap::iterator iter = m_bufMapOut.begin();
		iter != m_bufMapOut.end(); ++iter)
	{
	//	clear does not free memory. Only resets read and write pointers.
		iter->second.clear();
	}

	m_curOutProcs.clear();
	m_extractorInfos.clear();

//	reset m_bSendBuffersFixed
	m_bSendBuffersFixed = true;
	
//	done
	return true;
}

template <class TLayout>
void ParallelCommunicator<TLayout>::
enable_communication_debugging(const ProcessCommunicator& involvedProcs)
{
	static bool bFirstTime = true;
	if(bFirstTime){
		UG_LOG("WARNING: Communication debugging enabled in ParallelCommunicator.");
		UG_LOG(" Expect performance penalty!\n");
		bFirstTime = false;
	}
	
	m_bDebugCommunication = true;
	m_debugProcComm = involvedProcs;
}
	 
template <class TLayout>
void ParallelCommunicator<TLayout>::
disable_communication_debugging()
{
	m_bDebugCommunication = false;
}

template <class TLayout>
bool ParallelCommunicator<TLayout>::
communication_debugging_enabled()
{
	return m_bDebugCommunication;
}

}//	end of namespace pcl

#endif
