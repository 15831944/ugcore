/*
 * grid_function.h
 *
 *  Created on: 13.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION__
#define __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION__

#include "lib_disc/local_finite_element/local_finite_element_id.h"
#include "lib_disc/dof_manager/function_pattern.h"

namespace ug{

// predeclaration
template <typename TDomain>
class ApproximationSpace;

/// Base class for all Grid Functions
/**
 * This class is the base class for all grid functions. It basically only
 * stores the Dof distribution and registers itself at the DoFDistribution on
 * creation, such that the Grid function is adapted when the Distribution is
 * changed.
 */
class IGridFunction
{
	public:
	///	permutes all values
	/**
	 * This method permutes the values according to the passed mapping vector, i.e.
	 * it performs a permutation of the whole index set. The vector vIndNew must
	 * have the size of the number of indices and for each index it must return
	 * the new index, i.e. newIndex = vIndNew[oldIndex].
	 *
	 * \param[in]	vIndNew		mapping for each index
	 * \returns 	success flag
	 */
		virtual void permute_values(const std::vector<size_t>& vIndNew) = 0;

	///	copy values
	/**
	 * This method copies values between indices according to the passed mapping.
	 * The copy of the values is are performed as:
	 *
	 * 	for all i:	newIndex = vIndexMap[i].second
	 * 				oldIndex = vIndexMap[i].first
	 * 				value[newIndex] <- value[oldIndex]
	 *
	 * If the copy operation is known to be a disjunct composition (i.e. each index
	 * appears only in one operation), this can be specified by a flag. In
	 * this case the order in which the copying is performed is arbitrary and
	 * this will save a copy operation of the whole vector.
	 *
	 * \param[in]	vIndexMap		vector of index mappings (indexOld, indexNew)
	 * \param[in]	bDisjunct		flag, if permutation disjunct
	 * \returns 	success flag
	 */
		virtual void copy_values(const std::vector<std::pair<size_t, size_t> >& vIndexMap,
		                         bool bDisjunct = false) = 0;

	///	resize
	/**
	 * This method resizes the length of the vector.
	 *
	 * \param[in]	s				new size
	 * \param[in]	defaultValue	default value for new entries
	 */
		virtual void resize_values(size_t s, number defaultValue = 0.0) = 0;

	///	Destructor
		virtual ~IGridFunction() {}
};

template <typename TDD>
class IDDGridFunction : public IGridFunction
{
	public:
	///	iterator traits
		template <typename TElem>
		struct traits
		{
			typedef typename TDD::template traits<TElem>::geometric_object geometric_object;
			typedef typename TDD::template traits<TElem>::iterator iterator;
			typedef typename TDD::template traits<TElem>::const_iterator const_iterator;
		};

		template <int dim>
		struct dim_traits
		{
			typedef typename TDD::template dim_traits<dim>::geometric_base_object geometric_base_object;
			typedef typename TDD::template dim_traits<dim>::iterator iterator;
			typedef typename TDD::template dim_traits<dim>::const_iterator const_iterator;
		};

	///	type of multi indices
		typedef typename TDD::multi_index_type multi_index_type;

	public:
	///	constructor
		IDDGridFunction(SmartPtr<TDD> spDoFDistribution)
			: m_spDD(spDoFDistribution)
		{
			if(!m_spDD.is_valid()) UG_THROW_FATAL("DoF Distribution is null.");
			m_spDD->manage_grid_function(*this);
		}

	///	destructor
		~IDDGridFunction()
		{
			m_spDD->unmanage_grid_function(*this);
		}

	///	returns dof distribution
		SmartPtr<TDD> dof_distribution() {return m_spDD;}

	///	returns dof distribution
		ConstSmartPtr<TDD> dof_distribution() const {return m_spDD;}

	/// number of discrete functions
		size_t num_fct() const {return m_spDD->num_fct();}

	/// number of discrete functions on subset si
		size_t num_fct(int si) const {return m_spDD->num_fct(si);}

	/// returns the trial space of the discrete function fct
		LFEID local_finite_element_id(size_t fct) const
			{return m_spDD->local_finite_element_id(fct);}

	/// returns the name of the discrete function nr_fct
		std::string name(size_t fct) const {return m_spDD->name(fct);}

	/// returns fct id by name
		size_t fct_id_by_name(const char* name) const{return m_spDD->fct_id_by_name(name);}

	/// returns the dimension in which solution lives
		int dim(size_t fct) const {return m_spDD->dim(fct);}

	///	returns function pattern
		const FunctionPattern& function_pattern() const {return m_spDD->function_pattern();}

	/// returns true if the discrete function nr_fct is defined on subset s
		bool is_def_in_subset(size_t fct, int si) const {return m_spDD->is_def_in_subset(fct, si);}

	/// returns true if the discrete function nr_fct is defined everywhere
		bool is_def_everywhere(size_t fct) const {return m_spDD->is_def_everywhere(fct);}

	/// number of subsets
		int num_subsets() const {return m_spDD->num_subsets();}

	/// iterator for elements where this grid function is defined
	/// \{
		template <typename TElem>
		typename traits<TElem>::const_iterator begin() const
			{return m_spDD->template begin<TElem>();}

		template <typename TElem>
		typename traits<TElem>::const_iterator end() const
			{return m_spDD->template end<TElem>();}

		template <typename TElem>
		typename traits<TElem>::const_iterator begin(int si) const
			{return m_spDD->template begin<TElem>(si);}

		template <typename TElem>
		typename traits<TElem>::const_iterator end(int si) const
			{return m_spDD->template end<TElem>(si);}
	/// \}

	/////////////////////////////
	// DoF access
	/////////////////////////////

	/// return the number of dofs distributed
		size_t num_indices() const {return m_spDD->num_indices();}

	/// return the number of dofs distributed on subset si
		size_t num_indices(int si) const {return m_spDD->num_indices(si);}

	/// get all indices of the element
		template <typename TElem>
		void indices(TElem* elem, LocalIndices& ind, bool bHang = false) const
			{m_spDD->indices(elem, ind, bHang);}

	/// get multi indices on an finite element in canonical order
		template <typename TElem>
		size_t multi_indices(TElem* elem, size_t fct, std::vector<multi_index_type>& ind) const
			{return m_spDD->multi_indices(elem, fct, ind);}

	/// get multi indices on an geometric object in canonical order
		template <typename TElem>
		size_t inner_multi_indices(TElem* elem, size_t fct,	std::vector<multi_index_type>& ind) const
			{return m_spDD->inner_multi_indices(elem, fct, ind);}

	/// get algebra indices on an geometric object in canonical order
		template <typename TElem>
		size_t algebra_indices(TElem* elem, std::vector<size_t>& ind) const
			{return m_spDD->algebra_indices(elem, ind);}

	/// get algebra indices on an geometric object in canonical order
		template <typename TElem>
		size_t inner_algebra_indices(TElem* elem, std::vector<size_t>& ind) const
			{return m_spDD->inner_algebra_indices(elem, ind);}

	protected:
	///	DoF Distribution this GridFunction relies on
		SmartPtr<TDD> m_spDD;
};

/// represents numerical solutions on a grid using an algebraic vector
/**
 * A grid function brings approximation space and algebra together. For a given
 * DoF Distribution (e.g. a level dof distribution or a surface dof distribution)
 * the grid function stores the values of the DoFs in an algebraic vector.
 * In addition access to the grid elements is provided and the mapping between
 * grid elements and DoFs is provided.
 *
 * \tparam 	TDomain				domain type
 * \tparam	TDD					dof distribution type
 * \tparam	TAlgebra			algebra type
 */
template <typename TDomain, typename TDD, typename TAlgebra>
class GridFunction
	: 	public TAlgebra::vector_type,
	  	public IDDGridFunction<TDD>
{
	public:
		template <typename TElem>
		struct traits
		{
			typedef typename IDDGridFunction<TDD>::template traits<TElem>::geometric_object geometric_object;
			typedef typename IDDGridFunction<TDD>::template traits<TElem>::iterator iterator;
			typedef typename IDDGridFunction<TDD>::template traits<TElem>::const_iterator const_iterator;
		};

		template <int dim>
		struct dim_traits
		{
			typedef typename IDDGridFunction<TDD>::template dim_traits<dim>::geometric_base_object geometric_base_object;
			typedef typename IDDGridFunction<TDD>::template dim_traits<dim>::iterator iterator;
			typedef typename IDDGridFunction<TDD>::template dim_traits<dim>::const_iterator const_iterator;
		};

	public:
	///	This type
		typedef GridFunction<TDomain, TDD, TAlgebra> this_type;

	///	Type of Approximation space
		typedef ApproximationSpace<TDomain> approximation_space_type;

	///	Domain
		typedef TDomain domain_type;

	///	World Dimension
		static const int dim = domain_type::dim;

	///	Algebra type
		typedef TAlgebra algebra_type;

	///	Vector type used to store dof values
		typedef typename algebra_type::vector_type vector_type;

	///	Type of DoFDistribution
		typedef TDD dof_distribution_type;

	public:
		using IDDGridFunction<TDD>::num_indices;

	public:
	/// Initializing Constructor
		GridFunction(SmartPtr<approximation_space_type> approxSpace,
		             SmartPtr<TDD> spDoFDistr)
			: IDDGridFunction<TDD>(spDoFDistr), m_spDomain(approxSpace->domain())
		{
			check_algebra();
			resize_values(num_indices());
		};

	/// Initializing Constructor using surface dof distribution
		GridFunction(SmartPtr<approximation_space_type> approxSpace)
			: IDDGridFunction<TDD>(approxSpace->surface_dof_distribution()),
			  m_spDomain(approxSpace->domain())
		{
			check_algebra();
			resize_values(num_indices());
		};

	///	checks the algebra
		void check_algebra();

	/// Copy constructor
		GridFunction(const this_type& v) : IDDGridFunction<TDD>(v) {assign(v);}

	///	assigns another grid function
		this_type& operator=(const this_type& v) {assign(v); return *this;}

	/// clone
		this_type& clone(){return *(new this_type(*this));}

	/// copies the GridFunction v, except that the values are copied.
		virtual void clone_pattern(const this_type& v);

	///	assigns another GridFunction
		void assign(const this_type& v);

	///	assigns the values of a vector
		void assign(const vector_type& v);

	///	\copydoc IGridFunction::resize_values
		virtual void resize_values(size_t s, number defaultValue = 0.0);

	///	\copydoc IGridFunction::permute_values
		virtual	void permute_values(const std::vector<size_t>& vIndNew);

	///	\copydoc IGridFunction::copy_values
		virtual void copy_values(const std::vector<std::pair<size_t, size_t> >& vIndexMap,
		                         bool bDisjunct = false);

	/// Destructor
		virtual ~GridFunction() {}

	///	returns domain
		SmartPtr<domain_type> domain() {return m_spDomain;}

	///	returns const domain
		ConstSmartPtr<domain_type> domain() const {return m_spDomain;}

	/// access dof values
		inline number get_dof_value(size_t i, size_t comp) const
			{return BlockRef( (vector_type::operator[](i)), comp);}

	///	returns the position of the dofs of a function if available
		template <typename TElem>
		bool dof_positions(TElem* elem, size_t fct,
		                   std::vector<MathVector<dim> >& vPos) const;

		template <typename TElem>
		bool inner_dof_positions(TElem* elem, size_t fct,
		                         std::vector<MathVector<dim> >& vPos) const;

	protected:
	/// Approximation Space
		SmartPtr<domain_type> m_spDomain;
};

template <typename TDomain, typename TDD, typename TAlgebra>
const typename TAlgebra::vector_type &getVector(const GridFunction<TDomain, TDD, TAlgebra> &t)
{
	return *dynamic_cast<const GridFunction<TDomain, TDD, TAlgebra>*>(&t);
}


template <typename TDomain, typename TDD, typename TAlgebra>
inline std::ostream& operator<< (std::ostream& outStream, const GridFunction<TDomain, TDD, TAlgebra>& v)
{
	outStream << *dynamic_cast<const GridFunction<TDomain, TDD, TAlgebra>*>(&v);
	return outStream;
}

} // end namespace ug

// include implementation
#include "grid_function_impl.h"

#endif /* __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION__ */
