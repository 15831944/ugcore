//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m12 d03

#ifndef __H__LIB_GRID__MISC_UTIL__
#define __H__LIB_GRID__MISC_UTIL__

#include "lib_grid/lg_base.h"

namespace ug
{
/**
 * \brief contains miscellaneus methods that are related to GeometricObjects.
 * 
 * \defgroup lib_grid_algorithms_geom_obj_misc misc geometric object util
 * \ingroup lib_grid_algorithms
 * @{
 */

////////////////////////////////////////////////////////////////////////
//	CalculateGeometricObjectCenter
///	calculates the center for arbitrary geometric object
template<class TAAPosVRT>
UG_API
inline
typename TAAPosVRT::ValueType
CalculateGeometricObjectCenter(const GeometricObject* o, TAAPosVRT& aaPosVRT);

////////////////////////////////////////////////////////////////////////
//	CalculateCenter
///	calculates the center for a set of elements
/**	TIterator::value_type has to be compatible with
 *  VertexBase*, EdgeBase*, Face* or Volume*.
 */
template <class TIterator, class TAAPosVRT>
UG_API 
typename TAAPosVRT::ValueType
CalculateCenter(TIterator begin, TIterator end, TAAPosVRT& aaPos);

////////////////////////////////////////////////////////////////////////
//	FindClosestByCoordinate
///	returns the element of the given type, whose center is closest to coord.
/**
 * Returns the element, whose center is closest to the given coordinate.
 *
 * TVertexPositionAttachmentAccessor has to be an AttachmentAccessor,
 * where AttachmentAccessor::ValueType is a vector-type compatible to
 * the lgmath vector descriptor.
 * The Accessor has to access an attachment of the vertices,
 * to which the faces between iterBegin and iterEnd refer.
 */
template<class TElem, class TVertexPositionAttachmentAccessor>
UG_API 
TElem* FindClosestByCoordinate(const typename TVertexPositionAttachmentAccessor::ValueType& coord,
							   typename geometry_traits<TElem>::iterator iterBegin,
							   typename geometry_traits<TElem>::iterator iterEnd,
							   TVertexPositionAttachmentAccessor& aaPosVRT);

////////////////////////////////////////////////////////////////////////
///	Calculates the bounding box of a set of geometric objects
/**	TIterator has to be an iterator to a set containing elements of type
 * EdgeBase*, Face* or Volume*. An overload for VertexBase* exists.
 *
 * Make sure that TAAPos::ValueType == vector_t.
 */
template<class vector_t, class TIterator, class TAAPos>
UG_API 
void CalculateBoundingBox(vector_t& vMinOut, vector_t& vMaxOut,
						  TIterator begin, TIterator end,
						  TAAPos& aaPos);

////////////////////////////////////////////////////////////////////////
//	NumSharedVertices
///	returns the number of vertices that are shared by two objects
/**	This algorithm uses Grid::mark.
 *
 *	Valid types are EdgeBase*, Face*, Volume* and derivates of those.
 *	You may combine different types in one query.
 */
template <class TElemPtr1, class TElemPtr2>
UG_API 
size_t NumSharedVertices(Grid& grid, TElemPtr1 elem1, TElemPtr2 elem2);

////////////////////////////////////////////////////////////////////////
//	EraseConnectingElements
///	erases all elements that connect v1 and v2
UG_API 
void EraseConnectingElements(Grid& grid, VertexBase* v1, VertexBase* v2);

////////////////////////////////////////////////////////////////////////
//	EraseElements
///	erases all elements between iterBegin and iterEnd.
template <class TElem>
UG_API 
void EraseElements(Grid& grid, typename geometry_traits<TElem>::iterator iterBegin,
						typename geometry_traits<TElem>::iterator iterEnd);

/// @}				
}//	end of namespace

////////////////////////////////////////////////////////////////////////
//	include implementation of template methods.
#include "misc_util_impl.hpp"

#endif
