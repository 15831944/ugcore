/**
 * \file sparsematrix_util.h
 *
 * \author Martin Rupp
 *
 * \date 11.06.2010
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__UG__CPU_ALGEBRA__SPARSEMATRIX_UTIL__
#define __H__UG__CPU_ALGEBRA__SPARSEMATRIX_UTIL__

#include "../small_algebra/small_algebra.h"

namespace ug
{

/// \addtogroup lib_algebra
///	@{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateAsMultiplyOf:
//-------------------------
/**
 * \brief Calculates M = A*B*C.
 * \param M (out) Matrix M, M = A*B*C$
 * \param A (in) Matrix A
 * \param B (in) Matrix B
 * \param C (in) Matrix C
 */
template<typename ABC_type, typename A_type, typename B_type, typename C_type>
void CreateAsMultiplyOf(ABC_type &M, const A_type &A, const B_type &B, const C_type &C, double epsilonTruncation=0.0)
{
	UG_ASSERT(C.num_rows() == B.num_cols() && B.num_rows() == A.num_cols(), "sizes must match");

	// create output matrix M
	M.resize(A.num_rows(), C.num_cols());

	// speedup with array posInConnections, needs n memory
	// posInConnections[i]: index in the connections for current row (if not in row: -1)
	// tried this also with std::map, but took 1511.53 ms instead of 393.972 ms
	// searching in the connections is also slower

	std::vector<int> posInConnections(C.num_cols(), -1);

	// types
	std::vector<typename ABC_type::connection > con; con.reserve(16);
	std::vector<typename ABC_type::connection > con2; con2.reserve(16);

	typedef typename A_type::value_type avalue;
	typename block_multiply_traits<typename A_type::value_type, typename B_type::value_type>::ReturnType ab;
	typename C_type::value_type cvalue;

	typename ABC_type::connection c;

	typedef typename A_type::const_row_iterator cAiterator;
	typedef typename B_type::const_row_iterator cBiterator;
	typedef typename C_type::const_row_iterator cCiterator;

	// do
	for(size_t i=0; i < A.num_rows(); i++)
	{
		con.clear();
		for(cAiterator itA = A.begin_row(i); itA != A.end_row(i); ++itA)
		{
			if(itA.value() == 0.0) continue;

			for(cBiterator itB = B.begin_row(itA.index()); itB != B.end_row(itA.index()); ++itB)
			{
				if(itB.value() == 0.0) continue;
				AssignMult(ab, itA.value(), itB.value());

				for(cCiterator itC = C.begin_row(itB.index()); itC != C.end_row(itB.index()); ++itC)
				{
					cvalue = itC.value();
					if(cvalue == 0.0) continue;
					size_t indexTo = itC.index();

					if(posInConnections[indexTo] == -1)
					{
						// we havent visited node <indexTo>
						// so we need to add a Connection to the row
						// save the index of the connection in the row
						posInConnections[indexTo] = con.size();
						c.iIndex = indexTo;
						AssignMult(c.dValue, ab, cvalue);
						con.push_back(c);
					}
					else
					{
						// we have visited this node before,
						// so we know the index of the connection
						// -> add a*b*c
						AddMult(con[posInConnections[indexTo]].dValue, ab, cvalue);
					}

				}
			}
		}

		// reset posInConnections to -1
		for(size_t j=0; j<con.size(); j++) posInConnections[con[j].iIndex] = -1;
		if(epsilonTruncation != 0.0)
		{
			double m=0;
			for(size_t j=0; j<con.size(); j++)
			{
				double d = BlockNorm(con[j].dValue);
				if(d > m) m = d;
			}
			m *= epsilonTruncation;
			con2.clear();
			for(size_t j=0; j<con.size(); j++)
				if( BlockNorm(con[j].dValue) > m )
					con2.push_back(con[j]);
			M.set_matrix_row(i, &con2[0], con2.size());
		}
		else
			// set Matrix_type Row in AH
			M.set_matrix_row(i, &con[0], con.size());
	}

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateAsMultiplyOf:
//-------------------------
/**
 * \brief Calculates M = A*B.
 * \param M (out) Matrix M, M = A*B$
 * \param A (in) Matrix A
 * \param B (in) Matrix B
 */
template<typename AB_type, typename A_type, typename B_type>
void CreateAsMultiplyOf(AB_type &M, const A_type &A, const B_type &B)
{
	UG_ASSERT(B.num_rows() == A.num_cols(), "sizes must match");

	// create output matrix M
	M.resize(A.num_rows(), B.num_cols());

	std::vector<int> posInConnections(B.num_cols(), -1);

	// types
	std::vector<typename AB_type::connection > con; con.reserve(255);
	typename AB_type::connection c;
	typedef typename A_type::const_row_iterator cAiterator;
	typedef typename B_type::const_row_iterator cBiterator;

	// do
	for(size_t i=0; i < A.num_rows(); i++)
	{
		con.clear();
		for(cAiterator itA = A.begin_row(i); itA != A.end_row(i); ++itA)
		{
			if(itA.value() == 0.0) continue;

			for(cBiterator itB = B.begin_row(itA.index()); itB != B.end_row(itA.index()); ++itB)
			{
				if(itB.value() == 0.0) continue;
				size_t indexTo = itB.index();

				if(posInConnections[indexTo] == -1)
				{
					// we havent visited node <indexTo>
					// so we need to add a Connection to the row
					// save the index of the connection in the row
					posInConnections[indexTo] = con.size();
					c.iIndex = indexTo;
					AssignMult(c.dValue, itA.value(), itB.value());
					con.push_back(c);
				}
				else
				{
					// we have visited this node before,
					// so we know the index of the connection
					// -> add a*b*c
					AddMult(con[posInConnections[indexTo]].dValue, itA.value(), itB.value());
				}
			}
		}

		// reset posInConnections to -1
		for(size_t j=0; j<con.size(); j++) posInConnections[con[j].iIndex] = -1;
		// set Matrix_type Row in AH
		M.set_matrix_row(i, &con[0], con.size());
	}

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MatAdd:
//-------------------------
/**
 * \brief Calculates M = A + B
 * \param M (out) Matrix M, M = A + B
 * \param A (in) Matrix A
 * \param B (in) Matrix B
 * note: A and/or B may be equal to M.
 */
template<typename matrix_type>
void MatAdd(matrix_type &M, number &alpha1, const matrix_type &A, number &alpha2, const matrix_type &B)
{
	UG_ASSERT(A.num_rows() == B.num_rows() && A.num_cols() == B.num_cols(), "sizes must match");
	typedef typename matrix_type::const_row_iterator criterator;

	// create output matrix M
	if(&M != &A)
		M.resize(A.num_rows(), A.num_cols());

	// types
	std::vector<typename matrix_type::connection > con; con.reserve(10);

	typename matrix_type::connection c;
	for(size_t i=0; i < A.num_rows(); i++)
	{
		con.clear();
		criterator itA = A.begin_row(i), endA = A.end_row(i);
		criterator itB = B.begin_row(i), endB = B.end_row(i);

		while(itA != endA && itB != endB)
		{
			if(itA.index() == itB.index())
			{
				c.dValue = alpha1 * itA.value() + alpha2 * itB.value();
				c.iIndex = itA.index();
				++itA; ++itB;
			}
			else if (itA.index() < itB.index())
			{
				c.dValue = itA.value();
				c.dValue *= alpha1;
				c.iIndex = itA.index();
				++itA;
			}
			else
			{
				c.dValue = itB.value();
				c.dValue *= alpha2;
				c.iIndex = itB.index();
				++itB;
			}
			con.push_back(c);
		}
		while(itA != endA)
		{
			c.dValue = itA.value();
			c.dValue *= alpha1;
			c.iIndex = itA.index();
			++itA;
			con.push_back(c);
		}
		while(itB != endB)
		{
			c.dValue = itB.value();
			c.dValue *= alpha2;
			c.iIndex = itB.index();
			++itB;
			con.push_back(c);
		}

		M.set_matrix_row(i, &con[0], con.size());
	}
	M.defragment();
}

template<typename TMatrix>
void GetNeighborhood_worker(const TMatrix &A, size_t node, size_t depth, std::vector<size_t> &indices, std::vector<bool> &bVisited)
{
	if(depth==0) return;
	size_t iSizeBefore = indices.size();
	for(typename TMatrix::const_row_iterator it = A.begin_row(node); A.end_row(node); ++it)
	{
		if(it.value() == 0) continue;
		if(bVisited[it.index()] == false)
		{

			bVisited[it.index()] = true;
			indices.push_back(it.index());
		}
	}

	if(depth==1) return;
	size_t iSizeAfter = indices.size();
	for(size_t i=iSizeBefore; i<iSizeAfter; i++)
		GetNeighborhood_worker(A, indices[i], depth-1, indices, bVisited);
}

template<typename TMatrix>
void GetNeighborhood(const TMatrix &A, size_t node, size_t depth, std::vector<size_t> &indices, std::vector<bool> &bVisited, bool bResetVisitedFlags=true)
{

	indices.clear();

	if(bVisited[node] == false)
	{
		bVisited[node] = true;
		indices.push_back(node);
	}
	GetNeighborhood_worker(A, node, depth, indices, bVisited);

	if(bResetVisitedFlags)
		for(size_t i=0; i<indices.size(); i++)
			bVisited[i] = false;
}

template<typename TMatrix>
void GetNeighborhood(const TMatrix &A, size_t node, size_t depth, std::vector<size_t> &indices)
{
	std::vector<bool> bVisited(max(A.num_cols(), A.num_rows()), false);
	GetNeighborhood(A, node, depth, indices, bVisited, false);
}


template<typename TMatrix>
void MarkNeighbors(const TMatrix &A, size_t node, size_t depth, std::vector<bool> &bVisited)
{
	for(typename TMatrix::const_row_iterator it = A.begin_row(node); it != A.end_row(node); ++it)
	{
		if(it.value() == 0) continue;
		bVisited[it.index()] = true;
		MarkNeighbors(A, it.index(), depth, bVisited);
	}
}

template<typename TMatrix>
void GetNeighborhoodHierachy_worker(const TMatrix &A, size_t node, size_t depth, size_t maxdepth, std::vector< std::vector<size_t> > &indices, std::vector<bool> &bVisited)
{
	size_t iSizeBefore = indices[depth].size();
	for(typename TMatrix::const_row_iterator it = A.begin_row(node); it != A.end_row(node); ++it)
	{
		if(it.value() == 0) continue;
		if(bVisited[it.index()] == false)
		{
			bVisited[it.index()] = true;
			indices[depth].push_back(it.index());
		}
	}

	if(depth==maxdepth) return;
	size_t iSizeAfter = indices[depth].size();
	for(size_t i=iSizeBefore; i<iSizeAfter; i++)
		GetNeighborhoodHierachy_worker(A, indices[i], depth+1, maxdepth, indices, bVisited);
}

template<typename TMatrix>
void GetNeighborhoodHierachy(const TMatrix &A, size_t node, size_t depth, std::vector< std::vector<size_t> > &indices, std::vector<bool> &bVisited,
		bool bResetVisitedFlags=true)
{
	if(indices.size() != depth+1)
		indices.resize(depth+1);
	for(size_t i=0; i < depth+1; i++)
		indices[i].clear();

	bVisited[node] = true;
	indices[0].push_back(node);

	if(depth==0) return;

	for(size_t d = 0; d < depth; d++)
	{
		for(size_t i=0; i<indices[d].size(); i++)
		{
			size_t k = indices[d][i];
			for(typename TMatrix::const_row_iterator it = A.begin_row(k); it != A.end_row(k); ++it)
			{
				if(it.value() == 0) continue;
				if(bVisited[it.index()] == false)
				{
					bVisited[it.index()] = true;
					indices[d+1].push_back(it.index());
				}
			}

		}
	}

	if(bResetVisitedFlags)
		for(size_t i=0; i < depth+1; i++)
			for(size_t j=0; i<indices[j].size(); j++)
				bVisited[j] = false;
}


template<typename TMatrix>
void GetNeighborhoodHierachy(const TMatrix &A, size_t node, size_t depth, std::vector< std::vector<size_t> > &indices)
{
	std::vector<bool> bVisited(std::max(A.num_cols(), A.num_rows()), false);
	GetNeighborhoodHierachy(A, node, depth, indices, bVisited, false);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetNeighborhood:
//-------------------------
/**
 * \brief gets the neighborhood of a node in the connectivity graph of a SparseMatrix.
 * \param A (in) Matrix A
 * \param node (in) the node where to start
 * \param depth (in) the depth of neighborhood. 0 = empty.
 * \param indices (out) the indices of the neighbors
 * \param posInConnections array to speed up computation. Has to be posInConnections[i] = 0 for all i=0..A.num_rows(). Can be NULL.
 * \note the node itself is only included if there is a connection from node to node.
  */
#if 0
template<typename T>
void GetNeighborhood(SparseMatrix<T> &A, size_t node, size_t depth, std::vector<size_t> &indices, int *posInConnections=NULL)
{
	// perhaps this is better with recursion
	indices.clear();



	vector<typename SparseMatrix<T>::const_row_iterator> iterators;
	iterators.reserve(depth);

	iterators.push_back( A.begin_row(node) );

	while(iterators.size() != 0)
	{
		if(iterators.back().isEnd())
			iterators.pop_back();
		else
		{
			size_t index = iterators.back().index();
			++iterators.back();
			if(iterators.size() < depth)
				iterators.push_back( A.begin_row(index) );
			else
			{
				size_t pos;
				if(posInConnections == NULL)
				{
					for(pos=0; pos<indices.size(); pos++)
						if(indices[pos] == index)
							break;
					if(pos == indices.size())
						indices.push_back(index);
				}
				else
				{
					pos = posInConnections[index];
					if(pos == -1)
					{
						pos = posInConnections[index] = indices.size();
						indices.push_back(index);
					}
				}
				// else (count etc.)
			}
		}
	}

	// reset posInConnections
	if(posInConnections)
	{
		for(size_t i=0; i<indices.size(); i++)
			posInConnections[indices[i]] = -1;
	}

	// sort indices
	sort(indices.begin(), indices.end());
}
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsCloseToBoundary:
//-------------------------
/**
 * \brief determines if a node is close to a unconnected node in the connectivity graph of a SparseMatrix.
 * \param A (in) Matrix A
 * \param node (in) the node where to start
 * \param distance (in) up to which distance "close" is.
 * \return if there is a distance long path in graph(A) to an unconnected node, true. otherwise false.
  */
template<typename T>
bool IsCloseToBoundary(const SparseMatrix<T> &A, size_t node, size_t distance)
{
	if(distance == 0) return A.is_isolated(node);
	bool bFound = false;
	for(typename SparseMatrix<T>::const_row_iterator itA = A.begin_row(node); itA != A.end_row(node) && !bFound; ++itA)
		bFound = IsCloseToBoundary(A, itA.index(), distance-1);

	return bFound;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetDirichletRow:
//-------------------------
/**
 * set Dirichlet row for entry (i,alpha).
 * \param A (in) Matrix A
 * \param i (in) row to set dirichlet, that is A(i,i)(alpha, alpha) = 1.0, A(i,k)(alpha, beta) = 0.0 for all (k, beta) != (i, alpha)$.
 * \param alpha the alpha index
 */
template <typename T>
void SetDirichletRow(SparseMatrix<T>& A, size_t i, size_t alpha)
{
	BlockRef(A(i,i), alpha, alpha) = 1.0;
	for(typename SparseMatrix<T>::row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
	{
		typename SparseMatrix<T>::value_type& block = conn.value();
		for(size_t beta = 0; beta < (size_t) GetCols(block); ++beta)
		{
			if(conn.index() != i) BlockRef(block, alpha, beta) = 0.0;
			else if(beta != alpha) BlockRef(block, alpha, beta) = 0.0;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetDirichletRow:
//-------------------------
/**
 * set Dirichlet row for block i.
 * \param A (in) Matrix A
 * \param i (in) row to set dirichlet, that is A(i,i) = 1.0, A(i,k) = 0.0 for all k != i.
 */
template <typename T>
void SetDirichletRow(SparseMatrix<T>& A, size_t i)
{
	A(i,i) = 1.0;
	for(typename SparseMatrix<T>::row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
	{
		typename SparseMatrix<T>::value_type& block = conn.value();
		if(conn.index() != i) block = 0.0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetDirichletRow:
//-------------------------
/**
 * set Dirichlet row for block i.
 * \param[in/out] A Matrix A
 * \param[in] vIndex vector of row indices to set dirichlet, that is A(i,i) = 1.0, A(i,k) = 0.0 for all k != i.
 */
template <typename T>
void SetDirichletRow(SparseMatrix<T>& A, const std::vector<size_t> vIndex)
{
	std::vector<size_t>::const_iterator iter = vIndex.begin();
	std::vector<size_t>::const_iterator iterEnd = vIndex.end();

	for(; iter < iterEnd; ++iter)
	{
		const size_t i = *iter;
		UG_ASSERT(i < A.num_rows(), "Index to large in index set.");

		A(i,i) = 1.0;
		for(typename SparseMatrix<T>::row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
		{
			typename SparseMatrix<T>::value_type& block = conn.value();
			if(conn.index() != i) block = 0.0;
		}
	}
}


template<typename T, class TOStream>
void Serialize(TOStream &buf, const SparseMatrix<T> &A)
{
	Serialize(buf, A.num_rows());
	Serialize(buf, A.num_cols());

	for(size_t i=0; i < A.num_rows(); i++)
	{
		size_t num_connections = A.num_connections(i);

		// serialize number of connections
		Serialize(buf, num_connections);

		for(typename SparseMatrix<T>::const_row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
		{
			// serialize connection
			Serialize(buf, conn.index());
			Serialize(buf, conn.value());
		}
	}
}

template <typename T, class TIStream>
void Deserialize(TIStream& buf, SparseMatrix<T> &A)
{
	size_t numRows, numCols, num_connections;

	Deserialize(buf, numRows);
	Deserialize(buf, numCols);
	A.resize(numRows, numCols);

	std::vector<typename SparseMatrix<T>::connection> con; con.reserve(16);

	for(size_t i=0; i < A.num_rows; i++)
	{
		Deserialize(buf, num_connections);

		con.resize(num_connections);

		for(size_t j=0; j<num_connections; j++)
		{
			Deserialize(buf, con[j].iIndex);
			Deserialize(buf, con[j].dValue);
		}
		A.set_matrix_row(i, &con[0], num_connections);
	}
	A.defragment();
}


/// @}
} // end namespace ug


#endif
