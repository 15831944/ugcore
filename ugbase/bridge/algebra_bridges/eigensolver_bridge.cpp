/*
 * eigensolver_bridge.cpp
 *
 *  Created on: 07.07.2011
 *      Author: mrupp
 */

// extern headers
#include <iostream>
#include <sstream>
#include <string>

// include bridge
#include "bridge/bridge.h"
#include "bridge/util.h"

#include "lib_algebra/lib_algebra.h"

using namespace std;

#ifdef UG_PARALLEL
#include "lib_algebra/operator/eigensolver/pinvit.h"

namespace ug{
namespace bridge{
namespace Eigensolver{

/**
 * Class exporting the functionality. All functionality that is to
 * be used in scripts or visualization must be registered here.
 */
struct Functionality
{

/**
 * Function called for the registration of Algebra dependent parts.
 * All Functions and Classes depending on Algebra
 * are to be placed here when registering. The method is called for all
 * available Algebra types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TAlgebra>
static void Algebra(Registry& reg, string grp)
{
	string suffix = GetAlgebraSuffix<TAlgebra>();
	string tag = GetAlgebraTag<TAlgebra>();

//	typedefs for this algebra
	typedef typename TAlgebra::vector_type vector_type;
	typedef typename TAlgebra::matrix_type matrix_type;

#ifdef LAPACK_AVAILABLE
		string name = string("EigenSolver").append(suffix);
		typedef PINVIT<TAlgebra> T;
		reg.add_class_<T>(name, grp)
			.add_constructor()
			.add_method("add_vector", &T::add_vector,
						"", "vector")
			.add_method("set_preconditioner", &T::set_preconditioner,
						"", "Preconditioner")
			.add_method("set_linear_operator_A", &T::set_linear_operator_A,
						"", "LinearOperatorA")
			.add_method("set_linear_operator_B", &T::set_linear_operator_B,
						"", "LinearOperatorB")
			.add_method("set_max_iterations", &T::set_max_iterations,
							"", "precision")
			.add_method("set_precision", &T::set_precision,
							"", "precision")
			.add_method("set_pinvit", &T::set_pinvit, "", "iPINVIT", "1 = preconditioned inverse block iteration, 2 = preconditioned block gradient descent, 3 = LOBPCG")
			.add_method("apply", &T::apply);
		reg.add_class_to_group(name, "EigenSolver", tag);
#endif
}
}; // end Functionality
}// end Eigensolver


void RegisterBridge_Eigensolver(Registry& reg, string grp)
{
	grp.append("/Algebra/Solver");
	typedef Eigensolver::Functionality Functionality;
//#ifdef UG_CPU_1
	typedef boost::mpl::list<CPUAlgebra> AlgList;
/*#else
	typedef boost::mpl::list<> AlgList;
#endif*/

	try{
		RegisterAlgebraDependent<Functionality, AlgList>(reg,grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}

} // namespace bridge
} // namespace ug

#else
namespace ug{
namespace bridge{

void RegisterBridge_Eigensolver(Registry& reg, string grp)
{}

} // namespace bridge
} // namespace ug

#endif
