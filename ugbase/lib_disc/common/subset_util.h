/*
 * subset_util.h
 *
 *  Created on: 05.03.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__COMMON__SUBSET_UTIL__
#define __H__UG__LIB_DISC__COMMON__SUBSET_UTIL__


#include "lib_grid/tools/subset_handler_interface.h"
#include "lib_grid/tools/subset_handler_multi_grid.h"
#include "lib_grid/tools/subset_handler_grid.h"

namespace ug{

////////////////////////////////////////////////////////////////////////
/// returns if a subset is a regular grid
/**
 * This function returns if a subset contains constrained/constraining elements
 * such as hanging vertices, contrained edges/faces. In this case, the subset
 * does not form a regular grid.
 *
 *
 * \param[in]	sh			SubsetHandler
 * \param[in]	si			Subset Index
 *
 * \return		true		if subset is regular grid
 * 				false 		if subset is non-regular grid
 */
inline bool SubsetIsRegularGrid(const SubsetHandler& sh, int si);

////////////////////////////////////////////////////////////////////////
/// returns if a subset is a regular grid
/**
 * This function returns if a subset contains constrained/constraining elements
 * such as hanging vertices, contrained edges/faces. In this case, the subset
 * does not form a regular grid.
 *
 *
 * \param[in]	sh			SubsetHandler
 * \param[in]	si			Subset Index
 *
 * \return		true		if subset is regular grid
 * 				false 		if subset is non-regular grid
 */
inline bool SubsetIsRegularGrid(const MGSubsetHandler& sh, int si);

////////////////////////////////////////////////////////////////////////
/// returns if a subset is a regular grid
/**
 * This function returns if a subset contains constrained/constraining elements
 * such as hanging vertices, contrained edges/faces. In this case, the subset
 * does not form a regular grid.
 *
 *
 * \param[in]	sh			SubsetHandler
 * \param[in]	si			Subset Index
 *
 * \return		true		if subset is regular grid
 * 				false 		if subset is non-regular grid
 */
inline bool SubsetIsRegularGrid(const ISubsetHandler& sh, int si);

/// abbreviations for return types
enum {DIM_SUBSET_EMPTY_GRID = -1};

////////////////////////////////////////////////////////////////////////
///	Returns the dimension of geometric objects, that are contained in the subset
/**
 * This function returns the dimension of the subset. The dimension is simply
 * defined to be the highest reference dimension of all geometric objects
 * contained in the subset
 * If a ParallelCommunicator is passed, the highest dimension within all
 * procs in the ProcessCommunicator is returned.
 *
 * \param[in]	sh			ISubsetHandler
 * \param[in]	si			Subset Index
 *
 * \return		dimension					Dimension of Subset
 * 				DIM_SUBSET_EMPTY_GRID		if empty Grid given
 */
inline int DimensionOfSubset(const ISubsetHandler& sh, int si);

////////////////////////////////////////////////////////////////////////
///	Returns the dimension of geometric objects, that are contained in the subset handler
/**
 * This function returns the dimension of the subsets. The dimension is simply
 * defined to be the highest reference dimension of all geometric objects
 * contained the union of all subset
 * If a ParallelCommunicator is passed, the highest dimension within all
 * procs in the ProcessCommunicator is returned.
 *
 * \param[in]	sh			ISubsetHandler
 *
 * \return		dimension					Dimension of Subset
 * 				DIM_SUBSET_EMPTY_GRID		if empty Grid given
 */
inline int DimensionOfSubsets(const ISubsetHandler& sh);


} // end namespace ug

#include "subset_util_impl.h"

#endif /* __H__UG__LIB_DISC__COMMON__SUBSET_UTIL__ */
