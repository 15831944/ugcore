// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 15.04.2011 (m,d,y)

/**	This file defines some macros which should be used for profiling
 * the pcl. If PROFILE_PCL is defined, the profiling is executed,
 * if not, then no profiling will be done. You should enable PROFILE_PCL
 * only during code optimization, since it introduces a noticable overhead.
 */

#ifndef __H__UG__pcl_profiling__
#define __H__UG__pcl_profiling__

#ifdef PROFILE_PCL
	#include "common/profiler/profiler.h"
	#define PCL_PROFILE_FUNC()	PROFILE_FUNC()
	#define PCL_PROFILE(name)	PROFILE_BEGIN(name)
	#define PCL_PROFILE_END()	PROFILE_END()
#else
	#define PCL_PROFILE_FUNC()
	#define PCL_PROFILE(name)
	#define PCL_PROFILE_END()
#endif

#endif
