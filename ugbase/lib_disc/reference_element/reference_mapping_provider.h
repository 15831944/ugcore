/*
 * reference_mapping_provider.h
 *
 *  Created on: 29.06.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__REFERENCE_ELEMENT__REFERENCE_MAPPING_PROVIDER__
#define __H__UG__LIB_DISC__REFERENCE_ELEMENT__REFERENCE_MAPPING_PROVIDER__


#include "common/common.h"
#include "common/math/ugmath.h"
#include "lib_grid/lg_base.h"

namespace ug{

///	virtual base class for reference mappings
/**
 * This class is the base class for reference mappings in order to make them
 * selectable through a provider (on the price of virtual functions).
 *
 * \tparam	TDim		reference element dimensino
 * \tparam	TWorldDim	(physical) world dimension
 */
template <int TDim, int TWorldDim = TDim>
class DimReferenceMapping
{
	public:
	///	world dimension (range space dimension)
		static const int worldDim = TWorldDim;

	///	reference dimension (domain space dimension)
		static const int dim = TDim;

	public:
	///	returns if mapping is affine
		virtual bool is_linear() const = 0;

	///	refresh mapping for new set of corners
		virtual void update(const MathVector<worldDim>* vCornerCoord) = 0;

	///	refresh mapping for new set of corners
		virtual void update(const std::vector<MathVector<worldDim> >& vCornerCoord) = 0;

	///	map local coordinate to global coordinate
		virtual void local_to_global(MathVector<worldDim>& globPos,
		                             const MathVector<dim>& locPos) const = 0;

	///	map n local coordinate to global coordinate
		virtual void local_to_global(MathVector<worldDim>* vGlobPos,
									 const MathVector<dim>* vLocPos, size_t n) const = 0;

	///	map local coordinate to global coordinate for a vector of local positions
		virtual void local_to_global(std::vector<MathVector<worldDim> >& vGlobPos,
									 const std::vector<MathVector<dim> >& vLocPos) const = 0;

	///	returns jacobian
		virtual void jacobian(MathMatrix<worldDim, dim>& J,
							  const MathVector<dim>& locPos) const = 0;

	///	returns jacobian for n local positions
		virtual void jacobian(MathMatrix<worldDim, dim>* vJ,
							  const MathVector<dim>* vLocPos, size_t n) const = 0;

	///	returns jacobian for a vector of local positions
		virtual void jacobian(std::vector<MathMatrix<worldDim, dim> >& vJ,
							  const std::vector<MathVector<dim> >& vLocPos) const = 0;

	///	returns transposed of jacobian
		virtual void jacobian_transposed(MathMatrix<dim, worldDim>& JT,
		                                 const MathVector<dim>& locPos) const = 0;

	///	returns transposed of jacobian for n local positions
		virtual void jacobian_transposed(MathMatrix<dim, worldDim>* vJT,
										 const MathVector<dim>* vLocPos, size_t n) const = 0;

	///	returns transposed of jacobian for a vector of positions
		virtual void jacobian_transposed(std::vector<MathMatrix<dim, worldDim> >& vJT,
										 const std::vector<MathVector<dim> >& vLocPos) const = 0;

	///	returns transposed of the inverse of the jacobian
		virtual void jacobian_transposed_inverse(MathMatrix<worldDim, dim>& JTInv,
		                                         const MathVector<dim>& locPos) const = 0;

	///	returns transposed of the inverse of the jacobian for n local positions
		virtual void jacobian_transposed_inverse(MathMatrix<worldDim, dim>* vJTInv,
												 const MathVector<dim>* vLocPos, size_t n) const = 0;

	///	returns transposed of the inverse of the jacobian for a vector of positions
		virtual void jacobian_transposed_inverse(std::vector<MathMatrix<worldDim, dim> >& vJTInv,
												 const std::vector<MathVector<dim> >& vLocPos) const = 0;

	///	returns the determinate of the jacobian
		virtual number sqrt_gram_det(const MathVector<dim>& locPos) const = 0;

	///	returns the determinate of the jacobian for n local positions
		virtual void sqrt_gram_det(number* vDet, const MathVector<dim>* vLocPos, size_t n) const = 0;

	///	returns the determinate of the jacobian for a vector of local positions
		virtual void sqrt_gram_det(std::vector<number>& vDet,
								  const std::vector<MathVector<dim> >& vLocPos) const = 0;

	///	virtual destructor
		virtual ~DimReferenceMapping() {}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct UGError_ReferenceMappingMissing : public UGError
{
	UGError_ReferenceMappingMissing(int dim_, int worldDim_, ReferenceObjectID roid_)
		: UGError(""), dim(dim_), worldDim(worldDim_), roid(roid_)
	{
		std::stringstream ss; ss << "ReferenceMapping not found for "<<roid<<
									" from R^"<<dim<<" to R^"<<worldDim;
		UGError::push_msg(ss.str());
	}
	int dim;
	int worldDim;
	ReferenceObjectID roid;
};

/// class to provide reference mappings
/**
 *	This class provides references mappings. It is implemented as a Singleton.
 */
class ReferenceMappingProvider {
	private:
	// 	disallow private constructor
		ReferenceMappingProvider();

	// disallow copy and assignment (intentionally left unimplemented)
		ReferenceMappingProvider(const ReferenceMappingProvider&);
		ReferenceMappingProvider& operator=(const ReferenceMappingProvider&);

	// 	private destructor
		~ReferenceMappingProvider(){};

	// 	Singleton provider
		static ReferenceMappingProvider& inst()
		{
			static ReferenceMappingProvider myInst;
			return myInst;
		};

	//	This is very dirty implementation, since casting to void. But, it is
	//	efficient, easy and typesafe. Maybe it should be changed to something
	//	inherent typesafe (not using casts) some day
	//	holding all mappings (worldDim x dim x roid)
		void* m_vvvMapping[4][4][NUM_REFERENCE_OBJECTS];

	//	casts void to map
		template <int TDim, int TWorldDim>
		DimReferenceMapping<TDim, TWorldDim>* get_mapping(ReferenceObjectID roid)
		{
			UG_STATIC_ASSERT(TDim <= 3, only_implemented_for_ref_dim_smaller_equal_3);
			UG_STATIC_ASSERT(TWorldDim <= 3, only_implemented_for_ref_dim_smaller_equal_3);
			UG_ASSERT(roid < NUM_REFERENCE_OBJECTS, "Roid specified incorrectly.");
			UG_ASSERT(roid >= 0, "Roid specified incorrectly.");
			return reinterpret_cast<DimReferenceMapping<TDim, TWorldDim>*>
						(m_vvvMapping[TDim][TWorldDim][roid]);
		}

	//	casts map to void
		template <int TDim, int TWorldDim>
		void set_mapping(ReferenceObjectID roid, DimReferenceMapping<TDim, TWorldDim>& map)
		{
			m_vvvMapping[TDim][TWorldDim][roid] = reinterpret_cast<void*>(&map);
		}

	public:
	///	returns a reference to a DimReferenceMapping
	/**
	 * This class returns a reference mapping for a ReferenceObjectID. The
	 * reference dimension and the world dimension must be chosen as
	 * template arguments. An exception is throw if such an mapping does not
	 * exist.
	 *
	 * \param[in]	roid		Reference Object ID
	 * \tparam		TDim		reference element dimension
	 * \tparam		TWorldDim	(physical) world dimension
	 */
		template <int TDim, int TWorldDim>
		static DimReferenceMapping<TDim, TWorldDim>& get(ReferenceObjectID roid)
		{
			DimReferenceMapping<TDim, TWorldDim>* pMap = inst().get_mapping<TDim, TWorldDim>(roid);
			if(!pMap) throw(UGError_ReferenceMappingMissing(TDim, TWorldDim, roid));
			else return *pMap;
		}

	///	returns a reference to a DimReferenceMapping with updated element corners
	/**
	 * This class returns a reference mapping for a ReferenceObjectID. The
	 * reference dimension and the world dimension must be chosen as
	 * template arguments. An exception is throw if such an mapping does not
	 * exist.
	 *
	 * \param[in]	roid			Reference Object ID
	 * \param[in]	vCornerCoord	The corner coordinates of the element
	 * \tparam		TDim			reference element dimension
	 * \tparam		TWorldDim		(physical) world dimension
	 */
		template <int TDim, int TWorldDim>
		static DimReferenceMapping<TDim, TWorldDim>& get(ReferenceObjectID roid,
		                                                 const std::vector<MathVector<TWorldDim> >& vCornerCoord)
		{
			DimReferenceMapping<TDim, TWorldDim>& map = get<TDim, TWorldDim>(roid);
			map.update(vCornerCoord);
			return map;
		}

	///	returns a reference to a DimReferenceMapping with updated element corners
	/**
	 * This class returns a reference mapping for a ReferenceObjectID. The
	 * reference dimension and the world dimension must be chosen as
	 * template arguments. An exception is throw if such an mapping does not
	 * exist.
	 *
	 * \param[in]	roid			Reference Object ID
	 * \param[in]	vCornerCoord	The corner coordinates of the element
	 * \tparam		TDim			reference element dimension
	 * \tparam		TWorldDim		(physical) world dimension
	 */
		template <int TDim, int TWorldDim>
		static DimReferenceMapping<TDim, TWorldDim>& get(ReferenceObjectID roid,
														 const MathVector<TWorldDim>* vCornerCoord)
		{
			DimReferenceMapping<TDim, TWorldDim>& map = get<TDim, TWorldDim>(roid);
			map.update(vCornerCoord);
			return map;
		}

};

} // end namespace ug

#endif /* __H__UG__LIB_DISC__REFERENCE_ELEMENT__REFERENCE_MAPPING_PROVIDER__ */
