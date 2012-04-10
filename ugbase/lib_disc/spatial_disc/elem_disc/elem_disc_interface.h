/*
 * elem_disc_interface.h
 *
 *  Created on: 07.07.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__
#define __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__

// extern headers
#include <vector>
#include <string>

// intern headers
#include "lib_disc/common/local_algebra.h"
#include "lib_disc/time_disc/solution_time_series.h"
#include "lib_disc/function_spaces/approximation_space.h"
#include "lib_disc/local_finite_element/local_finite_element_id.h"
#include "lib_disc/reference_element/reference_element_traits.h"
#include "lib_disc/spatial_disc/ip_data/data_import_export.h"

namespace ug{

/**
 * Element Discretizations
 *
 * \defgroup lib_disc_elem_disc Elem Disc
 * \ingroup lib_discretization
 */

/// \ingroup lib_disc_elem_disc
/// @{

///	base class for all element-wise discretizations
/**
 * This class is the base class for element-wise discretizations. An
 * implementation of this class must provide local stiffness/mass-matrix
 * contribution of one element to the global jacobian and local contributions
 * of one element to the local defect.
 */
class IElemDisc
{
	public:
	///	Constructor
		IElemDisc(const char* functions = NULL, const char* subsets = NULL);

	///	Constructor
		IElemDisc(const std::vector<std::string>& vFct, const std::vector<std::string>& vSubset);

	////////////////////////////
	// Functions and Subsets

	///	sets functions by name list, divided by ','
		void set_functions(std::string functions);

	///	sets subset(s) by name list, divided by ','
		void set_subsets(std::string subsets);

	/// number of functions this discretization handles
		size_t num_fct() const {return m_vFct.size();}

	///	returns the symbolic functions
		const std::vector<std::string>& symb_fcts() const {return m_vFct;}

	/// number of functions this discretization handles
		size_t num_subsets() const {return m_vSubset.size();}

	///	returns the symbolic subsets
		const std::vector<std::string>& symb_subsets() const {return m_vSubset;}

	protected:
	///	vector holding name of all symbolic functions
		std::vector<std::string> m_vFct;

	///	vector holding name of all symbolic subsets
		std::vector<std::string> m_vSubset;

	////////////////////////////
	// IP Data and Coupling
	////////////////////////////
	public:
	///	registers a data import
		void register_import(IDataImport& Imp);

	///	registers a data export
		void register_export(SmartPtr<IDataExport> Exp);

	///	returns number of imports
		size_t num_imports() const {return m_vIImport.size();}

	/// returns an import
		IDataImport& get_import(size_t i);

	///	removes all imports
		void clear_imports() {m_vIImport.clear();}

	///	returns number of exports
		size_t num_exports() const {return m_vIExport.size();}

	/// returns an export
		SmartPtr<IDataExport> get_export(size_t i);

	///	removes all exports
		void clear_exports() {m_vIExport.clear();}

	protected:
	/// data imports
		std::vector<IDataImport*> m_vIImport;

	///	data exports
		std::vector<SmartPtr<IDataExport> > m_vIExport;

	public:
	////////////////////////////
	// Assembling functions
	////////////////////////////

	/// requests assembling for a finite element id
	/**
	 * This function is called before the assembling starts. In the vector
	 * exactly this->num_fct() Local Finite Element IDs must be passed. The
	 * IElemDisc-Implementation checks if it can assemble the set of LFEID and
	 * registers the corresponding assembling functions. If this is not the
	 * case instead false is returned.
	 *
	 * \param[in]		vLfeID		vector of Local Finite Element IDs
	 * \returns			true		if assemble routines are present and selected
	 * 					false		if no assembling for those Spaces available
	 */
		virtual bool request_finite_element_id(const std::vector<LFEID>& vLfeID) = 0;

	///	informs the assembling, that hanging nodes must be taken into account
	/**
	 * This method is called before the assembling of elements, that may have
	 * hanging nodes, constrained edges, etc. Thus, if the assembling must take
	 * special care, it can prepare for such needs. Typically other assembling
	 * functions are used then and registered.
	 *
	 * \param[in]		bNonRegular 	true iff non-regular grid used
	 * \returns			bool			true  if successful
	 * 									false if cannot be handled by disc
	 */
		virtual bool request_non_regular_grid(bool bNonRegular) = 0;

	///	returns if discretization acts on hanging nodes if present
	/**
	 * This function returns if a discretization really needs the hanging nodes
	 * in case of non-regular grid. This may not be the case for e.g. finite
	 * element assemblings but is needed for finite volumes
	 */
		virtual bool use_hanging() const {return false;}

	///	sets if assembling should be time-dependent
	/**
	 * This function specifies if the assembling is time-dependent.
	 *
	 * \param[in]	bTimeDependent	flag if time-dependent
	 * \param[in]	locTimeSeries	Time series of previous solutions
	 */
		void set_time_dependent(bool bTimeDependent,
		                        const LocalVectorTimeSeries* locTimeSeries = NULL);

	///	returns if assembling is time-dependent
		bool is_time_dependent() const {return m_bTimeDependent;}

	///	returns if local time series needed by assembling
	/**
	 * This callback must be implemented by a derived Elem Disc in order to handle
	 * time-dependent data. As return the derived Elem Disc can specify, if
	 * it really needs data from previous time steps for the (spatial) disc. The
	 * default is false.
	 *
	 * \returns 	if elem disc needs time series local solutions
	 */
		virtual bool requests_local_time_series() {return false;}

	///	sets the time point
		void set_time(const number time) {m_time = time;}

	///	returns currently set timepoint
		number time() const {return m_time;}

	///	returns the local time solutions
	/**
	 * This function returns the local time solutions. This a type of vector,
	 * that holds the local unknowns for each time point of a time series.
	 * Note, that the first sol is always the current (iterated, unknown)
	 * time point, while all remaining sols are the already computed time steps
	 * used e.g. in a time stepping scheme.
	 *
	 * \returns vLocalTimeSol		vector of local time Solutions
	 */
		const LocalVectorTimeSeries* local_time_solutions() const
			{return m_pLocalVectorTimeSeries;}

	/// prepare the timestep
	/**
	 * This function prepares a timestep (iff timedependent). This function is
	 * called once for every element before the spatial assembling procedure
	 * begins.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		template <typename TElem>
		void fast_prepare_timestep_elem(TElem* elem, const LocalVector& u);

	/// prepare the timestep
		virtual void prepare_timestep_elem(GeometricObject* elem, const LocalVector& u) {}

	///	prepares the loop over all elements of one type
	/**
	 * This function should prepare the element loop for elements of one fixed
	 * type. This function is called before e.g. the loop over all geometric
	 * objects of a chosen type is performed.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_prepare_elem_loop()
			{(this->*m_vPrepareElemLoopFct[m_id])();}

	///	virtual prepares the loop over all elements of one type
		virtual void prepare_elem_loop() {}

	///	prepare one elements for assembling
	/**
	 * This function prepares one Geometric object, that will be assembled in
	 * the next step.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 *
	 * \param[in]		elem		The geometric object
	 * \param[in]		u			The current local solution
	 */
		template <typename TElem>
		void fast_prepare_elem(TElem* elem, const LocalVector& u);

	///	virtual prepare one elements for assembling
		virtual void prepare_elem(GeometricObject* elem, const LocalVector& u) {}

	///	postprocesses the loop over all elements of one type
	/**
	 * This function should post process the element loop for elements of one fixed
	 * type. This function is called after e.g. the loop over all geometric
	 * objects of a chosen type has been performed.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_finish_elem_loop()
			{(this->*m_vFinishElemLoopFct[m_id])();}

	///	virtual postprocesses the loop over all elements of one type
		virtual void finish_elem_loop() {}

	/// finish the timestep
	/**
	 * This function finishes the timestep (iff timedependent). This function is
	 * called in the PostProcess of a timestep.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		template <typename TElem>
		void fast_finish_timestep_elem(TElem* elem, const number time, const LocalVector& u);

	/// virtual finish the timestep
		virtual void finish_timestep_elem(GeometricObject* elem, const number time, const LocalVector& u) {}

	/// Assembling of Jacobian (Stiffness part)
	/**
	 * This function assembles the local (stiffness) jacobian for the current
	 * solution u.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_ass_JA_elem(LocalMatrix& J, const LocalVector& u)
			{(this->*m_vElemJAFct[m_id])(J, u);}

	/// Assembling of Jacobian (Stiffness part)
		virtual void ass_JA_elem(GeometricObject* elem, LocalMatrix& J, const LocalVector& u) {}

	/// Assembling of Jacobian (Mass part)
	/**
	 * This function assembles the local (mass) jacobian for the current
	 * solution u.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_ass_JM_elem(LocalMatrix& J, const LocalVector& u)
			{(this->*m_vElemJMFct[m_id])(J, u);}

	/// Assembling of Jacobian (Mass part)
		virtual void ass_JM_elem(GeometricObject* elem, LocalMatrix& J, const LocalVector& u) {}

	/// Assembling of Defect (Stiffness part)
	/**
	 * This function assembles the local (stiffness) defect for the current
	 * solution u.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_ass_dA_elem(LocalVector& d, const LocalVector& u)
			{(this->*m_vElemdAFct[m_id])(d, u);}

	/// virtual Assembling of Defect (Stiffness part)
		virtual void ass_dA_elem(GeometricObject* elem, LocalVector& d, const LocalVector& u) {}

	/// Assembling of Defect (Mass part)
	/**
	 * This function assembles the local (mass) defect for the current
	 * solution u.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_ass_dM_elem(LocalVector& d, const LocalVector& u)
			{(this->*m_vElemdMFct[m_id])(d, u);}

	/// virtual Assembling of Defect (Mass part)
		virtual void ass_dM_elem(GeometricObject* elem, LocalVector& d, const LocalVector& u) {}

	/// Assembling of Right-Hand Side
	/**
	 * This function assembles the local rhs.
	 * <b>NOTE:</b>Before this method can be used, the method
	 * 'set_roid' must have been called to set the elem type.
	 */
		void fast_ass_rhs_elem(LocalVector& rhs)
			{(this->*m_vElemRHSFct[m_id])(rhs);}

	/// virtual Assembling of Right-Hand Side
		virtual void ass_rhs_elem(GeometricObject* elem, LocalVector& rhs) {}

	/// Virtual destructor
		virtual ~IElemDisc() {}

	protected:
	///	number of functions
		size_t m_numFct;

	///	flag if time dependent
		bool m_bTimeDependent;

	///	time point
		number m_time;

	///	list of local vectors for all solutions of the time series
		const LocalVectorTimeSeries* m_pLocalVectorTimeSeries;

	private:
	//	abbreviation for own type
		typedef IElemDisc T;

	// 	types of timestep function pointers
		typedef void (T::*PrepareTimestepElemFct)(const LocalVector& u);
		typedef void (T::*FinishTimestepElemFct)(const LocalVector& u);

	// 	types of loop function pointers
		typedef void (T::*PrepareElemLoopFct)();
		typedef void (T::*PrepareElemFct)(GeometricObject* obj, const LocalVector& u);
		typedef void (T::*FinishElemLoopFct)();

	// 	types of Jacobian assemble functions
		typedef void (T::*ElemJAFct)(LocalMatrix& J, const LocalVector& u);
		typedef void (T::*ElemJMFct)(LocalMatrix& J, const LocalVector& u);

	// 	types of Defect assemble functions
		typedef void (T::*ElemdAFct)(LocalVector& d, const LocalVector& u);
		typedef void (T::*ElemdMFct)(LocalVector& d, const LocalVector& u);

	// 	types of right hand side assemble functions
		typedef void (T::*ElemRHSFct)(LocalVector& d);

	protected:
	// 	register the functions
		template <typename TAssFunc> void set_prep_timestep_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_fsh_timestep_elem_fct(ReferenceObjectID id, TAssFunc func);

		template <typename TAssFunc> void set_prep_elem_loop_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_prep_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_fsh_elem_loop_fct(ReferenceObjectID id, TAssFunc func);

		template <typename TAssFunc> void set_ass_JA_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_ass_JM_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_ass_dA_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_ass_dM_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_ass_rhs_elem_fct(ReferenceObjectID id, TAssFunc func);

	///	sets usage of fast assemble functions
		void enable_fast_ass_elem(bool bEnable) {m_bFastAssembleEnabled = bEnable;}

	public:
	///	returns if fast assembling for elememts is used
		bool fast_ass_elem_enabled() const {return m_bFastAssembleEnabled;}

	/// sets the geometric object type
	/**
	 * This functions set the geometric object type of the object, that is
	 * assembled next. The user has to call this function before most of the
	 * assembling routines can be called. Keep in mind, that the elements are
	 * looped type by type, thus this function has to be called very few times.
	 */
		void set_roid(ReferenceObjectID id);

	private:
	///	flag if fast assemble is used
		bool m_bFastAssembleEnabled;

	// 	timestep function pointers
		PrepareTimestepElemFct 		m_vPrepareTimestepElemFct[NUM_REFERENCE_OBJECTS];
		FinishTimestepElemFct 		m_vFinishTimestepElemFct[NUM_REFERENCE_OBJECTS];

	// 	loop function pointers
		PrepareElemLoopFct 	m_vPrepareElemLoopFct[NUM_REFERENCE_OBJECTS];
		PrepareElemFct 		m_vPrepareElemFct[NUM_REFERENCE_OBJECTS];
		FinishElemLoopFct 	m_vFinishElemLoopFct[NUM_REFERENCE_OBJECTS];

	// 	Jacobian function pointers
		ElemJAFct 	m_vElemJAFct[NUM_REFERENCE_OBJECTS];
		ElemJMFct 	m_vElemJMFct[NUM_REFERENCE_OBJECTS];

	// 	Defect function pointers
		ElemdAFct 	m_vElemdAFct[NUM_REFERENCE_OBJECTS];
		ElemdMFct 	m_vElemdMFct[NUM_REFERENCE_OBJECTS];

	// 	Rhs function pointers
		ElemRHSFct 	m_vElemRHSFct[NUM_REFERENCE_OBJECTS];

	protected:
	/// current Geometric Object
		ReferenceObjectID m_id;
};

template <typename TDomain>
class IDomainElemDisc : public IElemDisc
{
	private:
	///	base class type
		typedef IElemDisc base_type;

	public:
	///	Domain type
		typedef TDomain domain_type;

	///	World dimension
		static const int dim = TDomain::dim;

	///	Position type
		typedef typename TDomain::position_type position_type;

	public:
	///	Constructor
		IDomainElemDisc(const char* functions = NULL, const char* subsets = NULL)
			: IElemDisc(functions, subsets), m_spApproxSpace(NULL) {};
		
	///	Constructor
		IDomainElemDisc(const std::vector<std::string>& vFct, const std::vector<std::string>& vSubset)
			: IElemDisc(vFct, vSubset), m_spApproxSpace(NULL) {};

	///	sets the approximation space
	/**	Calls protected virtual 'approximation_space_changed', when a new approximation space
	 * has been set. Note that 'approximation_space_changed' is only called once if the
	 * same approximation space is set multiple times.*/
		void set_approximation_space(SmartPtr<ApproximationSpace<domain_type> > approxSpace)
		{
		//	check whether the approximation space has already been set
			bool newApproxSpace = (m_spApproxSpace != approxSpace);

		//	remember approx space
			m_spApproxSpace = approxSpace;

		//	remember position accessor
			m_aaPos = m_spApproxSpace->domain()->position_accessor();

		//	invoke callback
			if(newApproxSpace)
				approximation_space_changed();
		}

	///	returns approximation space
		SmartPtr<ApproximationSpace<domain_type> > approx_space() {return m_spApproxSpace;}

	///	returns approximation space
		ConstSmartPtr<ApproximationSpace<domain_type> > approx_space() const {return m_spApproxSpace;}

	///	returns the domain
		domain_type& domain()
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain();
		}

	///	returns the domain
		const domain_type& domain() const
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain();
		}

	///	returns the function pattern
		const FunctionPattern& function_pattern() const {return *m_spApproxSpace->function_pattern();}

	///	returns if function pattern set
		bool fct_pattern_set() const {return m_spApproxSpace.valid();}

	///	returns the subset handler
		typename domain_type::subset_handler_type& subset_handler()
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain()->subset_handler();
		}

	///	returns the subset handler
		const typename domain_type::subset_handler_type& subset_handler() const
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain()->subset_handler();
		}

	///	returns the corner coordinates of an Element in a C++-array
		template<typename TElem>
		const position_type* element_corners(TElem* elem)
		{
			typedef typename reference_element_traits<TElem>::reference_element_type
							ref_elem_type;

		//	resize corners
			m_vCornerCoords.resize(ref_elem_type::num_corners);

		//	check domain
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set");

		//	extract corner coordinates
			for(size_t i = 0; i < m_vCornerCoords.size(); ++i)
			{
				VertexBase* vert = elem->vertex(i);
				m_vCornerCoords[i] = m_aaPos[vert];
			}

		//	return corner coords
			return &m_vCornerCoords[0];
		}

	protected:
	///	callback invoked, when approximation space is changed
		virtual void approximation_space_changed() {}

	protected:
	///	Position access
		typename TDomain::position_accessor_type m_aaPos;

	///	Corner Coordinates
		std::vector<position_type> m_vCornerCoords;

	///	Approximation Space
		SmartPtr<ApproximationSpace<domain_type> > m_spApproxSpace;

};
/// @}

} // end namespace ug

#include "elem_disc_interface_impl.h"

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__ */
