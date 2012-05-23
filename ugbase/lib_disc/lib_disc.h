/*
 * lib_disc.h
 *
 *  Created on: 09.03.2011
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__LIB__DISC__
#define __H__UG__LIB_DISC__LIB__DISC__

//////////////////////////////////////////////////////////////////////////////
//
// This file is intended to include all parts of lib_discretization
//
//////////////////////////////////////////////////////////////////////////////

// Domain
#include "domain.h"
#include "domain_util.h"

// Assemble interface
#include "assemble_interface.h"

// Common
#include "common/function_group.h"
#include "common/geometry_util.h"
#include "common/groups_util.h"
#include "common/local_algebra.h"
#include "common/multi_index.h"
#include "common/subset_group.h"

// DoF Manager
#include "dof_manager/function_pattern.h"

// Function Spaces
#include "function_spaces/approximation_space.h"
#include "function_spaces/grid_function_util.h"
#include "function_spaces/grid_function.h"
#include "function_spaces/interpolate.h"

// IO
#include "io/vtkoutput.h"

// local shape function set
#include "local_finite_element/local_shape_function_set.h"
#include "local_finite_element/local_shape_function_set.h"
#include "local_finite_element/local_finite_element_id.h"
#include "local_finite_element/lagrange/lagrange.h"
#include "local_finite_element/lagrange/lagrangep1.h"
#include "local_finite_element/common/lagrange1d.h"
#include "local_finite_element/common/polynomial1d.h"

// Operator
#include "operator/linear_operator/assembled_linear_operator.h"
#include "operator/linear_operator/projection_operator.h"
#include "operator/linear_operator/prolongation_operator.h"
#include "operator/linear_operator/multi_grid_solver/mg_solver.h"

#include "operator/non_linear_operator/assembled_non_linear_operator.h"
#include "operator/non_linear_operator/line_search.h"
#include "operator/non_linear_operator/newton_solver/newton.h"

// Parallelization
#ifdef UG_PARALLEL
#include "parallelization/parallelization_util.h"
#endif

// Quadrature
#include "quadrature/quadrature.h"

// Reference Elements
#include "reference_element/reference_element.h"

// Spatial Discretization
#include "spatial_disc/domain_disc_interface.h"
#include "spatial_disc/domain_disc.h"
#include "spatial_disc/subset_assemble_util.h"

#include "spatial_disc/disc_util/finite_element_geometry.h"
#include "spatial_disc/disc_util/finite_volume_geometry.h"
#include "spatial_disc/disc_util/finite_volume_output.h"
#include "spatial_disc/disc_util/finite_volume_util.h"
#include "spatial_disc/disc_util/hanging_finite_volume_geometry.h"

#include "spatial_disc/elem_disc/elem_disc_interface.h"
#include "spatial_disc/elem_disc/elem_disc_assemble_util.h"
#include "spatial_disc/elem_disc/convection_diffusion/convection_diffusion.h"
#include "spatial_disc/elem_disc/density_driven_flow/density_driven_flow.h"
#include "spatial_disc/elem_disc/inner_boundary/inner_boundary.h"
#include "spatial_disc/elem_disc/neumann_boundary/neumann_boundary.h"

#include "spatial_disc/ip_data/const_user_data.h"
#include "spatial_disc/ip_data/ip_data.h"
#include "spatial_disc/ip_data/data_import_export.h"
#include "spatial_disc/ip_data/data_linker.h"

#include "spatial_disc/constraints/constraint_interface.h"
#include "spatial_disc/constraints/continuity_constraints/p1_continuity_constraints.h"
#include "spatial_disc/constraints/dirichlet_boundary/lagrange_dirichlet_boundary.h"

// Time discretization
#include "time_disc/time_disc_interface.h"
#include "time_disc/theta_time_step.h"

#endif /* __H__UG__LIB_DISC__LIB__DISC__ */
