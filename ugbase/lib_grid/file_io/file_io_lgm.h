//	created by Sebastian Reiter, Nicolas Tessore
//	s.b.reiter@googlemail.com
//	y09 m03 d07

#include "lib_grid/grid/grid.h"
#include "lib_grid/common_attachments.h"
#include "lib_grid/subset_handler.h"

#ifndef __H__LIB_GRID__FILE_IO_LGM__
#define __H__LIB_GRID__FILE_IO_LGM__

namespace ug
{

bool ImportGridFromLGM(Grid& grid,
                       const char* filename,
                       APosition& aPos = aPosition,
                       SubsetHandler* pSurfaceHandler = NULL);

}//	end of namespace

#endif
