/*
 * assembled_non_linear_operator_impl.h
 *
 *  Created on: ..
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__ASSEMBLED_NON_LINEAR_OPERATOR_IMPL__
#define __H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__ASSEMBLED_NON_LINEAR_OPERATOR_IMPL__

#include "assembled_non_linear_operator.h"

namespace ug{

//	Prepare functions
template <typename TAlgebra>
void
AssembledOperator<TAlgebra>::prepare(vector_type& dOut, vector_type& uIn)
{
	if(m_pAss == NULL)
		UG_THROW_FATAL("Discretization not set.");

// 	Set Dirichlet - Nodes to exact values
	try{
		m_pAss->adjust_solution(uIn, m_gridLevel);
	}
	UG_CATCH_THROW("Cannot set dirichlet values in solution.");
}

// 	Compute d = L(u)
template <typename TAlgebra>
void
AssembledOperator<TAlgebra>::apply(vector_type& dOut, const vector_type& uIn)
{
	if(m_pAss == NULL)
		UG_THROW_FATAL("Discretization not set.");

//  assemble defect
	try{
		m_pAss->assemble_defect(dOut, uIn, m_gridLevel);
	}
	UG_CATCH_THROW("Could not assemble defect. Aborting.");
}

} // end namepace ug

#endif /*__H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__ASSEMBLED_NON_LINEAR_OPERATOR_IMPL__*/
