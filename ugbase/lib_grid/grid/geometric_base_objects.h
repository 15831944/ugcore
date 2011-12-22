//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m10 d09

#ifndef __H__LIB_GRID__GEOMETRIC_BASE_OBJECTS__
#define __H__LIB_GRID__GEOMETRIC_BASE_OBJECTS__

#include <list>
#include <cassert>
#include <iostream>
#include "common/types.h"
#include "common/common.h"
#include "common/assert.h"
#include "lib_grid/attachments/attachment_pipe.h"
#include "lib_grid/attachments/attached_list.h"
#include "common/util/hash.h"
#include "common/allocators/small_object_allocator.h"

namespace ug
{
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	CONSTANTS

////////////////////////////////////////////////////////////////////////
//	GeometricBaseObject
///	enumeration of the GeometricBaseObjects that make up a grid.
enum GeometricBaseObject
{
	VERTEX = 0,
	EDGE,
	FACE,
	VOLUME,
	NUM_GEOMETRIC_BASE_OBJECTS		//always last!!!
};

////////////////////////////////////////////////////////////////////////
//	Reference-Object IDs
///	these ids are used to identify the shape of a geometric object.
enum ReferenceObjectID
{
	ROID_UNKNOWN = -1,
	ROID_VERTEX,
	ROID_EDGE,
	ROID_TRIANGLE,
	ROID_QUADRILATERAL,
	ROID_TETRAHEDRON,
	ROID_HEXAHEDRON,
	ROID_PRISM,
	ROID_PYRAMID,
	NUM_REFERENCE_OBJECTS
};

////////////////////////////////////////////////////////////////////////
inline
std::ostream& operator<< (std::ostream& outStream, ReferenceObjectID type)
{
	switch(type)
	{
		case ROID_UNKNOWN: outStream << "(invalid)"; break;
		case ROID_VERTEX: outStream << "Vertex"; break;
		case ROID_EDGE: outStream << "Edge"; break;
		case ROID_TRIANGLE: outStream << "Triangle"; break;
		case ROID_QUADRILATERAL: outStream << "Quadrilateral"; break;
		case ROID_TETRAHEDRON: outStream << "Tetrahedron"; break;
		case ROID_HEXAHEDRON: outStream << "Hexahedron"; break;
		case ROID_PRISM: outStream << "Prism"; break;
		case ROID_PYRAMID: outStream << "Pyramid"; break;
		default: throw(UGFatalError("Unknown ReferenceObjectID in operator<<"));
	}
	return outStream;
};


////////////////////////////////////////////////////////////////////////
//	PREDECLARATIONS
class Grid;
template<class TElem>	class ElementStorage;

class GeometricObject;	//	geometric base object
class VertexBase;		//	base for all 0-dimensional grid objects.
class EdgeBase;			//	base for all 1-dimensional grid objects.
class Face;				//	base for all 2-dimensional grid objects.
class Volume;			//	base for all 3-dimensional grid objects.

class EdgeDescriptor;	//	describes an edge.
class FaceDescriptor;	//	describes a face.
class VolumeDescriptor;	//	describes a volume.

class EdgeVertices;		//	manages the vertices of an edge. Base for EdgeBase and EdgeDescriptor.
class FaceVertices;		//	manages the vertices of a face. Base for Face and FaceDescriptor.
class VolumeVertices;	//	manages the vertices of a volume. Base for Volume and VolumeDescriptor.

//	pointer-types. Primarily required for template-specializations.
typedef VertexBase*	PVertexBase;
typedef EdgeBase*		PEdgeBase;
typedef Face*			PFace;
typedef Volume*		PVolume;

typedef EdgeDescriptor*	PEdgeDescriptor;
typedef FaceDescriptor*	PFaceDescriptor;
typedef VolumeDescriptor*	PVolumeDescriptor;

typedef EdgeVertices*		PEdgeVertices;
typedef FaceVertices*		PFaceVertices;
typedef VolumeVertices*	PVolumeVertices;

template<> class attachment_traits<VertexBase*, ElementStorage<VertexBase> >;
template<> class attachment_traits<EdgeBase*, ElementStorage<EdgeBase> >;
template<> class attachment_traits<Face*, ElementStorage<Face> >;
template<> class attachment_traits<Volume*, ElementStorage<Volume> >;

/**
 * \brief Geometric objects are the building blocks of a grid.
 *
 * \defgroup lib_grid_geometric_objects geometric objects
 * \ingroup lib_grid
 */
////////////////////////////////////////////////////////////////////////
//	GeometricObject
///	The base class for all geometric objects, such as vertices, edges, faces, volumes, ...
/**
 * In order to be used by libGrid, all derivatives of GeometricObject
 * have to specialize geometry_traits<GeomObjectType>.
 *
 * \ingroup lib_grid_geometric_objects
 */
class GeometricObject/* : public SmallObject<>*/
{
	friend class Grid;
	friend class attachment_traits<VertexBase*, ElementStorage<VertexBase> >;
	friend class attachment_traits<EdgeBase*, ElementStorage<EdgeBase> >;
	friend class attachment_traits<Face*, ElementStorage<Face> >;
	friend class attachment_traits<Volume*, ElementStorage<Volume> >;

	public:
		virtual ~GeometricObject()	{}

	///	create an instance of the derived type
	/**	Make sure to overload this method in derivates of this class!*/
		virtual GeometricObject* create_empty_instance() const {return NULL;}

		virtual int shared_pipe_section() const = 0;
		virtual int base_object_type_id() const = 0;//	This method probably shouldn't be there!
	/**
	 * A reference object represents a class of geometric objects.
	 * Tetrahedrons, Triangles etc are such classes.
	 * Reference ids should be defined in the file in which concrete geometric objects are defined.
	 */
		virtual ReferenceObjectID reference_object_id() const = 0;///	returns the id of the reference-object.

	protected:
	///	ATTENTION: Use this method with extreme care!
	/**	This method is for internal use only and should almost never be called
	 * by a user of lib_grid. The method sets the attachment data index and is
	 * mainly used by attachment-traits classes.*/
		inline void set_grid_data_index(uint index)		{m_gridDataIndex = index;}

	///	Returns the grid attachment data index of a geometric object.
		inline uint grid_data_index() const				{return m_gridDataIndex;}

	protected:
		uint						m_gridDataIndex;//	index to grid-attached data.
};


////////////////////////////////////////////////////////////////////////
//	The geometry_traits. This class can be specialized by each element-type.
/**
 * In order to use a custom geometric object with libGrid, you have to
 * supply a specialization for geometry_traits.
 * Specializations have to specify the following types and methods:
 *
 * MANDATORY:
 * Types:
 * - geometric_base_object:		the geometric object from which TElem derives.
 * 					has to be either VertexBase, EdgeBase, Face or Volume.
 * - iterator:		An iterator that iterates over ElementContainer<BaseClass>
 * 					and which has a constructor that takes
 * 					ElementContainer<BaseClass>::iterator as an argument.
 * 					casts should be checked when in DEBUG mode!
 *
 * constants:
 * - SHARED_PIPE_SECTION: This constant should hold the pipes section in which your objects should be placed after creation, starting from 0. See the Grid-documentation for more information.
 * - BASE_OBJECT_TYPE_ID: Has to hold one of the GeometricBaseObject constants, or -1.
 *
 * OPTIONAL:
 * Types:
 * - Descriptor:	a class which can be passed to the constructor of the element.
 */
template <class TElem>
class geometry_traits
{};


////////////////////////////////////////////////////////////////////////////////////////////////
//	GenericGeometricObjectIterator
///	Use this class as a tool to create iterators to your own geometric objects.
template <class TValue, class TBaseIterator>
class GenericGeometricObjectIterator : public TBaseIterator
{
	friend class Grid;
	template <class TIterDest, class TIterSrc> friend TIterDest iterator_cast(const TIterSrc& iter);

	public:
		typedef TValue	value_type;

	public:
		GenericGeometricObjectIterator()	{}

		GenericGeometricObjectIterator(const GenericGeometricObjectIterator& iter) :
			TBaseIterator(iter)	{}

	///	note that the * operator is read only.
		inline TValue operator* () const	{return static_cast<TValue>(TBaseIterator::operator*());}

	protected:
		GenericGeometricObjectIterator(const TBaseIterator& iter) :
			TBaseIterator(iter)	{}
};

////////////////////////////////////////////////////////////////////////////////////////////////
//	ConstGenericGeometricObjectIterator
///	Use this class as a tool to create const_iterators to your own geometric objects.
template <class TValue, class TBaseIterator, class TConstBaseIterator>
class ConstGenericGeometricObjectIterator : public TConstBaseIterator
{
	friend class Grid;
	template <class TIterDest, class TIterSrc> friend TIterDest iterator_cast(const TIterSrc& iter);

	public:
		typedef TValue	value_type;

	public:
		ConstGenericGeometricObjectIterator()	{}

		ConstGenericGeometricObjectIterator(const ConstGenericGeometricObjectIterator& iter) :
			TConstBaseIterator(iter)	{}

	///	note that the * operator is read only.
		inline TValue operator* () const	{return static_cast<TValue>(TConstBaseIterator::operator*());}

	protected:
		ConstGenericGeometricObjectIterator(const TBaseIterator& iter) :
			TConstBaseIterator(iter)	{}

		ConstGenericGeometricObjectIterator(const TConstBaseIterator& iter) :
			TConstBaseIterator(iter)	{}
};

////////////////////////////////////////////////////////////////////////
//	iterator_cast
///	You should avoid casting whenever possible!
template <class TIterDest, class TIterSrc>
inline TIterDest
iterator_cast(const TIterSrc& iter)
{
	return TIterDest(iter);
}


////////////////////////////////////////////////////////////////////////////////////////////////
//	VertexBase
///	Base-class for all vertex-types
/**
 * Vertices are required in any grid.
 * They are the geometric objects of lowest dimension.
 * All other geometric objects of higher dimension reference vertices.
 *
 * \ingroup lib_grid_geometric_objects
 */
class VertexBase : public GeometricObject
{
	friend class Grid;
	public:
		typedef VertexBase geometric_base_object;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<VertexBase*>(pObj) != NULL;}

		virtual ~VertexBase()	{}

		virtual int shared_pipe_section() const	{return -1;}
		virtual int base_object_type_id() const	{return VERTEX;}
		virtual ReferenceObjectID reference_object_id() const	{return ROID_UNKNOWN;}

	///	returns a value that can be used for hashing.
	/**	this value is unique per grid.
	 *	It is only set correctly if the vertex has been created
	 *	by the grid or has been properly registered at it.*/
		inline uint32 get_hash_value() const	{return m_hashValue;}

	protected:
		uint32	m_hashValue;//	a unique value for each vertex in a grid.
};



////////////////////////////////////////////////////////////////////////////////////////////////
//	EdgeVertices
///	holds the vertices of an EdgeBase or an EdgeDescriptor.
/**	Please note that this class does not have a virtual destructor.*/
class EdgeVertices
{
	friend class Grid;
	public:
		inline VertexBase* vertex(uint index) const	{return m_vertices[index];}
		inline uint num_vertices() const			{return 2;}	// this method is supplied to allow the use of EdgeBase in template-methods that require a num_vertices() method.

	//	compatibility with std::vector for some template routines
	///	returns the number of vertices.
		inline size_t size() const	{return 2;}
	///	returns the i-th vertex.
		VertexBase* operator[](uint index) const {return m_vertices[index];}

	protected:
		inline void assign_edge_vertices(const EdgeVertices& ev)
		{
			m_vertices[0] = ev.m_vertices[0];
			m_vertices[1] = ev.m_vertices[1];
		}

	protected:
		VertexBase*	m_vertices[2];
};

////////////////////////////////////////////////////////////////////////////////////////////////
//	EdgeBase
///	Base-class for edges
/**
 * EdgeBase is the base class of all 1-dimensional geometric objects.
 * Edges connect two vertices.
 *
 * \ingroup lib_grid_geometric_objects
 */
class EdgeBase : public GeometricObject, public EdgeVertices
{
	friend class Grid;
	public:
		typedef EdgeBase geometric_base_object;

	//	lower dimensional Base Object
		typedef VertexBase lower_dim_base_object;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<EdgeBase*>(pObj) != NULL;}

		virtual ~EdgeBase()	{}

		virtual int shared_pipe_section() const	{return -1;}
		virtual int base_object_type_id() const	{return EDGE;}
		virtual ReferenceObjectID reference_object_id() const	{return ROID_UNKNOWN;}

		virtual int num_sides() const {return 2;}
	/**
	 * create 2 new edges, connecting the original edges end-points with vrtNew.
	 * Newly created edges have to be registered at a grid manually by the caller.
	 * If the caller does not register the edges in vGeomOut at a grid, he is
	 * responsible to free the associated memory (delete each element in vNewEdgesOut).
	 * Please note that refining an edge using this method does not automatically
	 * refine associated elements.
	 * Be sure to store the new edges in the right order. vNewEdgesOut should contain
	 * the edge connecting vertex(0) and newVertex first.
	 *
	 * You may pass an array of 2 vertices to pSubstituteVrts. If you do so, Those
	 * vertices will be used instead of the original ones.
	 */
		virtual bool refine(std::vector<EdgeBase*>& vNewEdgesOut,
											VertexBase* newVertex,
											VertexBase** pSubstituteVrts = NULL)	{return false;}

	protected:
		inline void set_vertex(uint index, VertexBase* pVrt)	{m_vertices[index] = pVrt;}
};


////////////////////////////////////////////////////////////////////////////////////////////////
//	EdgeDescriptor
///	Can be used to store information about an edge and to construct an edge.
class EdgeDescriptor : public EdgeVertices
{
	public:
		EdgeDescriptor();
		EdgeDescriptor(const EdgeDescriptor& ed);
		EdgeDescriptor(VertexBase* vrt1, VertexBase* vrt2);

		EdgeDescriptor& operator = (const EdgeDescriptor& ed);

		inline void set_vertex(uint index, VertexBase* vrt)	{m_vertices[index] = vrt;}
		inline void set_vertices(VertexBase* vrt1, VertexBase* vrt2)
			{
				m_vertices[0] = vrt1;
				m_vertices[1] = vrt2;
			}
};


/**	Please note that this class does not have a virtual destructor.*/
class FaceVertices
{
	public:
		typedef VertexBase* const* ConstVertexArray;

		virtual ~FaceVertices()							{}
		virtual VertexBase* vertex(uint index) const	{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return NULL;}
		virtual ConstVertexArray vertices() const		{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return NULL;}
		virtual size_t num_vertices() const				{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return 0;}

	//	compatibility with std::vector for some template routines
	///	returns the number of vertices.
		inline size_t size() const	{return num_vertices();}
	///	returns the i-th vertex.
		inline VertexBase* operator[](size_t index) const {return vertex(index);}
};

////////////////////////////////////////////////////////////////////////////////////////////////
//	Face
///	Faces are 2-dimensional objects.
/**
 * Base class for all 2-dimensional objects.
 * Faces connect three or more vertices.
 * A face should always be flat (if viewed in 3 dimensions).
 * You can not create an instance of Face. grids are constructed from derivatives of face.
 * The vertices of a face have always to be specified in counterclockwise order!
 *
 * \ingroup lib_grid_geometric_objects
 */
class Face : public GeometricObject, public FaceVertices
{
	friend class Grid;
	public:
		using FaceVertices::ConstVertexArray;

		typedef Face geometric_base_object;

	//	lower dimensional Base Object
		typedef EdgeBase lower_dim_base_object;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Face*>(pObj) != NULL;}

		virtual ~Face()	{}

	///	returns the i-th edge of the face.
	/**	This default implementation is reimplemented by derived classes for optimal speed.*/
		virtual EdgeDescriptor edge(int index) const
			{return EdgeDescriptor(vertex(index), vertex((index+1) % size()));}

	///	returns the i-th edge of the face.
	/**	This default implementation is reimplemented by derived classes for optimal speed.*/
		virtual void edge(int index, EdgeDescriptor& edOut)
			{edOut.set_vertices(vertex(index), vertex((index+1) % size()));}

		inline uint num_edges() const	{return num_vertices();}
		inline uint num_sides() const	{return num_edges();}

		virtual int shared_pipe_section() const	{return -1;}
		virtual int base_object_type_id() const	{return FACE;}
		virtual ReferenceObjectID reference_object_id() const	{return ROID_UNKNOWN;}

	/**	A default implementation is featured to allow empty instances of
	 *	this class. This is required to allow the use of this class
	 *	for compile-time method selection by dummy-parameters.
	 *	It is cruical that derived classes overload this method.*/
		virtual EdgeBase* create_edge(int index)	{return NULL;}	///< create the edge with index i and return it.


	/**
	 * The refine method can be used to create new elements by inserting new vertices
	 * on the face.
	 * The user that calls this function is responsible to either register the new
	 * faces with a grid (the grid from which the vertices are), or to take responsibility
	 * for deletion of the acquired memory (delete each element in vNewFacesOut).
	 * - Specify vertices that shall be inserted on edges with newEdgeVertices. Vertices
	 * are inserted on the edge that corresponds to their index. Use NULL to indicate
	 * that no vertex shall be inserted on the associated edge. newEdgeVertices has to point
	 * to an array that holds as many vertices as there are edges in the face.
	 * - If the method has to create a new inner vertex, it will be returned in newFaceVertexOut.
	 * - If you specify pvSubstituteVertices, the created faces will reference the vertices in
	 * pvSubstituteVertices. Note that pvSubstituteVertices has to contain exactly as many
	 * vertices as the refined Face. Vertices with the same index correlate.
	 */
		virtual bool refine(std::vector<Face*>& vNewFacesOut,
							VertexBase** newFaceVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase* newFaceVertex = NULL,
							VertexBase** pSubstituteVertices = NULL)	{return false;}

	/**
	 * The collapse_edge method creates new geometric objects by collapsing the specified edge.
	 * The user that calls this function is responsible to either register the new
	 * faces with a grid (the grid from which the vertices are), or to take responsibility
	 * for deletion of the acquired memory (delete each element in vNewFacesOut).
	 * - edgeIndex specifies the edge which shall be collapsed. edgeIndex has to lie
	 *   between 0 and this->num_edges().
	 * - Vertices adjacent to the collapsed edge will be replaced by newVertex.
	 * - If you specify pvSubstituteVertices, the created faces will reference the vertices in
	 * pvSubstituteVertices. Note that pvSubstituteVertices has to contain exactly as many
	 * vertices as the face in which you collapse the edge. Vertices with the same index correlate.
	 */
		virtual bool collapse_edge(std::vector<Face*>& vNewFacesOut,
								int edgeIndex, VertexBase* newVertex,
								VertexBase** pSubstituteVertices = NULL)	{return false;}

	/**
	 * The collapse_edgea method creates new geometric objects by collapsing the specified edges
	 * simultaneously. This method makes sense only for faces with more than 4 edges.
	 * The user that calls this function is responsible to either register the new
	 * faces with a grid (the grid from which the vertices are), or to take responsibility
	 * for deletion of the acquired memory (delete each element in vNewFacesOut).
	 * - for each entry in vNewEdgeVertices which is not NULL, the edge with the same index will
	 *    be collapsed and replaced by the specified vertex.
	 *    The size of vNewEdgeVertices has thus to be between 0 and this->num_edges().
	 * - If you specify pvSubstituteVertices, the created faces will reference the vertices in
	 * pvSubstituteVertices. Note that pvSubstituteVertices has to contain exactly as many
	 * vertices as the face in which you collapse the edge. Vertices with the same index correlate.
	 */
		virtual bool collapse_edges(std::vector<Face*>& vNewFacesOut,
								std::vector<VertexBase*>& vNewEdgeVertices,
								VertexBase** pSubstituteVertices = NULL)	{return false;}

// BEGIN Depreciated
	/**	creates the faces that result from the splitting of the edge with index 'splitEdgeIndex'.
	 *  The user that calls this function is responsible to either register those faces with the
	 *  same grid in which 'this' was registered, or to take over responsibility for deletion of the
	 *  acquired memory.
	 *  With pvSubstituteVertices you can optionally pass a vector of vertices, which will be used
	 *  instead of the original vertices of this face. If specified, pvSubstituteVertices has to
	 *  contain exactly as many vertices as the face itself.
	 */
		virtual void create_faces_by_edge_split(int splitEdgeIndex,
							VertexBase* newVertex,
							std::vector<Face*>& vNewFacesOut,
							VertexBase** pSubstituteVertices = NULL)	{};
// END Depreciated

	/**	creates the faces that result from the collapsing of the edge with index 'splitEdgeIndex'.*/
		//virtual void create_faces_by_edge_collapse(int collapseEdgeIndex,
		//						VertexBase* newVertex,
		//						std::vector<Face*>& vNewFacesOut) = 0;

	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{UG_ASSERT(0, "SHOULDN'T BE CALLED");}
};


///	constant that defines the maximal number of vertices per face.
/**	This constant is mainly used by FaceDescriptor.
 * If required, one should be able to increase it without any problems.*/
const int MAX_FACE_VERTICES = 4;

////////////////////////////////////////////////////////////////////////////////////////////////
//	FaceDescriptor
///	Can be queried for the edges and vertices of a face.
class FaceDescriptor : public FaceVertices
{
	public:
		FaceDescriptor();
		FaceDescriptor(uint numVertices);
		FaceDescriptor(const FaceDescriptor& fd);

		virtual ~FaceDescriptor()					{}

		FaceDescriptor& operator = (const FaceDescriptor& fd);

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return m_numVertices;}

		inline void set_num_vertices(uint numVertices)	{m_numVertices = numVertices;}
		inline void set_vertex(uint index, VertexBase* vrt)
			{m_vertices[index] = vrt;}

	protected:
		VertexBase*	m_vertices[MAX_FACE_VERTICES];
		uint		m_numVertices;
};


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//	VOLUMES

////////////////////////////////////////////////////////////////////////////////////////////////
//	VolumeVertices
///	holds the vertices of a Volume or a VolumeDescriptor
/**	Please note that this class does not have a virtual destructor.*/
class VolumeVertices
{
	public:
		typedef VertexBase* const* ConstVertexArray;

		virtual ~VolumeVertices()						{}

		virtual VertexBase* vertex(uint index) const	{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return NULL;}
		virtual ConstVertexArray vertices() const		{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return NULL;}
		virtual size_t num_vertices() const				{UG_ASSERT(0, "SHOULDN'T BE CALLED"); return 0;}

	//	compatibility with std::vector for some template routines
	///	returns the number of vertices.
		inline size_t size() const	{return num_vertices();}
	///	returns the i-th vertex.
		inline VertexBase* operator[](size_t index) const {return vertex(index);}
};

////////////////////////////////////////////////////////////////////////////////////////////////
//	Volume
///	Volumes are 3-dimensional objects.
/**
 * Base class for all 3-dimensional objects.
 * Volumes connect four or more vertices.
 *
 * default implementations of all methods are featured to allow
 * empty instances of this class.
 * This is required to allow the use of this class
 * for compile-time method selection by dummy-parameters.
 * It is cruical that derived classes overload those methods.
 *
 * \ingroup lib_grid_geometric_objects
 */
class Volume : public GeometricObject, public VolumeVertices
{
	friend class Grid;
	public:
		using VolumeVertices::ConstVertexArray;

		typedef Volume geometric_base_object;

	//	lower dimensional Base Object
		typedef Face lower_dim_base_object;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Volume*>(pObj) != NULL;}

		virtual ~Volume()	{}

		virtual EdgeDescriptor edge(int index) const				{return EdgeDescriptor(NULL, NULL);}
		virtual void edge(int index, EdgeDescriptor& edOut) const	{edOut = EdgeDescriptor(NULL, NULL);}
		virtual uint num_edges() const								{return 0;}

		virtual FaceDescriptor face(int index) const				{return FaceDescriptor(0);}
		virtual void face(int index, FaceDescriptor& fdOut) const	{fdOut = FaceDescriptor(0);}
		virtual uint num_faces() const								{return 0;}
		inline uint num_sides() const								{return num_faces();}

		virtual EdgeBase* create_edge(int index)	{return NULL;}	///< create the edge with index i and return it.
		virtual Face* create_face(int index)		{return NULL;}	///< create the face with index i and return it.
		
	///	returns the local indices of an edge of the volume.
	/**	Default implementation throws an instance of int.
	 *	This should be changed by making the method pure virtual.*/
		virtual void get_local_vertex_indices_of_edge(size_t& ind1Out,
													  size_t& ind2Out,
													  size_t edgeInd)
		{
		//	("Missing implementation of get_local_vertex_indices_of_face.")
			throw(int(0));
		};
		
	///	returns the local indices of a face of the volume.
	/**	Default implementation throws an instance of int.
	 *	This should be changed by making the method pure virtual.*/
		virtual void get_local_vertex_indices_of_face(std::vector<size_t>& indsOut,
													  size_t side)
		{
		//	("Missing implementation of get_local_vertex_indices_of_face.")
			throw(int(0));
		};

	/**
	 * The refine method can be used to create new elements by inserting new vertices
	 * on the volume.
	 *
	 * New volumes will be returned in vNewFacesOut.
	 * If a new vertex has to be created from the prototypeVertex (this happens in
	 * more complicated situations) the pointer to the new vertex is returned in
	 * ppNewVertexOut. ppNewVertexOut contains NULL if no new vertex has been created.
	 *
	 * The user that calls this function is responsible to either register the new
	 * volumes and the new vertex with a grid (the grid from which the referenced
	 * vertices are), or to take responsibility for deletion of the acquired memory
	 * (delete each element in vNewVolumesOut and ppNewVertexOut - if it is not NULL).
	 *
	 * - Specify vertices that shall be inserted on edges with vNewEdgeVertices. Vertices
	 *   are inserted on the edge that corresponds to its index. Use NULL to indicate
	 *   that no vertex shall be inserted on the associated edge.
	 * - Specify vertices that shall be inserted on faces with vNewFaceVertices. Vertices
	 *   are inserted on the face that corresponds to its index. Use NULL to indicate
	 *   that no vertex shall be inserted on the associated face.
	 * - If newVolumeVertex is not NULL, newVolumeVertex will be inserted in the center of
	 *   the volume.
	 * - via the prototypeVertex you can specify the vertex-type of the vertex that is
	 *   auto-inserted if the refine operation is too complex. In most cases this will be
	 *   an instance of a standard vertex (a concrete type is required. VertexBase will not do).
	 *   The prototypeVertex has not to be registered at any grid - it may be a temporary
	 *   instance.
	 * - If you specify pvSubstituteVertices, the created volumes will reference the vertices
	 *   in pvSubstituteVertices. Note that pvSubstituteVertices has to contain exactly as
	 *   many vertices as the refined Face. Vertices with the same index correlate.
	 */
		virtual bool refine(std::vector<Volume*>& vNewVolumesOut,
							VertexBase** ppNewVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase** newFaceVertices,
							VertexBase* newVolumeVertex,
							const VertexBase& prototypeVertex,
							VertexBase** pSubstituteVertices = NULL)	{return false;}

	/**
	 * The collapse_edge method creates new geometric objects by collapsing the specified edge.
	 * The user that calls this function is responsible to either register the new
	 * faces with a grid (the grid from which the vertices are), or to take responsibility
	 * for deletion of the acquired memory (delete each element in vNewFacesOut).
	 * - edgeIndex specifies the edge which shall be collapsed. edgeIndex has to lie
	 *   between 0 and this->num_edges().
	 * - Vertices adjacent to the collapsed edge will be replaced by newVertex.
	 * - If you specify pvSubstituteVertices, the created faces will reference the vertices in
	 * pvSubstituteVertices. Note that pvSubstituteVertices has to contain exactly as many
	 * vertices as the face in which you collapse the edge. Vertices with the same index correlate.
	 */
		virtual bool collapse_edge(std::vector<Volume*>& vNewVolumesOut,
								int edgeIndex, VertexBase* newVertex,
								std::vector<VertexBase*>* pvSubstituteVertices = NULL)	{return false;}

	/**
	 * Writes vertices to the volume-descriptor so that it defines a volume with
	 * flipped orientation. If you want to flip the orientation of a volume in a
	 * grid, please consider using the grids flip_orientation method.
	 *
	 * Please note: The default implementation returns the original volume and has
	 * to be reimplemented by derived classes.
	 */
	 	virtual void get_flipped_orientation(VolumeDescriptor& vdOut) const;
		
		virtual int shared_pipe_section() const	{return -1;}
		virtual int base_object_type_id() const	{return VOLUME;}
		virtual ReferenceObjectID reference_object_id() const	{return ROID_UNKNOWN;}

	/**	creates the volumes that result from the splitting of the edge with index 'splitEdgeIndex'.*/
		//virtual void create_volumes_by_edge_split(int splitEdgeIndex,
		//						VertexBase* newVertex,
		//						std::vector<Volume*>& vNewFacesOut) = 0;

	/**	creates the volumes that result from the collapsing of the edge with index 'splitEdgeIndex'.*/
		//virtual void create_Volumes_by_edge_collapse(int collapseEdgeIndex,
		//						VertexBase* newVertex,
		//						std::vector<Volume*>& vNewFacesOut) = 0;
	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{UG_ASSERT(0, "SHOULDN'T BE CALLED");}
};


///	constant that defines the maximal number of vertices per face.
/**	This constant is mainly used by VolumeDescriptor.
 * If required, one should be able to increase it without any problems.*/
const int MAX_VOLUME_VERTICES = 8;

////////////////////////////////////////////////////////////////////////////////////////////////
//	VolumeDescriptor
///	Can be queried for the edges, faces and vertices of a volume.
class VolumeDescriptor : public VolumeVertices
{
	public:
		VolumeDescriptor();
		VolumeDescriptor(uint numVertices, uint numEdges, uint numFaces);
		VolumeDescriptor(const VolumeDescriptor& vd);

		virtual ~VolumeDescriptor()										{}

		VolumeDescriptor& operator = (const VolumeDescriptor& vv);
		VolumeDescriptor& operator = (const VolumeVertices& vv);

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return m_numVertices;}

		inline void set_num_vertices(uint numVertices)		{m_numVertices = numVertices;}
		inline void set_vertex(uint index, VertexBase* vrt)	{m_vertices[index] = vrt;}

	protected:
		VertexBase*	m_vertices[MAX_VOLUME_VERTICES];
		uint		m_numVertices;
};


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/**	template helpers that return the geometric base object type
 *	given a pointer to a derived class of VertexBase, EdgeBase, Face or Volume.
 *
 *	e.g. PtrTypeToGeomObjBaseType<Vertex*>::base_type = VertexBase.
 */
template <class TGeomObjPtrType>
struct PtrTypeToGeomObjBaseType
{typedef void base_type;};

template <>
struct PtrTypeToGeomObjBaseType<VertexBase*>
{typedef VertexBase base_type;};

template <>
struct PtrTypeToGeomObjBaseType<EdgeBase*>
{typedef EdgeBase base_type;};

template <>
struct PtrTypeToGeomObjBaseType<Face*>
{typedef Face base_type;};

template <>
struct PtrTypeToGeomObjBaseType<Volume*>
{typedef Volume base_type;};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	hash-funtions for vertices
///	returns the hash-value of the vertex.
template <>
unsigned long hash_key<PVertexBase>(const PVertexBase& key);

////////////////////////////////////////////////////////////////////////
//	hash-funtions for edges
///	the hash-key is a function of vertex-hash-values.
/**
 * The hash value depends on the associated vertices.
 * If an EdgeBase (or EdgeDescriptor) has the same vertices
 * as another EdgeBase (or EdgeDescriptor), the hash-keys
 * are the same.
 */
template <>
unsigned long hash_key<PEdgeVertices>(const PEdgeVertices& key);

///	the hash-key is a function of vertex-hash-values.
/** \sa hash_key<PEdgeVertices>*/
template <>
unsigned long hash_key<PEdgeBase>(const PEdgeBase& key);

///	the hash-key is a function of vertex-hash-values.
/** \sa hash_key<PEdgeVertices>*/
template <>
unsigned long hash_key<PEdgeDescriptor>(const PEdgeDescriptor& key);

////////////////////////////////////////////////////////////////////////
//	hash-funtions for faces
///	the hash-key is a function of vertex-hash-values.
/**
 * The hash value depends on the associated vertices.
 * If an Face (or FaceDescriptor) has the same vertices
 * as another Face (or FaceDescriptor), the hash-keys
 * are the same.
 */
template <>
unsigned long hash_key<PFaceVertices>(const PFaceVertices& key);

///	the hash-key is a function of vertex-hash-values.
/**\sa hash_key<PFaceVertices>*/
template <>
unsigned long hash_key<PFace>(const PFace& key);

///	the hash-key is a function of vertex-hash-values.
/**\sa hash_key<PFaceVertices>*/
template <>
unsigned long hash_key<PFaceDescriptor>(const PFaceDescriptor& key);

////////////////////////////////////////////////////////////////////////
//	hash-funtions for volumes
///	the hash-key is a function of vertex-hash-values.
/**
 * The hash value depends on the associated vertices.
 * If an Volume (or VolumeDescriptor) has the same vertices
 * as another Volume (or VolumeDescriptor), the hash-keys
 * are the same.
 */
template <>
unsigned long hash_key<PVolumeVertices>(const PVolumeVertices& key);

///	the hash-key is a function of vertex-hash-values.
/**\sa hash_key<PVolumeVertices>*/
template <>
unsigned long hash_key<PVolume>(const PVolume& key);

///	the hash-key is a function of vertex-hash-values.
/**\sa hash_key<PVolumeVertices>*/
template <>
unsigned long hash_key<PVolumeDescriptor>(const PVolumeDescriptor& key);

}//	end of namespace

#endif
