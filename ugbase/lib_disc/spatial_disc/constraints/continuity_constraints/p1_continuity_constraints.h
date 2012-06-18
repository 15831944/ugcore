/*
 * p1_continuity_constraints.h
 *
 *  Created on: 01.03.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__CONSTRAINTS__CONTINUITY_CONSTRAINTS__P1_CONTINUITY_CONSTRAINTS__
#define __H__UG__LIB_DISC__SPATIAL_DISC__CONSTRAINTS__CONTINUITY_CONSTRAINTS__P1_CONTINUITY_CONSTRAINTS__

#include "lib_disc/assemble_interface.h"
#include "lib_grid/algorithms/geom_obj_util/vertex_util.h"
#include "lib_disc/spatial_disc/constraints/constraint_base.h"

namespace ug {

/// returns the vertices of the object constraining a hanging vertex
void CollectConstraining(std::vector<VertexBase*>& vConstrainingVrt,
                         ConstrainedVertex* hgVrt,
                         bool bClearContainer = true);


template <typename TDomain, typename TAlgebra>
class SymP1Constraints
	: public ConstraintBase<TDomain, TAlgebra,
	  	  	  	  	  	  	  SymP1Constraints<TDomain, TAlgebra> >
{
	public:
	// 	Algebra type
		typedef TAlgebra algebra_type;

	// 	Type of algebra matrix
		typedef typename algebra_type::matrix_type matrix_type;

	// 	Type of algebra vector
		typedef typename algebra_type::vector_type vector_type;

	public:
		virtual int type() const {return CT_CONSTRAINTS;}

		template <typename TDD>
		void adjust_jacobian(matrix_type& J, const vector_type& u,
		                     ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_defect(vector_type& d, const vector_type& u,
		                   ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_rhs(vector_type& rhs, const vector_type& u,
		                ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_linear(matrix_type& mat, vector_type& rhs,
		                   ConstSmartPtr<TDD> dd, number time);

		template <typename TDD>
		void adjust_solution(vector_type& u, ConstSmartPtr<TDD> dd,
		                     number time);
};



template <typename TDomain, typename TAlgebra>
class OneSideP1Constraints
	: public ConstraintBase<TDomain, TAlgebra,
	  	  	  	  	  	  OneSideP1Constraints<TDomain, TAlgebra> >
{
	public:
	// 	Algebra type
		typedef TAlgebra algebra_type;

	// 	Type of algebra matrix
		typedef typename algebra_type::matrix_type matrix_type;

	// 	Type of algebra vector
		typedef typename algebra_type::vector_type vector_type;

	protected:
		typedef ConstraintBase<TDomain, TAlgebra,
  	  	  	  	  OneSideP1Constraints<TDomain, TAlgebra> > base_type;

		using base_type::approximation_space;

	public:
		virtual int type() const {return CT_CONSTRAINTS;}

		template <typename TDD>
		void adjust_jacobian(matrix_type& J, const vector_type& u,
		                     ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_defect(vector_type& d, const vector_type& u,
		                   ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_rhs(vector_type& rhs, const vector_type& u,
		                ConstSmartPtr<TDD> dd, number time = 0.0);

		template <typename TDD>
		void adjust_linear(matrix_type& mat, vector_type& rhs,
		                   ConstSmartPtr<TDD> dd, number time);

		template <typename TDD>
		void adjust_solution(vector_type& u, ConstSmartPtr<TDD> dd,
		                     number time);
};

}; // namespace ug

#include "p1_continuity_constraints_impl.h"

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__CONSTRAINTS__CONTINUITY_CONSTRAINTS__P1_CONTINUITY_CONSTRAINTS__ */
