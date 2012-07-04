/*
 * data_evaluator.h
 *
 *  Created on: 17.12.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__DATA_EVALUATOR__
#define __H__UG__LIB_DISC__SPATIAL_DISC__DATA_EVALUATOR__

#include "lib_disc/spatial_disc/elem_disc/elem_disc_interface.h"
#include "data_import_export.h"

namespace ug{

/// helper class to evaluate data evaluation for local contributions during assembling
/**
 * This class is a helper class to facilitate the correct evaluation of data
 * during the assembling process. For a given set of IElemDisc this class
 * prepares the assembling, checks for DataImport/DataExports and schedules the
 * evaluation of the data in the correct order. In addition the coupling
 * contributions due to Import/Exports are computed by this class and can be
 * added to the local jacobian/defect.
 */
class DataEvaluator
{
	public:
	///	sets the elem discs to evaluate
		void set_elem_discs(const std::vector<IElemDisc*>& vElemDisc,
		                    const FunctionPattern& fctPat);

	///	sets the subset for data evaluation
		void set_subset(const int subset);

	///	sets in all IElemDiscs the time and previous solutions
		void set_time_dependent(bool bTimeDep,
		                        LocalVectorTimeSeries* locTimeSeries = NULL);

	///	sets the time point for data evaluation
		void set_time(const number time);

	///	returns if local time series is really needed for assembling
		bool time_series_needed() const {return m_bNeedLocTimeSeries;}

	///	requests in all IElemDisc to use HangingGrid
		void set_non_regular_grid(bool bNonRegularGrid);

	///	returns if one of the element discs needs hanging dofs
		bool use_hanging() const {return m_bUseHanging;}

		////////////////////////////////////////////
		// Regular assembling
		////////////////////////////////////////////

	///	prepares the element for all time-dependent IElemDiscs
		template <typename TElem>
		void prepare_timestep_elem(TElem* elem, LocalVector& u);

	///	prepares the element loop for all IElemDiscs
		template <typename TElem>
		void prepare_elem_loop(bool bMassPart = false);

	///	prepares the element for all IElemDiscs
		template <typename TElem>
		void prepare_elem(TElem* elem, LocalVector& u,
		                  const LocalIndices& ind,
		                  bool bDeriv = false, bool bMassPart = false);

	///	finishes the element for all time-dependent IElemDiscs
		template <typename TElem>
		void finish_timestep_elem(TElem* elem, const number time, LocalVector& u);

	///	computes all needed data on the element
		void compute_elem_data(LocalVector & u, bool bDeriv = false);

	///	compute local stiffness matrix for all IElemDiscs
		void ass_JA_elem(LocalMatrix& A, LocalVector& u);

	///	compute local mass matrix for all IElemDiscs
		void ass_JM_elem(LocalMatrix& M, LocalVector& u);

	///	compute local stiffness defect for all IElemDiscs
		void ass_dA_elem(LocalVector& d, LocalVector& u);

	///	compute local mass defect for all IElemDiscs
		void ass_dM_elem(LocalVector& d, LocalVector& u);

	///	compute local rhs for all IElemDiscs
		void ass_rhs_elem(LocalVector& rhs);

	///	finishes the element loop for all IElemDiscs
		void finish_elem_loop();

		////////////////////////////////////////////
		// Coupling
		////////////////////////////////////////////

	///	computes the linearized defect of imports in stiffness part of all IElemDiscs
		void compute_lin_defect_JA(LocalVector& u);

	///	computes the linearized defect of imports in mass part of all IElemDiscs
		void compute_lin_defect_JM(LocalVector& u);

	///	adds the contribution due to coupling to local stiffness matrix
		void add_coupl_JA(LocalMatrix& J);

	///	adds the contribution due to coupling to local mass matrix
		void add_coupl_JM(LocalMatrix& J);

	protected:
	///	Mapping between local functions of ElemDisc i and common FunctionGroup
		const FunctionIndexMapping& map(size_t i) const
		{
			UG_ASSERT(i < m_vElemDiscMap.size(), "Wrong index");
			return m_vElemDiscMap[i];
		}

	///	returns common function group of all needed functions
		const FunctionGroup& function_group() const {return m_commonFctGroup;}

	///	clears imports and ip data and mappings betweem commonFctGrp and local
		void clear_extracted_data_and_mappings();

	///	tries to add the last entry of vTryingToAdd to the eval data
		void add_data_to_eval_data(std::vector<SmartPtr<IIPData> >& vEvalData,
								   std::vector<SmartPtr<IIPData> >& vTryingToAdd);

	///	extracts imports and ipdata from IElemDiscs
		void extract_imports_and_ipdata(bool bMassPart = false);

	protected:
	///	current elem discs
		const std::vector<IElemDisc*>* m_pvElemDisc;

	///	common function group (all function of function pattern)
		FunctionGroup m_commonFctGroup;

	///	function group for all elem discs
		std::vector<FunctionGroup> m_vElemDiscFctGrp;

	///	Function mapping for each disc
		std::vector<FunctionIndexMapping> m_vElemDiscMap;

	///	flag if hanging nodes are used
		bool m_bUseHanging;

	///	flag indicating if elem disc needs local time series
		std::vector<bool> m_vbNeedLocTimeSeries;

	///	flag indicating if any elem disc needs local time series
		bool m_bNeedLocTimeSeries;

	///	local time series
		LocalVectorTimeSeries* m_pLocTimeSeries;

	////////////////////////////////
	// 	Data Import
	////////////////////////////////
	///	all data imports of elem discs
		std::vector<IDataImport*> m_vAllDataImport;
	///	data imports which are connected to non-zero derivative ip data in mass part
		std::vector<IDataImport*> m_vMassDataImport;
	///	data imports which are connected to non-zero derivative ip data in stiffness part
		std::vector<IDataImport*> m_vStiffDataImport;

	///	Function mapping for import (separated in stiff and mass part)
		std::vector<FunctionIndexMapping> m_vMassImpMap;
		std::vector<FunctionIndexMapping> m_vStiffImpMap;

	///	Function mapping data connected to the import
		std::vector<FunctionIndexMapping> m_vMassImpConnMap;
		std::vector<FunctionIndexMapping> m_vStiffImpConnMap;

	////////////////////////////////
	// 	IP Data
	////////////////////////////////

	///	constant data
		std::vector<SmartPtr<IIPData> > m_vConstData;

	///	position dependent data
		std::vector<SmartPtr<IIPData> > m_vPosData;

	///	dependent data
		std::vector<SmartPtr<IIPData> > m_vDependentIPData;
		std::vector<FunctionIndexMapping> m_vDependentMap;

	///	exports
		std::vector<SmartPtr<IDataExport > > m_vDataExport;
		std::vector<FunctionIndexMapping> m_vExpMap;

	///	data linker
		std::vector<SmartPtr<IIPData> > m_vDataLinker;
		std::vector<FunctionIndexMapping> m_vLinkerMap;
};

} // end namespace ug

#include "data_evaluator_impl.h"

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__DATA_EVALUATOR__ */
