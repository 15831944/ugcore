// created by Martin Rupp, 31.01.2011

#ifndef __H__PCL_TEST_LAYOUTS__
#define __H__PCL_TEST_LAYOUTS__

#include <vector>
#include <iomanip> // for 'std::setw()' etc.
#include <map>

#include "pcl_base.h"
#include "pcl_methods.h"
#include "pcl_communication_structs.h"
#include "pcl_communicator.h"
#include "pcl_process_communicator.h"

#include "common/util/binary_buffer.h"
#include "common/log.h"
#include "common/assert.h"
#include "common/serialization.h"

#include <boost/function.hpp>

namespace pcl
{

///	Trivial implementation of a to-value callback.
/**	TValue has to be constructable from TElem.*/
template <class TElem>
TElem TrivialToValue(TElem e)
{
	return e;
}
inline void PrintPC(const pcl::ProcessCommunicator &processCommunicator)
{
	UG_LOG(processCommunicator.size() << " involved procs: ");
	for(size_t i=0; i<processCommunicator.size(); i++)
	{	UG_LOG(processCommunicator.get_proc_id(i) << " "); }
	UG_LOG("\n");

}

#define PRINTPC(com) UG_LOG(__FUNCTION__ << " : " << __LINE__ << "\n"); PrintPC(com)
////////////////////////////////////////////////////////////////////////
//  TestLayoutIsDoubleEnded
/// tests if masterLayouts proc id's find a match in correspoding slaveLayouts proc ids.
/**
 * that is, iff processor P1 has processor P2 in his masterLayout, then processor P2 needs to have P1 in his slaveLayout
 */
template<typename TLayout>
bool TestLayoutIsDoubleEnded(const pcl::ProcessCommunicator processCommunicator,
		pcl::InterfaceCommunicator<TLayout> &com, TLayout &masterLayout, TLayout &slaveLayout)
{
	//PRINTPC(processCommunicator);
//	check if connections are double-ended
	std::vector<char> bMasterToProcess; bMasterToProcess.resize(processCommunicator.size(), 0x00);
	std::vector<char> bSlaveToProcess; bSlaveToProcess.resize(processCommunicator.size(), 0x00);

	for(typename TLayout::iterator iter = masterLayout.begin(); iter != masterLayout.end(); ++iter)
	{
		int id = processCommunicator.get_local_proc_id(masterLayout.proc_id(iter));
		if(id == -1)
			UG_LOG("Processor " << masterLayout.proc_id(iter) << " not in processCommunicator, but in MasterLayout\n")
		else bMasterToProcess[id] = true;
	}
	for(typename TLayout::iterator iter = slaveLayout.begin(); iter != slaveLayout.end(); ++iter)
	{
		int id = processCommunicator.get_local_proc_id(slaveLayout.proc_id(iter));
		if(id == -1)
			UG_LOG("Processor " << slaveLayout.proc_id(iter) << " not in processCommunicator, but in SlaveLayout\n")
		else bSlaveToProcess[id] = true;
	}

	for(size_t i=0; i<processCommunicator.size(); i++)
	{
		com.send_raw(processCommunicator.get_proc_id(i), &bMasterToProcess[i], sizeof(char));
		com.send_raw(processCommunicator.get_proc_id(i), &bSlaveToProcess[i], sizeof(char));
	}

	std::vector<ug::BinaryBuffer> masterToThisProcessMap(processCommunicator.size());
	std::vector<ug::BinaryBuffer> slaveToThisProcessMap(processCommunicator.size());
	for(size_t i=0; i < processCommunicator.size(); i++)
	{
		com.receive_raw(processCommunicator.get_proc_id(i), masterToThisProcessMap[i]);
		com.receive_raw(processCommunicator.get_proc_id(i), slaveToThisProcessMap[i]);
	}

	com.communicate();

	for(size_t i=0; i<processCommunicator.size(); i++)
	{
		char bMasterToThisProcess, bSlaveToThisProcess;
		ug::Deserialize(masterToThisProcessMap[i], bMasterToThisProcess);
		ug::Deserialize(slaveToThisProcessMap[i], bSlaveToThisProcess);

		int pid = processCommunicator.get_proc_id(i);
		if(bMasterToThisProcess != bSlaveToProcess[i])
		{
			UG_LOG("Process " << std::setw(4) << pid << " has " << (bMasterToThisProcess ? "a" : "no") << " master connection to this process (" << std::setw(4) << pcl::GetProcRank()
				<< "), but we have " << (bSlaveToProcess[i] ? "a" : "no") << " slave connection to " << std::setw(4) << pid << std::endl);
			return false;
		}
		if(bSlaveToThisProcess != bMasterToProcess[i]){
			UG_LOG("Process " << std::setw(4) << pid << " has " << (bSlaveToThisProcess ? "a" : "no") << " slave connection to this process (" << pcl::GetProcRank()
				<< "), but we have " << (bMasterToProcess[i] ? "a" : "no") << " master connection to " << std::setw(4) << pid << std::endl);
			return false;
		}
	}

	return true;
}

/// if processor P1 has a interface to P2, then the size of the interface P1->P2 has to be the same as the size of interface P2->P1
/**	You may specify a callback, which allows to print not the elements in the
 * interfaces directly, but to print associated values. This is useful, if e.g.
 * pointers are stored in the layouts.
 *
 * The callback-method takes a parameter of type TLayout::Element and has to
 * return a value of type TValue (TValue = TLayout::Element by default).
 */
template<typename TLayout, typename TValue>
bool TestSizeOfInterfacesInLayoutsMatch(pcl::InterfaceCommunicator<TLayout> &com,
					TLayout &masterLayout, TLayout &slaveLayout, bool bPrint=false,
					boost::function<TValue (typename TLayout::Element)> cbToValue
						= TrivialToValue<typename TLayout::Element>)
{
	typedef std::map<int, ug::BinaryBuffer>	BufferMap;
	typedef typename TLayout::Interface Interface;

	BufferMap sendMap, receiveMap;

	for(typename TLayout::iterator iter = slaveLayout.begin(); iter != slaveLayout.end(); ++iter)
	{
		Interface &interface = slaveLayout.interface(iter);
		int pid = slaveLayout.proc_id(iter);
		ug::BinaryBuffer& buffer = sendMap[pid];

		for(typename TLayout::Interface::iterator iter2 = interface.begin(); iter2 != interface.end(); ++iter2)
		{
			typename Interface::Element &element = interface.get_element(iter2);
			Serialize(buffer, cbToValue(element));
		}
		com.send_raw(pid, buffer.buffer(), buffer.write_pos(), false);
	}


	for(typename TLayout::iterator iter = masterLayout.begin(); iter != masterLayout.end(); ++iter)
	{
		int pid = masterLayout.proc_id(iter);
		com.receive_raw(pid, receiveMap[pid]);
	}

	com.communicate();

	bool layout_broken=false;
	for(typename TLayout::iterator iter = masterLayout.begin(); iter != masterLayout.end(); ++iter)
	{
		int pid = masterLayout.proc_id(iter);
		ug::BinaryBuffer &buffer = receiveMap[pid];
		bool broken=false;
		if(bPrint) { UG_LOG("Interface processor " << pcl::GetProcRank() << " <-> processor " << pid << " (Master <-> Slave):\n"); }
		typename TLayout::Interface &interface = masterLayout.interface(iter);
		for(typename TLayout::Interface::iterator iter2 = interface.begin(); iter2 != interface.end(); ++iter2)
		{
			if(!buffer.eof() == false)
			{
				broken =true;
				TValue value = cbToValue(interface.get_element(iter2));
				if(bPrint) {	UG_LOG(" " << std::setw(9) << value << " <-> " << "BROKEN!" << std::endl); }
			}
			else
			{
				TValue val1 = cbToValue(interface.get_element(iter2));
				TValue val2; Deserialize(buffer, val2);
				if(bPrint) { UG_LOG(" " << std::setw(9) << val1 << " <-> " << val2 << std::endl); }
			}
		}
		while(!buffer.eof())
		{
			broken = true;
			TValue value; Deserialize(buffer, value);

			if(bPrint) { UG_LOG(" BROKEN! -> " << std::setw(9) << value << std::endl); }
		}

		if(broken)
		{
			layout_broken=true;
			UG_LOG("Interface from processor " << std::setw(4) << pcl::GetProcRank() << " to processor " << std::setw(4) << pid << " is BROKEN!\n");

		}
	}

	if(layout_broken == true)
	{
		UG_LOG("One or more interfaces are broken\n");
		return false;
	}
	return true;
}

///	Checks whether the given layouts are consistent.
/**	Checks whether the given master and slave layout have the same number of elements
 * and prints associated elements if bPrint = true. If the callback cbToValue is
 * specified, then it is used during printing to print values associated with the
 * elements in the layouts instead of printing the elements directly.
 *
 * The methods returns true if all interfaces match and false if not. The return
 * values are consistent among all processes.
 */
template<typename TLayout, typename TValue>
bool TestLayout(const pcl::ProcessCommunicator &processCommunicator,
		pcl::InterfaceCommunicator<TLayout> &com, TLayout &masterLayout,
				TLayout &slaveLayout, bool bPrint=false,
				boost::function<TValue (typename TLayout::Element)> cbToValue
					= TrivialToValue<typename TLayout::Element>)
{
	if(bPrint)
	{
		UG_LOG("MasterLayout is to processes ");
		for(typename TLayout::iterator iter = masterLayout.begin(); iter != masterLayout.end(); ++iter)	{
			UG_LOG(" " << std::setw(4) << masterLayout.proc_id(iter) << " ");
		}
		UG_LOG("\nSlave Layout is to processes ");
		for(typename TLayout::iterator iter = slaveLayout.begin(); iter != slaveLayout.end(); ++iter) {
			UG_LOG(" " << std::setw(4) << slaveLayout.proc_id(iter) << " ");
		}
		UG_LOG("\n");
	}
	bool bDoubleEnded = TestLayoutIsDoubleEnded(processCommunicator,
			com, masterLayout, slaveLayout);
	if(!pcl::AllProcsTrue(bDoubleEnded, processCommunicator))
		return false;

	bool bSuccess = TestSizeOfInterfacesInLayoutsMatch<TLayout, TValue>(com, masterLayout,
															slaveLayout, bPrint, cbToValue);
	return pcl::AllProcsTrue(bSuccess, processCommunicator);
}

template<typename TLayout>
bool TestLayout(const pcl::ProcessCommunicator &processCommunicator,
		pcl::InterfaceCommunicator<TLayout> &com, TLayout &masterLayout,
				TLayout &slaveLayout, bool bPrint=false)
{
	return TestLayout<TLayout, typename TLayout::Element>(processCommunicator, com,
			masterLayout, slaveLayout, bPrint);
}

template<typename TLayout, typename TValue>
bool PrintLayout(const pcl::ProcessCommunicator &processCommunicator,
		pcl::InterfaceCommunicator<TLayout> &com, TLayout &masterLayout,
				TLayout &slaveLayout,
				boost::function<TValue (typename TLayout::Element)> cbToValue
					= TrivialToValue<typename TLayout::Element>)
{
	return TestLayout(processCommunicator, com, masterLayout, slaveLayout, true, cbToValue);
}


template<typename TLayout>
bool PrintLayout(const pcl::ProcessCommunicator &processCommunicator,
		pcl::InterfaceCommunicator<TLayout> &com, TLayout &masterLayout,
				TLayout &slaveLayout)
{
	return TestLayout(processCommunicator, com, masterLayout, slaveLayout, true);
}


#ifndef NDEBUG
	#define TESTLAYOUT(processCommunicator, com, master, slave) { if(TestLayout(processCommunicator,\
		com, master, slave) == false) { PrintLayout(processCommunicator, com, master, slave); UG_ASSERT(false, "layout broken"); } }
#else
	#define TESTLAYOUT(processCommunicator, com, master, slave)
#endif

}

#endif
