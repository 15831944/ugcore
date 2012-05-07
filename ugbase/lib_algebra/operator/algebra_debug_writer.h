/*
 * algebra_debug_writer.h
 *
 *  Created on: 18.01.2011
 *      Author: andreasvogel
 */

#ifndef __H__LIB_ALGEBRA__OPERATOR__ALGEBRA_DEBUG_WRITER__
#define __H__LIB_ALGEBRA__OPERATOR__ALGEBRA_DEBUG_WRITER__

#include "debug_writer.h"
#include "lib_algebra/common/connection_viewer_output.h"

namespace ug{

template <typename TAlgebra, int dim>
class AlgebraDebugWriter
	: public IDebugWriter<TAlgebra>
{
	public:
	///	type of matrix
		typedef TAlgebra algebra_type;

	///	type of vector
		typedef typename algebra_type::vector_type vector_type;

	///	type of matrix
		typedef typename algebra_type::matrix_type matrix_type;

	///	type of positions
		typedef MathVector<dim> position_type;

	public:
	///	Constructor
		AlgebraDebugWriter() : m_pPositions(NULL),  m_numPos(-1)
		{}

	///	sets the function
		void set_positions(position_type* pos, int n)
		{
			m_pPositions = pos;
			m_numPos = n;
		}

	///	write vector
		virtual void write_vector(const vector_type& vec,
		                          const char* filename)
		{
		//	check
			if(m_pPositions == NULL)
				UG_THROW("'AlgebraDebugWriter::write_vector':"
						" No reference positions set.\n");

		//	check number of positions
			if(vec.size() != (size_t)m_numPos)
				UG_THROW("'AlgebraDebugWriter::write_vector':"
						" Number of positions does not match.\n");

		//	get fresh name
			std::string name(filename);

		//	search for ending and remove
			size_t found = name.find_first_of(".");
			if(found != std::string::npos) name.resize(found);

		#ifdef UG_PARALLEL
		//	add process number
			int rank = pcl::GetProcRank();
			char ext[20];
			sprintf(ext, "_p%04d", rank);
			name.append(ext);
		#endif

		//	add ending
			name.append(".vec");

		//	write to file
			WriteVectorToConnectionViewer<vector_type, position_type>
				(name.c_str(), vec, m_pPositions, dim);
		}

	///	write matrix
		virtual void write_matrix(const matrix_type& mat,
		                          const char* filename)
		{
		//	check
			if(m_pPositions == NULL)
				UG_THROW("'AlgebraDebugWriter::write_matrix':"
						" No reference positions set.\n");

		//	check number of positions
			if(mat.num_rows() != (size_t)m_numPos || mat.num_cols() != (size_t)m_numPos)
				UG_THROW("'AlgebraDebugWriter::write_matrix':"
						" Number of positions does not match.\n");

		//	check name
			std::string name(filename);
			size_t iExtPos = name.find_last_of(".");
			if(iExtPos == std::string::npos || name.substr(iExtPos).compare(".mat") != 0)
				UG_THROW("Only '.mat' format supported for matrices, but"
								" filename is '"<<name<<"'.");

		//	write to file
			WriteMatrixToConnectionViewer<matrix_type, position_type>
				(name.c_str(), mat, m_pPositions, dim);
		}

	protected:
	//	Positions used in output
		position_type* m_pPositions;

	//	number of positions
		int m_numPos;
};

} // end namespace ug

#endif /* __H__LIB_ALGEBRA__OPERATOR__ALGEBRA_DEBUG_WRITER__ */
