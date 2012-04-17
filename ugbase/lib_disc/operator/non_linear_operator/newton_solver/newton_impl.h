/*
 * newton_impl.h
 *
 *  Created on: 18.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__NEWTON_SOLVER__NEWTON_IMPL__
#define __H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__NEWTON_SOLVER__NEWTON_IMPL__

#include <iostream>
#include <sstream>

#include "newton.h"
#include "lib_disc/function_spaces/grid_function_util.h"

#define PROFILE_NEWTON
#ifdef PROFILE_NEWTON
	#define NEWTON_PROFILE_FUNC()		PROFILE_FUNC()
	#define NEWTON_PROFILE_BEGIN(name)	PROFILE_BEGIN(name)
	#define NEWTON_PROFILE_END()		PROFILE_END()
#else
	#define NEWTON_PROFILE_FUNC()
	#define NEWTON_PROFILE_BEGIN(name)
	#define NEWTON_PROFILE_END()
#endif

namespace ug{

template <typename TAlgebra>
bool
NewtonSolver<TAlgebra>::
init(SmartPtr<IOperator<vector_type> > N)
{
	m_N = N.template cast_dynamic<AssembledOperator<TAlgebra> >();
	if(m_N.invalid())
		UG_THROW_FATAL("NewtonSolver: currently only works for AssembledDiscreteOperator.");

	m_pAss = m_N->get_assemble();
	return true;
}


template <typename TAlgebra>
void NewtonSolver<TAlgebra>::allocate_memory(const vector_type& u)
{
	// Jacobian
	m_J = CreateSmartPtr(new AssembledLinearOperator<TAlgebra>(*m_pAss));
	m_J->set_level(m_N->level());

	if(m_J.invalid()) UG_THROW_FATAL("Cannot allocate memory.");

	// defect
	m_d.resize(u.size()); m_d = u;

	// correction
	m_c.resize(u.size()); m_c = u;

	m_allocated = true;
}

template <typename TAlgebra>
bool NewtonSolver<TAlgebra>::prepare(vector_type& u)
{
	if(!m_allocated)
	{
		try{
			allocate_memory(u);
		}
		UG_CATCH_THROW("NewtonSolver: Cannot allocate memory.");
	}

//	Check for linear solver
	if(m_spLinearSolver.invalid())
		UG_THROW_FATAL("NewtonSolver::prepare: Linear Solver not set.");

//	Set dirichlet values
	m_N->prepare(m_d, u);

	return true;
}


template <typename TAlgebra>
NewtonSolver<TAlgebra>::~NewtonSolver()
{}


template <typename TAlgebra>
bool NewtonSolver<TAlgebra>::apply(vector_type& u)
{
//	increase call count
	m_dgbCall++;

//	resize
	m_d.resize(u.size());
	m_c.resize(u.size());

// 	Compute first Defect
	NEWTON_PROFILE_BEGIN(NewtonComputeDefect1);
	m_N->apply(m_d, u);
	NEWTON_PROFILE_END();

//	write start defect for debug
	int loopCnt = 0;
	char ext[20]; sprintf(ext, "_iter%03d", loopCnt);
	std::string name("NEWTON_Defect");
	name.append(ext);
	write_debug(m_d, name.c_str());
	write_debug(u, "NEWTON_StartSolution");

// 	increase offset of output for linear solver
	const int stdLinOffset = m_spLinearSolver->standard_offset();
	m_spLinearSolver->convergence_check()->set_offset(stdLinOffset + 3);

// 	set info string indicating the used linear solver
	std::stringstream ss; ss << "(Linear Solver: " << m_spLinearSolver->name() << ")";
	m_spConvCheck->set_info(ss.str());

// 	copy pattern
	vector_type s; s.resize(u.size()); s = u;

// 	start convergence check
	m_spConvCheck->start(m_d);

//	loop iteration
	while(!m_spConvCheck->iteration_ended())
	{
	// 	set c = 0
		NEWTON_PROFILE_BEGIN(NewtonSetCorretionZero);
		if(!m_c.set(0.0))
		{
			UG_LOG("ERROR in 'NewtonSolver::apply':"
					" Cannot reset correction to zero.\n");
			return false;
		}
		NEWTON_PROFILE_END();

	// 	Compute Jacobian
		NEWTON_PROFILE_BEGIN(NewtonComputeJacobian);
		m_J->init(u);
		NEWTON_PROFILE_END();

	//	Write Jacobian for debug
		std::string matname("NEWTON_Jacobian");
		matname.append(ext);
		write_debug(m_J->get_matrix(), matname.c_str());

	// 	Init Jacobi Inverse
		NEWTON_PROFILE_BEGIN(NewtonPrepareLinSolver);
		if(!m_spLinearSolver->init(m_J, u))
		{
			UG_LOG("ERROR in 'NewtonSolver::apply': Cannot init Inverse Linear "
					"Operator for Jacobi-Operator.\n");
			return false;
		}
		NEWTON_PROFILE_END();

	// 	Solve Linearized System
		NEWTON_PROFILE_BEGIN(NewtonApplyLinSolver);
		if(!m_spLinearSolver->apply(m_c, m_d))
		{
			UG_LOG("ERROR in 'NewtonSolver::apply': Cannot apply Inverse Linear "
					"Operator for Jacobi-Operator.\n");
			return false;
		}
		NEWTON_PROFILE_END();

	//	store convergence history
		const int numSteps = m_spLinearSolver->step();
		if(loopCnt >= (int)m_vTotalLinSolverSteps.size()) m_vTotalLinSolverSteps.resize(loopCnt+1);
		if(loopCnt >= (int)m_vLinSolverCalls.size()) m_vLinSolverCalls.resize(loopCnt+1, 0);
		m_vTotalLinSolverSteps[loopCnt] += numSteps;
		m_vLinSolverCalls[loopCnt] += 1;

	// 	Line Search
		if(m_spLineSearch.valid())
		{
			m_spLineSearch->set_offset("   #  ");
			NEWTON_PROFILE_BEGIN(NewtonLineSearch);
			if(!m_spLineSearch->search(m_N, u, m_c, m_d, m_spConvCheck->defect()))
			{
				UG_LOG("ERROR in 'NewtonSolver::apply': "
						"Newton Solver did not converge.\n");
				return false;
			}
			NEWTON_PROFILE_END();
		}
	// 	No line search: Compute new defect
		else
		{
		// 	update solution
			u -= m_c;

		// 	compute new Defect
			NEWTON_PROFILE_BEGIN(NewtonComputeDefect);
			m_N->prepare(m_d, u);
			m_N->apply(m_d, u);
			NEWTON_PROFILE_END();
		}

	//	update counter
		loopCnt++;
		sprintf(ext, "_iter%03d", loopCnt);

	// 	check convergence
		m_spConvCheck->update(m_d);


	//	write defect for debug
		std::string name("NEWTON_Defect"); name.append(ext);
		write_debug(m_d, name.c_str());
		std::string name2("NEWTON_Correction"); name2.append(ext);
		write_debug(m_c, name2.c_str());
	}

	// reset offset of output for linear solver to previous value
	m_spLinearSolver->convergence_check()->set_offset(stdLinOffset);

	return m_spConvCheck->post();
}

template <typename TAlgebra>
void
NewtonSolver<TAlgebra>::
print_average_convergence() const
{
	UG_LOG("\nNewton solver convergence history:\n");
	UG_LOG("Newton Step | Num Calls | Total Lin Iters | Avg Lin Iters \n");
	int allCalls = 0, allSteps = 0;
	for(int call = 0; call < (int)m_vLinSolverCalls.size(); ++call)
	{
		UG_LOG( " " << std::setw(10) << call+1 << " | ");
		UG_LOG(std::setw(9) << m_vLinSolverCalls[call] << " | ");
		allCalls += m_vLinSolverCalls[call];
		UG_LOG(std::setw(15) << m_vTotalLinSolverSteps[call] << " | ");
		allSteps += m_vTotalLinSolverSteps[call];
		UG_LOG(std::setw(13) << std::setprecision(2) << std::fixed << m_vTotalLinSolverSteps[call] / (double)m_vLinSolverCalls[call]);
		UG_LOG("\n");
	}
	UG_LOG( "        all | ");
	UG_LOG(std::setw(9) << allCalls << " | ");
	UG_LOG(std::setw(15) << allSteps << " | ");
	UG_LOG(std::setw(13) << std::setprecision(2) << std::fixed << allSteps / (double)allCalls);
	UG_LOG("\n");
}

template <typename TAlgebra>
size_t
NewtonSolver<TAlgebra>::
num_newton_steps() const
{
	return m_vLinSolverCalls.size();
}

template <typename TAlgebra>
int
NewtonSolver<TAlgebra>::
num_linsolver_calls(size_t call) const
{
	return m_vLinSolverCalls[call];
}

template <typename TAlgebra>
int
NewtonSolver<TAlgebra>::
num_linsolver_steps(size_t call) const
{
	return m_vTotalLinSolverSteps[call];
}

template <typename TAlgebra>
double
NewtonSolver<TAlgebra>::
average_linear_steps(size_t call) const
{
	return m_vTotalLinSolverSteps[call] / ((double)m_vLinSolverCalls[call]);
}

template <typename TAlgebra>
int
NewtonSolver<TAlgebra>::
total_linsolver_calls() const
{
	int allCalls = 0;
	for(size_t call = 0; call < m_vLinSolverCalls.size(); ++call)
		allCalls += m_vLinSolverCalls[call];
	return allCalls;
}

template <typename TAlgebra>
int
NewtonSolver<TAlgebra>::
total_linsolver_steps() const
{
	int allSteps = 0;
	for(size_t call = 0; call < m_vLinSolverCalls.size(); ++call)
		allSteps += m_vTotalLinSolverSteps[call];
	return allSteps;
}

template <typename TAlgebra>
double
NewtonSolver<TAlgebra>::
total_average_linear_steps() const
{
	return total_linsolver_steps()/((double)total_linsolver_calls());
}


template <typename TAlgebra>
void
NewtonSolver<TAlgebra>::
clear_average_convergence()
{
	m_vLinSolverCalls.clear();
	m_vTotalLinSolverSteps.clear();
}


}

#endif /* __H__UG__LIB_DISC__OPERATOR__NON_LINEAR_OPERATOR__NEWTON_SOLVER__NEWTON_IMPL__ */

