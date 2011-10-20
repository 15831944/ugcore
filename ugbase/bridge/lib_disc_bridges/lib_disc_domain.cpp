/*
 * lib_disc_bridge_domain_dependent.cpp
 *
 *  Created on: 22.09.2010
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>
#include <string>

// include bridge
#include "../bridge.h"
#include "registry/registry.h"

// lib_algebra includes
#include "lib_algebra/cpu_algebra_types.h"
#include "lib_algebra/operator/operator_util.h"
#include "lib_algebra/operator/operator_interface.h"
#include "lib_algebra/operator/operator_inverse_interface.h"

// lib_disc includes
#include "lib_disc/domain.h"
#include "lib_disc/function_spaces/grid_function.h"
#include "lib_disc/function_spaces/approximation_space.h"
#include "lib_disc/function_spaces/grid_function_util.h"
#include "lib_disc/function_spaces/interpolate.h"
#include "lib_disc/function_spaces/integrate.h"
#include "lib_disc/function_spaces/integrateDraft.h"
#include "lib_disc/function_spaces/error_indicator.h"
#include "lib_disc/dof_manager/cuthill_mckee.h"
#include "lib_disc/dof_manager/p1conform/p1conform.h"
#include "lib_disc/dof_manager/cuthill_mckee.h"
#include "lib_disc/dof_manager/lexorder.h"
#include "lib_disc/dof_manager/conform/conform.h"
#include "lib_disc/dof_manager/p1conform/p1conform.h"

#include "lib_disc/spatial_disc/domain_disc.h"
#include "lib_disc/spatial_disc/elem_disc/elem_disc_interface.h"
#include "lib_disc/spatial_disc/constraints/constraint_interface.h"
#include "lib_disc/spatial_disc/constraints/dirichlet_boundary/lagrange_dirichlet_boundary.h"
#include "lib_disc/spatial_disc/constraints/continuity_constraints/p1_continuity_constraints.h"


using namespace std;

namespace ug
{

namespace bridge
{

template <typename TDomain, typename TAlgebra, typename TDoFDistribution>
void RegisterLibDiscDomain__Algebra_DoFDistribution_Domain(Registry& reg, string parentGroup)
{
//	typedef
	static const int dim = TDomain::dim;
	typedef typename TAlgebra::vector_type vector_type;
	typedef typename TAlgebra::matrix_type matrix_type;
	typedef ApproximationSpace<TDomain, TDoFDistribution, TAlgebra> approximation_space_type;

#ifdef UG_PARALLEL
		typedef ParallelGridFunction<GridFunction<TDomain, TDoFDistribution, TAlgebra> > function_type;
#else
		typedef GridFunction<TDomain, TDoFDistribution, TAlgebra> function_type;
#endif

//	group string
	string approxGrp = parentGroup; approxGrp.append("/ApproximationSpace");

//	suffix and tag
	string dimAlgDDSuffix = GetDomainSuffix<TDomain>();
	dimAlgDDSuffix.append(GetAlgebraSuffix<TAlgebra>());
	dimAlgDDSuffix.append(GetDoFDistributionSuffix<TDoFDistribution>());

	string dimAlgDDTag = GetDomainTag<TDomain>();
	dimAlgDDTag.append(GetAlgebraTag<TAlgebra>());
	dimAlgDDTag.append(GetDoFDistributionTag<TDoFDistribution>());

//	GridFunction
	{
		string name = string("GridFunction").append(dimAlgDDSuffix);
		reg.add_class_<function_type, vector_type>(name, approxGrp)
			.template add_constructor<void (*)(approximation_space_type&)>("ApproximationSpace")
			.template add_constructor<void (*)(approximation_space_type&, typename approximation_space_type::dof_distribution_type&)>("ApproximationSpace#DoFDistribution")
			.add_method("assign", static_cast<bool (function_type::*)(const vector_type&)>(&function_type::assign),
						"Success", "Vector")
			.add_method("clone", &function_type::clone);
		reg.add_class_to_group(name, "GridFunction", dimAlgDDTag);
	}

//  ApproximationSpace
	{
		typedef ApproximationSpace<TDomain, TDoFDistribution, TAlgebra> T;
		typedef IApproximationSpace<TDomain> TBase;
		string name = string("ApproximationSpace").append(dimAlgDDSuffix);
		reg.add_class_<T, TBase>(name, approxGrp)
			.template add_constructor<void (*)(TDomain&)>("Domain")
			.add_method("set_grouping", &T::set_grouping)
			.add_method("print_statistic", static_cast<void (T::*)(int)>(&T::print_statistic))
			.add_method("print_statistic", static_cast<void (T::*)()>(&T::print_statistic))
			.add_method("print_layout_statistic", static_cast<void (T::*)(int)>(&T::print_layout_statistic))
			.add_method("print_layout_statistic", static_cast<void (T::*)()>(&T::print_layout_statistic))
			.add_method("print_local_dof_statistic", static_cast<void (T::*)(int)>(&T::print_local_dof_statistic))
			.add_method("print_local_dof_statistic", static_cast<void (T::*)()>(&T::print_local_dof_statistic))
			.add_method("defragment", &T::defragment)
			.add_method("get_surface_view", &T::get_surface_view)
			.add_method("get_surface_dof_distribution",  static_cast<const typename T::dof_distribution_type& (T::*)() const>(&T::get_surface_dof_distribution));
		reg.add_class_to_group(name, "ApproximationSpace", dimAlgDDTag);
	}

//	Order Cuthill-McKee
	{
		reg.add_function("OrderCuthillMcKee", static_cast<bool (*)(approximation_space_type&, bool)>(&OrderCuthillMcKee), approxGrp);
	}

//	Order lexicographically
	{
		typedef ApproximationSpace<TDomain, TDoFDistribution, TAlgebra> T;
		reg.add_function("OrderLex", (bool (*)(T&, const char*))&OrderLex, approxGrp);
	}

	string domDiscGrp = parentGroup; domDiscGrp.append("/SpatialDisc");

//	DomainDiscretization
	{
		typedef IDomainDiscretization<TDoFDistribution, TAlgebra> TBase;
		typedef DomainDiscretization<TDomain, TDoFDistribution, TAlgebra> T;
		typedef typename T::dof_distribution_type dof_distribution_type;
		string name = string("DomainDiscretization").append(dimAlgDDSuffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.template add_constructor<void (*)(approximation_space_type&)>("ApproximationSpace")
			.add_method("add", static_cast<void (T::*)(IDomainConstraint<TDomain, TDoFDistribution, TAlgebra>&)>(&T::add),
						"", "Post Process")
			.add_method("add", static_cast<void (T::*)(IDomainElemDisc<TDomain>&)>(&T::add),
						"", "Element Discretization")
			.add_method("add", static_cast<void (T::*)(IDiscretizationItem<TDomain,TDoFDistribution,TAlgebra>&)>(&T::add),
						"", "DiscItem")
			.add_method("assemble_linear", static_cast<bool (T::*)(matrix_type&, vector_type&, const vector_type&)>(&T::assemble_linear))
			.add_method("assemble_solution", static_cast<bool (T::*)(vector_type&)>(&T::assemble_solution))
			.add_method("assemble_mass_matrix", static_cast<bool (T::*)(matrix_type&, const vector_type&, const dof_distribution_type&)>(&T::assemble_mass_matrix))
			.add_method("assemble_mass_matrix", static_cast<bool (T::*)(matrix_type&, const vector_type&)>(&T::assemble_mass_matrix))
			.add_method("assemble_stiffness_matrix", static_cast<bool (T::*)(matrix_type&, const vector_type&, const dof_distribution_type&)>(&T::assemble_stiffness_matrix))
			.add_method("assemble_stiffness_matrix", static_cast<bool (T::*)(matrix_type&, const vector_type&)>(&T::assemble_stiffness_matrix))
			.add_method("assemble_rhs", static_cast<bool (T::*)(vector_type&, const vector_type&, const dof_distribution_type&)>(&T::assemble_rhs))
			.add_method("assemble_rhs", static_cast<bool (T::*)(vector_type&, const vector_type&)>(&T::assemble_rhs));
		reg.add_class_to_group(name, "DomainDiscretization", dimAlgDDTag);
	}

//	IDomainConstraint
	{
		std::string grp = parentGroup; grp.append("/Discretization/SpatialDisc");
		typedef IConstraint<TDoFDistribution, TAlgebra> TBase;
		typedef IDomainConstraint<TDomain, TDoFDistribution, TAlgebra> T;
		string name = string("IDomainConstraint").append(dimAlgDDSuffix);
		reg.add_class_<T, TBase>(name, grp);
		reg.add_class_to_group(name, "IDomainConstraint", dimAlgDDTag);
	}

//	OneSideP1ConstraintsPostProcess
	{
		std::string grp = parentGroup; grp.append("/Discretization/SpatialDisc");
		typedef OneSideP1ConstraintsPostProcess<TDomain, TDoFDistribution, TAlgebra> T;
		typedef IDomainConstraint<TDomain, TDoFDistribution, TAlgebra> baseT;
		string name = string("OneSideP1Constraints").append(dimAlgDDSuffix);
		reg.add_class_<T, baseT>(name, grp)
			.add_constructor();
		reg.add_class_to_group(name, "OneSideP1Constraints", dimAlgDDTag);
	}

//	SymP1ConstraintsPostProcess
	{
		std::string grp = parentGroup; grp.append("/Discretization/SpatialDisc");
		typedef SymP1ConstraintsPostProcess<TDomain, TDoFDistribution, TAlgebra> T;
		typedef IDomainConstraint<TDomain, TDoFDistribution, TAlgebra> baseT;
		string name = string("SymP1Constraints").append(dimAlgDDSuffix);
		reg.add_class_<T, baseT>(name, grp)
			.add_constructor();
		reg.add_class_to_group(name, "SymP1Constraints", dimAlgDDTag);
	}

//	LagrangeDirichletBoundary
	{
		typedef boost::function<bool (number& value, const MathVector<dim>& x, number time)> BNDNumberFunctor;
		typedef boost::function<void (number& value, const MathVector<dim>& x, number time)> NumberFunctor;
		typedef LagrangeDirichletBoundary<TDomain, TDoFDistribution, TAlgebra> T;
		typedef IDomainConstraint<TDomain, TDoFDistribution, TAlgebra> TBase;
		string name = string("DirichletBoundary").append(dimAlgDDSuffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.add_constructor()
			.add_method("add", static_cast<void (T::*)(BNDNumberFunctor&, const char*, const char*)>(&T::add),
						"Success", "Value#Function#Subsets")
			.add_method("add", static_cast<void (T::*)(NumberFunctor&, const char*, const char*)>(&T::add),
						"Success", "Value#Function#Subsets")
			.add_method("add",static_cast<void (T::*)(number, const char*, const char*)>(&T::add),
						"Success", "Constant Value#Function#Subsets")
			.add_method("clear", &T::clear);
		reg.add_class_to_group(name, "DirichletBoundary", dimAlgDDTag);
	}

//	IDiscretizationItem
	{
		typedef IDiscretizationItem<TDomain, TDoFDistribution, TAlgebra> T;
		string name = string("IDiscretizationItem").append(dimAlgDDSuffix);
		reg.add_class_<T>(name, domDiscGrp);
		reg.add_class_to_group(name, "IDiscretizationItem", dimAlgDDTag);
	}

//	MarkForRefinement_GradientIndicator
	{
		string grp("ug4/Refinement/");
		reg.add_function("MarkForRefinement_GradientIndicator",
						 &MarkForRefinement_GradientIndicator<function_type>, grp);
	}

	string grp = parentGroup; grp.append("");

//	InterpolateFunction
	{
		typedef bool (*fct_type)(
				const boost::function<void (number& res,const MathVector<dim>& x, number time)>&,
				function_type&, const char*, number);
		reg.add_function("InterpolateFunction",
						 static_cast<fct_type>(&InterpolateFunction<function_type>),
						 grp);

		typedef bool (*fct_type_subset)(
				const boost::function<void (number& res,const MathVector<dim>& x, number time)>&,
				function_type&, const char*, number, const char*);
		reg.add_function("InterpolateFunction",
						 static_cast<fct_type_subset>(&InterpolateFunction<function_type>),
						 grp);
	}

//	L2Error
	{
		typedef number (*fct_type)(
				const boost::function<void (number& res,const MathVector<dim>& x, number time)>&,
				function_type&, const char*, number);
		reg.add_function("L2Error",
						 static_cast<fct_type>(&L2Error<function_type>),
						 grp);

		typedef number (*fct_type_subset)(
				const boost::function<void (number& res,const MathVector<dim>& x, number time)>&,
				function_type&, const char*, number, const char*);
		reg.add_function("L2Error",
						 static_cast<fct_type_subset>(&L2Error<function_type>),
						 grp);
	}

//	L2ErrorDraft
	{
		typedef number (*fct_type)(
				const boost::function<void (number& res,const MathVector<dim>& x, number time)>&,
				function_type&, const char*, number, int, const char*);
		reg.add_function("L2ErrorDraft",
						 static_cast<fct_type>(&L2ErrorDraft<function_type>),
						 grp);
	}
}

template <typename TAlgebra, typename TDoFDistribution>
static bool RegisterLibDiscDomain__Algebra_DoFDistribution(Registry& reg, string parentGroup)
{
//	get group string
	string grp = parentGroup; grp.append("/Discretization");

	try
	{

#ifdef UG_DIM_1
//	Domain dependent part 1D
	{
		typedef Domain<1, MultiGrid, MGSubsetHandler> domain_type;
		RegisterLibDiscDomain__Algebra_DoFDistribution_Domain<domain_type, TAlgebra, TDoFDistribution>(reg, grp);
	}
#endif

#ifdef UG_DIM_2
//	Domain dependent part 2D
	{
		typedef Domain<2, MultiGrid, MGSubsetHandler> domain_type;
		RegisterLibDiscDomain__Algebra_DoFDistribution_Domain<domain_type, TAlgebra, TDoFDistribution>(reg, grp);
	}
#endif

#ifdef UG_DIM_3
//	Domain dependent part 3D
	{
		typedef Domain<3, MultiGrid, MGSubsetHandler> domain_type;
		RegisterLibDiscDomain__Algebra_DoFDistribution_Domain<domain_type, TAlgebra, TDoFDistribution>(reg, grp);
	}
#endif

	}
	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterLibDiscDomain__Algebra_DoFDistribution: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}

template <typename TAlgebra>
static bool RegisterLibDiscDomain__Algebra(Registry& reg, string parentGroup)
{
	bool bReturn = true;
#ifdef DOF_P1
	bReturn &= RegisterLibDiscDomain__Algebra_DoFDistribution<TAlgebra, P1DoFDistribution>(reg, parentGroup);
#endif
#ifdef DOF_GEN
	bReturn &= RegisterLibDiscDomain__Algebra_DoFDistribution<TAlgebra, DoFDistribution >(reg, parentGroup);
#endif

	return bReturn;
}

bool RegisterLibDisc_Domain(Registry& reg, string parentGroup)
{
	bool bReturn = true;
	bReturn &= RegisterLibDiscDomain__Algebra<CPUAlgebra>(reg, parentGroup);
//	bReturn &= RegisterLibDiscDomain__Algebra<CPUBlockAlgebra<2> >(reg, parentGroup);
	bReturn &= RegisterLibDiscDomain__Algebra<CPUBlockAlgebra<3> >(reg, parentGroup);
//	bReturn &= RegisterLibDiscDomain__Algebra<CPUBlockAlgebra<4> >(reg, parentGroup);
//	bReturn &= RegisterLibDiscDomain__Algebra<CPUVariableBlockAlgebra >(reg, parentGroup);
	return bReturn;
}

}//	end of namespace ug
}//	end of namespace interface
