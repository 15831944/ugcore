//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m01 d18

#include "neighborhood.h"
#include "lib_grid/algorithms/geom_obj_util/vertex_util.h"

using namespace std;

namespace ug
{
							
////////////////////////////////////////////////////////////////////////
//
void CollectNeighbors(std::vector<VertexBase*>& vNeighborsOut,
						Grid& grid, VertexBase* vrt, uint nbhType,
						CB_ConsiderEdge considerEdge,
						CB_ConsiderFace considerFace,
						CB_ConsiderVolume considerVol)
{
//	clear the container
	vNeighborsOut.clear();
	
//	begin marking
	grid.begin_marking();
	
//	mark vrt - this makes things easier
	grid.mark(vrt);
	
//	iterate through associated edges
	if(nbhType & NHT_EDGE_NEIGHBORS){
		Grid::AssociatedEdgeIterator iterEnd = grid.associated_edges_end(vrt);
		for(Grid::AssociatedEdgeIterator iter = grid.associated_edges_begin(vrt);
			iter != iterEnd; ++iter)
		{
			if(considerEdge(*iter)){
				VertexBase* neighbour = GetConnectedVertex(*iter, vrt);
				if(!grid.is_marked(neighbour)){
					grid.mark(neighbour);
					vNeighborsOut.push_back(neighbour);
				}
			}
		}
	}

//	iterate through associated faces
	if(nbhType & NHT_FACE_NEIGHBORS){
		Grid::AssociatedFaceIterator iterEnd = grid.associated_faces_end(vrt);
		for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(vrt);
			iter != iterEnd; ++iter)
		{
			if(considerFace(*iter)){
				Face* f = *iter;
				for(size_t i = 0; i < f->num_vertices(); ++i){
					VertexBase* neighbour = f->vertex(i);
					if(!grid.is_marked(neighbour)){
						grid.mark(neighbour);
						vNeighborsOut.push_back(neighbour);
					}
				}
			}
		}
	}

//	iterate through associated volumes
	if(nbhType & NHT_VOLUME_NEIGHBORS){
		Grid::AssociatedVolumeIterator iterEnd = grid.associated_volumes_end(vrt);
		for(Grid::AssociatedVolumeIterator iter = grid.associated_volumes_begin(vrt);
			iter != iterEnd; ++iter)
		{
			if(considerVol(*iter)){
				Volume* v = *iter;
				for(size_t i = 0; i < v->num_vertices(); ++i){
					VertexBase* neighbour = v->vertex(i);
					if(!grid.is_marked(neighbour)){
						grid.mark(neighbour);
						vNeighborsOut.push_back(neighbour);
					}
				}
			}
		}
	}
		
//	end marking
	grid.end_marking();
}
							
////////////////////////////////////////////////////////////////////////
//	CollectNeighbors
void CollectNeighbors(std::vector<EdgeBase*>& vNeighborsOut, EdgeBase* e,
					   Grid& grid, NeighborhoodType nbhType)
{
//	clear the container
	vNeighborsOut.clear();
	
//	default neighbourhood:
	if(nbhType == NHT_DEFAULT)
		nbhType = NHT_VERTEX_NEIGHBORS;

//	edges can only be vertex-neighbours
	if(nbhType != NHT_VERTEX_NEIGHBORS)
		return;

//	begin marking
	grid.begin_marking();
	
//	mark the edge
	grid.mark(e);
	
//	mark the vertices of the edge
	grid.mark(e->vertex(0));
	grid.mark(e->vertex(1));	
	
//	iterate over all edges that are connected to the vertices.
//	if the edge is not yet marked, we have to push it to vNeighboursOut.
	for(uint i = 0; i < 2; ++i)
	{
		Grid::AssociatedEdgeIterator iterEnd = grid.associated_edges_end(e->vertex(i));
		for(Grid::AssociatedEdgeIterator iter = grid.associated_edges_begin(e->vertex(i));
			iter != iterEnd; ++iter)
		{
			if(!grid.is_marked(*iter))
			{
				vNeighborsOut.push_back(*iter);
				grid.mark(*iter);
			}
		}
	}

//	end marking
	grid.end_marking();
}


////////////////////////////////////////////////////////////////////////
//	CollectNeighbors
void CollectNeighbors(std::vector<Face*>& vNeighborsOut, Face* f,
					   Grid& grid, NeighborhoodType nbhType)
{
//	clear the container
	vNeighborsOut.clear();
	
//	default neighbourhood:
	if(nbhType == NHT_DEFAULT)
		nbhType = NHT_EDGE_NEIGHBORS;

//	faces can't be face-neighbours
	if(nbhType == NHT_FACE_NEIGHBORS)
		return;

//	begin marking
	grid.begin_marking();
	
//	mark the face
	grid.mark(f);
	
//	mark the vertices of the face
	uint numVrts = f->num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		grid.mark(f->vertex(i));
	
//	in order to get the maximum speed-up, we'll try to use
//	associated elements in grid.
	if((nbhType == NHT_EDGE_NEIGHBORS)
		&& grid.option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES
								  | EDGEOPT_STORE_ASSOCIATED_FACES
								  | FACEOPT_AUTOGENERATE_EDGES))
	{
	//	iterate through associated edges
		Grid::AssociatedEdgeIterator eEnd = grid.associated_edges_end(f);
		for(Grid::AssociatedEdgeIterator eIter = grid.associated_edges_begin(f);
			eIter != eEnd; ++eIter)
		{
		//	iterate through associated folumes of the eace
			Grid::AssociatedFaceIterator fEnd = grid.associated_faces_end(*eIter);
			for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(*eIter);
				iter != fEnd; ++iter)
			{
			//	if the face is not yet marked, then add it to the neighbours
				if(!grid.is_marked(*iter))
				{
					grid.mark(*iter);
					vNeighborsOut.push_back(*iter);
				}
			}
		}
	//	we're done in here. end-marking and return.
		grid.end_marking();
		return;
	}


//	iterate over all faces that are connected to the vertices.
//	if the face shares the elements as required by nbhType and
//	it is not yet marked, we have to push it to vNeighboursOut.
//	to optimize speed we'll check both valid nbhTypes separately.
//	the first case indeed is a subcase of the second
//	(compare counted marked vertices against nbhType)
	switch(nbhType)
	{
	case NHT_VERTEX_NEIGHBORS:
		for(uint i = 0; i < numVrts; ++i)
		{
			Grid::AssociatedFaceIterator iterEnd = grid.associated_faces_end(f->vertex(i));
			for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(f->vertex(i));
				iter != iterEnd; ++iter)
			{
				if(!grid.is_marked(*iter))
				{
					vNeighborsOut.push_back(*iter);
					grid.mark(*iter);
				}
			}
		}
		break;

	case NHT_EDGE_NEIGHBORS:
		for(uint i = 0; i < numVrts; ++i)
		{
			Grid::AssociatedFaceIterator iterEnd = grid.associated_faces_end(f->vertex(i));
			for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(f->vertex(i));
				iter != iterEnd; ++iter)
			{
				Face* nf = *iter;
				if(!grid.is_marked(nf))
				{
				//	count the marked vertices that are contained by *iter
				//	if there are more than 1, the faces share an edge
				//	(at least in a regular grid)
					int counter = 0;
					
					for(uint j = 0; j < nf->num_vertices(); ++j)
					{
						if(grid.is_marked(nf->vertex(j)))
						{
							++counter;
							if(counter > 1)
							{
								vNeighborsOut.push_back(nf);
								grid.mark(nf);
								break;
							}
						}
					}
				}
			}
		}
		break;
	default:
		break;
	}

//	end marking
	grid.end_marking();
}

////////////////////////////////////////////////////////////////////////
//	CollectNeighbors
void CollectNeighbors(std::vector<Volume*>& vNeighborsOut, Volume* v,
					   Grid& grid, NeighborhoodType nbhType)
{
//	clear the container
	vNeighborsOut.clear();
	
//	default neighbourhood:
	if(nbhType == NHT_DEFAULT)
		nbhType = NHT_FACE_NEIGHBORS;

//	begin marking
	grid.begin_marking();
	
//	mark the volume
	grid.mark(v);
	
//	mark the vertices of the volume
	uint numVrts = v->num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		grid.mark(v->vertex(i));

//	in order to get the maximum speed-up, we'll try to use
//	associated elements in grid.
	if((nbhType == NHT_FACE_NEIGHBORS)
		&& grid.option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES
								  | FACEOPT_STORE_ASSOCIATED_VOLUMES
								  | VOLOPT_AUTOGENERATE_FACES))
	{
	//	iterate through associated faces
		Grid::AssociatedFaceIterator fEnd = grid.associated_faces_end(v);
		for(Grid::AssociatedFaceIterator fIter = grid.associated_faces_begin(v);
			fIter != fEnd; ++fIter)
		{
		//	iterate through associated volumes of the face
			Grid::AssociatedVolumeIterator vEnd = grid.associated_volumes_end(*fIter);
			for(Grid::AssociatedVolumeIterator iter = grid.associated_volumes_begin(*fIter);
				iter != vEnd; ++iter)
			{
			//	if the volume is not yet marked, then add it to the neighbours
				if(!grid.is_marked(*iter))
				{
					grid.mark(*iter);
					vNeighborsOut.push_back(*iter);
				}
			}
		}
	//	we're done in here. end-marking and return.
		grid.end_marking();
		return;
	}

//	iterate over all volumes that are connected to the vertices.
//	if the volume shares the elements as required by nbhType and
//	it is not yet marked, we have to push it to vNeighboursOut.
//	to optimize speed we'll check both valid nbhTypes separately.
//	the first case indeed is a subcase of the second
	if(nbhType == NHT_VERTEX_NEIGHBORS)
		for(uint i = 0; i < numVrts; ++i)
		{
			Grid::AssociatedVolumeIterator iterEnd = grid.associated_volumes_end(v->vertex(i));
			for(Grid::AssociatedVolumeIterator iter = grid.associated_volumes_begin(v->vertex(i));
				iter != iterEnd; ++iter)
			{
				if(!grid.is_marked(*iter))
				{
					vNeighborsOut.push_back(*iter);
					grid.mark(*iter);
				}
			}
		}
	else
	{
		for(uint i = 0; i < numVrts; ++i)
		{
			Grid::AssociatedVolumeIterator iterEnd = grid.associated_volumes_end(v->vertex(i));
			for(Grid::AssociatedVolumeIterator iter = grid.associated_volumes_begin(v->vertex(i));
				iter != iterEnd; ++iter)
			{
				Volume* nv = *iter;
				if(!grid.is_marked(nv))
				{
				//	count the marked vertices that are contained by *iter
				//	if there as many as in nbhTypes, the volume is a neighbour.
				//	(at least in a regular grid)
					int counter = 0;
					
					for(uint j = 0; j < nv->num_vertices(); ++j)
					{
						if(grid.is_marked(nv->vertex(j)))
						{
							++counter;
							if(counter >= nbhType)
							{
								vNeighborsOut.push_back(nv);
								grid.mark(nv);
								break;
							}
						}
					}
				}
			}
		}
	}

//	end marking
	grid.end_marking();
}

void CollectNeighborhood(std::vector<Face*>& facesOut, Grid& grid,
						  VertexBase* vrt, size_t range,
						  bool clearContainer)
{
	if(clearContainer)
		facesOut.clear();
	
	vector<VertexBase*> candidates;
	size_t rangeBegin = 0;
	size_t rangeEnd = 1;
	
	candidates.push_back(vrt);
	
	grid.begin_marking();
	grid.mark(vrt);
	
//	we iterate over the range
	for(size_t i_range = 0; i_range < range; ++i_range){
	//	iterate from candidatesStart to candidatesEnd
	//	this is important, since we can make sure that only triangles
	//	in the correct range are considered
		for(size_t i_vrt = rangeBegin; i_vrt < rangeEnd; ++i_vrt)
		{
			VertexBase* v = candidates[i_vrt];
		//	iterate over associated faces
			for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(v);
				iter != grid.associated_faces_end(v); ++iter)
			{
				Face* f = *iter;
				if(!grid.is_marked(f)){
					grid.mark(f);
					facesOut.push_back(f);
					for(size_t i = 0; i < f->num_vertices(); ++i){
						if(!grid.is_marked(f->vertex(i))){
							grid.mark(f->vertex(i));
							candidates.push_back(f->vertex(i));
						}
					}
				}
			}
		}
		
	//	prepare next iteration
		rangeBegin = rangeEnd;
		rangeEnd = candidates.size();
	}
	
	grid.end_marking();
}

}//	end of namespace
