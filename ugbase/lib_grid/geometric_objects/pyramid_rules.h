// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 31.05.2011 (m,d,y)

#ifndef __H__UG__pyramid_rules__
#define __H__UG__pyramid_rules__

//	only required for dummy-parameter ug::vector3*
#include "common/math/ugmath_types.h"

namespace ug{
namespace pyra_rules
{

////////////////////////////////////////////////////////////////////////////////
//	LOOKUP TABLES

const int NUM_VERTICES	= 5;
const int NUM_EDGES		= 8;
const int NUM_FACES		= 5;
const int NUM_TRIS		= 4;
const int NUM_QUADS		= 1;
const int MAX_NUM_INDS_OUT = 128;//todo: this is just an estimate!

///	the local vertex indices of the given edge
const int EDGE_VRT_INDS[][2] = {	{0, 1}, {1, 2}, {2, 3}, {3, 0},
									{4, 0}, {4, 1}, {4, 2}, {4, 3}};

///	the local vertex indices of the given face
const int FACE_VRT_INDS[][4] = {	{0, 1, 2, 3},	{0, 4, 1, -1},
									{1, 4, 2, -1},	{2, 4, 3, -1},
									{0, 3, 4, -1}};

///	the pyramids top
const int TOP_VERTEX = 4;

/*
///	indicates whether an edge is a bottom edge
const int IS_BOTTOM_EDGE[8] =	{1, 1, 1, 1, 0, 0, 0, 0};

///	index of the bottom face
const int BOTTOM_FACE = 0;
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//	NOTE: The lists below are all generated automatically

///	returns the j-th edge of the i-th face
const int FACE_EDGE_INDS[5][4] = 	{{0, 1, 2, 3}, {4, 5, 0, -1}, {5, 6, 1, -1},
									 {6, 7, 2, -1}, {3, 7, 4, -1}};

///	tells whether the i-th face contains the j-th edge
const int FACE_CONTAINS_EDGE[][8] =
						{{1, 1, 1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 1, 0, 0},
						 {0, 1, 0, 0, 0, 1, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 1},
						 {0, 0, 0, 1, 1, 0, 0, 1}};

///	Associates the index of the connecting edge with each tuple of vertices.
/**	Use two vertex indices to index into this table to retrieve the index
 * of their connecting edge.
 */
const int EDGE_FROM_VRTS[5][5] =	{{-1, 0, -1, 3, 4}, {0, -1, 1, -1, 5},
									 {-1, 1, -1, 2, 6}, {3, -1, 2, -1, 7},
									 {4, 5, 6, 7, -1}};

///	Associates the index of the connecting face with each triple of vertices.
/**	Use three vertex indices to index into this table to retrieve the index
 * of their connecting face.
 */
const int FACE_FROM_VRTS[5][5][5] =
				{{{-1, -1, -1, -1, -1}, {-1, -1, 0, 0, 1}, {-1, 0, -1, 0, -1},
				  {-1, 0, 0, -1, 4}, {-1, 1, -1, 4, -1}},
				 {{-1, -1, 0, 0, 1}, {-1, -1, -1, -1, -1}, {0, -1, -1, 0, 2},
				  {0, -1, 0, -1, -1}, {1, -1, 2, -1, -1}},
				 {{-1, 0, -1, 0, -1}, {0, -1, -1, 0, 2}, {-1, -1, -1, -1, -1},
				  {0, 0, -1, -1, 3}, {-1, 2, -1, 3, -1}},
				 {{-1, 0, 0, -1, 4}, {0, -1, 0, -1, -1}, {0, 0, -1, -1, 3},
				  {-1, -1, -1, -1, -1}, {4, -1, 3, -1, -1}},
				 {{-1, 1, -1, 4, -1}, {1, -1, 2, -1, -1}, {-1, 2, -1, 3, -1},
				  {4, -1, 3, -1, -1}, {-1, -1, -1, -1, -1}}};

///	given two edges, the table returns the face, which contains both (or -1)
const int FACE_FROM_EDGES[][8] =
					{{0, 0, 0, 0, 1, 1, -1, -1}, {0, 0, 0, 0, -1, 2, 2, -1},
					 {0, 0, 0, 0, -1, -1, 3, 3}, {0, 0, 0, 0, 4, -1, -1, 4},
					 {1, -1, -1, 4, 1, 1, -1, 4}, {1, 2, -1, -1, 1, 1, 2, -1},
					 {-1, 2, 3, -1, -1, 2, 2, 3}, {-1, -1, 3, 4, 4, -1, 3, 3}};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**	returns an array of integers, which contains the indices of the objects
 * resulting from the refinement of a pyramid.
 *
 *
 * \param newIndsOut	Array which has to be of size MAX_NUM_INDS_OUT.
 * 						When the algorithm is done, the array will contain
 * 						sequences of integers: {{numInds, ind1, ind2, ...}, ...}.
 * 						Old vertices are referenced by their local index. Vertices
 * 						created on an edge are indexed by the index of the edge +
 * 						NUM_VERTICES.
 * 						Vertices created on a face are referenced by
 * 						NUM_VERTICES + NUM_EDGES + index_of_face.
 * 						If an inner vertex has to be created, it is referenced
 * 						by NUM_VERTICES + NUM_EDGES + NUM_FACES (in this case
 * 						newCenterOut is set to true).
 *
 * \param newEdgeVrts	Array of size NUM_EDGES, which has to contain 1 for each
 * 						edge, which shall be refined and 0 for each edge, which
 * 						won't be refined.
 *
 * \param newCenterOut	If the refinement-rule requires a center vertex, then
 * 						this parameter will be set to true. If not, it is set to
 * 						false.
 *
 * \param corners		Ignored.
 *
 * \returns	the number of entries written to newIndsOut or 0, if the refinement
 * 			could not be performed.
 */
int Refine(int* newIndsOut, int* newEdgeVrts, bool& newCenterOut,
		   vector3* corners = NULL);

}//	end of namespace pyra_rules
}//	end of namespace ug

#endif
