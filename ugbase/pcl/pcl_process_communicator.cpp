//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m07 d06

#include <vector>
#include <cassert>
#include "pcl_methods.h"
#include "common/util/smart_pointer.h"
#include "pcl_process_communicator.h"
#include "common/log.h"
#include "common/assert.h"
#include "common/util/vector_util.h"
#include "pcl_profiling.h"

using namespace std;
using namespace ug;

namespace pcl
{

ProcessCommunicator::
ProcessCommunicator(ProcessCommunicatorDefaults pcd)
{
	switch(pcd)
	{
		case PCD_EMPTY:
			m_comm = SPCommWrapper(new CommWrapper(MPI_COMM_NULL, false));
			break;

		case PCD_WORLD:
			m_comm = SPCommWrapper(new CommWrapper(MPI_COMM_WORLD, false));
			break;
	}
}

size_t ProcessCommunicator::
size() const
{
	if(m_comm->m_mpiComm == MPI_COMM_NULL)
		return 0;
	
	int size;
	if(MPI_Comm_size(m_comm->m_mpiComm, &size) == MPI_SUCCESS)
		return (size_t)size;
		
	UG_LOG("  ERROR in ProcessCommunicator::size(): Unknown MPI Error. Returning 0.\n");
	return 0;
}

int ProcessCommunicator::
get_proc_id(size_t groupIndex) const
{
	if(m_comm->m_mpiComm == MPI_COMM_WORLD)
		return (int)groupIndex;
	return m_comm->m_procs[groupIndex];
}

int ProcessCommunicator::
get_local_proc_id(int globalProcID) const
{
	if(m_comm->m_mpiComm == MPI_COMM_WORLD)
		return globalProcID;

	const vector<int>& procs = m_comm->m_procs;
	if(globalProcID == pcl::GetProcRank())
	{
		int rank;
		MPI_Comm_rank(m_comm->m_mpiComm, &rank);
		UG_ASSERT(procs[rank] == pcl::GetProcRank(), "?");
		return rank;
	}

	for(size_t i = 0; i < procs.size(); ++i){
		if(globalProcID == procs[i])
			return (int)i;
	}

	UG_LOG("  ERROR in ProcessCommunicator::get_local_proc_id(): given id not contained in local proc list.\n");
	return -1;
}


ProcessCommunicator
ProcessCommunicator::
create_sub_communicator(bool participate) const
{
	PCL_PROFILE(pcl_ProcCom_create_sub_com);

//	if the current communicator is empty theres nothing to do
	if(empty())
		return ProcessCommunicator(PCD_EMPTY);

//	get the number of processes in the current communicator
	int size;
	MPI_Comm_size(m_comm->m_mpiComm, &size);

	if(size < 1)
		return ProcessCommunicator(PCD_EMPTY);

//	create a buffer and initialise it with 0
	vector<int> srcArray(size, 0);

//	if the process wants to participate, set his entry to 1
	int rank;
	MPI_Comm_rank(m_comm->m_mpiComm, &rank);
	if(participate)
		srcArray[rank] = 1;

//	synchronize the newProcs array between all processes in the communicator
	vector<int> destArray(size, 0);

	allreduce(&srcArray.front(), &destArray.front(),
			   size, PCL_DT_INT, PCL_RO_MAX);

//	build a local array that holds all the procs that shall go
//	into the new communicator
	vector<int> newProcs;
	newProcs.reserve(size);

	// note: ranks are ranks in the (group!) communicator m_comm->m_mpiComm
	// since we building the new group relative to the old,
	// we add to newProcs the group ranks
	// these are NOT the global ranks like in pcl::GetProcRank
	for(size_t i = 0; i < destArray.size(); ++i){
		if(destArray[i])
			newProcs.push_back(i);
	}

//	if newProcs is not empty, we'll build a new mpi-communicator.
	if(newProcs.empty())
		return ProcessCommunicator(PCD_EMPTY);

	MPI_Group grpOld;
	MPI_Group grpNew;
	MPI_Comm commNew;

	MPI_Comm_group(m_comm->m_mpiComm, &grpOld);
	MPI_Group_incl(grpOld, (int)newProcs.size(), &newProcs.front(), &grpNew);
	MPI_Comm_create(m_comm->m_mpiComm, grpNew, &commNew);

//	create a new ProcessCommunicator
//	if the process is not participating, MPI_Comm_create will return MPI_COMM_NULL
	if(commNew == MPI_COMM_NULL)
		return ProcessCommunicator(PCD_EMPTY);

	// calculate global ranks for our newProcs array:
	for(size_t i = 0; i < newProcs.size(); ++i)
		newProcs[i] = get_proc_id(newProcs[i]);

	// note: since get_proc_rank uses newProcs, don't sort newProcs
	// otherwise above code won't work. here it is now
	// newProcs[group rank] = global rank. (!)

//	the process participates - create the ProcessCommunicator
	ProcessCommunicator newProcComm;
	newProcComm.m_comm = SPCommWrapper(new CommWrapper(commNew, true));
	newProcComm.m_comm->m_procs = newProcs;

	return newProcComm;
}

void
ProcessCommunicator::
allreduce(const void* sendBuf, void* recBuf, int count,
		  DataType type, ReduceOperation op) const
{
	PCL_PROFILE(pcl_ProcCom_allreduce);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::allreduce: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::allreduce: empty communicator.\n");
	}

	MPI_Allreduce(const_cast<void*>(sendBuf), recBuf, count, type, op, m_comm->m_mpiComm);
}

void
ProcessCommunicator::
gather(const void* sendBuf, int sendCount, DataType sendType,
	   void* recBuf, int recCount, DataType recType, int root) const
{
	PCL_PROFILE(pcl_ProcCom_gather);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::gather: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::gather: empty communicator.\n");
	}
	
	MPI_Gather(const_cast<void*>(sendBuf), sendCount, sendType, recBuf,
			   recCount, recType, root, m_comm->m_mpiComm);
}

void
ProcessCommunicator::
gatherv(const void* sendBuf, int sendCount, DataType sendType,
		void* recBuf, int* recCounts, int* displs,
		DataType recType, int root) const
{
	PCL_PROFILE(pcl_ProcCom_gatherv);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::gather: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::gather: empty communicator.\n");
	}

	MPI_Gatherv(const_cast<void*>(sendBuf), sendCount, sendType, recBuf,
				recCounts, displs, recType, root, m_comm->m_mpiComm);
}

void
ProcessCommunicator::
allgather(const void* sendBuf, int sendCount, DataType sendType,
		  void* recBuf, int recCount, DataType recType) const
{
	PCL_PROFILE(pcl_ProcCom_allgather);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::allgather: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::allgather: empty communicator.\n");
	}
	
	MPI_Allgather(const_cast<void*>(sendBuf), sendCount, sendType, recBuf,
				  recCount, recType, m_comm->m_mpiComm);
}

void
ProcessCommunicator::
allgatherv(const void* sendBuf, int sendCount, DataType sendType,
			void* recBuf, int* recCounts, int* displs,
			DataType recType) const
{
	PCL_PROFILE(pcl_ProcCom_allgatherv);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::allgatherv: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::allgatherv: empty communicator.\n");
	}
	
	MPI_Allgatherv(const_cast<void*>(sendBuf), sendCount, sendType, recBuf,
				   recCounts, displs, recType, m_comm->m_mpiComm);
}

void
ProcessCommunicator::
send_data(void* pBuffer, int bufferSize, int destProc, int tag) const
{
	PCL_PROFILE(pcl_ProcCom_send_data);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::send_data: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::send_data: empty communicator.\n");
	}
	
	MPI_Request request;
	MPI_Status	status;
	
	MPI_Isend(pBuffer, bufferSize, MPI_UNSIGNED_CHAR, destProc, 
			  tag, m_comm->m_mpiComm, &request);
	MPI_Wait(&request, &status);
}

void
ProcessCommunicator::
send_data(void* pBuffer, int* pBufferSegSizes,
		  int* pRecProcMap, int numRecProcs, int tag) const
{
	PCL_PROFILE(pcl_ProcCom_send_data__to_many);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::send_data: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::send_data: empty communicator.\n");
	}

//	send data
	std::vector<MPI_Request> vSendRequests(numRecProcs);
		
	for(int i = 0; i < numRecProcs; ++i)
	{
		MPI_Isend(pBuffer, pBufferSegSizes[i], MPI_UNSIGNED_CHAR,
				  pRecProcMap[i], tag, m_comm->m_mpiComm, &vSendRequests[i]);
		pBuffer = (byte*)pBuffer + pBufferSegSizes[i];
	}
	
//	wait until data has been received
	if(numRecProcs > 0){
		std::vector<MPI_Status> vSendStates(numRecProcs);
		MPI_Waitall(numRecProcs, &vSendRequests.front(), &vSendStates.front());
	}
}

void
ProcessCommunicator::
receive_data(void* pBuffOut, int bufferSize, int srcProc, int tag) const
{
	PCL_PROFILE(pcl_ProcCom_recv_data);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::receive_data: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::receive_data: empty communicator.\n");
	}
	
	MPI_Request request;
	MPI_Status	status;
	
	MPI_Irecv(pBuffOut, bufferSize, MPI_UNSIGNED_CHAR,	
					srcProc, tag, m_comm->m_mpiComm, &request);
					
	MPI_Wait(&request, &status);
}

void ProcessCommunicator::
distribute_data(void* pBufferOut, int* pBufferOutSegSizes,
				int* pSenderProcMap, int numSenderProcs,
				void* pBuffer, int* pBufferSegSizes,
			  	int* pRecvProcMap, int numRecvProcs, int tag) const
{
	PCL_PROFILE(pcl_ProcCom_distribute_data);

	assert(!empty() &&
			"ERROR in ProcessCommunicator::distribute_data: empty communicator.");
	if(empty()){
		UG_LOG("ERROR in ProcessCommunicator::distribute_data: empty communicator.\n");
	}

//	used for mpi-communication.
	std::vector<MPI_Request> vSendRequests(numRecvProcs);
	std::vector<MPI_Request> vReceiveRequests(numSenderProcs);
	
//	wait until data has been received
	std::vector<MPI_Status> vSendStates(numRecvProcs);
	std::vector<MPI_Status> vReceiveStates(numSenderProcs);

//	shedule receives first
	for(int i = 0; i < numSenderProcs; ++i)
	{
		MPI_Irecv(pBufferOut, pBufferOutSegSizes[i], MPI_UNSIGNED_CHAR,	
				  pSenderProcMap[i], tag, m_comm->m_mpiComm,
				  &vReceiveRequests[i]);
		pBufferOut = (byte*)pBufferOut + pBufferOutSegSizes[i];
	}

//	now send the data
	for(int i = 0; i < numRecvProcs; ++i)
	{
		MPI_Isend(pBuffer, pBufferSegSizes[i], MPI_UNSIGNED_CHAR,
				  pRecvProcMap[i], tag, m_comm->m_mpiComm,
				  &vSendRequests[i]);
		pBuffer = (byte*)pBuffer + pBufferSegSizes[i];
	}

	if(numSenderProcs > 0)
		MPI_Waitall(numSenderProcs, &vReceiveRequests.front(), &vReceiveStates.front());
	if(numRecvProcs > 0)
		MPI_Waitall(numRecvProcs, &vSendRequests.front(), &vSendStates.front());
}

void ProcessCommunicator::
distribute_data(ug::BinaryStream& recvBufOut, int* segSizesOut,
				int* recvFromRanks, int numRecvFroms,
				void* sendBuf, int* sendSegSizes,
				int* sendToRanks, int numSendTos) const
{
	PCL_PROFILE(pcl_ProcCom_distribute_data__flex);

//	small helper arrays
	vector<int> tmpRecvSegSizes(numRecvFroms, sizeof(int));
	vector<int> tmpSendSegSizes(numSendTos, sizeof(int));

//	every process receives the size of the data-buffer first.
	vector<int> bufferSizes(numRecvFroms);

//	exchange buffer sizes (use an arbitrary tag)
	distribute_data(GetDataPtr(bufferSizes), GetDataPtr(tmpRecvSegSizes),
					recvFromRanks, numRecvFroms,
					sendSegSizes, GetDataPtr(tmpSendSegSizes),
					sendToRanks, numSendTos, 89347);

//	calculate buffer sizes and resize the binary stream
	int totalSize = 0;
	for(int i = 0; i < numRecvFroms; ++i){
		totalSize += bufferSizes[i];
	}

	recvBufOut.resize(totalSize);

//	now exchange the buffers
	distribute_data(recvBufOut.buffer(), GetDataPtr(bufferSizes),
					recvFromRanks, numRecvFroms,
					sendBuf, sendSegSizes,
					sendToRanks, numSendTos, 3458);
}

void ProcessCommunicator::
barrier() const
{
	PCL_PROFILE(pcl_ProcCom_barrier);
	MPI_Barrier(m_comm->m_mpiComm);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
ProcessCommunicator::CommWrapper::
CommWrapper(const MPI_Comm& comm,
			bool bReleaseComm) :
	m_mpiComm(comm),
	m_bReleaseCommunicator(bReleaseComm)
{}

ProcessCommunicator::CommWrapper::
~CommWrapper()
{
	if(m_bReleaseCommunicator)
		MPI_Comm_free(&m_mpiComm);
}

}//	end of namespace pcl
