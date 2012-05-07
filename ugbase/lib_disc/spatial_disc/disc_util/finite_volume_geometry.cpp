/*
 * finite_volume_geometry.cpp
 *
 *  Created on: 04.09.2010
 *      Author: andreasvogel
 */


#include "common/util/provider.h"
#include "finite_volume_geometry.h"
#include "lib_disc/reference_element/reference_element.h"
#include "lib_disc/quadrature/quadrature.h"

namespace ug{

/**
 * \tparam	dim			dimension of coordinates
 * \tparam	TRefElem	Reference element type
 * \tparam	maxMid		Maximum number of elements for all dimensions
 */
template <int dim, typename TRefElem, int maxMid>
static void ComputeMidPoints(const TRefElem& rRefElem,
                             const MathVector<dim> vCorner[],
                             MathVector<dim> vvMid[][maxMid])
{
// 	compute local midpoints for all geometric objects with  0 < d <= dim
	for(int d = 1; d <= dim; ++d)
	{
	// 	loop geometric objects of dimension d
		for(size_t i = 0; i < rRefElem.num(d); ++i)
		{
		// 	set first node
			const size_t coID0 = rRefElem.id(d, i, 0, 0);
			vvMid[d][i] = vCorner[coID0];

		// 	add corner coordinates of the corners of the geometric object
			for(size_t j = 1; j < rRefElem.num(d, i, 0); ++j)
			{
				const size_t coID = rRefElem.id(d, i, 0, j);
				vvMid[d][i] += vCorner[coID];
			}

		// 	scale for correct averaging
			vvMid[d][i] *= 1./(rRefElem.num(d, i, 0));
		}
	}
}

/**
 * \param[in]	i		indicates that scvf corresponds to i'th edge of ref elem
 */
template <typename TRefElem>
static void ComputeSCVFMidID(const TRefElem& rRefElem,
                                   MidID vMidID[], int i)
{
	static const int dim = TRefElem::dim;

//	set mid ids
	{
	// 	start at edge midpoint
		vMidID[0] = MidID(1,i);

	// 	loop up dimension
		if(dim == 2)
		{
			vMidID[1] = MidID(dim, 0); // center of element
		}
		else if (dim == 3)
		{
			vMidID[1] = MidID(2, rRefElem.id(1, i, 2, 0)); // side 0
			vMidID[2] = MidID(dim, 0); // center of element
			vMidID[3] = MidID(2, rRefElem.id(1, i, 2, 1)); // side 1
		}
	}
}

/**
 * \param[in]	i		indicates that scvf corresponds to i'th corner of ref elem
 */
template <typename TRefElem>
static void ComputeSCVMidID(const TRefElem& rRefElem,
                            MidID vMidID[], int i)
{
	static const int dim = TRefElem::dim;

	if(dim == 1)
	{
		vMidID[0] = MidID(0, i); // set node as corner of scv
		vMidID[1] = MidID(dim, 0);	// center of element
	}
	else if(dim == 2)
	{
		vMidID[0] = MidID(0, i); // set node as corner of scv
		vMidID[1] = MidID(1, rRefElem.id(0, i, 1, 0)); // edge 1
		vMidID[2] = MidID(dim, 0);	// center of element
		vMidID[3] = MidID(1, rRefElem.id(0, i, 1, 1)); // edge 2
	}
	else if(dim == 3 && (rRefElem.reference_object_id() != ROID_PYRAMID || i != 4))
	{
		vMidID[0] = MidID(0, i); // set node as corner of scv
		vMidID[1] = MidID(1, rRefElem.id(0, i, 1, 1)); // edge 1
		vMidID[2] = MidID(2, rRefElem.id(0, i, 2, 0)); // face 0
		vMidID[3] = MidID(1, rRefElem.id(0, i, 1, 0)); // edge 0
		vMidID[4] = MidID(1, rRefElem.id(0, i, 1, 2)); // edge 2
		vMidID[5] = MidID(2, rRefElem.id(0, i, 2, 2)); // face 2
		vMidID[6] = MidID(dim, 0);	// center of element
		vMidID[7] = MidID(2, rRefElem.id(0, i, 2, 1)); // face 1
	}
	// TODO: Implement last ControlVolume for Pyramid
	else if(dim == 3 && rRefElem.reference_object_id() == ROID_PYRAMID && i == 4)
	{
		// this scv has 10 corners
		throw(UGError("Last SCV for Pyramid must be implemented"));
	}
	else {throw(UGError("Dimension higher that 3 not implemented."));}
}

/**
 * \param[in]	i		indicates that scvf corresponds to i'th corner of ref elem
 */
template <typename TRefElem>
static void ComputeBFMidID(const TRefElem& rRefElem, int side,
                            MidID vMidID[], int co)
{
	static const int dim = TRefElem::dim;

//	number of corners of side
	const int coOfSide = rRefElem.num(dim-1, side, 0);

// 	set mid ids
	if(dim == 2)
	{
		vMidID[co%2] = MidID(0, rRefElem.id(1, side, 0, co)); // corner of side
		vMidID[(co+1)%2] = MidID(1, side); // side midpoint
	}
	else if (dim == 3)
	{
		vMidID[0] = MidID(0, rRefElem.id(2, side, 0, co)); // corner of side
		vMidID[1] = MidID(1, rRefElem.id(2, side, 1, co)); // edge co
		vMidID[2] = MidID(2, side); // side midpoint
		vMidID[3] = MidID(1, rRefElem.id(2, side, 1, (co -1 + coOfSide)%coOfSide)); // edge co-1
	}
}

template <int dim, int maxMid>
static void CopyCornerByMidID(MathVector<dim> vCorner[],
                              const MidID vMidID[],
                              MathVector<dim> vvMidPos[][maxMid],
                              const size_t numCo)
{
	for(size_t i = 0; i < numCo; ++i)
	{
		const size_t d = vMidID[i].dim;
		const size_t id = vMidID[i].id;
		vCorner[i] = vvMidPos[d][id];
	}
}

///	returns the number of subelements for a reference element and order
size_t NumSubElements(ReferenceObjectID roid, int p)
{
	switch(roid)
	{
		case ROID_EDGE: return p;
		case ROID_TRIANGLE: return p*p;
		case ROID_QUADRILATERAL: return p*p;
		case ROID_TETRAHEDRON: return (p*(p+1)*(5*p-2))/6;
		case ROID_PRISM: return p*p*p;
		case ROID_HEXAHEDRON: return p*p*p;
		default:
			UG_LOG("ERROR in 'NumSubElements': No subelement specification given"
					" for "<<roid<<". Aborting.");
			throw(UGError("Reference Element not supported."));
	}
}


template <int dim>
void ComputeMultiIndicesOfSubElement(std::vector<MathVector<dim, int> >* vvMultiIndex,
                                     bool* vIsBndElem,
                                     std::vector<int>* vElemBndSide,
                                     std::vector<size_t>* vIndex,
                                     ReferenceObjectID roid,
                                     int p);

template <>
void ComputeMultiIndicesOfSubElement<1>(std::vector<MathVector<1, int> >* vvMultiIndex,
                                        bool* vIsBndElem,
                                        std::vector<int>* vElemBndSide,
                                        std::vector<size_t>* vIndex,
                                        ReferenceObjectID roid,
                                        int p)
{
//	switch for roid
	size_t se = 0;
	switch(roid)
	{
		case ROID_EDGE:
			for(int i = 0; i < p; ++i)
			{
				vvMultiIndex[se].resize(2);
				vvMultiIndex[se][0] = MathVector<1,int>(i);
				vvMultiIndex[se][1] = MathVector<1,int>(i+1);

				// reset bnd info
				vIsBndElem[se] = false;
				vElemBndSide[se].clear(); vElemBndSide[se].resize(3, -1);

				if(i==0)
				{
					vIsBndElem[se] = true;
					vElemBndSide[se][0] = 0;
				}
				if(i==p-1)
				{
					vIsBndElem[se] = true;
					vElemBndSide[se][1] = 1;
				}
				++se;
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferenceEdge> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}
			break;
		default: throw(UGError("ReferenceElement not found."));
	}
}

template <>
void ComputeMultiIndicesOfSubElement<2>(std::vector<MathVector<2, int> >* vvMultiIndex,
                                        bool* vIsBndElem,
                                        std::vector<int>* vElemBndSide,
                                        std::vector<size_t>* vIndex,
                                        ReferenceObjectID roid,
                                        int p)
{
//	switch for roid
	size_t se = 0;
	switch(roid)
	{
		case ROID_TRIANGLE:
			for(int j = 0; j < p; ++j) { // y -direction
				for(int i = 0; i < p - j; ++i) { // x - direction
					vvMultiIndex[se].resize(3);
					vvMultiIndex[se][0] = MathVector<2,int>(i  , j  );
					vvMultiIndex[se][1] = MathVector<2,int>(i+1, j  );
					vvMultiIndex[se][2] = MathVector<2,int>(i  , j+1);

					// reset bnd info
					vIsBndElem[se] = false;
					vElemBndSide[se].clear(); vElemBndSide[se].resize(3, -1);

					if(i==0) // left
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][2] = 2;
					}
					if(j==0) // bottom
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][0] = 0;
					}
					if(i+j==p-1) // diag
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][1] = 1;
					}
					++se;
				}
			}

			for(int j = 1; j <= p; ++j) {
				for(int i = 1; i <= p - j; ++i) {
					vvMultiIndex[se].resize(3);
					vvMultiIndex[se][0] = MathVector<2,int>(i  , j  );
					vvMultiIndex[se][1] = MathVector<2,int>(i-1, j  );
					vvMultiIndex[se][2] = MathVector<2,int>(i  , j-1);

					// reset bnd info
					// all inner elems
					vIsBndElem[se] = false;
					vElemBndSide[se].clear(); vElemBndSide[se].resize(3, -1);
					++se;
				}
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferenceTriangle> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}

			break;
		case ROID_QUADRILATERAL:
			for(int j = 0; j < p; ++j) {
				for(int i = 0; i < p; ++i) {
					vvMultiIndex[se].resize(4);
					vvMultiIndex[se][0] = MathVector<2,int>(i  , j  );
					vvMultiIndex[se][1] = MathVector<2,int>(i+1, j  );
					vvMultiIndex[se][2] = MathVector<2,int>(i+1, j+1);
					vvMultiIndex[se][3] = MathVector<2,int>(i  , j+1);

					// reset bnd info
					vIsBndElem[se] = false;
					vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

					if(i==0) // left
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][3] = 3;
					}
					if(i==p-1) // right
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][1] = 1;
					}
					if(j==0) // bottom
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][0] = 0;
					}
					if(j==p-1) // top
					{
						vIsBndElem[se] = true;
						vElemBndSide[se][2] = 2;
					}
					++se;
				}
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferenceQuadrilateral> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}
			break;
		default: throw(UGError("ReferenceElement not found."));
	}

}

template <>
void ComputeMultiIndicesOfSubElement<3>(std::vector<MathVector<3, int> >* vvMultiIndex,
                                        bool* vIsBndElem,
                                        std::vector<int>* vElemBndSide,
                                        std::vector<size_t>* vIndex,
                                        ReferenceObjectID roid,
                                        int p)
{
//	switch for roid
	size_t se = 0;
	switch(roid)
	{
		case ROID_TETRAHEDRON:
			for(int k = 0; k < p; ++k) {
				for(int j = 0; j < p -k; ++j) {
					for(int i = 0; i < p -k -j; ++i) {
						vvMultiIndex[se].resize(4);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j  , k);
						vvMultiIndex[se][2] = MathVector<3,int>(i  , j+1, k);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j  , k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

						if(i==0) // left
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][2] = 2;
						}
						if(j==0) // front
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][3] = 3;
						}
						if(k==0) // bottom
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][0] = 0;
						}
						if(i+j+k==p-1) // diag
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][1] = 1;
						}
						++se;
					}
				}
			}
			//	build 4 tetrahedrons out of the remaining octogons
			for(int k = 0; k < p; ++k) {
				for(int j = 1; j < p -k; ++j) {
					for(int i = 0; i < p -k -j; ++i) {
						vvMultiIndex[se].resize(4);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i  , j-1, k+1);
						vvMultiIndex[se][2] = MathVector<3,int>(i  , j  , k+1);
						vvMultiIndex[se][3] = MathVector<3,int>(i+1, j-1, k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

						if(i==0) // left
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][0] = 2;
						}
						++se;

						vvMultiIndex[se].resize(4);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j-1, k+1);
						vvMultiIndex[se][2] = MathVector<3,int>(i+1, j  , k);
						vvMultiIndex[se][3] = MathVector<3,int>(i+1, j-1, k);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

						if(k==0) // bottom
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][2] = 0;
						}
						++se;

						vvMultiIndex[se].resize(4);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j-1, k);
						vvMultiIndex[se][2] = MathVector<3,int>(i+1, j-1, k+1);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j-1, k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

						if(j==1) // front
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][1] = 3;
						}
						++se;

						vvMultiIndex[se].resize(4);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j  , k);
						vvMultiIndex[se][2] = MathVector<3,int>(i+1, j-1, k+1);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j  , k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(4, -1);

						if(i+j+k==p-1) // diag
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][1] = 1;
						}
						++se;
					}
				}
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferenceTetrahedron> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}
			break;

		case ROID_PRISM:
			for(int k = 0; k < p; ++k) {
				for(int j = 0; j < p; ++j) {
					for(int i = 0; i < p - j; ++i) {
						vvMultiIndex[se].resize(6);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j  , k);
						vvMultiIndex[se][2] = MathVector<3,int>(i  , j+1, k);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j  , k+1);
						vvMultiIndex[se][4] = MathVector<3,int>(i+1, j  , k+1);
						vvMultiIndex[se][5] = MathVector<3,int>(i  , j+1, k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(5, -1);

						if(i==0) // left
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][3] = 3;
						}
						if(j==0) // front
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][1] = 1;
						}
						if(i+j==p-1) // diag
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][2] = 2;
						}
						if(k==0) // bottom
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][0] = 0;
						}
						if(k==p-1) // top
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][4] = 4;
						}
						++se;
					}
				}
			}
			for(int k = 0; k < p; ++k) {
				for(int j = 1; j <= p; ++j) {
					for(int i = 1; i <= p - j; ++i) {
						vvMultiIndex[se].resize(6);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  ,k);
						vvMultiIndex[se][1] = MathVector<3,int>(i-1, j  ,k);
						vvMultiIndex[se][2] = MathVector<3,int>(i  , j-1,k);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j  ,k+1);
						vvMultiIndex[se][4] = MathVector<3,int>(i-1, j  ,k+1);
						vvMultiIndex[se][5] = MathVector<3,int>(i  , j-1,k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(5, -1);

						if(k==0) // bottom
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][0] = 0;
						}
						if(k==p-1) // top
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][4] = 4;
						}
						++se;
					}
				}
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferencePrism> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}

			break;
		case ROID_HEXAHEDRON:
			for(int k = 0; k < p; ++k) {
				for(int j = 0; j < p; ++j) {
					for(int i = 0; i < p; ++i) {
						vvMultiIndex[se].resize(8);
						vvMultiIndex[se][0] = MathVector<3,int>(i  , j  , k);
						vvMultiIndex[se][1] = MathVector<3,int>(i+1, j  , k);
						vvMultiIndex[se][2] = MathVector<3,int>(i+1, j+1, k);
						vvMultiIndex[se][3] = MathVector<3,int>(i  , j+1, k);
						vvMultiIndex[se][4] = MathVector<3,int>(i  , j  , k+1);
						vvMultiIndex[se][5] = MathVector<3,int>(i+1, j  , k+1);
						vvMultiIndex[se][6] = MathVector<3,int>(i+1, j+1, k+1);
						vvMultiIndex[se][7] = MathVector<3,int>(i  , j+1, k+1);

						// reset bnd info
						vIsBndElem[se] = false;
						vElemBndSide[se].clear(); vElemBndSide[se].resize(6, -1);

						if(i==0) // left
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][4] = 4;
						}
						if(i==p-1) //right
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][2] = 2;
						}
						if(j==0) // front
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][1] = 1;
						}
						if(j==p-1) // back
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][3] = 3;
						}
						if(k==0) // bottom
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][0] = 0;
						}
						 if(k==p-1) // top
						{
							vIsBndElem[se] = true;
							vElemBndSide[se][5] = 5;
						}
						++se;
					}
				}
			}
			UG_ASSERT(se == NumSubElements(roid, p), "Wrong number of se="<<se<<
			          " computed, but must be "<<NumSubElements(roid, p));

			{
				FlexLagrangeLSFS<ReferenceHexahedron> set(p);
				for(size_t s = 0; s < se; ++s)
				{
					vIndex[s].resize(vvMultiIndex[s].size());
					for(size_t i = 0; i < vvMultiIndex[s].size(); ++i)
					{
						vIndex[s][i] = set.index(vvMultiIndex[s][i]);
					}
				}
			}

			break;
		default: throw(UGError("ReferenceElement not found."));
	}

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// FV1 Geometry for Reference Element Type
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <typename TElem, int TWorldDim>
FV1Geometry<TElem, TWorldDim>::
FV1Geometry()
	: m_pElem(NULL), m_rRefElem(Provider<ref_elem_type>::get()),
	  m_rTrialSpace(Provider<local_shape_fct_set_type>::get())
{
	update_local_data();
}

template <typename TElem, int TWorldDim>
bool FV1Geometry<TElem, TWorldDim>::
update_local_data()
{
// 	set corners of element as local centers of nodes
	for(size_t i = 0; i < m_rRefElem.num(0); ++i)
		m_vvLocMid[0][i] = m_rRefElem.corner(i);

//	compute local midpoints
	ComputeMidPoints<dim, ref_elem_type, maxMid>(m_rRefElem, m_vvLocMid[0], m_vvLocMid);

// 	set up local informations for SubControlVolumeFaces (scvf)
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	this scvf separates the given nodes
		m_vSCVF[i].From = m_rRefElem.id(1, i, 0, 0);
		m_vSCVF[i].To = m_rRefElem.id(1, i, 0, 1);

	//	compute mid ids of the scvf
		ComputeSCVFMidID(m_rRefElem, m_vSCVF[i].vMidID, i);

	// 	copy local corners of scvf
		CopyCornerByMidID<dim, maxMid>(m_vSCVF[i].vLocPos, m_vSCVF[i].vMidID, m_vvLocMid, SCVF::numCo);

	// 	integration point
		AveragePositions(m_vSCVF[i].localIP, m_vSCVF[i].vLocPos, SCVF::numCo);
	}

// 	set up local informations for SubControlVolumes (scv)
// 	each scv is associated to one corner of the element
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	store associated node
		m_vSCV[i].nodeId = i;

	//	compute mid ids scv
		ComputeSCVMidID(m_rRefElem, m_vSCV[i].midId, i);

	// 	copy local corners of scv
		CopyCornerByMidID<dim, maxMid>(m_vSCV[i].vLocPos, m_vSCV[i].midId, m_vvLocMid, m_vSCV[i].num_corners());
	}


// 	compute Shapes and Derivatives
	for(size_t i = 0; i < num_scvf(); ++i)
	{
		m_rTrialSpace.shapes(&(m_vSCVF[i].vShape[0]), m_vSCVF[i].local_ip());
		m_rTrialSpace.grads(&(m_vSCVF[i].vLocalGrad[0]), m_vSCVF[i].local_ip());
	}

	for(size_t i = 0; i < num_scv(); ++i)
	{
		m_rTrialSpace.shapes(&(m_vSCV[i].vShape[0]), m_vSCV[i].local_ip());
		m_rTrialSpace.grads(&(m_vSCV[i].vLocalGrad[0]), m_vSCV[i].local_ip());
	}

// 	copy ip positions in a list for Sub Control Volumes Faces (SCVF)
	for(size_t i = 0; i < num_scvf(); ++i)
		m_vLocSCVF_IP[i] = scvf(i).local_ip();

	return true;
}

/// update data for given element
template <typename TElem, int TWorldDim>
bool
FV1Geometry<TElem, TWorldDim>::
update(TElem* elem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
// 	If already update for this element, do nothing
	if(m_pElem == elem) return true; else m_pElem = elem;

// 	remember global position of nodes
	for(size_t i = 0; i < m_rRefElem.num(0); ++i)
		m_vvGloMid[0][i] = vCornerCoords[i];

//	compute global midpoints
	ComputeMidPoints<worldDim, ref_elem_type, maxMid>(m_rRefElem, m_vvGloMid[0], m_vvGloMid);

// 	compute global informations for scvf
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	// 	copy local corners of scvf
		CopyCornerByMidID<worldDim, maxMid>(m_vSCVF[i].vGloPos, m_vSCVF[i].vMidID, m_vvGloMid, SCVF::numCo);

	// 	integration point
		AveragePositions(m_vSCVF[i].globalIP, m_vSCVF[i].vGloPos, SCVF::numCo);

	// 	normal on scvf
		traits::NormalOnSCVF(m_vSCVF[i].Normal, m_vSCVF[i].vGloPos, m_vvGloMid[0]);
	}

// 	compute size of scv
	for(size_t i = 0; i < num_scv(); ++i)
	{
	// 	copy global corners
		CopyCornerByMidID<worldDim, maxMid>(m_vSCV[i].vGloPos, m_vSCV[i].midId, m_vvGloMid, m_vSCV[i].num_corners());

	// 	compute volume of scv
		if(m_vSCV[i].numCo != 10)
			m_vSCV[i].Vol = ElementSize<scv_type, worldDim>(m_vSCV[i].vGloPos);
	//	special case for pyramid, last scv
		else throw(UGError("Pyramid Not Implemented"));
	}

// 	Shapes and Derivatives
	m_mapping.update(vCornerCoords);

//	if mapping is linear, compute jacobian only once and copy
	if(ReferenceMapping<ref_elem_type, worldDim>::isLinear)
	{
		MathMatrix<worldDim,dim> JtInv;
		m_mapping.jacobian_transposed_inverse(JtInv, m_vSCVF[0].local_ip());
		const number detJ = m_mapping.jacobian_det(m_vSCVF[0].local_ip());

		for(size_t i = 0; i < num_scvf(); ++i)
		{
			m_vSCVF[i].JtInv = JtInv;
			m_vSCVF[i].detj = detJ;
		}

		for(size_t i = 0; i < num_scv(); ++i)
		{
			m_vSCV[i].JtInv = JtInv;
			m_vSCV[i].detj = detJ;
		}
	}
//	else compute jacobian for each integration point
	else
	{
		for(size_t i = 0; i < num_scvf(); ++i)
		{
			m_mapping.jacobian_transposed_inverse(m_vSCVF[i].JtInv, m_vSCVF[i].local_ip());
			m_vSCVF[i].detj = m_mapping.jacobian_det(m_vSCVF[i].local_ip());
		}
		for(size_t i = 0; i < num_scv(); ++i)
		{
			m_mapping.jacobian_transposed_inverse(m_vSCV[i].JtInv, m_vSCV[i].local_ip());
			m_vSCV[i].detj = m_mapping.jacobian_det(m_vSCV[i].local_ip());
		}
	}

//	compute global gradients
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t sh = 0 ; sh < num_scv(); ++sh)
			MatVecMult(m_vSCVF[i].vGlobalGrad[sh], m_vSCVF[i].JtInv, m_vSCVF[i].vLocalGrad[sh]);

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t sh = 0 ; sh < num_scv(); ++sh)
			MatVecMult(m_vSCV[i].vGlobalGrad[sh], m_vSCV[i].JtInv, m_vSCV[i].vLocalGrad[sh]);

// 	Copy ip pos in list for SCVF
	for(size_t i = 0; i < num_scvf(); ++i)
		m_vGlobSCVF_IP[i] = scvf(i).global_ip();

//	if no boundary subsets required, return
	if(num_boundary_subsets() == 0 || ish == NULL) return true;
	else return update_boundary_faces(elem, vCornerCoords, ish);
}

template <typename TElem, int TWorldDim>
bool
FV1Geometry<TElem, TWorldDim>::
update_boundary_faces(TElem* elem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
//	get grid
	Grid& grid = *(ish->grid());

//	vector of subset indices of side
	std::vector<int> vSubsetIndex;

//	get subset indices for sides (i.e. edge in 2d, faces in 3d)
	if(dim == 1) {
		std::vector<VertexBase*> vVertex;
		CollectVertices(vVertex, grid, elem);
		vSubsetIndex.resize(vVertex.size());
		for(size_t i = 0; i < vVertex.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vVertex[i]);
	}
	if(dim == 2) {
		std::vector<EdgeBase*> vEdges;
		CollectEdgesSorted(vEdges, grid, elem);
		vSubsetIndex.resize(vEdges.size());
		for(size_t i = 0; i < vEdges.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vEdges[i]);
	}
	if(dim == 3) {
		std::vector<Face*> vFaces;
		CollectFacesSorted(vFaces, grid, elem);
		vSubsetIndex.resize(vFaces.size());
		for(size_t i = 0; i < vFaces.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vFaces[i]);
	}

//	loop requested subset
	typename std::map<int, std::vector<BF> >::iterator it;
	for (it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); ++it)
	{
	//	get subset index
		const int bndIndex = (*it).first;

	//	get vector of BF for element
		std::vector<BF>& vBF = (*it).second;

	//	clear vector
		vBF.clear();

	//	current number of bf
		size_t curr_bf = 0;

	//	loop sides of element
		for(size_t side = 0; side < vSubsetIndex.size(); ++side)
		{
		//	skip non boundary sides
			if(vSubsetIndex[side] != bndIndex) continue;

		//	number of corners of side
			const int coOfSide = m_rRefElem.num(dim-1, side, 0);

		//	resize vector
			vBF.resize(curr_bf + coOfSide);

		//	loop corners
			for(int co = 0; co < coOfSide; ++co)
			{
			//	get current bf
				BF& bf = vBF[curr_bf];

			//	set node id == scv this bf belongs to
				bf.nodeId = m_rRefElem.id(dim-1, side, 0, co);

			//	Compute MidID for BF
				ComputeBFMidID(m_rRefElem, side, bf.vMidID, co);

			// 	copy corners of bf
				CopyCornerByMidID<dim, maxMid>(bf.vLocPos, bf.vMidID, m_vvLocMid, BF::numCo);
				CopyCornerByMidID<worldDim, maxMid>(bf.vGloPos, bf.vMidID, m_vvGloMid, BF::numCo);

			// 	integration point
				AveragePositions(bf.localIP, bf.vLocPos, BF::numCo);
				AveragePositions(bf.globalIP, bf.vGloPos, BF::numCo);

			// 	normal on scvf
				traits::NormalOnSCVF(bf.Normal, bf.vGloPos, m_vvGloMid[0]);

			//	compute volume
				bf.Vol = VecTwoNorm(bf.Normal);

				m_rTrialSpace.shapes(&(bf.vShape[0]), bf.localIP);
				m_rTrialSpace.grads(&(bf.vLocalGrad[0]), bf.localIP);

				m_mapping.jacobian_transposed_inverse(bf.JtInv, bf.localIP);
				bf.detj = m_mapping.jacobian_det(bf.localIP);

				for(size_t sh = 0 ; sh < num_scv(); ++sh)
					MatVecMult(bf.vGlobalGrad[sh], bf.JtInv, bf.vLocalGrad[sh]);

			//	increase curr_bf
				++curr_bf;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Dim-dependent Finite Volume Geometry
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <int TDim, int TWorldDim>
bool
DimFV1Geometry<TDim, TWorldDim>::
update_local_data()
{
//	get reference element
	try{
	const DimReferenceElement<dim>& rRefElem
		= ReferenceElementProvider::get<dim>(m_roid);

// 	set corners of element as local centers of nodes
	for(size_t i = 0; i < rRefElem.num(0); ++i)
		m_vvLocMid[0][i] = rRefElem.corner(i);

//	compute local midpoints
	ComputeMidPoints<dim, DimReferenceElement<dim>, maxMid>
										(rRefElem, m_vvLocMid[0], m_vvLocMid);

//	set number of scvf / scv of this roid
	m_numSCV = rRefElem.num(0); // number of corners
	m_numSCVF = rRefElem.num(1); // number of edges

// 	set up local informations for SubControlVolumeFaces (scvf)
// 	each scvf is associated to one edge of the element
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	this scvf separates the given nodes
		m_vSCVF[i].From = rRefElem.id(1, i, 0, 0);
		m_vSCVF[i].To = rRefElem.id(1, i, 0, 1);

	//	compute mid ids of the scvf
		ComputeSCVFMidID(rRefElem, m_vSCVF[i].vMidID, i);

	// 	copy local corners of scvf
		CopyCornerByMidID<dim, maxMid>(m_vSCVF[i].vLocPos, m_vSCVF[i].vMidID, m_vvLocMid, SCVF::numCo);

	// 	integration point
		AveragePositions(m_vSCVF[i].localIP, m_vSCVF[i].vLocPos, SCVF::numCo);
	}

// 	set up local informations for SubControlVolumes (scv)
// 	each scv is associated to one corner of the element
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	store associated node
		m_vSCV[i].nodeId = i;

	//	compute mid ids scv
		ComputeSCVMidID(rRefElem, m_vSCV[i].vMidID, i);

	// 	copy local corners of scv
		CopyCornerByMidID<dim, maxMid>(m_vSCV[i].vLocPos, m_vSCV[i].vMidID, m_vvLocMid, m_vSCV[i].num_corners());
	}

	/////////////////////////
	// Shapes and Derivatives
	/////////////////////////

	try{
	const DimLocalShapeFunctionSet<dim>& TrialSpace =
		LocalShapeFunctionSetProvider::get<dim>(m_roid, LFEID(LFEID::LAGRANGE, 1));

	for(size_t i = 0; i < num_scvf(); ++i)
	{
		m_vSCVF[i].numSH = TrialSpace.num_sh();
		TrialSpace.shapes(&(m_vSCVF[i].vShape[0]), m_vSCVF[i].localIP);
		TrialSpace.grads(&(m_vSCVF[i].vLocalGrad[0]), m_vSCVF[i].localIP);
	}

	for(size_t i = 0; i < num_scv(); ++i)
	{
		m_vSCV[i].numSH = TrialSpace.num_sh();
		TrialSpace.shapes(&(m_vSCV[i].vShape[0]), m_vSCV[i].vLocPos[0]);
		TrialSpace.grads(&(m_vSCV[i].vLocalGrad[0]), m_vSCV[i].vLocPos[0]);
	}

	}catch(UG_ERROR_LocalShapeFunctionSetNotRegistered& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

	}catch(UG_ERROR_ReferenceElementMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

// 	copy ip positions in a list for Sub Control Volumes Faces (SCVF)
	for(size_t i = 0; i < num_scvf(); ++i)
		m_vLocSCVF_IP[i] = scvf(i).local_ip();

	return true;
}


/// update data for given element
template <int TDim, int TWorldDim>
bool
DimFV1Geometry<TDim, TWorldDim>::
update(GeometricObject* pElem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
// 	If already update for this element, do nothing
	if(m_pElem == pElem) return true; else m_pElem = pElem;

//	refresh local data, if different roid given
	if(m_roid != pElem->reference_object_id())
	{
	//	remember new roid
		m_roid = (ReferenceObjectID) pElem->reference_object_id();

	//	update local data
		if(!update_local_data()) return false;
	}

//	get reference element
	try{
	const DimReferenceElement<dim>& rRefElem
		= ReferenceElementProvider::get<dim>(m_roid);

// 	remember global position of nodes
	for(size_t i = 0; i < rRefElem.num(0); ++i)
		m_vvGloMid[0][i] = vCornerCoords[i];

//	compute local midpoints
	ComputeMidPoints<worldDim, DimReferenceElement<dim>, maxMid>(rRefElem, m_vvGloMid[0], m_vvGloMid);

// 	compute global informations for scvf
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	// 	copy local corners of scvf
		CopyCornerByMidID<worldDim, maxMid>(m_vSCVF[i].vGloPos, m_vSCVF[i].vMidID, m_vvGloMid, SCVF::numCo);

	// 	integration point
		AveragePositions(m_vSCVF[i].globalIP, m_vSCVF[i].vGloPos, SCVF::numCo);

	// 	normal on scvf
		traits::NormalOnSCVF(m_vSCVF[i].Normal, m_vSCVF[i].vGloPos, m_vvGloMid[0]);
	}

// 	compute size of scv
	for(size_t i = 0; i < num_scv(); ++i)
	{
	// 	copy global corners
		CopyCornerByMidID<worldDim, maxMid>(m_vSCV[i].vGloPos, m_vSCV[i].vMidID, m_vvGloMid, m_vSCV[i].num_corners());

	// 	compute volume of scv
		if(m_vSCV[i].numCorners != 10)
			m_vSCV[i].Vol = ElementSize<scv_type, worldDim>(m_vSCV[i].vGloPos);
	// 	special case for pyramid, last scv
		else throw(UGError("Pyramid Not Implemented"));
	}

//	get reference mapping
	try{
	DimReferenceMapping<dim, worldDim>& rMapping = ReferenceMappingProvider::get<dim, worldDim>(m_roid);
	rMapping.update(vCornerCoords);

	//\todo compute with on virt. call
//	compute jacobian for linear mapping
	if(rMapping.is_linear())
	{
		MathMatrix<worldDim,dim> JtInv;
		rMapping.jacobian_transposed_inverse(JtInv, m_vSCVF[0].local_ip());
		const number detJ = rMapping.jacobian_det(m_vSCVF[0].local_ip());

		for(size_t i = 0; i < num_scvf(); ++i)
		{
			m_vSCVF[i].JtInv = JtInv;
			m_vSCVF[i].detj = detJ;
		}

		for(size_t i = 0; i < num_scv(); ++i)
		{
			m_vSCV[i].JtInv = JtInv;
			m_vSCV[i].detj = detJ;
		}
	}
//	else compute jacobian for each integration point
	else
	{
		for(size_t i = 0; i < num_scvf(); ++i)
		{
			rMapping.jacobian_transposed_inverse(m_vSCVF[i].JtInv, m_vSCVF[i].local_ip());
			m_vSCVF[i].detj = rMapping.jacobian_det(m_vSCVF[i].local_ip());
		}
		for(size_t i = 0; i < num_scv(); ++i)
		{
			rMapping.jacobian_transposed_inverse(m_vSCV[i].JtInv, m_vSCV[i].local_ip());
			m_vSCV[i].detj = rMapping.jacobian_det(m_vSCV[i].local_ip());
		}
	}

//	compute global gradients
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t sh = 0; sh < num_scv(); ++sh)
			MatVecMult(m_vSCVF[i].vGlobalGrad[sh], m_vSCVF[i].JtInv, m_vSCVF[i].vLocalGrad[sh]);

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t sh = 0; sh < num_scv(); ++sh)
			MatVecMult(m_vSCV[i].vGlobalGrad[sh], m_vSCV[i].JtInv, m_vSCV[i].vLocalGrad[sh]);

// 	copy ip points in list (SCVF)
	for(size_t i = 0; i < num_scvf(); ++i)
		m_vGlobSCVF_IP[i] = scvf(i).global_ip();

	}catch(UG_ERROR_ReferenceElementMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}
	}catch(UG_ERROR_ReferenceMappingMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

//	if no boundary subsets required, return
	if(num_boundary_subsets() == 0 || ish == NULL) return true;
	else return update_boundary_faces(pElem, vCornerCoords, ish);
}

template <int TDim, int TWorldDim>
bool
DimFV1Geometry<TDim, TWorldDim>::
update_boundary_faces(GeometricObject* pElem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
//	get grid
	Grid& grid = *(ish->grid());

//	vector of subset indices of side
	std::vector<int> vSubsetIndex;

//	get subset indices for sides (i.e. edge in 2d, faces in 3d)
	if(dim == 1) {
		std::vector<VertexBase*> vVertex;
		CollectVertices(vVertex, grid, pElem);
		vSubsetIndex.resize(vVertex.size());
		for(size_t i = 0; i < vVertex.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vVertex[i]);
	}
	if(dim == 2) {
		std::vector<EdgeBase*> vEdges;
		CollectEdgesSorted(vEdges, grid, pElem);
		vSubsetIndex.resize(vEdges.size());
		for(size_t i = 0; i < vEdges.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vEdges[i]);
	}
	if(dim == 3) {
		std::vector<Face*> vFaces;
		CollectFacesSorted(vFaces, grid, pElem);
		vSubsetIndex.resize(vFaces.size());
		for(size_t i = 0; i < vFaces.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vFaces[i]);
	}

	try{
	const DimReferenceElement<dim>& rRefElem
		= ReferenceElementProvider::get<dim>(m_roid);

	try{
	DimReferenceMapping<dim, worldDim>& rMapping = ReferenceMappingProvider::get<dim, worldDim>(m_roid);
	rMapping.update(vCornerCoords);

	try{
	const DimLocalShapeFunctionSet<dim>& TrialSpace =
		LocalShapeFunctionSetProvider::get<dim>(m_roid, LFEID(LFEID::LAGRANGE, 1));

//	loop requested subset
	typename std::map<int, std::vector<BF> >::iterator it;
	for (it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); ++it)
	{
	//	get subset index
		const int bndIndex = (*it).first;

	//	get vector of BF for element
		std::vector<BF>& vBF = (*it).second;

	//	clear vector
		vBF.clear();

	//	current number of bf
		size_t curr_bf = 0;

	//	loop sides of element
		for(size_t side = 0; side < vSubsetIndex.size(); ++side)
		{
		//	skip non boundary sides
			if(vSubsetIndex[side] != bndIndex) continue;

		//	number of corners of side
			const int coOfSide = rRefElem.num(dim-1, side, 0);

		//	resize vector
			vBF.resize(curr_bf + coOfSide);

		//	loop corners
			for(int co = 0; co < coOfSide; ++co)
			{
			//	get current bf
				BF& bf = vBF[curr_bf];

			//	set node id == scv this bf belongs to
				bf.nodeId = rRefElem.id(dim-1, side, 0, co);

			//	Compute MidID for BF
				ComputeBFMidID(rRefElem, side, bf.vMidID, co);

			// 	copy corners of bf
				CopyCornerByMidID<dim, maxMid>(bf.vLocPos, bf.vMidID, m_vvLocMid, BF::numCo);
				CopyCornerByMidID<worldDim, maxMid>(bf.vGloPos, bf.vMidID, m_vvGloMid, BF::numCo);

			// 	integration point
				AveragePositions(bf.localIP, bf.vLocPos, BF::numCo);
				AveragePositions(bf.globalIP, bf.vGloPos, BF::numCo);

			// 	normal on scvf
				traits::NormalOnSCVF(bf.Normal, bf.vGloPos, m_vvGloMid[0]);

			//	compute volume
				bf.Vol = VecTwoNorm(bf.Normal);

			//	compute shapes and grads
				bf.numSH = TrialSpace.num_sh();
				TrialSpace.shapes(&(bf.vShape[0]), bf.localIP);
				TrialSpace.grads(&(bf.vLocalGrad[0]), bf.localIP);

			//	get reference mapping
				rMapping.jacobian_transposed_inverse(bf.JtInv, bf.localIP);
				bf.detj = rMapping.jacobian_det(bf.localIP);

			//	compute global gradients
				for(size_t sh = 0 ; sh < num_scv(); ++sh)
					MatVecMult(bf.vGlobalGrad[sh], bf.JtInv, bf.vLocalGrad[sh]);

			//	increase curr_bf
				++curr_bf;
			}
		}
	}

	}catch(UG_ERROR_ReferenceElementMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}
	}catch(UG_ERROR_ReferenceMappingMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}
	}catch(UG_ERROR_LocalShapeFunctionSetNotRegistered& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

//	done
	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// FV Geometry for Reference Element Type (all order, FVHO)
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <int TOrder, typename TElem, int TWorldDim, int TQuadOrderSCVF, int TQuadOrderSCV>
FVGeometry<TOrder, TElem, TWorldDim, TQuadOrderSCVF, TQuadOrderSCV>::
FVGeometry()
	: m_pElem(NULL), m_rRefElem(Provider<ref_elem_type>::get()),
	  m_rTrialSpace(Provider<local_shape_fct_set_type>::get()),
	  m_rSCVFQuadRule(Provider<scvf_quad_rule_type>::get()),
	  m_rSCVQuadRule(Provider<scv_quad_rule_type>::get())
{
	update_local_data();
}

template <int TOrder, typename TElem, int TWorldDim, int TQuadOrderSCVF, int TQuadOrderSCV>
bool FVGeometry<TOrder, TElem, TWorldDim, TQuadOrderSCVF, TQuadOrderSCV>::
update_local(ReferenceObjectID roid, int orderShape,
                  int quadOrderSCVF, int quadOrderSCV)
{
	if(roid != geometry_traits<TElem>::REFERENCE_OBJECT_ID)
	{
		UG_LOG("ERROR in 'FVGeometry::update': Geometry only for "
				<<geometry_traits<TElem>::REFERENCE_OBJECT_ID<<", but "
				<<roid<<" requested.\n");
		return false;
	}
	if(orderShape != TOrder)
	{
		UG_LOG("ERROR in 'FVGeometry::update': Geometry only for shape order"
				<<TOrder<<", but "<<orderShape<<" requested.\n");
		return false;
	}
	if(quadOrderSCVF > TQuadOrderSCVF)
	{
		UG_LOG("ERROR in 'FVGeometry::update': Geometry only for scvf integration order "
				<< TQuadOrderSCVF<<", but order "<<quadOrderSCVF<<" requested.\n");
		return false;
	}
	if(quadOrderSCV > TQuadOrderSCV)
	{
		UG_LOG("ERROR in 'FVGeometry::update': Geometry only for scv integration order "
				<< TQuadOrderSCV<<", but order "<<quadOrderSCV<<" requested.\n");
		return false;
	}

	return true;
}


template <int TOrder, typename TElem, int TWorldDim, int TQuadOrderSCVF, int TQuadOrderSCV>
bool FVGeometry<TOrder, TElem, TWorldDim, TQuadOrderSCVF, TQuadOrderSCV>::
update_local_data()
{
//	get reference object id
	ReferenceObjectID roid = ref_elem_type::REFERENCE_OBJECT_ID;

//	determine corners of sub elements
	bool vIsBndElem[numSubElem];
	std::vector<int> vElemBndSide[numSubElem];
	std::vector<size_t> vIndex[numSubElem];
	std::vector<MathVector<dim,int> > vMultiIndex[numSubElem];

	ComputeMultiIndicesOfSubElement<dim>(vMultiIndex, vIsBndElem,
										 vElemBndSide, vIndex, roid, p);

//	directions of counting
	MathVector<dim> direction[dim];
	for(int i = 0; i < dim; ++i){direction[i] = 0.0; direction[i][i] = 1.0;}

	for(size_t se = 0; se < numSubElem; ++se)
	{
		for(int co = 0; co < ref_elem_type::num_corners; ++co)
		{
		//	compute corners of sub elem in local coordinates
			MathVector<dim> pos; pos = 0.0;
			for(int i = 0; i < dim; ++i)
			{
				const number frac = vMultiIndex[se][co][i] / ((number)p);
				VecScaleAppend(pos, frac, direction[i]);
			}
			m_vSubElem[se].vvLocMid[0][co] = pos;

		//	get multi index for corner
			m_vSubElem[se].vDoFID[co] = vIndex[se][co];
		}

	//	remember if boundary element
		m_vSubElem[se].isBndElem = vIsBndElem[se];

	//	remember boundary sides
		m_vSubElem[se].vElemBndSide = vElemBndSide[se];
	}

//	compute mid points for all sub elements
	for(size_t se = 0; se < numSubElem; ++se)
		ComputeMidPoints<dim, ref_elem_type, maxMid>
				(m_rRefElem, m_vSubElem[se].vvLocMid[0], m_vSubElem[se].vvLocMid);

// 	set up local informations for SubControlVolumeFaces (scvf)
// 	each scvf is associated to one edge of the sub-element
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	get corresponding subelement
		const size_t se = i / numSCVFPerSubElem;
		const size_t locSCVF = i % numSCVFPerSubElem;

	//	this scvf separates the given nodes
		const size_t locFrom =  m_rRefElem.id(1, locSCVF, 0, 0);
		const size_t locTo =  m_rRefElem.id(1, locSCVF, 0, 1);

		m_vSCVF[i].From = m_vSubElem[se].vDoFID[locFrom];
		m_vSCVF[i].To = m_vSubElem[se].vDoFID[locTo];

	//	compute mid ids of the scvf
		ComputeSCVFMidID(m_rRefElem, m_vSCVF[i].vMidID, locSCVF);

	// 	copy local corners of scvf
		CopyCornerByMidID<dim, maxMid>
			(m_vSCVF[i].vLocPos, m_vSCVF[i].vMidID, m_vSubElem[se].vvLocMid, SCVF::numCo);

	// 	compute integration points
		m_vSCVF[i].vWeight = m_rSCVFQuadRule.weights();
		ReferenceMapping<scvf_type, dim> map(m_vSCVF[i].vLocPos);
		for(size_t ip = 0; ip < m_rSCVFQuadRule.size(); ++ip)
			map.local_to_global(m_vSCVF[i].vLocalIP[ip], m_rSCVFQuadRule.point(ip));
	}


// 	set up local informations for SubControlVolumes (scv)
// 	each scv is associated to one corner of the sub-element
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	get corresponding subelement
		const size_t se = i / numSCVPerSubElem;
		const size_t locSCV = i % numSCVPerSubElem;

	//	store associated node
		m_vSCV[i].nodeId = m_vSubElem[se].vDoFID[locSCV];;

	//	compute mid ids scv
		ComputeSCVMidID(m_rRefElem, m_vSCV[i].vMidID, locSCV);

	// 	copy local corners of scv
		CopyCornerByMidID<dim, maxMid>
			(m_vSCV[i].vLocPos, m_vSCV[i].vMidID, m_vSubElem[se].vvLocMid, m_vSCV[i].num_corners());

	// 	compute integration points
		m_vSCV[i].vWeight = m_rSCVQuadRule.weights();
		ReferenceMapping<scv_type, dim> map(m_vSCV[i].vLocPos);
		for(size_t ip = 0; ip < m_rSCVQuadRule.size(); ++ip)
			map.local_to_global(m_vSCV[i].vLocalIP[ip], m_rSCVQuadRule.point(ip));
	}

	/////////////////////////
	// Shapes and Derivatives
	/////////////////////////

	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
		{
			m_rTrialSpace.shapes(&(m_vSCVF[i].vvShape[ip][0]), m_vSCVF[i].local_ip(ip));
			m_rTrialSpace.grads(&(m_vSCVF[i].vvLocalGrad[ip][0]), m_vSCVF[i].local_ip(ip));
		}

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
		{
			m_rTrialSpace.shapes(&(m_vSCV[i].vvShape[ip][0]), m_vSCV[i].local_ip(ip));
			m_rTrialSpace.grads(&(m_vSCV[i].vvLocalGrad[ip][0]), m_vSCV[i].local_ip(ip));
		}

// 	copy ip positions in a list for Sub Control Volumes Faces (SCVF)
	size_t allIP = 0;
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
			m_vLocSCVF_IP[allIP++] = scvf(i).local_ip(ip);

// 	copy ip positions in a list for Sub Control Volumes (SCV)
	allIP = 0;
	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
			m_vLocSCV_IP[allIP++] = scv(i).local_ip(ip);

	return true;
}


/// update data for given element
template <int TOrder, typename TElem, int TWorldDim, int TQuadOrderSCVF, int TQuadOrderSCV>
bool
FVGeometry<TOrder, TElem, TWorldDim, TQuadOrderSCVF, TQuadOrderSCV>::
update(TElem* pElem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
// 	If already update for this element, do nothing
	if(m_pElem == pElem) return true; else m_pElem = pElem;

//	update reference mapping
	m_rMapping.update(vCornerCoords);

// 	compute global informations for scvf
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	map local corners of scvf to global
		for(size_t co = 0; co < m_vSCVF[i].num_corners(); ++co)
			m_rMapping.local_to_global(m_vSCVF[i].vGloPos[co], m_vSCVF[i].vLocPos[co]);

	//	map local ips of scvf to global
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
			m_rMapping.local_to_global(m_vSCVF[i].vGlobalIP[ip], m_vSCVF[i].local_ip(ip));

	// 	normal on scvf
		traits::NormalOnSCVF(m_vSCVF[i].Normal, m_vSCVF[i].vGloPos, vCornerCoords);
	}

// 	compute size of scv
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	map local corners of scvf to global
		for(size_t co = 0; co < m_vSCV[i].num_corners(); ++co)
			m_rMapping.local_to_global(m_vSCV[i].vGloPos[co], m_vSCV[i].vLocPos[co]);

	//	map local ips of scvf to global
		for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
			m_rMapping.local_to_global(m_vSCV[i].vGlobalIP[ip], m_vSCV[i].local_ip(ip));

	// 	compute volume of scv
		if(m_vSCV[i].numCo != 10)
			m_vSCV[i].Vol = ElementSize<scv_type, worldDim>(m_vSCV[i].vGloPos);
	//	special case for pyramid, last scv
		else throw(UGError("Pyramid Not Implemented"));
	}

//	if mapping is linear, compute jacobian only once and copy
	if(ReferenceMapping<ref_elem_type, worldDim>::isLinear)
	{
		MathMatrix<worldDim,dim> JtInv;
		m_rMapping.jacobian_transposed_inverse(JtInv, m_vSCVF[0].local_ip(0));
		const number detJ = m_rMapping.jacobian_det(m_vSCVF[0].local_ip(0));
		for(size_t i = 0; i < num_scvf(); ++i)
			for(size_t ip = 0; ip < scvf(i).num_ip(); ++ip)
			{
				m_vSCVF[i].vJtInv[ip] = JtInv;
				m_vSCVF[i].vDetJ[ip] = detJ;
			}

		for(size_t i = 0; i < num_scv(); ++i)
			for(size_t ip = 0; ip < scv(i).num_ip(); ++ip)
			{
				m_vSCV[i].vJtInv[ip] = JtInv;
				m_vSCV[i].vDetJ[ip] = detJ;
			}
	}
//	else compute jacobian for each integration point
	else
	{
		for(size_t i = 0; i < num_scvf(); ++i)
			for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
			{
				m_rMapping.jacobian_transposed_inverse(m_vSCVF[i].vJtInv[ip], m_vSCVF[i].local_ip(ip));
				m_vSCVF[i].vDetJ[ip] = m_rMapping.jacobian_det(m_vSCVF[i].local_ip(ip));
			}

		for(size_t i = 0; i < num_scv(); ++i)
			for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
			{
				m_rMapping.jacobian_transposed_inverse(m_vSCV[i].vJtInv[ip], m_vSCV[i].local_ip(ip));
				m_vSCV[i].vDetJ[ip] = m_rMapping.jacobian_det(m_vSCV[i].local_ip(ip));
			}
	}

//	compute global gradients
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < scvf(i).num_ip(); ++ip)
			for(size_t sh = 0 ; sh < nsh; ++sh)
				MatVecMult(m_vSCVF[i].vvGlobalGrad[ip][sh], m_vSCVF[i].vJtInv[ip], m_vSCVF[i].vvLocalGrad[ip][sh]);

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < scv(i).num_ip(); ++ip)
			for(size_t sh = 0 ; sh < nsh; ++sh)
				MatVecMult(m_vSCV[i].vvGlobalGrad[ip][sh], m_vSCV[i].vJtInv[ip], m_vSCV[i].vvLocalGrad[ip][sh]);

// 	Copy ip pos in list for SCVF
	size_t allIP = 0;
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < scvf(i).num_ip(); ++ip)
			m_vGlobSCVF_IP[allIP++] = scvf(i).global_ip(ip);

	allIP = 0;
	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < scv(i).num_ip(); ++ip)
			m_vGlobSCV_IP[allIP++] = scv(i).global_ip(ip);

//	if no boundary subsets required, return
	if(num_boundary_subsets() == 0 || ish == NULL) return true;
	else return update_boundary_faces(pElem, vCornerCoords, ish);
}

template <int TOrder, typename TElem, int TWorldDim, int TQuadOrderSCVF, int TQuadOrderSCV>
bool
FVGeometry<TOrder, TElem, TWorldDim, TQuadOrderSCVF, TQuadOrderSCV>::
update_boundary_faces(TElem* pElem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
//	get grid
	Grid& grid = *(ish->grid());

//	vector of subset indices of side
	std::vector<int> vSubsetIndex;

//	get subset indices for sides (i.e. edge in 2d, faces in 3d)
	if(dim == 1) {
		std::vector<VertexBase*> vVertex;
		CollectVertices(vVertex, grid, pElem);
		vSubsetIndex.resize(vVertex.size());
		for(size_t i = 0; i < vVertex.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vVertex[i]);
	}
	if(dim == 2) {
		std::vector<EdgeBase*> vEdges;
		CollectEdgesSorted(vEdges, grid, pElem);
		vSubsetIndex.resize(vEdges.size());
		for(size_t i = 0; i < vEdges.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vEdges[i]);
	}
	if(dim == 3) {
		std::vector<Face*> vFaces;
		CollectFacesSorted(vFaces, grid, pElem);
		vSubsetIndex.resize(vFaces.size());
		for(size_t i = 0; i < vFaces.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vFaces[i]);
	}

//	loop requested subset
	typename std::map<int, std::vector<BF> >::iterator it;
	for (it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); ++it)
	{
	//	get subset index
		const int bndIndex = (*it).first;

	//	get vector of BF for element
		std::vector<BF>& vBF = (*it).second;

	//	clear vector
		vBF.clear();

	//	current number of bf
		size_t curr_bf = 0;

	//	loop subelements
		for(size_t se = 0; se < numSubElem; ++se)
		{
		//	skip inner sub elements
			if(!m_vSubElem[se].isBndElem) continue;

		//	loop sides of element
			for(size_t side = 0; side < m_vSubElem[se].vElemBndSide.size(); ++side)
			{
			//	get whole element bnd side
				const int elemBndSide = m_vSubElem[se].vElemBndSide[side];

			//	skip non boundary sides
				if(elemBndSide == -1 || vSubsetIndex[elemBndSide] != bndIndex) continue;

			//	number of corners of side
				const int coOfSide = m_rRefElem.num(dim-1, elemBndSide, 0);

			//	resize vector
				vBF.resize(curr_bf + coOfSide);

			//	loop corners
				for(int co = 0; co < coOfSide; ++co)
				{
				//	get current bf
					BF& bf = vBF[curr_bf];

				//	set node id == scv this bf belongs to
					const int refNodeId = m_rRefElem.id(dim-1, elemBndSide, 0, co);
					bf.nodeId = m_vSubElem[se].vDoFID[refNodeId];

				//	Compute MidID for BF
					ComputeBFMidID(m_rRefElem, elemBndSide, bf.vMidID, co);

				// 	copy corners of bf
					CopyCornerByMidID<dim, maxMid>
						(bf.vLocPos, bf.vMidID, m_vSubElem[se].vvLocMid, BF::numCo);
					CopyCornerByMidID<worldDim, maxMid>
						(bf.vGloPos, bf.vMidID, m_vSubElem[se].vvGloMid, BF::numCo);

				// 	normal on scvf
					traits::NormalOnSCVF(bf.Normal, bf.vGloPos, m_vSubElem[se].vvGloMid[0]);

				//	compute local integration points
					bf.vWeight = m_rSCVFQuadRule.weights();
					ReferenceMapping<scvf_type, dim> map(bf.vLocPos);
					for(size_t ip = 0; ip < m_rSCVFQuadRule.size(); ++ip)
						map.local_to_global(bf.vLocalIP[ip], m_rSCVFQuadRule.point(ip));

				//	compute global integration points
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
						m_rMapping.local_to_global(bf.vGlobalIP[ip], bf.vLocalIP[ip]);

				//	compute volume
					bf.Vol = VecTwoNorm(bf.Normal);

				//	compute shapes and gradients
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
					{
						m_rTrialSpace.shapes(&(bf.vvShape[ip][0]), bf.local_ip(ip));
						m_rTrialSpace.grads(&(bf.vvLocalGrad[ip][0]), bf.local_ip(ip));

						m_rMapping.jacobian_transposed_inverse(bf.vJtInv[ip], bf.local_ip(ip));
						bf.vDetJ[ip] = m_rMapping.jacobian_det(bf.local_ip(ip));
					}

				//	compute global gradient
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
						for(size_t sh = 0 ; sh < bf.num_sh(); ++sh)
							MatVecMult(bf.vvGlobalGrad[ip][sh],
							           bf.vJtInv[ip], bf.vvLocalGrad[ip][sh]);

				//	increase curr_bf
					++curr_bf;
				}
			}
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// FV Geometry (all order, FVHO)   DIM FV
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <int TDim, int TWorldDim>
DimFVGeometry<TDim, TWorldDim>::
DimFVGeometry()	: m_pElem(NULL) {}



template <int TDim, int TWorldDim>
bool DimFVGeometry<TDim, TWorldDim>::
update_local(ReferenceObjectID roid, int orderShape, int quadOrderSCVF, int quadOrderSCV)
{
//	save setting we prepare the local data for
	m_roid = roid;
	m_orderShape = orderShape;
	m_quadOrderSCVF = quadOrderSCVF;
	m_quadOrderSCV = quadOrderSCV;

//	resize sub elements
	m_numSubElem = NumSubElements(m_roid, m_orderShape);
	m_vSubElem.resize(m_numSubElem);

//	get the multi indices for the sub elements and the boundary flags
	bool* vIsBndElem = new bool[m_numSubElem];
	std::vector<std::vector<int> > vElemBndSide(m_numSubElem);
	std::vector<std::vector<MathVector<dim,int> > > vMultiIndex(m_numSubElem);
	std::vector<std::vector<size_t> > vIndex(m_numSubElem);
	ComputeMultiIndicesOfSubElement<dim>(&vMultiIndex[0], &vIsBndElem[0],
	                                     &vElemBndSide[0], &vIndex[0], m_roid, m_orderShape);

//	get reference element
	try{
	const DimReferenceElement<dim>& rRefElem
				= ReferenceElementProvider::get<dim>(m_roid);

//	directions of counting
	MathVector<dim> direction[dim];
	for(int i = 0; i < dim; ++i){direction[i] = 0.0; direction[i][i] = 1.0;}

	for(size_t se = 0; se < m_numSubElem; ++se)
	{
		for(size_t co = 0; co < vMultiIndex[se].size(); ++co)
		{
		//	compute corners of sub elem in local coordinates
			MathVector<dim> pos; pos = 0.0;
			for(int i = 0; i < dim; ++i)
			{
				const number frac = vMultiIndex[se][co][i] / ((number)m_orderShape);
				VecScaleAppend(pos, frac, direction[i]);
			}
			m_vSubElem[se].vvLocMid[0][co] = pos;

		//	get multi index for corner
			m_vSubElem[se].vDoFID[co] = vIndex[se][co];
		}

	//	remember if boundary element
		m_vSubElem[se].isBndElem = vIsBndElem[se];

	//	remember boundary sides
		m_vSubElem[se].vElemBndSide = vElemBndSide[se];
	}

	delete[] vIsBndElem;

//	compute mid points for all sub elements
	for(size_t se = 0; se < m_numSubElem; ++se)
		ComputeMidPoints<dim, DimReferenceElement<dim>, maxMid>
				(rRefElem, m_vSubElem[se].vvLocMid[0], m_vSubElem[se].vvLocMid);

//	number of scvf/scv per subelem
	m_numSCVFPerSubElem = rRefElem.num(1);
	m_numSCVPerSubElem = rRefElem.num(0);

	m_numSCVF = m_numSCVFPerSubElem * m_numSubElem;
	m_numSCV = m_numSCVPerSubElem * m_numSubElem;

	m_vSCVF.resize(m_numSCVF);
	m_vSCV.resize(m_numSCV);


//	get trial space
	try{
	const DimLocalShapeFunctionSet<dim>& rTrialSpace =
		LocalShapeFunctionSetProvider::get<dim>(m_roid, LFEID(LFEID::LAGRANGE, m_orderShape));

//	request for quadrature rule
	try{
	const ReferenceObjectID scvfRoid = scvf_type::REFERENCE_OBJECT_ID;
	const QuadratureRule<dim-1>& rSCVFQuadRule
			= QuadratureRuleProvider<dim-1>::get_rule(scvfRoid, m_quadOrderSCVF);

	const int nipSCVF = rSCVFQuadRule.size();
	m_numSCVFIP = m_numSCVF * nipSCVF;

// 	set up local informations for SubControlVolumeFaces (scvf)
// 	each scvf is associated to one edge of the sub-element
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	get corresponding subelement
		const size_t se = i / m_numSCVFPerSubElem;
		const size_t locSCVF = i % m_numSCVFPerSubElem;

	//	this scvf separates the given nodes
		const size_t locFrom =  rRefElem.id(1, locSCVF, 0, 0);
		const size_t locTo =  rRefElem.id(1, locSCVF, 0, 1);

		m_vSCVF[i].From = m_vSubElem[se].vDoFID[locFrom];
		m_vSCVF[i].To = m_vSubElem[se].vDoFID[locTo];

	//	compute mid ids of the scvf
		ComputeSCVFMidID(rRefElem, m_vSCVF[i].vMidID, locSCVF);

	// 	copy local corners of scvf
		CopyCornerByMidID<dim, maxMid>
			(m_vSCVF[i].vLocPos, m_vSCVF[i].vMidID, m_vSubElem[se].vvLocMid, SCVF::numCo);

	// 	compute integration points
		m_vSCVF[i].vWeight = rSCVFQuadRule.weights();
		m_vSCVF[i].nip = nipSCVF;

		m_vSCVF[i].vLocalIP.resize(nipSCVF);
		m_vSCVF[i].vGlobalIP.resize(nipSCVF);

		m_vSCVF[i].vvShape.resize(nipSCVF);
		m_vSCVF[i].vvLocalGrad.resize(nipSCVF);
		m_vSCVF[i].vvGlobalGrad.resize(nipSCVF);
		m_vSCVF[i].vJtInv.resize(nipSCVF);
		m_vSCVF[i].vDetJ.resize(nipSCVF);

		m_vSCVF[i].nsh = rTrialSpace.num_sh();

		ReferenceMapping<scvf_type, dim> map(m_vSCVF[i].vLocPos);
		for(size_t ip = 0; ip < rSCVFQuadRule.size(); ++ip)
			map.local_to_global(m_vSCVF[i].vLocalIP[ip], rSCVFQuadRule.point(ip));
	}

	}catch(UG_ERROR_QuadratureRuleNotRegistered& ex){
		UG_LOG("ERROR in DimFVGeometry::update: " << ex.get_msg() << ".\n");
		return false;
	}

//	request for quadrature rule
	try{
	const ReferenceObjectID scvRoid = scv_type::REFERENCE_OBJECT_ID;
	const QuadratureRule<dim>& rSCVQuadRule
			= QuadratureRuleProvider<dim>::get_rule(scvRoid, m_quadOrderSCV);

	const int nipSCV = rSCVQuadRule.size();
	m_numSCVIP = m_numSCV * nipSCV;

// 	set up local informations for SubControlVolumes (scv)
// 	each scv is associated to one corner of the sub-element
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	get corresponding subelement
		const size_t se = i / m_numSCVPerSubElem;
		const size_t locSCV = i % m_numSCVPerSubElem;

	//	store associated node
		m_vSCV[i].nodeId = m_vSubElem[se].vDoFID[locSCV];;

	//	compute mid ids scv
		ComputeSCVMidID(rRefElem, m_vSCV[i].vMidID, locSCV);

	// 	copy local corners of scv
		CopyCornerByMidID<dim, maxMid>
			(m_vSCV[i].vLocPos, m_vSCV[i].vMidID, m_vSubElem[se].vvLocMid, m_vSCV[i].num_corners());

	// 	compute integration points
		m_vSCV[i].vWeight = rSCVQuadRule.weights();
		m_vSCV[i].nip = nipSCV;

		m_vSCV[i].vLocalIP.resize(nipSCV);
		m_vSCV[i].vGlobalIP.resize(nipSCV);

		m_vSCV[i].vvShape.resize(nipSCV);
		m_vSCV[i].vvLocalGrad.resize(nipSCV);
		m_vSCV[i].vvGlobalGrad.resize(nipSCV);
		m_vSCV[i].vJtInv.resize(nipSCV);
		m_vSCV[i].vDetJ.resize(nipSCV);

		m_vSCV[i].nsh = rTrialSpace.num_sh();

		if(dim == 3 && roid != ROID_PYRAMID) m_vSCV[i].numCo = 8;

		ReferenceMapping<scv_type, dim> map(m_vSCV[i].vLocPos);
		for(size_t ip = 0; ip < rSCVQuadRule.size(); ++ip)
			map.local_to_global(m_vSCV[i].vLocalIP[ip], rSCVQuadRule.point(ip));
	}

	}catch(UG_ERROR_QuadratureRuleNotRegistered& ex){
		UG_LOG("ERROR in DimFVGeometry::update: " << ex.get_msg() << ".\n");
		return false;
	}

	/////////////////////////
	// Shapes and Derivatives
	/////////////////////////

	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
		{
			m_vSCVF[i].vvShape[ip].resize(m_vSCVF[i].nsh);
			m_vSCVF[i].vvLocalGrad[ip].resize(m_vSCVF[i].nsh);
			m_vSCVF[i].vvGlobalGrad[ip].resize(m_vSCVF[i].nsh);

			rTrialSpace.shapes(&(m_vSCVF[i].vvShape[ip][0]), m_vSCVF[i].local_ip(ip));
			rTrialSpace.grads(&(m_vSCVF[i].vvLocalGrad[ip][0]), m_vSCVF[i].local_ip(ip));
		}

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
		{
			m_vSCV[i].vvShape[ip].resize(m_vSCV[i].nsh);
			m_vSCV[i].vvLocalGrad[ip].resize(m_vSCV[i].nsh);
			m_vSCV[i].vvGlobalGrad[ip].resize(m_vSCV[i].nsh);

			rTrialSpace.shapes(&(m_vSCV[i].vvShape[ip][0]), m_vSCV[i].local_ip(ip));
			rTrialSpace.grads(&(m_vSCV[i].vvLocalGrad[ip][0]), m_vSCV[i].local_ip(ip));
		}

	}catch(UG_ERROR_LocalShapeFunctionSetNotRegistered& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

	}catch(UG_ERROR_ReferenceElementMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

// 	copy ip positions in a list for Sub Control Volumes Faces (SCVF)
	m_vLocSCVF_IP.resize(m_numSCVFIP);
	m_vGlobSCVF_IP.resize(m_numSCVFIP);
	size_t allIP = 0;
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
			m_vLocSCVF_IP[allIP++] = scvf(i).local_ip(ip);

// 	copy ip positions in a list for Sub Control Volumes (SCV)
	m_vLocSCV_IP.resize(m_numSCVIP);
	m_vGlobSCV_IP.resize(m_numSCVIP);
	allIP = 0;
	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < m_vSCV[i].num_ip(); ++ip)
			m_vLocSCV_IP[allIP++] = scv(i).local_ip(ip);

	return true;
}


/// update data for given element
template <int TDim, int TWorldDim>
bool
DimFVGeometry<TDim, TWorldDim>::
update(GeometricObject* pElem, const MathVector<worldDim>* vCornerCoords,
       int orderShape, int quadOrderSCVF, int quadOrderSCV,
       const ISubsetHandler* ish)
{
// 	If already update for this element, do nothing
	if(m_pElem == pElem) return true; else m_pElem = pElem;

//	get reference element type
	ReferenceObjectID roid = (ReferenceObjectID)pElem->reference_object_id();

//	if already prepared for this roid, skip update of local values
	if(m_roid != roid || orderShape != m_orderShape ||
	   quadOrderSCVF != m_quadOrderSCVF || quadOrderSCV != m_quadOrderSCV)
		if(!update_local(roid, orderShape, quadOrderSCVF, quadOrderSCV))
			return false;

//	get reference element mapping
	try{
	DimReferenceMapping<dim, worldDim>& rMapping
		= ReferenceMappingProvider::get<dim, worldDim>(roid);

//	update reference mapping
	rMapping.update(vCornerCoords);

// 	compute global informations for scvf
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	//	map local corners of scvf to global
		for(size_t co = 0; co < m_vSCVF[i].num_corners(); ++co)
			rMapping.local_to_global(m_vSCVF[i].vGloPos[co], m_vSCVF[i].vLocPos[co]);

	//	map local ips of scvf to global
		for(size_t ip = 0; ip < m_vSCVF[i].num_ip(); ++ip)
			rMapping.local_to_global(m_vSCVF[i].vGlobalIP[ip], m_vSCVF[i].local_ip(ip));

	// 	normal on scvf
		traits::NormalOnSCVF(m_vSCVF[i].Normal, m_vSCVF[i].vGloPos, vCornerCoords);
	}

// 	compute size of scv
	for(size_t i = 0; i < num_scv(); ++i)
	{
	//	map local corners of scvf to global
		rMapping.local_to_global(&m_vSCV[i].vGloPos[0], &m_vSCV[i].vLocPos[0], m_vSCV[i].num_corners());

	//	map local ips of scvf to global
			rMapping.local_to_global(&m_vSCV[i].vGlobalIP[0], &m_vSCV[i].vLocalIP[0], m_vSCV[i].num_ip());

	// 	compute volume of scv
		if(m_vSCV[i].numCo != 10)
			m_vSCV[i].Vol = ElementSize<scv_type, worldDim>(m_vSCV[i].vGloPos);
	//	special case for pyramid, last scv
		else throw(UGError("Pyramid Not Implemented"));
	}

	for(size_t i = 0; i < num_scvf(); ++i)
	{
		rMapping.jacobian_transposed_inverse(&m_vSCVF[i].vJtInv[0], &m_vSCVF[i].vLocalIP[0], m_vSCVF[i].num_ip());
		rMapping.jacobian_det(&m_vSCVF[i].vDetJ[0], &m_vSCVF[i].vLocalIP[0], m_vSCVF[i].num_ip());
	}

	for(size_t i = 0; i < num_scv(); ++i)
	{
		rMapping.jacobian_transposed_inverse(&m_vSCV[i].vJtInv[0], &m_vSCV[i].vLocalIP[0], m_vSCV[i].num_ip());
		rMapping.jacobian_det(&m_vSCV[i].vDetJ[0], &m_vSCV[i].vLocalIP[0], m_vSCV[i].num_ip());
	}

	}catch(UG_ERROR_ReferenceMappingMissing& ex){
		UG_LOG("ERROR in FEGeometry::update: " << ex.get_msg() << ".\n");
		return false;
	}

//	compute global gradients
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < scvf(i).num_ip(); ++ip)
			for(size_t sh = 0 ; sh < m_vSCVF[i].nsh; ++sh)
				MatVecMult(m_vSCVF[i].vvGlobalGrad[ip][sh], m_vSCVF[i].vJtInv[ip], m_vSCVF[i].vvLocalGrad[ip][sh]);

	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < scv(i).num_ip(); ++ip)
			for(size_t sh = 0 ; sh < m_vSCV[i].nsh; ++sh)
				MatVecMult(m_vSCV[i].vvGlobalGrad[ip][sh], m_vSCV[i].vJtInv[ip], m_vSCV[i].vvLocalGrad[ip][sh]);

// 	Copy ip pos in list for SCVF
	size_t allIP = 0;
	for(size_t i = 0; i < num_scvf(); ++i)
		for(size_t ip = 0; ip < scvf(i).num_ip(); ++ip)
			m_vGlobSCVF_IP[allIP++] = scvf(i).global_ip(ip);

	allIP = 0;
	for(size_t i = 0; i < num_scv(); ++i)
		for(size_t ip = 0; ip < scv(i).num_ip(); ++ip)
			m_vGlobSCV_IP[allIP++] = scv(i).global_ip(ip);

//	if no boundary subsets required, return
	if(num_boundary_subsets() == 0 || ish == NULL) return true;
	else return update_boundary_faces(pElem, vCornerCoords, ish);
}

template <int TDim, int TWorldDim>
bool
DimFVGeometry<TDim, TWorldDim>::
update_boundary_faces(GeometricObject* pElem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
//	get grid
	Grid& grid = *(ish->grid());

//	vector of subset indices of side
	std::vector<int> vSubsetIndex;

//	get subset indices for sides (i.e. edge in 2d, faces in 3d)
	if(dim == 1) {
		std::vector<VertexBase*> vVertex;
		CollectVertices(vVertex, grid, pElem);
		vSubsetIndex.resize(vVertex.size());
		for(size_t i = 0; i < vVertex.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vVertex[i]);
	}
	if(dim == 2) {
		std::vector<EdgeBase*> vEdges;
		CollectEdgesSorted(vEdges, grid, pElem);
		vSubsetIndex.resize(vEdges.size());
		for(size_t i = 0; i < vEdges.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vEdges[i]);
	}
	if(dim == 3) {
		std::vector<Face*> vFaces;
		CollectFacesSorted(vFaces, grid, pElem);
		vSubsetIndex.resize(vFaces.size());
		for(size_t i = 0; i < vFaces.size(); ++i)
			vSubsetIndex[i] = ish->get_subset_index(vFaces[i]);
	}

//	get reference element mapping
	try{
	DimReferenceMapping<dim, worldDim>& rMapping
		= ReferenceMappingProvider::get<dim, worldDim>(m_roid);

	try{
	const DimReferenceElement<dim>& rRefElem
		= ReferenceElementProvider::get<dim>(m_roid);

	try{
	const ReferenceObjectID scvfRoid = scvf_type::REFERENCE_OBJECT_ID;
	const QuadratureRule<dim-1>& rSCVFQuadRule
			= QuadratureRuleProvider<dim-1>::get_rule(scvfRoid, m_quadOrderSCVF);

	try{
	const DimLocalShapeFunctionSet<dim>& rTrialSpace =
		LocalShapeFunctionSetProvider::get<dim>(m_roid, LFEID(LFEID::LAGRANGE, m_orderShape));

//	update reference mapping
	rMapping.update(vCornerCoords);

//	loop requested subset
	typename std::map<int, std::vector<BF> >::iterator it;
	for (it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); ++it)
	{
	//	get subset index
		const int bndIndex = (*it).first;

	//	get vector of BF for element
		std::vector<BF>& vBF = (*it).second;

	//	clear vector
		vBF.clear();

	//	current number of bf
		size_t curr_bf = 0;

	//	loop subelements
		for(size_t se = 0; se < m_numSubElem; ++se)
		{
		//	skip inner sub elements
			if(!m_vSubElem[se].isBndElem) continue;

		//	loop sides of element
			for(size_t side = 0; side < m_vSubElem[se].vElemBndSide.size(); ++side)
			{
			//	get whole element bnd side
				const int elemBndSide = m_vSubElem[se].vElemBndSide[side];

			//	skip non boundary sides
				if(elemBndSide == -1 || vSubsetIndex[elemBndSide] != bndIndex) continue;

			//	number of corners of side
				const int coOfSide = rRefElem.num(dim-1, elemBndSide, 0);

			//	resize vector
				vBF.resize(curr_bf + coOfSide);

			//	loop corners
				for(int co = 0; co < coOfSide; ++co)
				{
				//	get current bf
					BF& bf = vBF[curr_bf];

				//	set node id == scv this bf belongs to
					const int refNodeId = rRefElem.id(dim-1, elemBndSide, 0, co);
					bf.nodeId = m_vSubElem[se].vDoFID[refNodeId];

				//	Compute MidID for BF
					ComputeBFMidID(rRefElem, elemBndSide, bf.vMidID, co);

				// 	copy corners of bf
					CopyCornerByMidID<dim, maxMid>
						(bf.vLocPos, bf.vMidID, m_vSubElem[se].vvLocMid, BF::numCo);
					CopyCornerByMidID<worldDim, maxMid>
						(bf.vGloPos, bf.vMidID, m_vSubElem[se].vvGloMid, BF::numCo);

				// 	normal on scvf
					traits::NormalOnSCVF(bf.Normal, bf.vGloPos, m_vSubElem[se].vvGloMid[0]);

				//	compute local integration points
					bf.vWeight = rSCVFQuadRule.weights();
					ReferenceMapping<scvf_type, dim> map(bf.vLocPos);
					for(size_t ip = 0; ip < rSCVFQuadRule.size(); ++ip)
						map.local_to_global(bf.vLocalIP[ip], rSCVFQuadRule.point(ip));

				//	compute global integration points
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
						rMapping.local_to_global(bf.vGlobalIP[ip], bf.vLocalIP[ip]);

				//	compute volume
					bf.Vol = VecTwoNorm(bf.Normal);

				//	compute shapes and gradients
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
					{
						rTrialSpace.shapes(&(bf.vvShape[ip][0]), bf.local_ip(ip));
						rTrialSpace.grads(&(bf.vvLocalGrad[ip][0]), bf.local_ip(ip));
					}

					rMapping.jacobian_transposed_inverse(&bf.vJtInv[0], &bf.vLocalIP[0], bf.num_ip());
					rMapping.jacobian_det(&bf.vDetJ[0], &bf.vLocalIP[0], bf.num_ip());

				//	compute global gradient
					for(size_t ip = 0; ip < bf.num_ip(); ++ip)
						for(size_t sh = 0 ; sh < num_scv(); ++sh)
							MatVecMult(bf.vvGlobalGrad[ip][sh],
							           bf.vJtInv[ip], bf.vvLocalGrad[ip][sh]);

				//	increase curr_bf
					++curr_bf;
				}
			}
		}
	}

	}catch(UG_ERROR_QuadratureRuleNotRegistered& ex){
		UG_LOG("ERROR in DimFVGeometry::update: " << ex.get_msg() << ".\n");
		return false;
	}

	}catch(UG_ERROR_ReferenceMappingMissing& ex){
		UG_LOG("ERROR in FEGeometry::update: " << ex.get_msg() << ".\n");
		return false;
	}

	}catch(UG_ERROR_LocalShapeFunctionSetNotRegistered& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

	}catch(UG_ERROR_ReferenceElementMissing& ex)
	{
		UG_LOG("ERROR in 'DimFV1Geometry::update': "<<ex.get_msg()<<"\n");
		return false;
	}

	return true;
}












////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// FV1ManifoldBoundary
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <typename TElem, int TWorldDim>
FV1ManifoldBoundary<TElem, TWorldDim>::
FV1ManifoldBoundary() : m_pElem(NULL), m_rRefElem(Provider<ref_elem_type>::get())
{
	// set corners of element as local centers of nodes
	for (size_t i = 0; i < m_rRefElem.num(0); ++i)
		m_locMid[0][i] = m_rRefElem.corner(i);

	// compute local midpoints for all geometric objects with  0 < d <= dim
	for (int d = 1; d <= dim; ++d)
	{
		// loop geometric objects of dimension d
		for(size_t i = 0; i < m_rRefElem.num(d); ++i)
		{
			// set first node
			const size_t coID0 = m_rRefElem.id(d, i, 0, 0);
			m_locMid[d][i] = m_locMid[0][coID0];

			// add corner coordinates of the corners of the geometric object
			for(size_t j = 1; j < m_rRefElem.num(d, i, 0); ++j)
			{
				const size_t coID = m_rRefElem.id(d, i, 0, j);
				m_locMid[d][i] += m_locMid[0][coID];
			}

			// scale for correct averaging
			m_locMid[d][i] *= 1./(m_rRefElem.num(d, i, 0));
		}
	}

	// set up local information for Boundary Faces (bf)
	// each bf is associated to one corner of the element
	for (size_t i = 0; i < num_bf(); ++i)
	{
		m_vBF[i].nodeId = i;

		if (dim == 1) // Edge
		{
			m_vBF[i].midId[0] = MidID(0, i);	// set node as corner of bf
			m_vBF[i].midId[1] = MidID(dim, 0);	// center of bnd element
			
			// copy local corners of bf
			copy_local_corners(m_vBF[i]);
			
			// local integration point
			AveragePositions(m_vBF[i].localIP, m_vBF[i].vLocPos, 2);
		}
		else if (dim == 2)	// Quadrilateral
		{
			m_vBF[i].midId[0] = MidID(0, i); // set node as corner of bf
			m_vBF[i].midId[1] = MidID(1, m_rRefElem.id(0, i, 1, 0)); // edge 1
			m_vBF[i].midId[2] = MidID(dim, 0);	// center of bnd element
			m_vBF[i].midId[3] = MidID(1, m_rRefElem.id(0, i, 1, 1)); // edge 2
			
			// copy local corners of bf
			copy_local_corners(m_vBF[i]);
			
			// local integration point
			AveragePositions(m_vBF[i].localIP, m_vBF[i].vLocPos, 4);
		}
		else {UG_ASSERT(0, "Dimension higher that 2 not implemented.");}
	}

	/////////////
	// Shapes
	/////////////
	// A word of warning: This is only meaningful,
	// if the trial space is piecewise linear on tetrahedrons/triangles!
	for (size_t i = 0; i < num_bf(); ++i)
	{
		const LocalShapeFunctionSet<ref_elem_type>& TrialSpace =
				LocalShapeFunctionSetProvider::
					get<ref_elem_type>
					(LFEID(LFEID::LAGRANGE, 1));

		const size_t num_sh = m_numBF;
		m_vBF[i].vShape.resize(num_sh);

		TrialSpace.shapes(&(m_vBF[i].vShape[0]), m_vBF[i].localIP);
	}

	///////////////////////////
	// Copy ip pos in list
	///////////////////////////

	// 	loop Boundary Faces (BF)
	m_vLocBFIP.clear();
	for (size_t i = 0; i < num_bf(); ++i)
	{
	//	get current BF
		const BF& rBF = bf(i);

	// 	loop ips
		for (size_t ip = 0; ip < rBF.num_ip(); ++ip)
		{
			m_vLocBFIP.push_back(rBF.local_ip(ip));
		}
	}
}


/// update data for given element
template <typename TElem, int TWorldDim>
bool
FV1ManifoldBoundary<TElem, TWorldDim>::
update(TElem* elem, const MathVector<worldDim>* vCornerCoords, const ISubsetHandler* ish)
{
	// 	If already update for this element, do nothing
	if (m_pElem == elem) return true;
	else m_pElem = elem;

	// 	remember global position of nodes
	for (size_t i = 0; i < m_rRefElem.num(0); ++i)
		m_gloMid[0][i] = vCornerCoords[i];

	// 	compute global midpoints for all the other geometric objects (with  0 < d <= dim)
	for (int d = 1; d <= dim; ++d)
	{
		// 	loop geometric objects of dimension d
		for (size_t i = 0; i < m_rRefElem.num(d); ++i)
		{
			// set first node
			const size_t coID0 = m_rRefElem.id(d, i, 0, 0);
			m_gloMid[d][i] = m_gloMid[0][coID0];

		// 	add corner coordinates of the corners of the geometric object
			for (size_t j = 1; j < m_rRefElem.num(d, i, 0); ++j)
			{
				const size_t coID = m_rRefElem.id(d, i, 0, j);
				m_gloMid[d][i] += m_gloMid[0][coID];
			}

		// 	scale for correct averaging
			m_gloMid[d][i] *= 1./(m_rRefElem.num(d, i, 0));
		}
	}
	
	// set local integration points
	for (size_t i = 0; i < num_bf(); ++i)
	{
		// copy global corners of bf
		copy_global_corners(m_vBF[i]);
		
		if (dim == 1) // Edge
			{AveragePositions(m_vBF[i].localIP, m_vBF[i].vLocPos, 2);}
		else if (dim == 2)	// Quadrilateral
			{AveragePositions(m_vBF[i].localIP, m_vBF[i].vLocPos, 4);}
		else {UG_ASSERT(0, "Dimension higher than 2 not implemented.");}
	}
	
	// 	compute size of BFs
	for (size_t i = 0; i < num_bf(); ++i)
	{
	// 	copy global corners
		copy_global_corners(m_vBF[i]);

	// 	compute volume of bf
		m_vBF[i].vol = ElementSize<bf_type, worldDim>(m_vBF[i].vGloPos);
	}
	
	///////////////////////////
	// Copy ip pos in list
	///////////////////////////

	// 	loop Boundary Faces (BF)
	m_vGlobBFIP.clear();
	for (size_t i = 0; i < num_bf(); ++i)
	{
	//	get current BF
		const BF& rBF = bf(i);

	// 	loop ips
		for (size_t ip = 0; ip < rBF.num_ip(); ++ip)
		{
			m_vGlobBFIP.push_back(rBF.global_ip(ip));
		}
	}

	return true;
}

//////////////////////
// FV1Geometry

template class FV1Geometry<Edge, 1>;
template class FV1Geometry<Edge, 2>;
template class FV1Geometry<Edge, 3>;

template class FV1Geometry<Triangle, 2>;
template class FV1Geometry<Triangle, 3>;

template class FV1Geometry<Quadrilateral, 2>;
template class FV1Geometry<Quadrilateral, 3>;

template class FV1Geometry<Tetrahedron, 3>;
template class FV1Geometry<Prism, 3>;
template class FV1Geometry<Pyramid, 3>;
template class FV1Geometry<Hexahedron, 3>;

//////////////////////
// DimFV1Geometry
template class DimFV1Geometry<1, 1>;
template class DimFV1Geometry<1, 2>;
template class DimFV1Geometry<1, 3>;

template class DimFV1Geometry<2, 2>;
template class DimFV1Geometry<2, 3>;

template class DimFV1Geometry<3, 3>;

//////////////////////
// FVGeometry

template class FVGeometry<1, Triangle, 2>;
template class FVGeometry<1, Quadrilateral, 2>;
template class FVGeometry<1, Tetrahedron, 3>;
template class FVGeometry<1, Prism, 3>;
template class FVGeometry<1, Hexahedron, 3>;

template class FVGeometry<2, Triangle, 2>;
template class FVGeometry<2, Quadrilateral, 2>;
template class FVGeometry<2, Tetrahedron, 3>;
template class FVGeometry<2, Prism, 3>;
template class FVGeometry<2, Hexahedron, 3>;

template class FVGeometry<3, Triangle, 2>;
template class FVGeometry<3, Quadrilateral, 2>;
template class FVGeometry<3, Tetrahedron, 3>;
template class FVGeometry<3, Prism, 3>;
template class FVGeometry<3, Hexahedron, 3>;

//////////////////////
// DimFVGeometry
//template class DimFVGeometry<1, 1>;
//template class DimFVGeometry<1, 2>;
//template class DimFVGeometry<1, 3>;

template class DimFVGeometry<2, 2>;
template class DimFVGeometry<2, 3>;

template class DimFVGeometry<3, 3>;


//////////////////////
// Manifold
template class FV1ManifoldBoundary<Edge, 2>;
template class FV1ManifoldBoundary<Triangle, 3>;
template class FV1ManifoldBoundary<Quadrilateral, 3>;

} // end namespace ug
