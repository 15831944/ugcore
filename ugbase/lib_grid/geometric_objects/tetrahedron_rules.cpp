// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 27.05.2011 (m,d,y)

#include <cassert>
#include <iostream>
#include "tetrahedron_rules.h"
#include "rule_util.h"

using namespace std;

namespace ug{
namespace tet_rules
{

int Refine(int* newIndsOut, int* newEdgeVrts, bool& newCenterOut)
{
	newCenterOut = false;
//	If a refinement rule is not implemented, fillCount will stay at 0.
//	Before returning, we'll check, whether fillCount is at 0 and will
//	perform refinement with an additional vertex in this case.

//	corner status is used to mark vertices, which are connected to refined edges
//	the value tells, how many associated edges are refined.
	int cornerStatus[4] = {0, 0, 0, 0};

//	here we'll store the index of each edge, which will be refined.
//	Size of the valid sub-array is numNewVrts, which is calculated below.
	int refEdgeInds[NUM_EDGES];

//	count the number of new vertices and fill newVrtEdgeInds
	int numNewVrts = 0;
	for(int i = 0; i < NUM_EDGES; ++i){
		if(newEdgeVrts[i]){
			refEdgeInds[numNewVrts] = i;
			++numNewVrts;

		//	adjust corner status of associated vertices
			const int* evi = EDGE_VRT_INDS[i];
			++cornerStatus[evi[0]];
			++cornerStatus[evi[1]];
		}
	}

//	the fillCount tells how much data has already been written to newIndsOut.
	int fillCount = 0;

//	depending on the number of new vertices, we will now apply different
//	refinement rules. Further distinction may have to be done.
	switch(numNewVrts){
		case 0:
		{
		//	simply put the default tetrahedron back to newIndsOut
			newIndsOut[fillCount++] = 4;
			newIndsOut[fillCount++] = 0;
			newIndsOut[fillCount++] = 1;
			newIndsOut[fillCount++] = 2;
			newIndsOut[fillCount++] = 3;
		}break;

		case 1:
		{
			int refEdgeInd = refEdgeInds[0];
		//	the two faces which are not connected to the refined edge
		//	serve as bottom for the new tetrahedrons.
			for(int i_face = 0; i_face < NUM_FACES; ++i_face){
				if(!FACE_CONTAINS_EDGE[i_face][refEdgeInd]){
					const int* fvi = FACE_VRT_INDS[i_face];

					newIndsOut[fillCount++] = 4;
					newIndsOut[fillCount++] = fvi[0];
					newIndsOut[fillCount++] = fvi[1];
					newIndsOut[fillCount++] = fvi[2];
					newIndsOut[fillCount++] = NUM_VERTICES + refEdgeInd;
				}
			}
		}break;

		case 2:
		{
		//	two types exist: The two refined edges share a vertex or not.
			if(OPPOSED_EDGE[refEdgeInds[0]] == refEdgeInds[1]){
			//	they do not share an edge
			}
			else{
			//	they share an edge
			//	We have to create a pyramid and a tetrahedron.
			//	get the triangle, which contains both refEdges
				int refTriInd = FACE_FROM_EDGES[refEdgeInds[0]][refEdgeInds[1]];
				const int* f = FACE_VRT_INDS[refTriInd];

			//	find the edge (v0, v1) in refTri, which won't be refined
				int v0 = -1; int v1 = -1; int v2 = -1;
				for(int i = 0; i < 3; ++i){
					v0 = f[i];
					v1 = f[(i+1)%3];
					v2 = f[(i+2)%3];
					if(cornerStatus[v0] == 1 && cornerStatus[v1] == 1)
						break;
				}

			//	make sure that v2 is connected to two refined edges
				assert(cornerStatus[v2] == 2);

			//	get the new vertex on edge v1v2 and on v2v0
				int v1v2 = EDGE_FROM_VRTS[v1][v2] + NUM_VERTICES;
				int v2v0 = EDGE_FROM_VRTS[v2][v0] + NUM_VERTICES;

			//	get the top (vertex with cornerState 0)
				int vtop = -1;
				for(int i = 0; i < NUM_VERTICES; ++i){
					if(cornerStatus[i] == 0){
						vtop = i;
						break;
					}
				}

			//	now lets build the pyramid
				int& fi = fillCount;
				int* inds = newIndsOut;
				inds[fi++] = 5;
				inds[fi++] = v0;	inds[fi++] = v1;
				inds[fi++] = v1v2; 	inds[fi++] = v2v0;
				inds[fi++] = vtop;

			//	and now the terahedron
				inds[fi++] = 4;
				inds[fi++] = v2;	inds[fi++] = vtop;
				inds[fi++] = v2v0;	inds[fi++] = v1v2;
			}
		}break;

		case 3:
		{
		//	different possibilities exist. First we'll treat the one,
		//	where all new vertices lie on the edges of one triangle.
		//	Note that refTriInd could be -1 after the call.
			int refTriInd = FACE_FROM_EDGES[refEdgeInds[0]][refEdgeInds[1]];
			if(refTriInd == FACE_FROM_EDGES[refEdgeInds[1]][refEdgeInds[2]])
			{
			//	all three lie in one plane (refTriInd has to be != -1 to get here)
			//	get the top (vertex with cornerState 0)
				int vtop = -1;
				for(int i = 0; i < NUM_VERTICES; ++i){
					if(cornerStatus[i] == 0){
						vtop = i;
						break;
					}
				}

				const int* f = FACE_VRT_INDS[refTriInd];

			//	create four new tetrahedrons
				const int v0 = f[0]; const int v1 = f[1]; const int v2 = f[2];
				const int v0v1 = EDGE_FROM_VRTS[v0][v1] + NUM_VERTICES;
				const int v1v2 = EDGE_FROM_VRTS[v1][v2] + NUM_VERTICES;
				const int v2v0 = EDGE_FROM_VRTS[v2][v0] + NUM_VERTICES;

				int& fi = fillCount;
				int* inds = newIndsOut;
				inds[fi++] = 4;
				inds[fi++] = v0;	inds[fi++] = vtop;
				inds[fi++] = v0v1;	inds[fi++] = v2v0;
				inds[fi++] = 4;
				inds[fi++] = v1;	inds[fi++] = vtop;
				inds[fi++] = v1v2;	inds[fi++] = v0v1;
				inds[fi++] = 4;
				inds[fi++] = v2;	inds[fi++] = vtop;
				inds[fi++] = v2v0;	inds[fi++] = v1v2;
				inds[fi++] = 4;
				inds[fi++] = v0v1;	inds[fi++] = vtop;
				inds[fi++] = v1v2;	inds[fi++] = v2v0;
			}
			else{
			//	we have to further distinguish.
			//	gather the corners with corner-state 3 and corner state 1
				int corner3 = -1;
				int freeCorner[NUM_VERTICES];
				int freeCount = 0;
				for(int i = 0; i < NUM_VERTICES; ++i){
					if(cornerStatus[i] == 3)
						corner3 = i;
					else
						freeCorner[freeCount++] = i;
				}

				if(corner3 != -1){
				//	a corner with corner state 3 exists.
					assert(freeCount == 3);

				// get the face which won't be refined (required for correct orientation)
					int freeTriInd = FACE_FROM_VRTS[freeCorner[0]][freeCorner[1]]
					                               [freeCorner[2]];

				//	the free tri is the new base, corner3 the top
					const int* f = FACE_VRT_INDS[freeTriInd];
					int v2v3 = EDGE_FROM_VRTS[f[2]][corner3] + NUM_VERTICES;
					int v1v3 = EDGE_FROM_VRTS[f[1]][corner3] + NUM_VERTICES;
					int v0v3 = EDGE_FROM_VRTS[f[0]][corner3] + NUM_VERTICES;

				//	add a prism and a tetrahedron
					int& fi = fillCount;
					int* inds = newIndsOut;
					inds[fi++] = 6;
					inds[fi++] = f[0]; inds[fi++] = f[1]; inds[fi++] = f[2];
					inds[fi++] = v0v3; inds[fi++] = v1v3; inds[fi++] = v2v3;

					inds[fi++] = 4;
					inds[fi++] = v2v3; inds[fi++] = corner3; inds[fi++] = v0v3;
					inds[fi++] = v1v3;
				}
			}
		}break;

		case 4:
		{
		/*THIS IS COMPLICATED!!!
		//	multiple settings with 4 refined edges exist.
		//	currently only one is directly supported.

		//	check whether all vertices have corner state 2
			bool all2 = true;
			for(int i = 0; i < NUM_VERTICES; ++i){
				if(cornerStatus[i] !=2){
					all2 = false;
					break;
				}
			}

			if(all2){
				cout << "CASE_4_straight cut\n";
			}
			else
				cout << "CASE_4_UNKNOWN\n";
		*/
		}break;

	//	REGULAR REFINEMENT
		case 6:
		{
			int& fi = fillCount;
			int* inds = newIndsOut;
			inds[fi++] = 4;
			inds[fi++] = 0;					inds[fi++] = NUM_VERTICES + 0;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 3;

			inds[fi++] = 4;
			inds[fi++] = 1;					inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = 2;					inds[fi++] = NUM_VERTICES + 2;
			inds[fi++] = NUM_VERTICES + 1;	inds[fi++] = NUM_VERTICES + 5;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 0;	inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 0;
			inds[fi++] = NUM_VERTICES + 4;	inds[fi++] = NUM_VERTICES + 3;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 3;	inds[fi++] = NUM_VERTICES + 2;
			inds[fi++] = NUM_VERTICES + 5;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 2;	inds[fi++] = NUM_VERTICES + 1;
			inds[fi++] = NUM_VERTICES + 5;	inds[fi++] = NUM_VERTICES + 4;

			inds[fi++] = 4;
			inds[fi++] = NUM_VERTICES + 3;	inds[fi++] = NUM_VERTICES + 4;
			inds[fi++] = NUM_VERTICES + 5;	inds[fi++] = 3;
		}break;
	}

	if(fillCount == 0){
	//	call recursive refine
		fillCount = shared_rules::RecursiveRefine(newIndsOut, newEdgeVrts,
									FACE_VRT_INDS, FACE_EDGE_INDS, NUM_VERTICES,
									NUM_EDGES, NUM_FACES);

	//	the rule requires a new center vertex
		newCenterOut = true;
	}

	return fillCount;
}


}//	end of namespace tet_rules
}//	end of namespace ug
