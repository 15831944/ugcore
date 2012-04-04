// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 23.12.2011 (m,d,y)

#ifndef __H__UG__geometric_objects_3d__
#define __H__UG__geometric_objects_3d__

#include "../grid/grid.h"
#include "common/math/ugmath.h"
#include "common/assert.h"
#include "geometric_objects_0d.h"
#include "geometric_objects_1d.h"
#include "geometric_objects_2d.h"

namespace ug
{

////////////////////////////////////////////////////////////////////////
///	These numbers define where in the volume-section-container a volume will be stored.
/**	The order of the constants must not be changed! Algorithms may exist that rely on it.*/
enum VolumeContainerSections
{
	CSVOL_NONE = -1,
	CSVOL_TETRAHEDRON = 0,
	CSVOL_HEXAHEDRON = 1,
	CSVOL_PRISM = 2,
	CSVOL_PYRAMID = 3
};

////////////////////////////////////////////////////////////////////////
//	TetrahedronDescriptor
///	only used to initialize a tetrahedron. for all other tasks you should use VolumeDescripor.
/**
 * please be sure to pass the vertices in the correct order:
 * v1, v2, v3: bottom-vertices in counterclockwise order (if viewed from the top).
 * v4: top
 */
class UG_API TetrahedronDescriptor
{
	public:
		TetrahedronDescriptor()	{}
		TetrahedronDescriptor(const TetrahedronDescriptor& td);
		TetrahedronDescriptor(const VolumeVertices& vv);
		TetrahedronDescriptor(VertexBase* v1, VertexBase* v2, VertexBase* v3, VertexBase* v4);

		inline uint num_vertices() const	{return 4;}
		inline VertexBase* vertex(uint index) const	{return m_vertex[index];}

	protected:
		VertexBase*	m_vertex[4];
};

////////////////////////////////////////////////////////////////////////
//	Tetrahedron
///	the most simple volume-element.
/**
 * order of vertices should be the same as described in \sa TetrahedronDescriptor
 *
 * \ingroup lib_grid_geometric_objects
 */
class UG_API Tetrahedron : public Volume
{
	public:
		typedef Volume BaseClass;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Tetrahedron*>(pObj) != NULL;}

		Tetrahedron()	{}
		Tetrahedron(const TetrahedronDescriptor& td);
		Tetrahedron(VertexBase* v1, VertexBase* v2, VertexBase* v3, VertexBase* v4);

		virtual GeometricObject* create_empty_instance() const	{return new Tetrahedron;}

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return 4;}

		virtual EdgeDescriptor edge_desc(int index) const;
		virtual void edge_desc(int index, EdgeDescriptor& edOut) const;
		virtual uint num_edges() const;

		virtual FaceDescriptor face_desc(int index) const;
		virtual void face_desc(int index, FaceDescriptor& fdOut) const;
		virtual uint num_faces() const;

		virtual EdgeBase* create_edge(int index);	///< create the edge with index i and return it.
		virtual Face* create_face(int index);		///< create the face with index i and return it.

		virtual void get_local_vertex_indices_of_edge(size_t& ind1Out,
													  size_t& ind2Out,
													  size_t edgeInd);

		virtual void get_local_vertex_indices_of_face(std::vector<size_t>& indsOut,
													  size_t side);

	///	Creates new volume elements through refinement.
	/**	Make sure that newEdgeVertices contains 6 vertex pointers.
	 *	newFaceVertices is ignored for Tetrahedrons.*/
		virtual bool refine(std::vector<Volume*>& vNewVolumesOut,
							VertexBase** ppNewVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase** newFaceVertices,
							VertexBase* newVolumeVertex,
							const VertexBase& prototypeVertex,
							VertexBase** pSubstituteVertices = NULL);

		virtual bool collapse_edge(std::vector<Volume*>& vNewVolumesOut,
								int edgeIndex, VertexBase* newVertex,
								std::vector<VertexBase*>* pvSubstituteVertices = NULL);

		virtual void get_flipped_orientation(VolumeDescriptor& vdOut) const;

		virtual int container_section() const	{return CSVOL_TETRAHEDRON;}
		virtual ReferenceObjectID reference_object_id() const {return ROID_TETRAHEDRON;}

	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{m_vertices[index] = pVrt;}

	protected:
		VertexBase*	m_vertices[4];
};

template <>
class geometry_traits<Tetrahedron>
{
	public:
		typedef GenericGeometricObjectIterator<Tetrahedron*, VolumeIterator>		iterator;
		typedef ConstGenericGeometricObjectIterator<Tetrahedron*, VolumeIterator,
															ConstVolumeIterator>	const_iterator;

		typedef TetrahedronDescriptor Descriptor;
		typedef Volume 		geometric_base_object;

		enum
		{
			CONTAINER_SECTION = CSVOL_TETRAHEDRON,
			BASE_OBJECT_ID = VOLUME
		};
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_TETRAHEDRON;
};

typedef geometry_traits<Tetrahedron>::iterator			TetrahedronIterator;
typedef geometry_traits<Tetrahedron>::const_iterator	ConstTetrahedronIterator;


////////////////////////////////////////////////////////////////////////
//	HexahedronDescriptor
///	only used to initialize a hexahedron. for all other tasks you should use VolumeDescripor.
/**
 * please be sure to pass the vertices in the correct order:
 * v1, v2, v3, v4: bottom-vertices in counterclockwise order (if viewed from the top).
 * v5, v6, v7, v8: top-vertices in counterclockwise order (if viewed from the top).
 */
class UG_API HexahedronDescriptor
{
	public:
		HexahedronDescriptor()	{}
		HexahedronDescriptor(const HexahedronDescriptor& td);
		HexahedronDescriptor(const VolumeVertices& vv);
		HexahedronDescriptor(VertexBase* v1, VertexBase* v2, VertexBase* v3, VertexBase* v4,
							VertexBase* v5, VertexBase* v6, VertexBase* v7, VertexBase* v8);

		inline uint num_vertices() const	{return 8;}
		inline VertexBase* vertex(uint index) const	{return m_vertex[index];}

	protected:
		VertexBase*	m_vertex[8];
};

////////////////////////////////////////////////////////////////////////
//	Hexahedron
///	A volume element with 6 quadrilateral sides.
/**
 * Order of vertices should be the same as described in \sa HexahedronDescriptor
 *
 * \ingroup lib_grid_geometric_objects
 */
class UG_API Hexahedron : public Volume
{
	public:
		typedef Volume BaseClass;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Hexahedron*>(pObj) != NULL;}

		Hexahedron()	{}
		Hexahedron(const HexahedronDescriptor& td);
		Hexahedron(VertexBase* v1, VertexBase* v2, VertexBase* v3, VertexBase* v4,
					VertexBase* v5, VertexBase* v6, VertexBase* v7, VertexBase* v8);

		virtual GeometricObject* create_empty_instance() const	{return new Hexahedron;}

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return 8;}

		virtual EdgeDescriptor edge_desc(int index) const;
		virtual void edge_desc(int index, EdgeDescriptor& edOut) const;
		virtual uint num_edges() const;

		virtual FaceDescriptor face_desc(int index) const;
		virtual void face_desc(int index, FaceDescriptor& fdOut) const;
		virtual uint num_faces() const;

		virtual EdgeBase* create_edge(int index);	///< create the edge with index i and return it.
		virtual Face* create_face(int index);		///< create the face with index i and return it.

	///	see Volume::refine for a detailed description.
		virtual bool refine(std::vector<Volume*>& vNewVolumesOut,
							VertexBase** ppNewVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase** newFaceVertices,
							VertexBase* newVolumeVertex,
							const VertexBase& prototypeVertex,
							VertexBase** pSubstituteVertices = NULL);

		virtual bool collapse_edge(std::vector<Volume*>& vNewVolumesOut,
								int edgeIndex, VertexBase* newVertex,
								std::vector<VertexBase*>* pvSubstituteVertices = NULL);

		virtual void get_flipped_orientation(VolumeDescriptor& vdOut) const;

		virtual int container_section() const	{return CSVOL_HEXAHEDRON;}
		virtual ReferenceObjectID reference_object_id() const {return ROID_HEXAHEDRON;}

	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{m_vertices[index] = pVrt;}

	protected:
		VertexBase*	m_vertices[8];
};

template <>
class geometry_traits<Hexahedron>
{
	public:
		typedef GenericGeometricObjectIterator<Hexahedron*, VolumeIterator>			iterator;
		typedef ConstGenericGeometricObjectIterator<Hexahedron*, VolumeIterator,
															 ConstVolumeIterator>	const_iterator;

		typedef HexahedronDescriptor Descriptor;
		typedef Volume 		geometric_base_object;

		enum
		{
			CONTAINER_SECTION = CSVOL_HEXAHEDRON,
			BASE_OBJECT_ID = VOLUME
		};
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_HEXAHEDRON;
};

typedef geometry_traits<Hexahedron>::iterator	HexahedronIterator;
typedef geometry_traits<Hexahedron>::const_iterator	ConstHexahedronIterator;


////////////////////////////////////////////////////////////////////////
//	PrismDescriptor
///	only used to initialize a prism. for all other tasks you should use VolumeDescripor.
/**
 * please be sure to pass the vertices in the correct order:
 * v1, v2, v3: bottom-vertices in counterclockwise order (if viewed from the top).
 * v4, v5, v6: top-vertices in counterclockwise order (if viewed from the top).
 */
class UG_API PrismDescriptor
{
	public:
		PrismDescriptor()	{}
		PrismDescriptor(const PrismDescriptor& td);
		PrismDescriptor(const VolumeVertices& vv);
		PrismDescriptor(VertexBase* v1, VertexBase* v2, VertexBase* v3,
						VertexBase* v4, VertexBase* v5, VertexBase* v6);

		inline uint num_vertices() const	{return 6;}
		inline VertexBase* vertex(uint index) const	{return m_vertex[index];}

	protected:
		VertexBase*	m_vertex[6];
};

////////////////////////////////////////////////////////////////////////
//	Prism
///	A volume element with 2 triangle and 3 quadrilateral sides.
/**
 * order of vertices should be the same as described in \sa PrismDescriptor
 *
 * \ingroup lib_grid_geometric_objects
 */
class UG_API Prism : public Volume
{
	public:
		typedef Volume BaseClass;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Prism*>(pObj) != NULL;}

		Prism()	{}
		Prism(const PrismDescriptor& td);
		Prism(VertexBase* v1, VertexBase* v2, VertexBase* v3,
				VertexBase* v4, VertexBase* v5, VertexBase* v6);

		virtual GeometricObject* create_empty_instance() const	{return new Prism;}

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return 6;}

		virtual EdgeDescriptor edge_desc(int index) const;
		virtual void edge_desc(int index, EdgeDescriptor& edOut) const;
		virtual uint num_edges() const;

		virtual FaceDescriptor face_desc(int index) const;
		virtual void face_desc(int index, FaceDescriptor& fdOut) const;
		virtual uint num_faces() const;

		virtual EdgeBase* create_edge(int index);	///< create the edge with index i and return it.
		virtual Face* create_face(int index);		///< create the face with index i and return it.

	///	see Volume::refine for a detailed description.
		virtual bool refine(std::vector<Volume*>& vNewVolumesOut,
							VertexBase** ppNewVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase** newFaceVertices,
							VertexBase* newVolumeVertex,
							const VertexBase& prototypeVertex,
							VertexBase** pSubstituteVertices = NULL);

		virtual bool collapse_edge(std::vector<Volume*>& vNewVolumesOut,
								int edgeIndex, VertexBase* newVertex,
								std::vector<VertexBase*>* pvSubstituteVertices = NULL);

		virtual void get_flipped_orientation(VolumeDescriptor& vdOut) const;

		virtual int container_section() const	{return CSVOL_PRISM;}
		virtual ReferenceObjectID reference_object_id() const {return ROID_PRISM;}

	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{m_vertices[index] = pVrt;}

	protected:
		VertexBase*	m_vertices[6];
};

template <>
class geometry_traits<Prism>
{
	public:
		typedef GenericGeometricObjectIterator<Prism*, VolumeIterator>				iterator;
		typedef ConstGenericGeometricObjectIterator<Prism*, VolumeIterator,
															 ConstVolumeIterator>	const_iterator;

		typedef PrismDescriptor Descriptor;
		typedef Volume 		geometric_base_object;

		enum
		{
			CONTAINER_SECTION = CSVOL_PRISM,
			BASE_OBJECT_ID = VOLUME
		};
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_PRISM;
};

typedef geometry_traits<Prism>::iterator		PrismIterator;
typedef geometry_traits<Prism>::const_iterator	ConstPrismIterator;


////////////////////////////////////////////////////////////////////////
//	PyramidDescriptor
///	only used to initialize a pyramids. for all other tasks you should use VolumeDescripor.
/**
 * please be sure to pass the vertices in the correct order:
 * v1, v2, v3, v4: bottom-vertices in counterclockwise order (if viewed from the top).
 * v5: top-vertex.
 */
class UG_API PyramidDescriptor
{
	public:
		PyramidDescriptor()	{}
		PyramidDescriptor(const PyramidDescriptor& td);
		PyramidDescriptor(const VolumeVertices& vv);
		PyramidDescriptor(VertexBase* v1, VertexBase* v2, VertexBase* v3,
						VertexBase* v4, VertexBase* v5);

		inline uint num_vertices() const	{return 5;}
		inline VertexBase* vertex(uint index) const	{return m_vertex[index];}

	protected:
		VertexBase*	m_vertex[5];
};

////////////////////////////////////////////////////////////////////////
//	Pyramid
///	A volume element with 4 triangle and 1 quadrilateral sides.
/**
 * order of vertices should be the same as described in \sa PyramidDescriptor
 *
 * \ingroup lib_grid_geometric_objects
 */
class UG_API Pyramid : public Volume
{
	public:
		typedef Volume BaseClass;

	public:
		inline static bool type_match(GeometricObject* pObj)	{return dynamic_cast<Pyramid*>(pObj) != NULL;}

		Pyramid()	{}
		Pyramid(const PyramidDescriptor& td);
		Pyramid(VertexBase* v1, VertexBase* v2, VertexBase* v3,
				VertexBase* v4, VertexBase* v5);

		virtual GeometricObject* create_empty_instance() const	{return new Pyramid;}

		virtual VertexBase* vertex(uint index) const	{return m_vertices[index];}
		virtual ConstVertexArray vertices() const		{return m_vertices;}
		virtual size_t num_vertices() const				{return 5;}

		virtual EdgeDescriptor edge_desc(int index) const;
		virtual void edge_desc(int index, EdgeDescriptor& edOut) const;
		virtual uint num_edges() const;

		virtual FaceDescriptor face_desc(int index) const;
		virtual void face_desc(int index, FaceDescriptor& fdOut) const;
		virtual uint num_faces() const;

		virtual EdgeBase* create_edge(int index);	///< create the edge with index i and return it.
		virtual Face* create_face(int index);		///< create the face with index i and return it.

	///	see Volume::refine for a detailed description.
		virtual bool refine(std::vector<Volume*>& vNewVolumesOut,
							VertexBase** ppNewVertexOut,
							VertexBase** newEdgeVertices,
							VertexBase** newFaceVertices,
							VertexBase* newVolumeVertex,
							const VertexBase& prototypeVertex,
							VertexBase** pSubstituteVertices = NULL);

		virtual bool collapse_edge(std::vector<Volume*>& vNewVolumesOut,
								int edgeIndex, VertexBase* newVertex,
								std::vector<VertexBase*>* pvSubstituteVertices = NULL);

		virtual void get_flipped_orientation(VolumeDescriptor& vdOut) const;

		virtual int container_section() const	{return CSVOL_PYRAMID;}
		virtual ReferenceObjectID reference_object_id() const {return ROID_PYRAMID;}

	protected:
		virtual void set_vertex(uint index, VertexBase* pVrt)	{m_vertices[index] = pVrt;}

	protected:
		VertexBase*	m_vertices[5];
};

template <>
class geometry_traits<Pyramid>
{
	public:
		typedef GenericGeometricObjectIterator<Pyramid*, VolumeIterator>			iterator;
		typedef ConstGenericGeometricObjectIterator<Pyramid*, VolumeIterator,
															 ConstVolumeIterator>	const_iterator;

		typedef PyramidDescriptor Descriptor;
		typedef Volume 		geometric_base_object;

		enum
		{
			CONTAINER_SECTION = CSVOL_PYRAMID,
			BASE_OBJECT_ID = VOLUME
		};
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_PYRAMID;
};

typedef geometry_traits<Pyramid>::iterator			PyramidIterator;
typedef geometry_traits<Pyramid>::const_iterator	ConstPyramidIterator;

}//	end of namespace

#endif
