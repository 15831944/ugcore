// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 09.03.2011 (m,d,y)
 
#include <vector>
#include <map>
#include "common/assert.h"
#include "grid_distribution.h"
#include "util/distribution_util.h"
#include "parallelization_util.h"
#include "lib_grid/algorithms/attachment_util.h"

using namespace std;

namespace ug{

////////////////////////////////////////////////////////////////////////////////
/**	Copies vertices from mgSrc to mgDest, but only if a vertex with the
 * same global id is not already present in mgDest.
 * vrtLayout will be slightly adjusted: If a vertex was already present,
 * the corresponding node entry in the layout is replaced by the
 * existing one.
 *
 * The given attachment accessors have to be associated with mgDest.
 */
static void CopyNewElements(MultiGrid& mgDest, MultiGrid& mgSrc,
							RedistributionVertexLayout& vrtLayout,
							RedistributionEdgeLayout& edgeLayout,
							RedistributionFaceLayout& faceLayout,
							RedistributionVolumeLayout& volLayout,
							Grid::AttachmentAccessor<VertexBase, AVertexBase>& aaVrt,
							Grid::AttachmentAccessor<EdgeBase, AEdgeBase>& aaEdge,
							Grid::AttachmentAccessor<Face, AFace>& aaFace,
							Grid::AttachmentAccessor<Volume, AVolume>& aaVol);

////////////////////////////////////////////////////////////////////////////////
static void DeserializeRedistributedGrid(MultiGrid& mg, GridLayoutMap& glm,
										istream& in, vector<int>& recvFromRanks,
										const GridDataSerializationHandler& deserializer);

////////////////////////////////////////////////////////////////////////////////
template <class TGeomObj>
void CreateInterfaces(MultiGrid& mg, GridLayoutMap& glm,
					vector<RedistributionNodeLayout<TGeomObj*> >& redistLayouts);


////////////////////////////////////////////////////////////////////////////////
template <class TGeomObj, class TAATargetProcs, class TAAGlobalIDs>
static
void LogTargetProcs(MultiGrid& mg, TAATargetProcs& aaTargetProcs,
					  TAAGlobalIDs& aaIDs)
{
	typedef typename geometry_traits<TGeomObj>::iterator GeomObjIter;
	UG_LOG("  TARGET-PROCS:\n");
	for(size_t lvl = 0; lvl < mg.num_levels(); ++lvl){
		UG_LOG("    level " << lvl << ":\n");
		for(GeomObjIter iter = mg.begin<VertexBase>(lvl);
			iter != mg.end<VertexBase>(lvl); ++iter)
		{
		//	log the id
			TGeomObj* o = *iter;
			UG_LOG("      " << aaIDs[o].first << "_" << aaIDs[o].second << ":\t");

		//	log the target procs
			vector<int>& v = aaTargetProcs[o];
			for(size_t i = 0; i < v.size(); ++i){
				UG_LOG(" " << v[i]);
			}

			UG_LOG(endl);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
template <class TGeomObj, class TAATransferInfos, class TAAGlobalIDs>
static
void LogTransferInfos(MultiGrid& mg, TAATransferInfos& aaTransferInfos,
					  TAAGlobalIDs& aaIDs)
{
	typedef typename geometry_traits<TGeomObj>::iterator GeomObjIter;
	UG_LOG("  TRANSFER-INFOS:\n");
	for(size_t lvl = 0; lvl < mg.num_levels(); ++lvl){
		UG_LOG("    level " << lvl << ":\n");
		for(GeomObjIter iter = mg.begin<VertexBase>(lvl);
			iter != mg.end<VertexBase>(lvl); ++iter)
		{
		//	log the id
			TGeomObj* o = *iter;
			UG_LOG("      " << aaIDs[o].first << "_" << aaIDs[o].second << ":\t");

		//	log the target procs
			vector<RedistributionNodeTransferInfo>& infos = aaTransferInfos[o];
			for(size_t i = 0; i < infos.size(); ++i){
				UG_LOG("src: " << infos[i].srcProc);
				UG_LOG(", target: " << infos[i].targetProc);
				UG_LOG(", bMove: " << infos[i].bMove << "  ||  ");
			}

			UG_LOG(endl);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
bool RedistributeGrid(DistributedGridManager& distGridMgrInOut,
					  SubsetHandler& shPartition,
					  const GridDataSerializationHandler& serializer,
					  const GridDataSerializationHandler& deserializer,
					  bool createVerticalInterfaces,
					  vector<int>* processMap,
					  const pcl::ProcessCommunicator& procComm)
{

//	Algorithm outline:
//	- Distribute global IDs (if they were not already computed),
//	- Prepare redistribution groups
//		* This involves immediate updates to associated interfaces:
//		  Each process has to inform neighbor processes to where elements
//		  are sent, which lie in common interfaces.
//		  Note that several migration forms are possible: move, copy
//		* GlobalIDs have to be associated with each element.
//		* Make sure not to specify interfaces to the process to which the data is sent.
//	- CommunicateInvolvedProcesses
//	- Distribute the redistribution groups
//		* This involves serializing the associated grid-parts
//		* At this point one would also want to serialize custom user-data.
//	- Update local interfaces.
//		* This can be safely done here, since all redistribution groups are complete
//		  and since all neighbors are informed which interface elements go where.
//		* Note that this is not the final update. We're only erasing entries which were
//		  sent to other processes or entries in interfaces where neighbor processes
//		  informed us that the entries will be moved to other processes.
//		  We will also create new entries for those moved neighbor elements - as long
//		  as we didn't move our elements to another process, too.
//		* If connected entries on a neighbor proc A are moved to another process B,
//		  note that if an interface IAB already exists, there may also already exist some
//		  connections. Don't readd those!
//	- Erase all parts of the grid which are no longer used.
//		* Note that associated entries are already removed from local interfaces.
//	- Create new elements and add new interface entries.
//		* Here we use the global IDs to check whether an element already exists
//		  on the given process.
//		* The check can e.g. be implemented using hashes. Make sure to add all new
//		  elements to that hash.
//		* Create temporary arrays into which the referenced elements are sorted.
//		  This will make it easy to add the new interface entries.
//		* Note that if a referenced element already existed, it is also possible,
//		  that associated interface entries already exist. This has to be checked
//		  before new interface entries are added.
//

//todo: currently the local proc also serializes and deserializes all data
//		which he sends to itself. This involves clearing the whole grid
//		and recreating it from the serialized data.
//		It was implemented this way since it simply required less code.
//		In the future one may consider changing this behavior, so that
//		parts which stay on the grid will simply be maintained.
//		Affected parts of the algorithm are
//		* COMMUNICATE INVOLVED PROCESSES
//		* INTERMEDIATE CLEANUP
//		Note that one has to carefully adjust interfaces to the ones given
//		in the local redistribution map.

	if(!distGridMgrInOut.get_assigned_grid())
		return false;

	//int localProcRank = pcl::GetProcRank();

	MultiGrid& mg = *distGridMgrInOut.get_assigned_grid();
	GridLayoutMap& glm = distGridMgrInOut.grid_layout_map();

//	The selector will be of frequent use to speed up some algorithms
	MGSelector msel(mg);

//	Since we will change huge parts of the underlying grid and the grid-layout-map,
//	we'll disable auto-insertion of elements in the distributed-grid-manager.
//	This means we carefully have to take care of all interface changes.
	distGridMgrInOut.enable_ordered_element_insertion(false);

////////////////////////////////
//	GLOBAL IDS
//todo:	only create global ids if they aren't already present
	CreateAndDistributeGlobalIDs<VertexBase>(mg, glm);
	CreateAndDistributeGlobalIDs<EdgeBase>(mg, glm);
	CreateAndDistributeGlobalIDs<Face>(mg, glm);
	CreateAndDistributeGlobalIDs<Volume>(mg, glm);

////////////////////////////////
//	REDISTRIBITION LAYOUTS
//	we have to store the layouts for all the processes.
	vector<RedistributionVertexLayout> vertexLayouts;
	vector<RedistributionEdgeLayout> edgeLayouts;
	vector<RedistributionFaceLayout> faceLayouts;
	vector<RedistributionVolumeLayout> volumeLayouts;

//	create the redistribution layouts
//	we want to access the created target-procs and transfer-info-vec attachments
//	later on.
	typedef Attachment<vector<int> > AIntVec;
	AIntVec aTargetProcs;
	ARedistributionNodeTransferInfoVec aTransferInfos;

	Grid::AttachmentAccessor<VertexBase, AIntVec>
		aaTargetProcsVRT(mg, aTargetProcs, true);
	Grid::AttachmentAccessor<EdgeBase, AIntVec>
		aaTargetProcsEDGE(mg, aTargetProcs, true);
	Grid::AttachmentAccessor<Face, AIntVec>
		aaTargetProcsFACE(mg, aTargetProcs, true);
	Grid::AttachmentAccessor<Volume, AIntVec>
		aaTargetProcsVOL(mg, aTargetProcs, true);

	Grid::AttachmentAccessor<VertexBase, ARedistributionNodeTransferInfoVec>
		aaTransferInfosVRT(mg, aTransferInfos, true);
	Grid::AttachmentAccessor<EdgeBase, ARedistributionNodeTransferInfoVec>
		aaTransferInfosEDGE(mg, aTransferInfos, true);
	Grid::AttachmentAccessor<Face, ARedistributionNodeTransferInfoVec>
		aaTransferInfosFACE(mg, aTransferInfos, true);
	Grid::AttachmentAccessor<Volume, ARedistributionNodeTransferInfoVec>
		aaTransferInfosVOL(mg, aTransferInfos, true);

//	Now create the redistribution layouts. Note that this involves
//	some communication in order to synchronize interfaces between
//	connected procs.
	CreateRedistributionLayouts(vertexLayouts, edgeLayouts, faceLayouts,
					volumeLayouts, distGridMgrInOut, shPartition, false,
					createVerticalInterfaces, &msel, processMap,
					&aTargetProcs, &aTransferInfos);



//BEGIN - ONLY FOR DEBUG
/*
	LogTargetProcs<VertexBase>(mg, aaTargetProcsVRT, aaIDVRT);


	LogTransferInfos<VertexBase>(mg, aaTransferInfosVRT, aaIDVRT);
*/
	TestRedistributionLayouts(vertexLayouts);
//END - ONLY FOR DEBUG



////////////////////////////////
//	COMMUNICATE INVOLVED PROCESSES
//	each process has to know with which other processes it
//	has to communicate.
	vector<int> sendToRanks, recvFromRanks, sendPartitionInds;

//	for each subset which is not emtpy we'll have to send data to
//	the associated process.
	for(int si = 0; si < shPartition.num_subsets(); ++si){
		if(!shPartition.empty(si)){
			int toProc = si;
		//	if a process map exists, we'll use the associated process
			if(processMap)
				toProc = processMap->at(si);

			sendToRanks.push_back(toProc);
			sendPartitionInds.push_back(si);
		}
	}

	pcl::CommunicateInvolvedProcesses(recvFromRanks, sendToRanks, procComm);


//BEGIN - ONLY FOR DEBUG
	UG_LOG("  send to ranks:");
	for(size_t i = 0; i < sendToRanks.size(); ++i){
		UG_LOG(" " << sendToRanks[i]);
	}
	UG_LOG(endl);

	UG_LOG("  receive from ranks:");
	for(size_t i = 0; i < recvFromRanks.size(); ++i){
		UG_LOG(" " << recvFromRanks[i]);
	}
	UG_LOG(endl);
//END - ONLY FOR DEBUG



////////////////////////////////
//	SERIALIZATION
//	For each send-to-process (except the local process itself) we'll fill
//	a binary stream with the data that shall be sent.
//	a temporary int-attachment is required at all geometric objects
	AInt aLocalInd;
	mg.attach_to_all(aLocalInd);

//	out and sendSegSizes will be used to distribute the grid.
	BinaryStream out;
	vector<int> outSegSizes;

//	the magic number is used for debugging to make sure that the stream is read correctly
	int magicNumber1 = 75234587;
	int magicNumber2 = 560245;

	for(size_t i_to = 0; i_to < sendToRanks.size(); ++i_to){
		int partInd = sendPartitionInds[i_to];

	//	the last size is required to calculate the size of the new segment
		size_t oldSize = out.size();

	//	write a magic number for debugging purposes
		out.write((char*)&magicNumber1, sizeof(int));

	//	First we'll serialize the global ids of all distributed elements
		Serialize(out, vertexLayouts[partInd].m_globalIDs);
		Serialize(out, edgeLayouts[partInd].m_globalIDs);
		Serialize(out, faceLayouts[partInd].m_globalIDs);
		Serialize(out, volumeLayouts[partInd].m_globalIDs);

	//	Now let's serialize the grid and the redistribution layout interfaces
		SerializeGridAndDistributionLayouts(out, mg, vertexLayouts[partInd],
				edgeLayouts[partInd], faceLayouts[partInd], volumeLayouts[partInd],
				aLocalInd, aLocalInd, aLocalInd, aLocalInd, &msel);

	//	next thing is to serialize data associated with the layouts nodes
	//	first write the infos of all serializers
		serializer.write_infos(out);
	//	now serialize data associated with vertices, edges, faces and volumes
		serializer.serialize(out, vertexLayouts[partInd].node_vec().begin(),
							 vertexLayouts[partInd].node_vec().end());
		serializer.serialize(out, edgeLayouts[partInd].node_vec().begin(),
							 edgeLayouts[partInd].node_vec().end());
		serializer.serialize(out, faceLayouts[partInd].node_vec().begin(),
							 faceLayouts[partInd].node_vec().end());
		serializer.serialize(out, volumeLayouts[partInd].node_vec().begin(),
							 volumeLayouts[partInd].node_vec().end());

	//	write a magic number for debugging purposes
		out.write((char*)&magicNumber2, sizeof(int));

	//	size of the segment we just wrote to out
		UG_LOG("seg size of block for partition " << partInd << ": "
				<< out.size() - oldSize << endl);
		outSegSizes.push_back((int)(out.size() - oldSize));
	}

//	now distribute the packs between involved processes
	BinaryStream in;
	vector<int> inSegSizes(recvFromRanks.size());

	procComm.distribute_data(in, &inSegSizes.front(),
							&recvFromRanks.front(), (int)recvFromRanks.size(),
							out.buffer(), &outSegSizes.front(),
							&sendToRanks.front(), (int)sendToRanks.size());

	UG_LOG("Size of in buffer: " << in.size() << endl);

////////////////////////////////
//	INTERMEDIATE CLEANUP
//	we'll erase everything and deserialize even the local grid from stream
	mg.clear_geometry();
	glm.clear();

/*
	{
	//	select all elements which shall be cleared.
	//	we'll do this by first selecting all elements on the local proc
	//	and afterwards inverting the selection.
		SelectNodesInLayout(msel, vertexLayouts[localPartSI]);
		SelectNodesInLayout(msel, edgeLayouts[localPartSI]);
		SelectNodesInLayout(msel, faceLayouts[localPartSI]);
		SelectNodesInLayout(msel, volumeLayouts[localPartSI]);
		InvertSelection(msel);

	//	erase the grid parts and all selected interface entries
		for(size_t lvl = 0; lvl < msel.num_levels(); ++lvl){
			mg.erase(msel.vertices_begin(lvl), msel.vertices_end(lvl));
			mg.erase(msel.edges_begin(lvl), msel.edges_end(lvl));
			mg.erase(msel.faces_begin(lvl), msel.faces_end(lvl));
			mg.erase(msel.volumes_begin(lvl), msel.volumes_end(lvl));
		}
	}
*/
////////////////////////////////
//	DESERILAIZATION
	UG_LOG("\ndeserializing...\n");
	DeserializeRedistributedGrid(mg, glm, in, recvFromRanks, deserializer);
	UG_LOG("deserialization done\n\n");

////////////////////////////////
//	UPDATE THE DISTRIBUTED GRID MANAGER
	glm.remove_empty_interfaces();
	distGridMgrInOut.enable_ordered_element_insertion(true);
	distGridMgrInOut.grid_layouts_changed(false);


//BEGIN - ONLY FOR DEBUG
/*
UG_LOG("has vertex masters: " << glm.has_layout<VertexBase>(INT_H_MASTER) << endl);
UG_LOG("has vertex slaves: " << glm.has_layout<VertexBase>(INT_H_SLAVE) << endl);
VertexLayout& masterLayout = glm.get_layout<VertexBase>(INT_H_MASTER);
VertexLayout& slaveLayout = glm.get_layout<VertexBase>(INT_H_SLAVE);

UG_LOG("has master interface to 0: " << masterLayout.interface_exists(0) << endl);
UG_LOG("has master interface to 1: " << masterLayout.interface_exists(1) << endl);
UG_LOG("has master interface to 2: " << masterLayout.interface_exists(2) << endl);
UG_LOG("has master interface to 3: " << masterLayout.interface_exists(3) << endl);

UG_LOG("has slave interface to 0: " << slaveLayout.interface_exists(0) << endl);
UG_LOG("has slave interface to 1: " << slaveLayout.interface_exists(1) << endl);
UG_LOG("has slave interface to 2: " << slaveLayout.interface_exists(2) << endl);
UG_LOG("has slave interface to 3: " << slaveLayout.interface_exists(3) << endl);
*/

//TestGridLayoutMap(mg, glm);

//END - ONLY FOR DEBUG


////////////////////////////////
//	CLEAN UP
	mg.detach_from_all(aLocalInd);
	mg.detach_from_all(aTargetProcs);
	mg.detach_from_all(aTransferInfos);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
static void DeserializeRedistributedGrid(MultiGrid& mg, GridLayoutMap& glm,
										istream& in, vector<int>& recvFromRanks,
										const GridDataSerializationHandler& deserializer)
{
//	before deserializing the received data, we'll sort the data we received
//	by the ranks of the processes from which we received data. This is
//	important to make sure that elements on different processes are added
//	to the interfaces in the right order.
//	vector of pairs <procRank, readPos>
/*
	std::vector<pair<int, size_t> >	recvProcsSorted;
	size_t readPos = 0;
	for(size_t i = 0; i < recvFromRanks.size(); ++i){
		recvProcsSorted.push_back(make_pair(recvFromRanks[i], readPos));
		readPos += inSegSizes[i];
	}

	sort(recvProcsSorted.begin(), recvProcsSorted.end());


//BEGIN - ONLY FOR DEBUG
	UG_LOG("RecProcsSorted:");
	for(size_t i = 0; i < recvProcsSorted.size(); ++i){
		UG_LOG(" (" << recvProcsSorted[i].first << ", " << recvProcsSorted[i].second << ")");
	}
	UG_LOG(endl);
//END - ONLY FOR DEBUG
*/

//	the magic number is used for debugging to make sure that the stream is read correctly
	int magicNumber1 = 75234587;
	int magicNumber2 = 560245;

//	deserialize the redist-layouts
//	read redistribution layouts from the stream

//	this multi-grid will be used for deserialization.
//	Note that it uses GRIDOPT_NONE for maximal performance.
//	We'll attach pointers to the associated elements in the target grid.
	MultiGrid tmg(GRIDOPT_NONE);
	AVertexBase aVrt;
	AEdgeBase aEdge;
	AFace aFace;
	AVolume aVol;
	Grid::AttachmentAccessor<VertexBase, AVertexBase> aaVrt(tmg, aVrt, true);
	Grid::AttachmentAccessor<EdgeBase, AEdgeBase> aaEdge(tmg, aEdge, true);
	Grid::AttachmentAccessor<Face, AFace> aaFace(tmg, aFace, true);
	Grid::AttachmentAccessor<Volume, AVolume> aaVol(tmg, aVol, true);

//	those layouts will be used to deserialize the received data
	vector<RedistributionVertexLayout> vrtLayouts(recvFromRanks.size());
	vector<RedistributionEdgeLayout> edgeLayouts(recvFromRanks.size());
	vector<RedistributionFaceLayout> faceLayouts(recvFromRanks.size());
	vector<RedistributionVolumeLayout> volLayouts(recvFromRanks.size());

	for(size_t i = 0; i < recvFromRanks.size(); ++i){
		RedistributionVertexLayout& vrtLayout = vrtLayouts[i];
		RedistributionEdgeLayout& edgeLayout = edgeLayouts[i];
		RedistributionFaceLayout& faceLayout = faceLayouts[i];
		RedistributionVolumeLayout& volLayout = volLayouts[i];

		//int recvFrom = recvProcsSorted[i].first;

	//	set the read position of the in-stream to the data section
	//	of the process we're currently processing.
		//in.reset();
		//in.read_jump(recvProcsSorted[i].second);

	//	read the magic number and make sure that it matches our magicNumber
		int tmp = 0;
		in.read((char*)&tmp, sizeof(int));
		if(tmp != magicNumber1){
			UG_LOG("ERROR in RedistributeGrid: ");
			UG_LOG("Magic number mismatch before deserialization.\n");
		}

	//	First we'll deserialize the global ids of all distributed elements
		Deserialize(in, vrtLayout.m_globalIDs);
		Deserialize(in, edgeLayout.m_globalIDs);
		Deserialize(in, faceLayout.m_globalIDs);
		Deserialize(in, volLayout.m_globalIDs);

/*delete this
		if(!DeserializeMultiGridElements(mg, in, &vrtLayout.node_vec(),
							&edgeLayout.node_vec(), &faceLayout.node_vec(),
							&volLayout.node_vec()))
		{
			UG_LOG("DeserializeMultiGridElements failed.\n");
		}
*/
		tmg.clear_geometry();
		DeserializeMultiGridElements(tmg, in, &vrtLayout.node_vec(),
							&edgeLayout.node_vec(), &faceLayout.node_vec(),
							&volLayout.node_vec());

		DeserializeDistributionLayoutInterfaces(vrtLayout, in);
		DeserializeDistributionLayoutInterfaces(edgeLayout, in);
		DeserializeDistributionLayoutInterfaces(faceLayout, in);
		DeserializeDistributionLayoutInterfaces(volLayout, in);

	//	before we'll deserialize the associated data, we'll create the new
	//	elements in the target grid.
	//	copy elements - layouts are upadted to the new elements on the fly.
		CopyNewElements(mg, tmg, vrtLayout, edgeLayout, faceLayout, volLayout,
						aaVrt, aaEdge, aaFace, aaVol);


	//	now deserialize the data associated with the elements in the layouts node-vecs
	//	first read the infos of all deserializers
		deserializer.read_infos(in);
	//	now deserialize data associated with vertices, edges, faces and volumes
		deserializer.deserialize(in, vrtLayout.node_vec().begin(),
							 	 vrtLayout.node_vec().end());
		deserializer.deserialize(in, edgeLayout.node_vec().begin(),
							 	 edgeLayout.node_vec().end());
		deserializer.deserialize(in, faceLayout.node_vec().begin(),
							 	 faceLayout.node_vec().end());
		deserializer.deserialize(in, volLayout.node_vec().begin(),
							 	 volLayout.node_vec().end());

	//	read the magic number and make sure that it matches our magicNumber
		tmp = 0;
		in.read((char*)&tmp, sizeof(int));
		if(tmp != magicNumber2){
			UG_LOG("ERROR in RedistributeGrid: ");
			UG_LOG("Magic number mismatch after deserialization.\n");
		}
/*
//BEGIN - ONLY FOR DEBUG
		UG_LOG("received global vertex ids:");
		for(size_t i = 0; i < vrtLayout.m_globalIDs.size(); ++i){
			UG_LOG(" (" << vrtLayout.m_globalIDs[i].first << ", " << vrtLayout.m_globalIDs[i].second << ")");
		}
		UG_LOG(endl);
//END - ONLY FOR DEBUG
*/
	}

//	Now that everything has been deserialized, we can create the interfaces
UG_LOG("Creating vertex interfaces...\n");
	CreateInterfaces(mg, glm, vrtLayouts);
UG_LOG("Creating edge interfaces...\n");
	CreateInterfaces(mg, glm, edgeLayouts);
UG_LOG("Creating face interfaces...\n");
	CreateInterfaces(mg, glm, faceLayouts);
UG_LOG("Creating volume interfaces...\n");
	CreateInterfaces(mg, glm, volLayouts);
}

template <class TGeomObj>
void CreateInterfaces(MultiGrid& mg, GridLayoutMap& glm,
					vector<RedistributionNodeLayout<TGeomObj*> >& redistLayouts)
{
//todo: if the redistLayouts are empty, we should return immediately.

//	In order to make sure that associated interfaces are created in the same order
//	on different processes, we have to sort them by pair<proc1, proc2> where proc1 is
//	the smaller rank.
	typedef RedistributionNodeLayout<TGeomObj*>		RedistLayout;
	typedef typename RedistLayout::Interface		RedistInterface;
	typedef typename RedistLayout::InterfaceMap		InterfaceMap;
	typedef typename InterfaceMap::iterator			InterfaceIter;

//	We use a multi-map. This is fine since values with the same
//	keys can be processed in arbitrary order.
	typedef multimap<pair<int, int>,
				pair<RedistLayout*, InterfaceIter> > 	SortedMap;

//	the layout and interface types which we want to fill
	typedef GridLayoutMap::Types<TGeomObj>		Types;
	typedef typename Types::Layout::LevelLayout	Layout;
	typedef typename Types::Interface			Interface;

//	the position attachment is only accessed for debug purposes
//todo:	remove this
	//Grid::AttachmentAccessor<VertexBase, APosition> aaPos(mg, aPosition);

//	to avoid double interface entries we sadly have to attach vectors
//	to each element type, which stores in which interfaces the entry already lies.
//	vector<pair<targetProc, type> >
	typedef vector<pair<int, byte> >	ConnectionVec;
	typedef Attachment<ConnectionVec> 	AConnectionVec;
	AConnectionVec aConVec;
	Grid::AttachmentAccessor<TGeomObj, AConnectionVec> aaConVec(mg, aConVec, true);

	//int localRank = pcl::GetProcRank();
/*
	UG_LOG("\nPrinting redistribution layout before interface creation:\n");
	PrintRedistributionLayouts(redistLayouts);
	UG_LOG("\n");
*/
//	we'll create interfaces level by level
	for(size_t lvl = 0; lvl < mg.num_levels(); ++lvl)
	{
	//	store all interfaces together with their layouts in this sortMap.
	//	The key is a pair of old-source-proc and old-target-proc, where
	//	the smaller value is the first in the key.
		SortedMap sortMap;

		for(size_t i_layout = 0; i_layout < redistLayouts.size(); ++i_layout)
		{
			RedistLayout& layout = redistLayouts[i_layout];
			InterfaceMap& imap = layout.interface_map(lvl);

			int oldSrcProc = layout.get_source_proc();
			for(InterfaceIter iiter = imap.begin(); iiter != imap.end(); ++iiter)
			{
				int oldTargetProc = iiter->first.second;
				pair<int, int> key = make_pair(min(oldSrcProc, oldTargetProc),
												max(oldSrcProc, oldTargetProc));
				sortMap.insert(make_pair(key, make_pair(&layout, iiter)));
			}
		}

	//	the map is filled. We can create the interfaces now.
		for(typename SortedMap::iterator iter = sortMap.begin();
			iter != sortMap.end(); ++iter)
		{
			RedistLayout& redistLayout = *(iter->second.first);
		//	All right - this is really nasty!
		//	SortMapIter->Value->InterfaceIter->RedistInterface
			RedistInterface& redistInterface = iter->second.second->second;
		//	SortMapIter->Value->InterfaceIter->ProcPair->ConnectedProc
			int targetProc = iter->second.second->first.first;

			vector<TGeomObj*>& nodes = redistLayout.node_vec();

		//	to speed things up, we'll cache the target interfaces and layouts.
			Layout* pcurLayout = NULL;
			unsigned int curLayoutKey = 0;
			Interface* pcurInterface = NULL;

		//	iterate over all entries of redistLayout
			for(size_t i = 0; i < redistInterface.size(); ++i)
			{
				DistributionInterfaceEntry& entry = redistInterface[i];
				TGeomObj* node = nodes[entry.localID];

			//	first check whether the node is already contained in an
			//	interface of the given type to the given proc.
				pair<int, byte> connection(targetProc, entry.type);
				ConnectionVec& conVec = aaConVec[node];

			//	if the connection has already been established, we'll continue
			//	with the next node.
				if(find(conVec.begin(), conVec.end(), connection) != conVec.end())
					continue;

			//	we're caching the last layout and interface to avoid too
			//	many lookups
				if((!pcurLayout) || (curLayoutKey != entry.type)){
					pcurLayout = &glm.template get_layout<TGeomObj>(entry.type).
																layout_on_level(lvl);
					curLayoutKey = entry.type;
					pcurInterface = &pcurLayout->interface(targetProc);
				}

			//	copy the element into the interface
				pcurInterface->push_back(node);
			//	store the connection
				conVec.push_back(connection);

			//	debug output
				UG_LOG("Added Interface entry to " << targetProc);
				UG_LOG(": " << entry.type);
				if((int)geometry_traits<TGeomObj>::BASE_OBJECT_TYPE_ID
					== (int)VERTEX)
				{
					UG_LOG(endl);
					//UG_LOG(" at " << aaPos[node] << endl);
				}
				else{
					UG_LOG(endl);
				}
			}
		}
	}

	mg.detach_from<TGeomObj>(aConVec);
}

/**	Copies vertices from mgSrc to mgDest, but only if a vertex with the
 * same global id is not already present in mgDest.
 * vrtLayout will be slightly adjusted: If a vertex was already present,
 * the corresponding node entry in the layout is replaced by the
 * existing one.
 *
 * The given attachment accessors have to be associated with mgDest.
 *
 * \todo	This method should utilize hashes to increase performance
 */
static void CopyNewElements(MultiGrid& mgDest, MultiGrid& mgSrc,
							RedistributionVertexLayout& vrtLayout,
							RedistributionEdgeLayout& edgeLayout,
							RedistributionFaceLayout& faceLayout,
							RedistributionVolumeLayout& volLayout,
							Grid::AttachmentAccessor<VertexBase, AVertexBase>& aaVrt,
							Grid::AttachmentAccessor<EdgeBase, AEdgeBase>& aaEdge,
							Grid::AttachmentAccessor<Face, AFace>& aaFace,
							Grid::AttachmentAccessor<Volume, AVolume>& aaVol)
{
//	used to access the global ids of elements already present on the local process
	Grid::AttachmentAccessor<VertexBase, AGeomObjID> aaIDVRT(mgDest, aGeomObjID);
	Grid::AttachmentAccessor<EdgeBase, AGeomObjID> aaIDEDGE(mgDest, aGeomObjID);
	Grid::AttachmentAccessor<Face, AGeomObjID> aaIDFACE(mgDest, aGeomObjID);
	Grid::AttachmentAccessor<Volume, AGeomObjID> aaIDVOL(mgDest, aGeomObjID);

//	iterate over all vertices in mgSrc and create new ones in mgDest
	size_t vrtCounter = 0;
	size_t edgeCounter = 0;
	size_t faceCounter = 0;
	size_t volCounter = 0;

//	reused for efficient element creation
	EdgeDescriptor ed;
	FaceDescriptor fd;
	VolumeDescriptor vd;

	for(size_t lvl = 0; lvl < mgSrc.num_levels(); ++lvl){
	//	create hashes for existing geometric objects
		Hash<VertexBase*, GeomObjID>	vrtHash(1.5 * (mgDest.num<VertexBase>(lvl)
													  + mgSrc.num<VertexBase>(lvl)));
		Hash<EdgeBase*, GeomObjID>		edgeHash(1.5 * (mgDest.num<EdgeBase>(lvl)
													  + mgSrc.num<EdgeBase>(lvl)));
		Hash<Face*, GeomObjID>			faceHash(1.5 * (mgDest.num<Face>(lvl)
													  + mgSrc.num<Face>(lvl)));
		Hash<Volume*, GeomObjID>		volHash(1.5 * (mgDest.num<Volume>(lvl)
													  + mgSrc.num<Volume>(lvl)));

	//	add existing elements to the hashes
		for(VertexBaseIterator iter = mgDest.begin<VertexBase>(lvl);
			iter != mgDest.end<VertexBase>(lvl); ++iter)
		{vrtHash.add(*iter, aaIDVRT[*iter]);}

		for(EdgeBaseIterator iter = mgDest.begin<EdgeBase>(lvl);
			iter != mgDest.end<EdgeBase>(lvl); ++iter)
		{edgeHash.add(*iter, aaIDEDGE[*iter]);}

		for(FaceIterator iter = mgDest.begin<Face>(lvl);
			iter != mgDest.end<Face>(lvl); ++iter)
		{faceHash.add(*iter, aaIDFACE[*iter]);}

		for(VolumeIterator iter = mgDest.begin<Volume>(lvl);
			iter != mgDest.end<Volume>(lvl); ++iter)
		{volHash.add(*iter, aaIDVOL[*iter]);}

	//	copy vertices
		for(VertexBaseIterator iter = mgSrc.begin<VertexBase>(lvl);
			iter != mgSrc.end<VertexBase>(lvl); ++iter, ++vrtCounter)
		{
			VertexBase* vrt = *iter;
			VertexBase* nVrt = NULL;

		//	make sure that mgSrc and vrtLayout are synchronous
			UG_ASSERT(vrt == vrtLayout.node_vec()[vrtCounter], "mgSrc and vrtLayout are asynchronous.");

		//	check whether the object already exists
			GeomObjID curID = vrtLayout.m_globalIDs[vrtCounter];
			Hash<VertexBase*, GeomObjID>::Iterator findIter = vrtHash.begin(curID);

			if(findIter != vrtHash.end(curID))
			{
			//	the vertex already exists.
				nVrt = *findIter;
			}
			else{
			//	the vertex didn't yet exist. create it.
				GeometricObject* parent = mgSrc.get_parent(vrt);
				if(parent){
					int type = parent->base_object_type_id();
					switch(type){
						case VERTEX:
							nVrt = *mgDest.create_by_cloning(vrt, aaVrt[parent]);
							break;
						case EDGE:
							nVrt = *mgDest.create_by_cloning(vrt, aaEdge[parent]);
							break;
						case FACE:
							nVrt = *mgDest.create_by_cloning(vrt, aaFace[parent]);
							break;
						case VOLUME:
							nVrt = *mgDest.create_by_cloning(vrt, aaVol[parent]);
							break;
					}
				}
				else
					nVrt = *mgDest.create_by_cloning(vrt, lvl);
			}

			UG_ASSERT(nVrt, "Vertex couldn't be created.");

		//	update the vertex layout
			vrtLayout.node_vec()[vrtCounter] = nVrt;

		//	copy the global id
			aaIDVRT[nVrt] = curID;

		//	store a reference to the new vertex
			aaVrt[vrt] = nVrt;

		}

	//	copy the edges
		for(EdgeBaseIterator iter = mgSrc.begin<EdgeBase>(lvl);
			iter != mgSrc.end<EdgeBase>(lvl); ++iter, ++edgeCounter)
		{
			EdgeBase* e = *iter;
			EdgeBase* ne = NULL;

		//	check whether the object already exists
			GeomObjID curID = edgeLayout.m_globalIDs[edgeCounter];
			Hash<EdgeBase*, GeomObjID>::Iterator findIter = edgeHash.begin(curID);

			if(findIter != edgeHash.end(curID))
			{
			//	the edge already exists.
				ne = *findIter;
			}
			else{
			//	the edge didn't yet exist. create it.
				ed.set_vertices(aaVrt[e->vertex(0)], aaVrt[e->vertex(1)]);
				GeometricObject* parent = mgSrc.get_parent(e);
				if(parent){
					int type = parent->base_object_type_id();
					switch(type){
						case EDGE:
							ne = *mgDest.create_by_cloning(e, ed, aaEdge[parent]);
							break;
						case FACE:
							ne = *mgDest.create_by_cloning(e, ed, aaFace[parent]);
							break;
						case VOLUME:
							ne = *mgDest.create_by_cloning(e, ed, aaVol[parent]);
							break;
					}
				}
				else
					ne = *mgDest.create_by_cloning(e, ed, lvl);
			}

		//	update the edge layout
			edgeLayout.node_vec()[edgeCounter] = ne;

		//	copy the global id
			aaIDEDGE[ne] = curID;

		//	store a reference to the new edge
			aaEdge[e] = ne;
		}

	//	copy the faces
		for(FaceIterator iter = mgSrc.begin<Face>(lvl);
			iter != mgSrc.end<Face>(lvl); ++iter, ++faceCounter)
		{
			Face* f = *iter;
			Face* nf = NULL;

		//	check whether the object already exists
			GeomObjID curID = faceLayout.m_globalIDs[faceCounter];
			Hash<Face*, GeomObjID>::Iterator findIter = faceHash.begin(curID);

			if(findIter != faceHash.end(curID))
			{
			//	the face already exists.
				nf = *findIter;
			}
			else{
			//	the face didn't yet exist. create it.
				fd.set_num_vertices(f->num_vertices());
				for(size_t i = 0; i < fd.num_vertices(); ++i)
					fd.set_vertex(i, aaVrt[f->vertex(i)]);

				GeometricObject* parent = mgSrc.get_parent(f);
				if(parent){
					int type = parent->base_object_type_id();
					switch(type){
						case FACE:
							nf = *mgDest.create_by_cloning(f, fd, aaFace[parent]);
							break;
						case VOLUME:
							nf = *mgDest.create_by_cloning(f, fd, aaVol[parent]);
							break;
					}
				}
				else
					nf = *mgDest.create_by_cloning(f, fd, lvl);
			}

		//	update the face layout
			faceLayout.node_vec()[faceCounter] = nf;

		//	copy the global id
			aaIDFACE[nf] = curID;

		//	store a reference to the new face
			aaFace[f] = nf;
		}

	//	copy the volumes
		for(VolumeIterator iter = mgSrc.begin<Volume>(lvl);
			iter != mgSrc.end<Volume>(lvl); ++iter, ++volCounter)
		{
			Volume* v = *iter;
			Volume* nv = NULL;

		//	check whether the object already exists
			GeomObjID curID = volLayout.m_globalIDs[volCounter];
			Hash<Volume*, GeomObjID>::Iterator findIter = volHash.begin(curID);

			if(findIter != volHash.end(curID))
			{
			//	the volume already exists.
				nv = *findIter;
			}
			else{
			//	the face didn't yet exist. create it.
				vd.set_num_vertices(v->num_vertices());
				for(size_t i = 0; i < vd.num_vertices(); ++i)
					vd.set_vertex(i, aaVrt[v->vertex(i)]);

				GeometricObject* parent = mgSrc.get_parent(v);
				if(parent){
					UG_ASSERT(parent->base_object_type_id() == VOLUME, "volumes can only be children to volumes.");
					nv = *mgDest.create_by_cloning(v, vd, aaVol[parent]);
				}
				else
					nv = *mgDest.create_by_cloning(v, vd, lvl);
			}

		//	update the volume layout
			volLayout.node_vec()[volCounter] = nv;

		//	copy the global id
			aaIDVOL[nv] = curID;

		//	store a reference to the new volume
			aaVol[v] = nv;
		}
	}
}

}// end of namespace
