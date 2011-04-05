//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m10 d15

#ifndef __H__LIB_GRID__GRID_CONSTANTS__
#define __H__LIB_GRID__GRID_CONSTANTS__

namespace ug
{

/// \addtogroup lib_grid
/// @{

////////////////////////////////////////////////////////////////////////
//	VertexOptions
///	Used to specify the way in which Grid manages vertex-specific data.
enum VertexOptions
{
	VRTOPT_NONE = 0x00000000,
	VRTOPT_STORE_ASSOCIATED_EDGES = 0x00000001,
	VRTOPT_STORE_ASSOCIATED_FACES = 0x00000002,
	VRTOPT_STORE_ASSOCIATED_VOLUMES = 0x00000004,
	//VRTOPT_STORE_ADJACENT_VERTICES = 0x00000008	//not yet implemented
};

////////////////////////////////////////////////////////////////////////
//	EdgeOptions
///	Used to specify the way in which Grid manages edge-specific data.
enum EdgeOptions
{
	EDGEOPT_NONE = 0x00000000,
	EDGEOPT_STORE_ASSOCIATED_FACES = 0x00000100,
	EDGEOPT_STORE_ASSOCIATED_VOLUMES = 0x00000200,
	//EDGEOPT_STORE_ADJACENT_EDGES = 0x00000400	//not yet implemented
};

////////////////////////////////////////////////////////////////////////
//	FaceOptions
///	Used to specify the way in which Grid manages face-specific data.
enum FaceOptions
{
	FACEOPT_NONE = 0x00000000,
	FACEOPT_STORE_ASSOCIATED_EDGES = 0x00010000,	///< minor speed-improvement for grid.get_edge(Face*, int)
	FACEOPT_STORE_ASSOCIATED_VOLUMES = 0x00020000,
	//FACEOPT_STORE_ADJACENT_FACES = 0x00040000, //not yet implemented
	FACEOPT_AUTOGENERATE_EDGES = 0x00080000
};

////////////////////////////////////////////////////////////////////////
//	VolumeOptions
///	Used to specify the way in which Grid manages volume-specific data.
enum VolumeOptions
{
	VOLOPT_NONE = 0x00000000,
	VOLOPT_STORE_ASSOCIATED_EDGES = 0x01000000,		///< minor speed-improvement for grid.get_edge(Volume*, int)
	VOLOPT_STORE_ASSOCIATED_FACES = 0x02000000,		///< speed-improvement for grid.get_face(Face*, int) ~15%
	//VOLOPT_STORE_ADJACENT_VOLUMES = 0x04000000, //not yet implemented
	VOLOPT_AUTOGENERATE_EDGES = 0x08000000,
	VOLOPT_AUTOGENERATE_FACES = 0x10000000
};

////////////////////////////////////////////////////////////////////////
//	GridOptions
///	Used to specify the way in which Grid behaves.
enum GridOptions
{
	GRIDOPT_NONE = 0x00000000,
	GRIDOPT_NO_INTERCONNECTION = 0x00000000,
	GRIDOPT_VERTEXCENTRIC_INTERCONNECTION =	  VRTOPT_STORE_ASSOCIATED_EDGES
											| VRTOPT_STORE_ASSOCIATED_FACES
											| VRTOPT_STORE_ASSOCIATED_VOLUMES,

	GRIDOPT_STANDARD_INTERCONNECTION =	  GRIDOPT_VERTEXCENTRIC_INTERCONNECTION
										//| EDGEOPT_STORE_ASSOCIATED_FACES
										//| EDGEOPT_STORE_ASSOCIATED_VOLUMES
										//| FACEOPT_STORE_ASSOCIATED_VOLUMES
										| FACEOPT_STORE_ASSOCIATED_EDGES
										| VOLOPT_STORE_ASSOCIATED_EDGES
										| VOLOPT_STORE_ASSOCIATED_FACES
										| FACEOPT_AUTOGENERATE_EDGES
										| VOLOPT_AUTOGENERATE_FACES,

	GRIDOPT_FULL_INTERCONNECTION =	  GRIDOPT_STANDARD_INTERCONNECTION
									//| FACEOPT_STORE_ASSOCIATED_EDGES
									//| VOLOPT_STORE_ASSOCIATED_EDGES
									//| VOLOPT_STORE_ASSOCIATED_FACES,
									| EDGEOPT_STORE_ASSOCIATED_FACES
									| EDGEOPT_STORE_ASSOCIATED_VOLUMES
									| FACEOPT_STORE_ASSOCIATED_VOLUMES,

	GRIDOPT_DEFAULT = GRIDOPT_NONE
};

/// @}
}

#endif
