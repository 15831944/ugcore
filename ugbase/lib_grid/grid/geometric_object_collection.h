// created by Sebastian Reiter
// y09 m07 d31
// s.b.reiter@googlemail.com

#ifndef __H__LIB_GRID__GEOMETRIC_OBJECT_COLLECTION__
#define __H__LIB_GRID__GEOMETRIC_OBJECT_COLLECTION__

#include <list>
#include "geometric_base_objects.h"
#include "common/util/section_container.h"
#include "element_storage.h"
#include "geometric_base_object_traits.h"

namespace ug
{

/// \addtogroup lib_grid
/// @{

////////////////////////////////////////////////////////////////////////
//	GeometricObjectCollection
///	a helper class that holds a collection of possibly unconnected geometric-objects.
/**
 * This class is a simple helper class..
 * Its purpose is to make it easy to pass a collection of geometric-objects
 * to a function while maintaining the possibility to iterate over different
 * sub-types of geometric-objects seperatly.
 *
 * In contrary to \sa GeometricObjectCollection, the
 * GeometricObjectCollection allows access to the elements through
 * different levels.
 *
 * Please note that a GeometricObjectCollection is only valid as long as
 * the object from which you received the collection still exists.
 *
 * A GeometricObjectCollection can only be queried for iterators and 
 * element-counts. You may not insert new elements or remove
 * old ones (at least not directly).
 *
 * Classes that you can query for their GeometricObjectCollection
 * are for example ug::Grid, ug::MultiGrid, ug::SubsetHandler,
 * ug::MGSubsetHandler, ug::Selector, ug::MGSelector.
 *
 * As long as the object that provides the GeometricObjectCollection
 * is still valid, the GeometricObjectCollection will always hold the current
 * geometric objects of the source-object (i.e. a grid, a selector or a subset-handler),
 * as long as new objects are inserted into existing subsets (SubsetHandler) or
 * existing levels (MultiGrid). Insertion or removal of subsets or levels is
 * not reflected by the goc and can lead to severe errors.
 * Make sure to retrieve a new goc if such changes happened.
 *
 * Please note that a GeometricObjectCollection does not necessarily represent
 * a topological closed part of a grid.
 * A Collection can for example hold faces without their
 * associated vertices.
 *
 * How to use GeometricObjectCollection:
 * Once you retrieved an instance (let's call it goc) you can query it for
 * iterators like this:
 * VertexBaseIterator iter = goc.vertices_begin(0);
 * or if you want to iterate over triangles of level 1 type the following:
 * TriangleIterator iter = goc.begin<Triangle>(1);
 *
 * if you want to get the number of hexahedrons in level 0 you would go like this:
 * uint numHexas = goc.num<Hexahedron>(0);
 */
class UG_API GeometricObjectCollection
{
	public:
	///	The traits class holds some important types for each element-type
		template <class TElem>
		struct traits{
			typedef typename geometry_traits<TElem>::iterator		iterator;
			typedef typename geometry_traits<TElem>::const_iterator	const_iterator;
		};

	///	initializes the instance with an estimate of the number of levels.
	/**	The estimate does not have to match exactly. However, if it does
	 *  it makes things faster.*/
		GeometricObjectCollection(size_t levelEstimate = 1);
		
	///	initializes level 0 with the given sections.
		GeometricObjectCollection(ElementStorage<VertexBase>::SectionContainer* vrtCon,
								ElementStorage<EdgeBase>::SectionContainer* edgeCon,
								ElementStorage<Face>::SectionContainer* faceCon,
								ElementStorage<Volume>::SectionContainer* volCon);

	//	copy constructor.
		GeometricObjectCollection(const GeometricObjectCollection& mgoc);
		
		GeometricObjectCollection& operator =(const GeometricObjectCollection& mgoc);
		
	///	only used during creation by the methods that create the collection
		void add_level(ElementStorage<VertexBase>::SectionContainer* vrtCon,
						ElementStorage<EdgeBase>::SectionContainer* edgeCon,
						ElementStorage<Face>::SectionContainer* faceCon,
						ElementStorage<Volume>::SectionContainer* volCon);

	///	returns the number of levels
		inline size_t num_levels() const		{return m_levels.size();}
		
	//	Iterators
	//	begin
	/**	returns the begin iterator for the specified level.
	 *	If no level is given iterators for level 0 are returned.*/
		template <class TGeomObj>
		inline
		typename geometry_traits<TGeomObj>::iterator
		begin(size_t level = 0);

	//	end
	/**	returns the end iterator for the specified level.
	 *	If no level is given iterators for level 0 are returned.*/
		template <class TGeomObj>
		inline
		typename geometry_traits<TGeomObj>::iterator
		end(size_t level = 0);

		inline VertexBaseIterator	vertices_begin(size_t level = 0)	{return begin<VertexBase>(level);}
		inline VertexBaseIterator	vertices_end(size_t level = 0)		{return end<VertexBase>(level);}
		inline EdgeBaseIterator		edges_begin(size_t level = 0)		{return begin<EdgeBase>(level);}
		inline EdgeBaseIterator		edges_end(size_t level = 0)			{return end<EdgeBase>(level);}
		inline FaceIterator			faces_begin(size_t level = 0)		{return begin<Face>(level);}
		inline FaceIterator			faces_end(size_t level = 0)			{return end<Face>(level);}
		inline VolumeIterator		volumes_begin(size_t level = 0)		{return begin<Volume>(level);}
		inline VolumeIterator		volumes_end(size_t level = 0)		{return end<Volume>(level);}

	//	const iterators
	//	begin
		template <class TGeomObj>
		inline
		typename geometry_traits<TGeomObj>::const_iterator
		begin(size_t level = 0) const;

	//	end
		template <class TGeomObj>
		inline
		typename geometry_traits<TGeomObj>::const_iterator
		end(size_t level = 0) const;

		inline ConstVertexBaseIterator	vertices_begin(size_t level = 0) const	{return begin<VertexBase>(level);}
		inline ConstVertexBaseIterator	vertices_end(size_t level = 0) const	{return end<VertexBase>(level);}
		inline ConstEdgeBaseIterator	edges_begin(size_t level = 0) const		{return begin<EdgeBase>(level);}
		inline ConstEdgeBaseIterator	edges_end(size_t level = 0) const		{return end<EdgeBase>(level);}
		inline ConstFaceIterator		faces_begin(size_t level = 0) const		{return begin<Face>(level);}
		inline ConstFaceIterator		faces_end(size_t level = 0) const		{return end<Face>(level);}
		inline ConstVolumeIterator		volumes_begin(size_t level = 0) const	{return begin<Volume>(level);}
		inline ConstVolumeIterator		volumes_end(size_t level = 0) const		{return end<Volume>(level);}
		
	//	element numbers
		template <class TGeomObj>
		size_t num() const;
		
		inline size_t num_vertices() const	{return num<VertexBase>();}
		inline size_t num_edges() const		{return num<EdgeBase>();}
		inline size_t num_faces() const		{return num<Face>();}
		inline size_t num_volumes() const	{return num<Volume>();}
		
		template <class TGeomObj>
		inline
		size_t num(size_t level) const;
		
		inline size_t num_vertices(size_t level) const	{return num<VertexBase>(level);}
		inline size_t num_edges(size_t level) const		{return num<EdgeBase>(level);}
		inline size_t num_faces(size_t level) const		{return num<Face>(level);}
		inline size_t num_volumes(size_t level) const	{return num<Volume>(level);}
		
	protected:
		void assign(const GeometricObjectCollection& goc);

		template <class TGeomObj> inline
		const typename ElementStorage<typename geometry_traits<TGeomObj>::geometric_base_object>::
		SectionContainer* get_container(size_t level) const;
		
		template <class TGeomObj> inline
		typename ElementStorage<typename geometry_traits<TGeomObj>::geometric_base_object>::
		SectionContainer* get_container(size_t level);
				
	protected:
		struct ContainerCollection{
			ContainerCollection()	{}
			ContainerCollection(ElementStorage<VertexBase>::SectionContainer* vrtCon,
								ElementStorage<EdgeBase>::SectionContainer* edgeCon,
								ElementStorage<Face>::SectionContainer* faceCon,
								ElementStorage<Volume>::SectionContainer* volCon);

			ElementStorage<VertexBase>::SectionContainer*	vrtContainer;
			ElementStorage<EdgeBase>::SectionContainer*		edgeContainer;
			ElementStorage<Face>::SectionContainer*			faceContainer;
			ElementStorage<Volume>::SectionContainer*		volContainer;
		};
		
		typedef std::vector<ContainerCollection> ContainerVec;
		//typedef std::vector<GeometricObjectCollection> GOCVec;

	protected:
		ContainerVec	m_levels;
};

/// @}
}//	end of namespace

////////////////////////////////////////////////
//	include implementation
#include "geometric_object_collection_impl.hpp"

#endif
