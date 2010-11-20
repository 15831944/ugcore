//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m10 d10

#include <cassert>
#include <algorithm>
#include "grid.h"
#include "grid_util.h"
#include "common/common.h"

using namespace std;

namespace ug
{
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//	implementation of Grid

////////////////////////////////////////////////////////////////////////
//	constructors
Grid::Grid() : m_aVertexContainer(false), m_aEdgeContainer(false),
				m_aFaceContainer(false), m_aVolumeContainer(false)
{
	m_hashCounter = 0;
	m_currentMark = 0;
	m_options = GRIDOPT_NONE;
	change_options(GRIDOPT_DEFAULT);
}

Grid::Grid(uint options) : m_aVertexContainer(false), m_aEdgeContainer(false),
							m_aFaceContainer(false), m_aVolumeContainer(false)
{
	m_hashCounter = 0;
	m_currentMark = 0;
	m_options = GRIDOPT_NONE;
	change_options(options);
}

Grid::Grid(const Grid& grid) : m_aVertexContainer(false), m_aEdgeContainer(false),
							m_aFaceContainer(false), m_aVolumeContainer(false)
{
	m_hashCounter = 0;
	m_currentMark = 0;
	m_options = GRIDOPT_NONE;
	
	assign_grid(grid);
}

Grid::~Grid()
{
//	tell registered grid-observers that the grid is to be destroyed.
	for(ObserverContainer::iterator iter = m_gridObservers.begin();
		iter != m_gridObservers.end(); ++iter)
	{
		(*iter)->grid_to_be_destroyed(this);
	}

//	unregister all observers	
	while(!m_gridObservers.empty())
	{
		unregister_observer(m_gridObservers.back());
	}
	
//	erase all elements
	clear_geometry();
	
//	remove marks - would be done anyway...
	remove_marks();
}

void Grid::clear()
{
	clear_geometry();
	clear_attachments();
}

void Grid::clear_geometry()
{
//	disable all options to speed it up
	uint opts = get_options();
	set_options(GRIDOPT_NONE);
	
	erase(begin<Volume>(), end<Volume>());
	erase(begin<Face>(), end<Face>());
	erase(begin<EdgeBase>(), end<EdgeBase>());
	erase(begin<VertexBase>(), end<VertexBase>());
	
//	reset options
	set_options(opts);
}

void Grid::clear_attachments()
{
	vector<AttachmentEntry>	vEntries;

//	iterate through all attachment pipes
	for(int i = 0; i < NUM_GEOMETRIC_BASE_OBJECTS; ++i)
	{
		AttachmentPipe& ap = m_elementStorage[i].m_attachmentPipe;
		
	//	collect all attachment entries
		vEntries.clear();
		for(AttachmentPipe::ConstAttachmentEntryIterator iter = ap.attachments_begin();
			iter != ap.attachments_end(); ++iter)
		{
				vEntries.push_back(*iter);
		}
	
	//	iterate through the entries in the vector and delete the ones
	//	that have an enabled pass-on behaviour
		for(size_t j = 0; j < vEntries.size(); ++j)
		{
			const AttachmentEntry& ae = vEntries[j];
			
			if(ae.m_userData == 1){
				ap.detach(*ae.m_pAttachment);
			}
		}
	}
}


Grid& Grid::operator = (const Grid& grid)
{
//	clears the grid and calls assign_grid afterwards.
//	we're disabling any options, since new options will
//	be set during assign_grid anyway. This might speed
//	things up a little
	set_options(GRIDOPT_NONE);
	clear_geometry();
	assign_grid(grid);
	
	return *this;
}


void Grid::assign_grid(const Grid& grid)
{
//TODO: notify a grid observer that copying has started

//	we need a vertex-map that allows us to find a vertex in the new grid
//	given a vertex in the old one.
	vector<VertexBase*>	vrtMap(grid.attachment_container_size<VertexBase>(), NULL);

//	we need index-lists that allow us to copy attachments later on
	vector<int> vSrcDataIndex[NUM_GEOMETRIC_BASE_OBJECTS];
	vector<int>& vSrcDataIndexVRT = vSrcDataIndex[VERTEX];
	vector<int>& vSrcDataIndexEDGE = vSrcDataIndex[EDGE];
	vector<int>& vSrcDataIndexFACE = vSrcDataIndex[FACE];
	vector<int>& vSrcDataIndexVOL = vSrcDataIndex[VOLUME];

//	copy all vertices
	vSrcDataIndexVRT.resize(grid.num<VertexBase>());
	ConstVertexBaseIterator vrtsEnd = grid.end<VertexBase>();
	for(ConstVertexBaseIterator iter = grid.begin<VertexBase>(); iter != vrtsEnd; ++iter)
	{
		VertexBase* vrt = *iter;
		VertexBase* nVrt = *create_by_cloning(vrt);
		vrtMap[grid.get_attachment_data_index(vrt)] = nVrt;
		vSrcDataIndexVRT[get_attachment_data_index(nVrt)] = grid.get_attachment_data_index(vrt);
	}

//	copy all edges
	vSrcDataIndexEDGE.resize(grid.num<EdgeBase>());
	ConstEdgeBaseIterator edgesEnd = grid.end<EdgeBase>();
	for(ConstEdgeBaseIterator iter = grid.begin<EdgeBase>(); iter != edgesEnd; ++iter)
	{
		EdgeBase* e = *iter;
		EdgeBase* nE = *create_by_cloning(e, EdgeDescriptor(
											vrtMap[grid.get_attachment_data_index(e->vertex(0))],
											vrtMap[grid.get_attachment_data_index(e->vertex(1))]));
		vSrcDataIndexEDGE[get_attachment_data_index(nE)] = grid.get_attachment_data_index(e);
	}

//	copy all faces
	vSrcDataIndexFACE.resize(grid.num<Face>());
	FaceDescriptor fd;
	ConstFaceIterator facesEnd = grid.end<Face>();
	for(ConstFaceIterator iter = grid.begin<Face>(); iter != facesEnd; ++iter)
	{
		Face* f = *iter;
		uint numVrts = f->num_vertices();

	//	fill the face descriptor
		if(numVrts != fd.num_vertices())
			fd.set_num_vertices(numVrts);
		
		for(uint i = 0; i < numVrts; ++i)
			fd.set_vertex(i, vrtMap[grid.get_attachment_data_index(f->vertex(i))]);

	//	create the new face
		Face* nF = *create_by_cloning(f, fd);

		vSrcDataIndexFACE[get_attachment_data_index(nF)] = grid.get_attachment_data_index(f);
	}

//	copy all volumes
	vSrcDataIndexVOL.resize(grid.num<Volume>());
	VolumeDescriptor vd;
	ConstVolumeIterator volsEnd = grid.end<Volume>();
	for(ConstVolumeIterator iter = grid.begin<Volume>(); iter != volsEnd; ++iter)
	{
		Volume* v = *iter;
		uint numVrts = v->num_vertices();

	//	fill the volume descriptor
		if(numVrts != vd.num_vertices())
			vd.set_num_vertices(numVrts);

		for(uint i = 0; i < numVrts; ++i)
			vd.set_vertex(i, vrtMap[grid.get_attachment_data_index(v->vertex(i))]);

	//	create the volume
		Volume* nV = *create_by_cloning(v, vd);

		vSrcDataIndexVOL[get_attachment_data_index(nV)] = grid.get_attachment_data_index(v);
	}

//	enable options
	enable_options(grid.get_options());

//	copy attachments that may be passed on
	for(int i = 0; i < NUM_GEOMETRIC_BASE_OBJECTS; ++i)
	{
		const AttachmentPipe& apSrc = grid.m_elementStorage[i].m_attachmentPipe;
		AttachmentPipe& apDest = m_elementStorage[i].m_attachmentPipe;
		for(AttachmentPipe::ConstAttachmentEntryIterator iter = apSrc.attachments_begin();
			iter != apSrc.attachments_end(); ++iter)
		{
			const AttachmentEntry& ae = *iter;
			if(ae.m_userData == 1){
			//	attach the attachment to this grid
				apDest.attach(*ae.m_pAttachment, ae.m_userData);
				const IAttachmentDataContainer& conSrc = *ae.m_pContainer;
				IAttachmentDataContainer& conDest = *apDest.get_data_container(*ae.m_pAttachment);

			//	we use the containers copy-method
				conSrc.copy_to_buffer(conDest.get_data_buffer(), &vSrcDataIndex[i].front(),
										(int)vSrcDataIndex[i].size());
			}
		}
	}

//TODO: notify a grid observer that copying has ended
}


VertexBaseIterator Grid::create_by_cloning(VertexBase* pCloneMe, GeometricObject* pParent)
{
	VertexBase* pNew = reinterpret_cast<VertexBase*>(pCloneMe->create_empty_instance());
	register_vertex(pNew, pParent);
	return iterator_cast<VertexBaseIterator>(pNew->m_entryIter);
}

EdgeBaseIterator Grid::create_by_cloning(EdgeBase* pCloneMe, const EdgeVertices& ev, GeometricObject* pParent)
{
	EdgeBase* pNew = reinterpret_cast<EdgeBase*>(pCloneMe->create_empty_instance());
	pNew->set_vertex(0, ev.vertex(0));
	pNew->set_vertex(1, ev.vertex(1));
	register_edge(pNew, pParent);
	return iterator_cast<EdgeBaseIterator>(pNew->m_entryIter);
}

FaceIterator Grid::create_by_cloning(Face* pCloneMe, const FaceVertices& fv, GeometricObject* pParent)
{
	Face* pNew = reinterpret_cast<Face*>(pCloneMe->create_empty_instance());
	uint numVrts = fv.num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		pNew->set_vertex(i, fv.vertex(i));
	register_face(pNew, pParent);
	return iterator_cast<FaceIterator>(pNew->m_entryIter);
}

VolumeIterator Grid::create_by_cloning(Volume* pCloneMe, const VolumeVertices& vv, GeometricObject* pParent)
{
	Volume* pNew = reinterpret_cast<Volume*>(pCloneMe->create_empty_instance());
	uint numVrts = vv.num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		pNew->set_vertex(i, vv.vertex(i));
	register_volume(pNew, pParent);
	return iterator_cast<VolumeIterator>(pNew->m_entryIter);
}

////////////////////////////////////////////////////////////////////////
//	erase functions
void Grid::erase(GeometricObject* geomObj)
{
	assert(geomObj->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Vertex*). Invalid pipe section!");

	uint objType = geomObj->base_object_type_id();
	switch(objType)
	{
		case VERTEX:
			erase(dynamic_cast<VertexBase*>(geomObj));
			break;
		case EDGE:
			erase(dynamic_cast<EdgeBase*>(geomObj));
			break;
		case FACE:
			erase(dynamic_cast<Face*>(geomObj));
			break;
		case VOLUME:
			erase(dynamic_cast<Volume*>(geomObj));
			break;
	};
}

void Grid::erase(VertexBase* vrt)
{
	assert((vrt != NULL) && "ERROR in Grid::erase(Vertex*): invalid pointer)");
	assert(vrt->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Vertex*). Invalid pipe section!");

	unregister_vertex(vrt);

	delete vrt;
}

void Grid::erase(EdgeBase* edge)
{
	assert((edge != NULL) && "ERROR in Grid::erase(Edge*): invalid pointer)");
	assert(edge->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Edge*). Invalid pipe section!");

	unregister_edge(edge);

	delete edge;
}

void Grid::erase(Face* face)
{
	assert((face != NULL) && "ERROR in Grid::erase(Face*): invalid pointer)");
	assert(face->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Face*). Invalid pipe section!");

	unregister_face(face);

	delete face;
}

void Grid::erase(Volume* vol)
{
	assert((vol != NULL) && "ERROR in Grid::erase(Volume*): invalid pointer)");
	assert(vol->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Volume*). Invalid pipe section!");

	unregister_volume(vol);

	delete vol;
}

//	the geometric-object-collection:
GeometricObjectCollection Grid::get_geometric_object_collection()
{
	return GeometricObjectCollection(&m_elementStorage[VERTEX].m_sectionContainer,
									 &m_elementStorage[EDGE].m_sectionContainer,
									 &m_elementStorage[FACE].m_sectionContainer,
									 &m_elementStorage[VOLUME].m_sectionContainer);
}

void Grid::flip_orientation(Face* f)
{
//	inverts the order of vertices.
	uint numVrts = (int)f->num_vertices();
	vector<VertexBase*> vVrts(numVrts);
	
	uint i;
	for(i = 0; i < numVrts; ++i)
		vVrts[i] = f->m_vertices[i];
		
	for(i = 0; i < numVrts; ++i)
		f->m_vertices[i] = vVrts[numVrts - 1 - i];
}

void Grid::flip_orientation(Volume* vol)
{
//	flips the orientation of volumes
//	get the descriptor for the flipped volume
	VolumeDescriptor vd;
	vol->get_flipped_orientation(vd);
	
//	change vertex order of the original volume
	size_t numVrts = vol->num_vertices();
	for(size_t i = 0; i < numVrts; ++i)
		vol->m_vertices[i] = vd.vertex(i);
}

size_t Grid::vertex_fragmentation()
{
	return m_elementStorage[VERTEX].m_attachmentPipe.num_data_entries() - m_elementStorage[VERTEX].m_attachmentPipe.num_elements();
}

size_t Grid::edge_fragmentation()
{
	return m_elementStorage[EDGE].m_attachmentPipe.num_data_entries() - m_elementStorage[EDGE].m_attachmentPipe.num_elements();
}

size_t Grid::face_fragmentation()
{
	return m_elementStorage[FACE].m_attachmentPipe.num_data_entries() - m_elementStorage[FACE].m_attachmentPipe.num_elements();
}

size_t Grid::volume_fragmentation()
{
	return m_elementStorage[VOLUME].m_attachmentPipe.num_data_entries() - m_elementStorage[VOLUME].m_attachmentPipe.num_elements();
}

////////////////////////////////////////////////////////////////////////
//	pass_on_values
void Grid::pass_on_values(Grid::AttachmentPipe& attachmentPipe,
							GeometricObject* pSrc, GeometricObject* pDest)
{
	for(AttachmentPipe::ConstAttachmentEntryIterator iter = attachmentPipe.attachments_begin();
		iter != attachmentPipe.attachments_end(); iter++)
	{
		if((*iter).m_userData == 1)
			(*iter).m_pContainer->copy_data(get_attachment_data_index(pSrc),
											get_attachment_data_index(pDest));
	}
}

void Grid::pass_on_values(VertexBase* objSrc, VertexBase* objDest)
{
	pass_on_values(m_elementStorage[VERTEX].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(EdgeBase* objSrc, EdgeBase* objDest)
{
	pass_on_values(m_elementStorage[EDGE].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(Face* objSrc, Face* objDest)
{
	pass_on_values(m_elementStorage[FACE].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(Volume* objSrc, Volume* objDest)
{
	pass_on_values(m_elementStorage[VOLUME].m_attachmentPipe, objSrc, objDest);
}

////////////////////////////////////////////////////////////////////////
//	options
void Grid::set_options(uint options)
{
	change_options(options);
}

uint Grid::get_options() const
{
	return m_options;
}

void Grid::enable_options(uint options)
{
	change_options(m_options | options);
}

void Grid::disable_options(uint options)
{
	change_options(m_options & (~options));
}

bool Grid::option_is_enabled(uint option) const
{
	return (m_options & option) == option;
}

void Grid::change_options(uint optsNew)
{
	change_vertex_options(optsNew &	0x000000FF);
	change_edge_options(optsNew & 	0x0000FF00);
	change_face_options(optsNew & 	0x00FF0000);
	change_volume_options(optsNew &	0xFF000000);
	assert((m_options == optsNew) && "Grid::change_options failed");
}
/*
void Grid::register_observer(GridObserver* observer, uint observerType)
{
//	check which elements have to be observed and store pointers to the observers.
//	avoid double-registration!
	ObserverContainer* observerContainers[] = {&m_gridObservers, &m_vertexObservers,
												&m_edgeObservers, & m_faceObservers, &m_volumeObservers};

	uint observerTypes[] = {OT_GRID_OBSERVER, OT_VERTEX_OBSERVER, OT_EDGE_OBSERVER, OT_FACE_OBSERVER, OT_VOLUME_OBSERVER};
	for(int i = 0; i < 5; ++i)
	{
		if((observerType & observerTypes[i]) == observerTypes[i])
		{
			ObserverContainer::iterator iter = find(observerContainers[i]->begin(), observerContainers[i]->end(), observer);
			if(iter == observerContainers[i]->end())
				observerContainers[i]->push_back(observer);
		}
	}

//	if the observer is a grid observer, notify him about the registration
	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
		observer->registered_at_grid(this);
}

void Grid::unregister_observer(GridObserver* observer)
{
//	check where the observer has been registered and erase the corresponding entries.
	ObserverContainer* observerContainers[] = {&m_gridObservers, &m_vertexObservers,
												&m_edgeObservers, & m_faceObservers, &m_volumeObservers};

	bool unregisterdFromGridObservers = false;
	for(int i = 0; i < 5; ++i)
	{
		ObserverContainer::iterator iter = find(observerContainers[i]->begin(), observerContainers[i]->end(), observer);
		if(iter != observerContainers[i]->end())
		{
			if(i == 0)
				unregisterdFromGridObservers = true;
			observerContainers[i]->erase(iter);
		}
	}

//	if the observer is a grid observer, notify him about the unregistration
	if(unregisterdFromGridObservers)
		observer->unregistered_from_grid(this);
}
*/
void Grid::register_observer(GridObserver* observer, uint observerType)
{
//	check which elements have to be observed and store pointers to the observers.
//	avoid double-registration!
	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_gridObservers.begin(),
												m_gridObservers.end(), observer);
		if(iter == m_gridObservers.end())
			m_gridObservers.push_back(observer);
	}

	if((observerType & OT_VERTEX_OBSERVER) == OT_VERTEX_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_vertexObservers.begin(),
												m_vertexObservers.end(), observer);
		if(iter == m_vertexObservers.end())
			m_vertexObservers.push_back(observer);
	}

	if((observerType & OT_EDGE_OBSERVER) == OT_EDGE_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_edgeObservers.begin(),
												m_edgeObservers.end(), observer);
		if(iter == m_edgeObservers.end())
			m_edgeObservers.push_back(observer);
	}

	if((observerType & OT_FACE_OBSERVER) == OT_FACE_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_faceObservers.begin(),
												m_faceObservers.end(), observer);
		if(iter == m_faceObservers.end())
			m_faceObservers.push_back(observer);
	}

	if((observerType & OT_VOLUME_OBSERVER) == OT_VOLUME_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_volumeObservers.begin(),
												m_volumeObservers.end(), observer);
		if(iter == m_volumeObservers.end())
			m_volumeObservers.push_back(observer);
	}

//	if the observer is a grid observer, notify him about the registration
//	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
//		observer->registered_at_grid(this);
}

void Grid::unregister_observer(GridObserver* observer)
{
//	check where the observer has been registered and erase the corresponding entries.
	//bool unregisterdFromGridObservers = false;

	{
		ObserverContainer::iterator iter = find(m_gridObservers.begin(),
												m_gridObservers.end(), observer);
		if(iter != m_gridObservers.end())
			m_gridObservers.erase(iter);

//		unregisterdFromGridObservers = true;
	}

	{
		ObserverContainer::iterator iter = find(m_vertexObservers.begin(),
												m_vertexObservers.end(), observer);
		if(iter != m_vertexObservers.end())
			m_vertexObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_edgeObservers.begin(),
												m_edgeObservers.end(), observer);
		if(iter != m_edgeObservers.end())
			m_edgeObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_faceObservers.begin(),
												m_faceObservers.end(), observer);
		if(iter != m_faceObservers.end())
			m_faceObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_volumeObservers.begin(),
												m_volumeObservers.end(), observer);
		if(iter != m_volumeObservers.end())
			m_volumeObservers.erase(iter);
	}

//	if the observer is a grid observer, notify him about the unregistration
//	if(unregisterdFromGridObservers)
//		observer->unregistered_from_grid(this);

}


////////////////////////////////////////////////////////////////////////
//	associated edge access
Grid::AssociatedEdgeIterator Grid::associated_edges_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_EDGES." << endl);
		vertex_store_associated_edges(true);
	}
	return m_aaEdgeContainerVERTEX[vrt].begin();
}

Grid::AssociatedEdgeIterator Grid::associated_edges_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_EDGES." << endl);
		vertex_store_associated_edges(true);
	}
	return m_aaEdgeContainerVERTEX[vrt].end();
}

Grid::AssociatedEdgeIterator Grid::associated_edges_begin(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(face): auto-enabling FACEOPT_STORE_ASSOCIATED_EDGES." << endl);
		face_store_associated_edges(true);
	}
	return m_aaEdgeContainerFACE[face].begin();
}

Grid::AssociatedEdgeIterator Grid::associated_edges_end(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(face): auto-enabling FACEOPT_STORE_ASSOCIATED_EDGES." << endl);
		face_store_associated_edges(true);
	}
	return m_aaEdgeContainerFACE[face].end();
}

Grid::AssociatedEdgeIterator Grid::associated_edges_begin(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_EDGES." << endl);
		volume_store_associated_edges(true);
	}
	return m_aaEdgeContainerVOLUME[vol].begin();
}

Grid::AssociatedEdgeIterator Grid::associated_edges_end(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_EDGES." << endl);
		volume_store_associated_edges(true);
	}
	return m_aaEdgeContainerVOLUME[vol].end();
}

////////////////////////////////////////////////////////////////////////
//	associated face access
Grid::AssociatedFaceIterator Grid::associated_faces_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_FACES." << endl);
		vertex_store_associated_faces(true);
	}
	return m_aaFaceContainerVERTEX[vrt].begin();
}

Grid::AssociatedFaceIterator Grid::associated_faces_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_FACES." << endl);
		vertex_store_associated_faces(true);
	}
	return m_aaFaceContainerVERTEX[vrt].end();
}

Grid::AssociatedFaceIterator Grid::associated_faces_begin(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_FACES." << endl);
		edge_store_associated_faces(true);
	}
	return m_aaFaceContainerEDGE[edge].begin();
}

Grid::AssociatedFaceIterator Grid::associated_faces_end(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_FACES." << endl);
		edge_store_associated_faces(true);
	}
	return m_aaFaceContainerEDGE[edge].end();
}

Grid::AssociatedFaceIterator Grid::associated_faces_begin(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_FACES." << endl);
		volume_store_associated_faces(true);
	}
	return m_aaFaceContainerVOLUME[vol].begin();
}

Grid::AssociatedFaceIterator Grid::associated_faces_end(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_FACES." << endl);
		volume_store_associated_faces(true);
	}
	return m_aaFaceContainerVOLUME[vol].end();
}

////////////////////////////////////////////////////////////////////////
//	associated volume access
Grid::AssociatedVolumeIterator Grid::associated_volumes_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		vertex_store_associated_volumes(true);
	}
	return m_aaVolumeContainerVERTEX[vrt].begin();
}

Grid::AssociatedVolumeIterator Grid::associated_volumes_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		vertex_store_associated_volumes(true);
	}
	return m_aaVolumeContainerVERTEX[vrt].end();
}

Grid::AssociatedVolumeIterator Grid::associated_volumes_begin(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		edge_store_associated_volumes(true);
	}
	return m_aaVolumeContainerEDGE[edge].begin();
}

Grid::AssociatedVolumeIterator Grid::associated_volumes_end(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		edge_store_associated_volumes(true);
	}
	return m_aaVolumeContainerEDGE[edge].end();
}

Grid::AssociatedVolumeIterator Grid::associated_volumes_begin(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(face): auto-enabling FACEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		face_store_associated_volumes(true);
	}
	return m_aaVolumeContainerFACE[face].begin();
}

Grid::AssociatedVolumeIterator Grid::associated_volumes_end(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(face): auto-enabling FACEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		face_store_associated_volumes(true);
	}
	return m_aaVolumeContainerFACE[face].end();
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	neighbourhood access
EdgeBase* Grid::get_edge(VertexBase* v1, VertexBase* v2)
{
	EdgeDescriptor ed(v1, v2);
	return find_edge_in_associated_edges(v1, ed);
}

EdgeBase* Grid::get_edge(EdgeVertices& ev)
{
	return find_edge_in_associated_edges(ev.vertex(0), ev);
}

EdgeBase* Grid::get_edge(Face* f, int ind)
{
//	get the descriptor of the i-th edge
	EdgeDescriptor ed;
	f->edge(ind, ed);
	
//	check whether the face stores associated edges
	if(option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES))
	{
	//	it does. get the edge that matches.
		return find_edge_in_associated_edges(f, ed);
	}
	else
	{
	//	it doesn't. find the edge by checking vertices.
		return find_edge_in_associated_edges(ed.vertex(0), ed);
	}
	
	return NULL;
}

EdgeBase* Grid::get_edge(Volume* v, int ind)
{
//	get the descriptor of the i-th edge
	EdgeDescriptor ed;
	v->edge(ind, ed);
	
//	check whether the face stores associated edges
	if(option_is_enabled(VOLOPT_STORE_ASSOCIATED_EDGES))
	{
	//	it does. get the edge that matches.
		return find_edge_in_associated_edges(v, ed);
	}
	else
	{
	//	it doesn't. find the edge by checking vertices.
		return find_edge_in_associated_edges(ed.vertex(0), ed);
	}
	
	return NULL;
}

Face* Grid::get_face(FaceVertices& fv)
{
	return find_face_in_associated_faces(fv.vertex(0), fv);
}

Face* Grid::get_face(Volume* v, int ind)
{
	FaceDescriptor fd;
	v->face(ind, fd);
	
//	check whether the volume stores associated faces
	if(option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES))
	{
		return find_face_in_associated_faces(v, fd);
	}
	else {
	//	it does not. check associated faces of the first vertex of fd.
		return find_face_in_associated_faces(fd.vertex(0), fd);
	}
	return NULL;
}

Volume* Grid::get_volume(VolumeVertices& vv)
{
	return find_volume_in_associated_volumes(vv.vertex(0), vv);
}

////////////////////////////////////////////////////////////////////////
//	sides
template <>
EdgeBase::lower_dim_base_object*
Grid::get_side<EdgeBase>(EdgeBase* obj, size_t side)
{
	assert(side >= 0 && side < 2 && "ERROR in Grid::get_side(EdgeBase*, ...): Bad side index!");
	return obj->vertex(side);
}

template <>
Face::lower_dim_base_object*
Grid::get_side<Face>(Face* obj, size_t side)
{
	assert(side >= 0 && side < obj->num_edges() && "ERROR in Grid::get_side(Face*, ...): Bad side index!");
	return get_edge(obj, side);
}

template <>
Volume::lower_dim_base_object*
Grid::get_side<Volume>(Volume* obj, size_t side)
{
	assert(side >= 0 && side < obj->num_faces() && "ERROR in Grid::get_side(Volume*, ...): Bad side index!");
	return get_face(obj, side);
}

////////////////////////////////////////////////////////////////////////
//	marks
void Grid::init_marks()
{
//	attach marks to the elements
	if(m_currentMark == 0)
	{
	//	marks have not yet been initialized - do that now
	//	attach m_currentMark with default value 0
	//	(0 is never the currentMark while marks are active).
		m_currentMark = 1;
		attach_to_vertices_dv(m_aMark, 0);
		attach_to_edges_dv(m_aMark, 0);
		attach_to_faces_dv(m_aMark, 0);
		attach_to_volumes_dv(m_aMark, 0);
		
		m_aaMarkVRT.access(*this, m_aMark);
		m_aaMarkEDGE.access(*this, m_aMark);
		m_aaMarkFACE.access(*this, m_aMark);
		m_aaMarkVOL.access(*this, m_aMark);

		m_bMarking = false;
	}
}

void Grid::reset_marks()
{
//	set all marks to 0 and m_currentMark to 1
	m_currentMark = 1;
	AMark::ContainerType* pContainer;

//	reset vertex marks
	pContainer = get_attachment_data_container<VertexBase>(m_aMark);
	for(uint i = 0; i < pContainer->size(); ++i)
		pContainer->get_elem(i) = 0;

//	reset edge marks
	pContainer = get_attachment_data_container<EdgeBase>(m_aMark);
	for(uint i = 0; i < pContainer->size(); ++i)
		pContainer->get_elem(i) = 0;

//	reset face marks
	pContainer = get_attachment_data_container<Face>(m_aMark);
	for(uint i = 0; i < pContainer->size(); ++i)
		pContainer->get_elem(i) = 0;

//	reset volume marks
	pContainer = get_attachment_data_container<Volume>(m_aMark);
	for(uint i = 0; i < pContainer->size(); ++i)
		pContainer->get_elem(i) = 0;	
}

void Grid::remove_marks()
{
	if(m_currentMark != 0)
	{
		m_currentMark = 0;
		detach_from_vertices(m_aMark);
		detach_from_edges(m_aMark);
		detach_from_faces(m_aMark);
		detach_from_volumes(m_aMark);
	}
}

void Grid::begin_marking()
{
	if(m_currentMark == 0)
	{
	//	marks are disabled. we have to activate them
		init_marks();
	}
	
	if(m_bMarking){
		throw(UGFatalError("ERROR in Grid::begin_marking(): marking is already active. Don't forget to call end_marking when you're done with marking."));
	}
	
//	increase currentMark
	++m_currentMark;
	
//	check whether we have to reset-marks
	if(m_currentMark == -1)
		reset_marks();
		
//	set m_bMarking to true
	m_bMarking = true;
}

void Grid::end_marking()
{
	m_bMarking = false;
}

void Grid::clear_marks()
{
	if(m_bMarking){
		end_marking();
		begin_marking();
	}
	else{
		begin_marking();
		end_marking();
	}
}

}//	end of namespace
