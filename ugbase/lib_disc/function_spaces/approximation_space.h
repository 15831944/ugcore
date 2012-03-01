/*
 * approximation_space.h
 *
 *  Created on: 19.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE__
#define __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE__

#ifdef UG_PARALLEL
	#include "lib_disc/parallelization/parallelization.h"
#endif

#include "lib_disc/dof_manager/mg_dof_distribution.h"
#include "lib_disc/dof_manager/level_dof_distribution.h"
#include "lib_disc/dof_manager/surface_dof_distribution.h"
#include "grid_function.h"

namespace ug{

/// describes the ansatz spaces on a domain
/**
 * This class provides grid function spaces on a domain.
 *
 * The Domain defines a partition of the Grid/Multigrid in terms of subsets.
 * The user can add discrete functions on this subsets or unions of them.
 *
 * Once finalized, this function pattern is fixed. Internally DoF indices are
 * created. Using this Approximation Space the user can create GridFunctions
 * of the following types:
 *
 * - surface grid function = grid function representing the space on the surface grid
 * - level grid function = grid function representing the space on a level grid
 *  (NOTE: 	For a fully refined Multigrid a level grid covers the whole domain.
 *  		However for a locally/adaptively refined MultiGrid the level grid
 *  		solution is only living on a part of the domain)
 */
class IApproximationSpace
{
	public:
	///	Type of Subset Handler
		typedef MGSubsetHandler subset_handler_type;

	public:
	///	Constructor
		IApproximationSpace(SmartPtr<subset_handler_type> spMGSH);

	///	Constructor setting the grouping flag
		IApproximationSpace(SmartPtr<subset_handler_type> spMGSH, bool bGroup);

	///	clears all functions
		void clear() {m_spFunctionPattern->clear();}

	///	adds function using string to indicate finite element type
		void add_fct(const char* name, const char* type, int order)
			{m_spFunctionPattern->add_fct(name, type, order);}

	///	adds function using string to indicate finite element type
		void add_fct(const char* name, const char* type,
							 int order, const char* subsets)
			{m_spFunctionPattern->add_fct(name, type, order, subsets);}

	/// get underlying subset handler
		ConstSmartPtr<MGSubsetHandler> subset_handler() const {return m_spMGSH;}

	///	returns the function pattern
		ConstSmartPtr<FunctionPattern> function_pattern() const {return m_spFunctionPattern;}

	///	returns the number of level
		size_t num_levels() const {return m_spMGSH->num_levels();}

	///	returns if dofs are grouped
		bool grouped() const {return m_bGrouped;}

	///	returns the level dof distributions
		std::vector<ConstSmartPtr<SurfaceDoFDistribution> >	surface_dof_distributions() const;

	///	returns the level dof distribution
		SmartPtr<SurfaceDoFDistribution> surface_dof_distribution(int level = GridLevel::TOPLEVEL);

	///	returns the level dof distribution
		ConstSmartPtr<SurfaceDoFDistribution> surface_dof_distribution(int level = GridLevel::TOPLEVEL) const;

	///	returns the surface view
		ConstSmartPtr<SurfaceView> surface_view() const {return m_spSurfaceView;}

	///	returns the level dof distributions
		std::vector<ConstSmartPtr<LevelDoFDistribution> > level_dof_distributions() const;

	///	returns the level dof distribution
		SmartPtr<LevelDoFDistribution> level_dof_distribution(int level);

	///	returns the level dof distribution
		ConstSmartPtr<LevelDoFDistribution> level_dof_distribution(int level) const;

	///	prints statistic about DoF Distribution
		void print_statistic(int verboseLev = 1) const;

	///	prints statistic about DoF Distribution
		void print_statistic() const {print_statistic(1);}

	///	prints statistic on layouts
		void print_layout_statistic(int verboseLev = 1) const;

	///	prints statistic on layouts
		void print_layout_statistic() const {print_layout_statistic(1);}

	///	prints statistic on local dof distribution
		void print_local_dof_statistic(int verboseLev = 1) const;

	///	prints statistic on local dof distribution
		void print_local_dof_statistic() const {print_local_dof_statistic(1);}

	///	initializes all level dof distributions
		void init_level();

	///	initializes all surface dof distributions
		void init_surface();

		// \todo: should be removed an handled automatically
	///	defragments the index set of the DoF Distribution
		void defragment();

	protected:
	///	creates level DoFDistribution if needed
		void level_dd_required(size_t fromLevel, size_t toLevel);

	///	creates surface DoFDistribution if needed
		void surf_dd_required(size_t fromLevel, size_t toLevel);

	///	creates surface DoFDistribution if needed
		void top_surf_dd_required();

	///	creates the surface view and the surface level views requested
		void surface_level_view_required(size_t fromLevel, size_t toLevel);

	///	creates the surface view and the surface level views requested
		void top_surface_level_view_required();

	protected:
	///	print info about local dof statistic for a DoFDistribution
		void print_local_dof_statistic(ConstSmartPtr<MGDoFDistribution> dd, int verboseLev) const;

	///	prints number of dofs
		template <typename TDD>
		void print_statistic(ConstSmartPtr<TDD> dd, int verboseLev) const;

	///	prints statistic about DoF Distribution
		template <typename TDD>
		void print_parallel_statistic(ConstSmartPtr<TDD> dd, int verboseLev) const;

	///	sets the distributed grid manager
#ifdef UG_PARALLEL
		void set_dist_grid_mgr(DistributedGridManager* pDistGrdMgr) {m_pDistGridMgr = pDistGrdMgr;}
#endif

	protected:
	/// grid or multigrid or subsethandler, where elements are stored
		SmartPtr<MGSubsetHandler> m_spMGSH;

	///	function pattern
		SmartPtr<FunctionPattern> m_spFunctionPattern;

	///	flag if DoFs should be grouped
		bool m_bGrouped;

	///	MG Level DoF Distribution
		SmartPtr<LevelMGDoFDistribution> m_spLevMGDD;

	///	all Level DoF Distributions
		std::vector<SmartPtr<LevelDoFDistribution> > m_vLevDD;

	///	all Surface DoF Distributions
		std::vector<SmartPtr<SurfaceDoFDistribution> > m_vSurfDD;

	///	top Surface DoF Distributions
		SmartPtr<SurfaceDoFDistribution> m_spTopSurfDD;

	///	Surface View
		SmartPtr<SurfaceView> m_spSurfaceView;

	///	all Surface Level Views
		std::vector<SmartPtr<SurfaceLevelView> > m_vSurfLevView;

	///	top surface level view
		SmartPtr<SurfaceLevelView> m_spTopSurfLevView;

#ifdef UG_PARALLEL
	///	Pointer to GridManager
		DistributedGridManager* m_pDistGridMgr;
#endif
};

/// base class for approximation spaces without type of algebra or dof distribution
template <typename TDomain>
class ApproximationSpace : public IApproximationSpace
{
	public:
	///	Domain type
		typedef TDomain domain_type;

	///	World Dimension
		static const int dim = domain_type::dim;

	///	Subset Handler type
		typedef typename domain_type::subset_handler_type subset_handler_type;

	public:
	/// constructor
		ApproximationSpace(SmartPtr<TDomain> domain);

	/// Return the domain
		ConstSmartPtr<TDomain> domain() const {return m_spDomain;}

	///	Return the domain
		SmartPtr<TDomain> domain() {return m_spDomain;}

	protected:
	///	Domain, where solution lives
		SmartPtr<TDomain> m_spDomain;
};


} // end namespace ug

#endif /* __H__UG__LIB_DISC__FUNCTION_SPACE__APPROXIMATION_SPACE__ */
