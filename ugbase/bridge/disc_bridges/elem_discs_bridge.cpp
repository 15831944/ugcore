/*
 * elem_discs_bridge.cpp
 *
 *  Created on: 20.05.2011
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>
#include <string>

// include bridge
#include "bridge/bridge.h"
#include "bridge/util.h"

// lib_disc includes
#include "lib_disc/domain.h"

#include "lib_disc/spatial_disc/disc_util/conv_shape_interface.h"
#include "lib_disc/spatial_disc/disc_util/conv_shape.h"

#include "lib_disc/spatial_disc/elem_disc/neumann_boundary/neumann_boundary.h"
#include "lib_disc/spatial_disc/elem_disc/inner_boundary/inner_boundary.h"
#include "lib_disc/spatial_disc/elem_disc/inner_boundary/FV1CalciumERElemDisc.h"

using namespace std;

namespace ug{
namespace bridge{
namespace ElemDiscs{

/**
 * Class exporting the functionality. All functionality that is to
 * be used in scripts or visualization must be registered here.
 */
struct Functionality
{

/**
 * Function called for the registration of Domain dependent parts.
 * All Functions and Classes depending on the Domain
 * are to be placed here when registering. The method is called for all
 * available Domain types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TDomain>
static void Domain(Registry& reg, string grp)
{
	string suffix = GetDomainSuffix<TDomain>();
	string tag = GetDomainTag<TDomain>();
	static const int dim = TDomain::dim;

	string approxGrp = grp; approxGrp.append("/ApproximationSpace");
	string elemGrp = grp; elemGrp.append("/SpatialDisc/ElemDisc");

//	DomainElemDisc base class
	{
		typedef IDomainElemDisc<TDomain> T;
		typedef IElemDisc TBase;
		string name = string("IDomainElemDisc").append(suffix);
		reg.add_class_<T, TBase >(name, elemGrp);
		reg.add_class_to_group(name, "IDomainElemDisc", tag);
	}

//	Neumann Boundary
	{
		typedef NeumannBoundary<TDomain> T;
		typedef IDomainElemDisc<TDomain> TBase;
		string name = string("NeumannBoundary").append(suffix);
		reg.add_class_<T, TBase >(name, elemGrp)
			.template add_constructor<void (*)(const char*)>("Function(s)")
			.add_method("add", static_cast<void (T::*)(number, const char*, const char*)>(&T::add))
			.add_method("add", static_cast<void (T::*)(SmartPtr<UserData<number, dim> >, const char*, const char*)>(&T::add))
			.add_method("add", static_cast<void (T::*)(SmartPtr<UserData<number, dim, bool> >, const char*, const char*)>(&T::add))
			.add_method("add", static_cast<void (T::*)(SmartPtr<UserData<MathVector<dim>, dim> >, const char*, const char*)>(&T::add))
#ifdef UG_FOR_LUA
			.add_method("add", static_cast<void (T::*)(const char*, const char*, const char*)>(&T::add))
#endif
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "NeumannBoundary", tag);
	}

//	Inner Boundaries
	{
		typedef FV1InnerBoundaryElemDisc<TDomain> T;
		typedef IDomainElemDisc<TDomain> TBase;
		string name = string("FV1InnerBoundary").append(suffix);
		reg.add_class_<T, TBase >(name, elemGrp);
		reg.add_class_to_group(name, "FV1InnerBoundary", tag);
	
		typedef FV1CalciumERElemDisc<TDomain> T1;
		typedef FV1InnerBoundaryElemDisc<TDomain> TBase1;
		name = string("FV1InnerBoundaryCalciumER").append(suffix);
		reg.add_class_<T1, TBase1>(name, elemGrp)
			.template add_constructor<void (*)(const char*, const char*)>("Function(s)#Subset(s)")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "FV1InnerBoundaryCalciumER", tag);
	
	}


/////////////////////////////////////////////////////////////////////////////
// Convection Shapes
/////////////////////////////////////////////////////////////////////////////

	string upGrp = grp; upGrp.append("/SpatialDisc/Upwind");

//	IConvectionShapes
	{
		typedef IConvectionShapes<dim> T;
		string name = string("IConvectionShapes").append(suffix);
		reg.add_class_<T>(name, upGrp);
		reg.add_class_to_group(name, "IConvectionShapes", tag);
	}

//	ConvectionShapesNoUpwind
	{
		typedef ConvectionShapesNoUpwind<dim> T;
		typedef IConvectionShapes<dim> TBase;
		string name = string("NoUpwind").append(suffix);
		reg.add_class_<T, TBase>(name, upGrp)
			.add_constructor()
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "NoUpwind", tag);
	}

//	ConvectionShapesFullUpwind
	{
		typedef ConvectionShapesFullUpwind<dim> T;
		typedef IConvectionShapes<dim> TBase;
		string name = string("FullUpwind").append(suffix);
		reg.add_class_<T, TBase>(name, upGrp)
			.add_constructor()
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "FullUpwind", tag);
	}

//	ConvectionShapesWeightedUpwind
	{
		typedef ConvectionShapesWeightedUpwind<dim> T;
		typedef IConvectionShapes<dim> TBase;
		string name = string("WeightedUpwind").append(suffix);
		reg.add_class_<T, TBase>(name, upGrp)
			.add_method("set_weight", &T::set_weight)
			.add_constructor()
			.template add_constructor<void (*)(number)>("weight")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "WeightedUpwind", tag);
	}

//	ConvectionShapesPartialUpwind
	{
		typedef ConvectionShapesPartialUpwind<dim> T;
		typedef IConvectionShapes<dim> TBase;
		string name = string("PartialUpwind").append(suffix);
		reg.add_class_<T, TBase>(name, upGrp)
			.add_constructor()
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "PartialUpwind", tag);
	}
}

/**
 * Function called for the registration of Domain and Algebra independent parts.
 * All Functions and Classes not depending on Domain and Algebra
 * are to be placed here when registering.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
static void Common(Registry& reg, string grp)
{
//	get group string
	grp.append("/Discretization");

//	Elem Discs
	{
		string elemGrp = grp; elemGrp.append("/ElemDisc");
		typedef IElemDisc T;
		reg.add_class_<T>("IElemDisc", elemGrp);
	}
}
};

}// namespace ElemDiscs

void RegisterBridge_ElemDiscs(Registry& reg, string grp)
{
	typedef ElemDiscs::Functionality Functionality;

	try{
		RegisterCommon<Functionality>(reg,grp);
		RegisterDomainDependent<Functionality>(reg,grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}

}//	end of namespace bridge
}//	end of namespace ug
