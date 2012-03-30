/*
 * grid_function_util.h
 *
 *  Created on: 17.08.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_UTIL__
#define __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_UTIL__

#include <boost/function.hpp>
#include "../../common/os_dependent/file_util.h"

#include "./grid_function.h"
#include "lib_algebra/cpu_algebra/sparsematrix_print.h"
#include "lib_algebra/operator/interface/operator.h"
#include "lib_algebra/operator/debug_writer.h"
#include "lib_algebra/operator/vector_writer.h"
#include "lib_disc/io/vtkoutput.h"
#include "lib_disc/spatial_disc/constraints/constraint_interface.h"
#include "lib_disc/dof_manager/mg_dof_distribution.h"
#include <vector>
#include <string>
#include "lib_algebra/common/connection_viewer_output.h"
#include "lib_algebra/common/csv_gnuplot_output.h"

#ifdef UG_PARALLEL
#include "pcl/pcl.h"
#endif

namespace ug {

template<typename TDomain, typename TDD>
void ExtractPositions(ConstSmartPtr<TDomain> domain, ConstSmartPtr<TDD> dd,
		std::vector<MathVector<TDomain::dim> >& vPos);

template<typename TFunction>
void ExtractPositions(const TFunction &u,
		std::vector<MathVector<TFunction::domain_type::dim> >& vPos) {
	typedef typename TFunction::domain_type domain_type;
	typedef typename TFunction::dof_distribution_type dof_distribution_type;
	ExtractPositions<domain_type, dof_distribution_type>(u.domain(),
			u.dof_distribution(), vPos);
}

template<class TFunction>
void WriteMatrixToConnectionViewer(const char *filename,
		const typename TFunction::algebra_type::matrix_type &A,
		const TFunction &u) {

	PROFILE_FUNC();
//	check name
	std::string name(filename);
	size_t iExtPos = name.find_last_of(".");
	if (iExtPos == std::string::npos
			|| name.substr(iExtPos).compare(".mat") != 0)
		UG_THROW_FATAL("Only '.mat' format supported for matrices, but"
		" filename is '"<<name<<"'.");

//	position array
	const static int dim = TFunction::domain_type::dim;
	std::vector<MathVector<dim> > vPos;
	ExtractPositions(u, vPos);

// 	write matrix
	WriteMatrixToConnectionViewer(name.c_str(), A, &vPos[0], dim);
}

template<typename TGridFunction>
void SaveMatrixForConnectionViewer(
		TGridFunction& u,
		MatrixOperator<typename TGridFunction::algebra_type::matrix_type,
				typename TGridFunction::vector_type>& A,
		const char* filename) {
	PROFILE_FUNC();
//	forward
	WriteMatrixToConnectionViewer(filename, A.get_matrix(), u);
}

template<class TFunction>
void WriteVectorToConnectionViewer(const char *filename,
		const typename TFunction::algebra_type::vector_type &b,
		const TFunction &u) {
	PROFILE_FUNC();
//	check name
	std::string name(filename);
	size_t iExtPos = name.find_last_of(".");
	if (iExtPos == std::string::npos
			|| name.substr(iExtPos).compare(".vec") != 0)
		UG_THROW_FATAL("Only '.vec' format supported for vectors, but"
		" filename is '"<<name<<"'.");

// 	get positions of vertices
	const static int dim = TFunction::domain_type::dim;
	std::vector<MathVector<dim> > vPos;
	ExtractPositions(u, vPos);

//	write vector
	WriteVectorToConnectionViewer(name.c_str(), b, &vPos[0], dim);
}

template<class TFunction>
bool WriteVectorToConnectionViewer(
		const char *filename,
		const typename TFunction::algebra_type::matrix_type &A,
		const typename TFunction::algebra_type::vector_type &b,
		const TFunction &u,
		const typename TFunction::algebra_type::vector_type *pCompareVec = NULL) {
	PROFILE_FUNC();
//	get dimension
	const static int dim = TFunction::domain_type::dim;

//	check name

	std::string name(filename);
	size_t iExtPos = name.find_last_of(".");
	if (iExtPos == std::string::npos
			|| name.substr(iExtPos).compare(".vec") != 0) {
		UG_LOG("Only '.vec' format supported for vectors.\n");
		return false;
	}

// 	get positions of vertices
	std::vector<MathVector<dim> > positions;
	ExtractPositions(u, positions);

//	write vector
	WriteVectorToConnectionViewer(name.c_str(), A, b, &positions[0], dim,
			pCompareVec);

//	we're done
	return true;
}

template<typename TGridFunction>
void SaveVectorForConnectionViewer(TGridFunction& b, const char* filename) {
	PROFILE_FUNC();
	WriteVectorToConnectionViewer(filename, b, b);
}

template<typename TGridFunction>
bool SaveVectorForConnectionViewer(
		TGridFunction& u,
		MatrixOperator<typename TGridFunction::algebra_type::matrix_type,
				typename TGridFunction::vector_type>& A,
		const char* filename) {
	PROFILE_FUNC();
	return WriteVectorToConnectionViewer(filename, A.get_matrix(), u, u);
}

template<typename TGridFunction>
bool SaveVectorForConnectionViewer(
		TGridFunction& u,
		TGridFunction& compareVec,
		MatrixOperator<typename TGridFunction::algebra_type::matrix_type,
				typename TGridFunction::vector_type>& A,
		const char* filename) {
//	forward
	PROFILE_FUNC();
	return WriteVectorToConnectionViewer(filename, A.get_matrix(), u, u,
			&compareVec);
}

// Same as before, but for comma separated value (CSV)
template<class TFunction>
bool WriteVectorCSV(const char *filename,
		const typename TFunction::algebra_type::vector_type &b,
		const TFunction &u) {
	PROFILE_FUNC();
//	get dimension
	const static int dim = TFunction::domain_type::dim;

//	check name
	std::string name(filename);
	size_t iExtPos = name.find_last_of(".");
	if (iExtPos == std::string::npos
			|| name.substr(iExtPos).compare(".csv") != 0) {
		UG_LOG("Only '.csv' format supported for vectors.\n");
		return false;
	}

//	extended filename
//	add p000X extension in parallel
#ifdef UG_PARALLEL
	name.resize(iExtPos);
	int rank = pcl::GetProcRank();
	char ext[20];
	sprintf(ext, "_p%05d.csv", rank);
	name.append(ext);
#endif

// 	get positions of vertices
	std::vector<MathVector<dim> > positions;
	ExtractPositions(u, positions);

//	write vector
	WriteVectorCSV(name.c_str(), b, &positions[0], dim);

//	we're done
	return true;
}

template<typename TGridFunction>
bool SaveVectorCSV(TGridFunction& b, const char* filename) {
	PROFILE_FUNC();
	return WriteVectorCSV(filename, b, b);
}

template<typename TDomain, typename TAlgebra>
class GridFunctionDebugWriter: public IDebugWriter<TAlgebra> {
	//	dimension
	static const int dim = TDomain::dim;

	//  base directory for output
	std::string m_baseDir;

public:
	///	type of matrix
	typedef TAlgebra algebra_type;

	///	type of vector
	typedef typename algebra_type::vector_type vector_type;

	///	type of matrix
	typedef typename algebra_type::matrix_type matrix_type;

	///	type of approximation space
	typedef ApproximationSpace<TDomain> approximation_space_type;

public:
	///	Constructor
	GridFunctionDebugWriter(
			SmartPtr<ApproximationSpace<TDomain> > spApproxSpace) :
			m_baseDir("."), m_spApproxSpace(spApproxSpace), bConnViewerOut(
					true), bVTKOut(true), m_printConsistent(true) {
		reset();
	}


	///	sets the function
	void set_grid_level(const GridLevel& gridLevel) {
		m_gridLevel = gridLevel;
	}

	///	returns current grid level
	GridLevel grid_level() const {
		return m_gridLevel;
	}

	///	sets to toplevel on surface
	void reset() {
		set_grid_level(GridLevel(GridLevel::TOPLEVEL, GridLevel::SURFACE));

	}

	// set the base directory for output files (.vec and .mat)
	inline void set_base_dir(const char* const baseDir) {
		m_baseDir = std::string(baseDir);
	}

	//	sets if writing to vtk
	void set_vtk_output(bool b) {
		bVTKOut = b;
	}

	//	sets if writing to conn viewer
	void set_conn_viewer_output(bool b) {
		bConnViewerOut = b;
	}

	//	sets if data shall be made consistent before printing
	void set_print_consistent(bool b) {
		m_printConsistent = b;
	}

	///	write vector
	virtual void write_vector(const vector_type& vec, const char* filename) {
		//	write to conn viewer
		if (bConnViewerOut)
			write_vector_to_conn_viewer(vec, filename);

		//	write to vtk
		if (bVTKOut)
			write_vector_to_vtk(vec, filename);
	}

	///	write matrix
	virtual void write_matrix(const matrix_type& mat, const char* filename) {
		//	something to do ?
		if (!bConnViewerOut)
			return;

		std::string name(m_baseDir); name.append("/").append(filename);

		if (!FileExists(m_baseDir.c_str())) {
			UG_WARNING("GridFunctionDebugWriter::write_matrix: directory "
						<< m_baseDir << "does not exist.");
			UG_WARNING("GridFunctionDebugWriter::write_matrix: using cwd "
						"as basedir.");
			name = "./"; name.append(filename);
		}

		size_t iExtPos = name.find_last_of(".");
		if (iExtPos == std::string::npos
				|| name.substr(iExtPos).compare(".mat") != 0)
			UG_THROW_FATAL("Only '.mat' format supported for matrices, but"
			" filename is '"<<name<<"'.");

		//	write to file
		extract_positions(m_gridLevel);
		std::vector<MathVector<dim> >& vPos =
				this->template get_positions<dim>();
		WriteMatrixToConnectionViewer(name.c_str(), mat, &vPos[0], dim);
	}

protected:
	///	reads the positions
	void extract_positions(const GridLevel& gridLevel) {
		//	extract positions for this grid function
		std::vector<MathVector<dim> >& vPos =
				this->template get_positions<dim>();

		vPos.clear();

		if (gridLevel.type() == GridLevel::SURFACE) {
			ExtractPositions<TDomain, SurfaceDoFDistribution>(
					m_spApproxSpace->domain(),
					m_spApproxSpace->surface_dof_distribution(
							gridLevel.level()), vPos);
		} else if (gridLevel.type() == GridLevel::LEVEL) {
			ExtractPositions<TDomain, LevelDoFDistribution>(
					m_spApproxSpace->domain(),
					m_spApproxSpace->level_dof_distribution(gridLevel.level()),
					vPos);
		} else {
			UG_THROW_FATAL("Cannot find grid level");
		}
	}

	///	write vector
	virtual void write_vector_to_conn_viewer(const vector_type& vec,
			const char* filename) {
		std::string name(m_baseDir); name.append("/").append(filename);

		if (!FileExists(m_baseDir.c_str())) {
			UG_WARNING("GridFunctionDebugWriter::write_vector_to_conn_viewer: directory "
						<< m_baseDir << "does not exist.");
			UG_WARNING("GridFunctionDebugWriter::write_vector_to_conn_viewer: using cwd "
						"as basedir.");
			name = "./"; name.append(filename);
		}

		size_t iExtPos = name.find_last_of(".");
		if (iExtPos == std::string::npos
				|| name.substr(iExtPos).compare(".vec") != 0)
			UG_THROW_FATAL("Only '.vec' format supported for vectors, but"
			" filename is '"<<name<<"'.");

		//	write
		extract_positions(m_gridLevel);
		std::vector<MathVector<dim> >& vPos =
				this->template get_positions<dim>();
		WriteVectorToConnectionViewer(name.c_str(), vec, &vPos[0], dim);
	}

	void write_vector_to_vtk(const vector_type& vec, const char* filename) {
		//	check name
		std::string name(m_baseDir); name.append("/").append(filename);

		if (!FileExists(m_baseDir.c_str())) {
			UG_WARNING("GridFunctionDebugWriter::write_vector_to_vtk: directory "
						<< m_baseDir << "does not exist.");
			UG_WARNING("GridFunctionDebugWriter::write_vector_to_vtk: using cwd "
						"as basedir.");
			name = "./"; name.append(filename);
		}

		if (m_gridLevel.type() == GridLevel::LEVEL) {
			typedef GridFunction<TDomain, LevelDoFDistribution, TAlgebra> TGridFunction;
			TGridFunction vtkFunc(
					m_spApproxSpace,
					m_spApproxSpace->level_dof_distribution(
							m_gridLevel.level()));
			vtkFunc.assign(vec);
			VTKOutput<TGridFunction> out;
			out.print(filename, vtkFunc, m_printConsistent);
		} else if (m_gridLevel.type() == GridLevel::SURFACE) {
			typedef GridFunction<TDomain, SurfaceDoFDistribution, TAlgebra> TGridFunction;
			TGridFunction vtkFunc(
					m_spApproxSpace,
					m_spApproxSpace->surface_dof_distribution(
							m_gridLevel.level()));
			vtkFunc.assign(vec);
			VTKOutput<TGridFunction> out;
			out.print(filename, vtkFunc, m_printConsistent);
		} else
			UG_THROW_FATAL("Cannot find grid level.");
	}

protected:
	//	underlying approximation space
	SmartPtr<approximation_space_type> m_spApproxSpace;

	//	flag if write to conn viewer
	bool bConnViewerOut;

	//	flag if write to vtk
	bool bVTKOut;

	//	flag if data shall be made consistent before printing
	bool m_printConsistent;

	//	current grid level
	GridLevel m_gridLevel;

};

template<typename TGridFunction>
class GridFunctionPositionProvider: public IPositionProvider<
		TGridFunction::domain_type::dim> {
public:
	///	Constructor
	GridFunctionPositionProvider() :
			m_pGridFunc(NULL) {
	}

	void set_reference_grid_function(const TGridFunction& u) {
		m_pGridFunc = &u;
	}

	virtual bool get_positions(
			std::vector<MathVector<TGridFunction::domain_type::dim> >&vec) {
		UG_ASSERT(m_pGridFunc != NULL,
				"provide a grid function with set_reference_grid_function");
		ExtractPositions(*m_pGridFunc, vec);
		return true;
	}

protected:
	const TGridFunction *m_pGridFunc;
};

template<typename TGridFunction, typename TVector>
class GridFunctionVectorWriter: public IVectorWriter<TVector> {
public:
	typedef typename TVector::value_type value_type;
	typedef typename TGridFunction::domain_type domain_type;
	typedef TVector vector_type;
	typedef boost::function<
			void(number& value, const MathVector<domain_type::dim>& x,
					number time)> NumberFunctor;

public:
	///	Constructor
	GridFunctionVectorWriter() :
			m_pGridFunc(NULL) {
	}

	void set_user_data(const NumberFunctor& userData) {
		m_userData = userData;
	}

	void set_reference_grid_function(const TGridFunction& u) {
		m_pGridFunc = &u;
	}

	/*virtual double calculate(MathVector<3> &v, double time)
	 {
	 }*/

	virtual bool update(vector_type &vec) {
		UG_ASSERT(m_pGridFunc != NULL,
				"provide a grid function with set_reference_grid_function");
		UG_ASSERT(m_userData != NULL, "provide user data with set_user_data");

		const TGridFunction &u = *m_pGridFunc;

		//	get position accessor

		const typename domain_type::position_accessor_type& aaPos =
				u.domain()->position_accessor();

		//	number of total dofs
		int nr = u.num_indices();

		//	resize positions
		vec.resize(nr);

		typedef typename TGridFunction::template traits<VertexBase>::const_iterator const_iterator;

		//	loop all subsets
		for (int si = 0; si < u.num_subsets(); ++si) {
			//	loop all vertices
			const_iterator iter = u.template begin<VertexBase>(si);
			const_iterator iterEnd = u.template end<VertexBase>(si);

			for (; iter != iterEnd; ++iter) {
				//	get vertex
				VertexBase* v = *iter;

				//	algebra indices vector
				std::vector<size_t> ind;

				//	load indices associated with vertex
				u.inner_algebra_indices(v, ind);

				number t = 0.0;

				number d;
				m_userData(d, aaPos[v], t);

				//	write
				for (size_t i = 0; i < ind.size(); ++i) {
					const size_t index = ind[i];
					for (size_t alpha = 0; alpha < GetSize(vec[index]); ++alpha)
						BlockRef(vec[index], alpha) = d;
				}
			}
		}
		return true;
	}

protected:
	const TGridFunction *m_pGridFunc;
	NumberFunctor m_userData;
};

template<typename TGridFunction>
class GridFunctionVectorWriterDirichlet0: public IVectorWriter<
		typename TGridFunction::algebra_type::vector_type> {
public:
	typedef typename TGridFunction::domain_type domain_type;

	typedef typename TGridFunction::approximation_space_type approximation_space_type;

	typedef typename TGridFunction::algebra_type algebra_type;
	typedef typename algebra_type::vector_type vector_type;
	typedef typename vector_type::value_type value_type;

public:
	///	Constructor
	GridFunctionVectorWriterDirichlet0() :
			m_pApproxSpace(NULL), m_spPostProcess(NULL), m_level(-1) {
	}

	void set_level(size_t level) {
		m_level = level;
	}

	void init(SmartPtr<IConstraint<algebra_type> > pp,
			approximation_space_type& approxSpace) {
		m_spPostProcess = pp;
		m_pApproxSpace = &approxSpace;
	}

	/*virtual double calculate(MathVector<3> &v, double time)
	 {
	 }*/

	virtual bool update(vector_type &vec) {
		UG_ASSERT(m_spPostProcess.valid(), "provide a post process with init");
		UG_ASSERT(m_pApproxSpace != NULL, "provide approximation space init");

		size_t numDoFs = 0;
		GridLevel gl;
		if (m_level == (size_t) -1) {
			numDoFs = m_pApproxSpace->surface_dof_distribution()->num_indices();
			gl = GridLevel();
		} else {
			numDoFs =
					m_pApproxSpace->level_dof_distribution(m_level)->num_indices();
			gl = GridLevel(m_level, GridLevel::LEVEL);
		}
		vec.resize(numDoFs);
		vec.set(1.0);

		m_spPostProcess->adjust_defect(vec, vec, gl);

		return true;
	}

protected:
	approximation_space_type * m_pApproxSpace;
	SmartPtr<IConstraint<algebra_type> > m_spPostProcess;
	size_t m_level;
};

} // end namespace ug

#endif /* __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_UTIL__ */
