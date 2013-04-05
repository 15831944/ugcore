/**
 * \file pinvit.h
 *
 * \author Martin Rupp
 *
 * \date 01.11.2010
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__UG__LIB_ALGEBRA__PINVIT_H__
#define __H__UG__LIB_ALGEBRA__PINVIT_H__

#include "common/util/string_util.h"
#include "lib_algebra/small_algebra/lapack/eigenvalue2.h"
#include <complex>
#include "common/util/sort_util.h"
#include "additional_math.h"

#define UG_ASSERT_EQUAL(a, b, txt) UG_ASSERT(a == b, txt << " [" << UG_TO_STRING(a) << " = " << a << "] == [" << UG_TO_STRING(b) << " = " << b << "] ")
#define UG_ASSERT_SMALLER_EQUAL(a, b, txt) UG_ASSERT(a <= b, txt << " [" << UG_TO_STRING(a) << " = " << a << "] <= [" << UG_TO_STRING(b) << " = " << b << "] ")
#define UG_ASSERT_SMALLER(a, b, txt) UG_ASSERT(a < b, txt << " [" << UG_TO_STRING(a) << " = " << a << "] < [" << UG_TO_STRING(b) << " = " << b << "] ")
#define UG_ASSERT_GREATER_EQUAL(a, b, txt) UG_ASSERT(a >= b, txt << " [" << UG_TO_STRING(a) << " = " << a << "] >= [" << UG_TO_STRING(b) << " = " << b << "] ")
#define UG_ASSERT_GREATER(a, b, txt) UG_ASSERT(a > b, txt << " [" << UG_TO_STRING(a) << " = " << a << "] > [" << UG_TO_STRING(b) << " = " << b << "] ")


// constructors
namespace ug{


/**
 * PINVIT Eigensolver
 *
 * This Eigensolver solves problems of the form
 * Ax = lambda B x
 * For sparse matrices A and B, and we want to find the smallest (in terms of abs(lambda) ) e.g. 1-100 solutions (eigenvalues) of the problem.
 * For this we need a preconditioner, calculating c = Pd (e.g. Jacobi, Gauss-Seidel, Geometric/Algebraic Multigrid, ILU).
 *
 * This implements the PINVIT(s) methods, with s=2 = LOBPCG
 * see Martin Rupp - Berechnung der Resonanzschwingungen einer Gitarrendecke (Diploma thesis)
 * and Andrew Knyazew, Toward the optimal Preconditioned Eigensolver: Locally Optimal Block Preconditioned
 * 	Conjugate Gradient Method. http://epubs.siam.org/doi/pdf/10.1137/S1064827500366124
 *
 * iPINVIT=1  -> Preconditioned Inverse Block Iteration [Neymeyr]
 * iPINVIT=2  -> Preconditioned Block Gradient Method
 * iPINVIT=3  -> LOBPCG (locally optimal block preconditioned gradient) [Knyazew]
 * iPINVIT>=4 -> gerneralized methods.
 *
 * example:
 * \code
 * EigenSolver eig;
 * eig:set_linear_operator_A(A)
 * eig:set_linear_operator_B(B)
 * eig:set_max_iterations(50)
 * eig:set_precision(evPrec)
 * eig:set_preconditioner(gmg)
 * eig:set_max_iterations(30)
 * eig:set_pinvit(2)
 * eig:set_debug(dbgWriter)
 * ev = {}
 * for i=1,nev do
 *   print("adding ev "..i)
 * 	 ev[i] = GridFunction(approxSpace)
 * 	 ev[i]:set_random(-1.0, 1.0)
 * 	 domainDisc:adjust_solution(ev[i])
 * 	 eig:add_vector(ev[i])
 * end
 * \endcode
 *

 */
template<typename TAlgebra>
class PINVIT
		: public DebugWritingObject<TAlgebra>
{
public:
// 	Algebra type
	typedef TAlgebra algebra_type;

// 	Vector type
	typedef typename TAlgebra::vector_type vector_type;
	typedef typename TAlgebra::matrix_type matrix_type;

///	Base type
	typedef DebugWritingObject<TAlgebra> base_type;


private:
	using base_type::set_debug;
	using base_type::debug_writer;
	using base_type::write_debug;

	//ILinearOperator<vector_type,vector_type>* m_pA;
	//ILinearOperator<vector_type,vector_type>* m_pB;
	SmartPtr<ILinearOperator<vector_type> > m_pA;

	typedef typename IPreconditioner<TAlgebra>::matrix_operator_type matrix_operator_type;
	matrix_operator_type *m_pB;

	double m_dMinimumDefectToCalcCorrection;

	std::vector<vector_type*> px;
	SmartPtr<ILinearIterator<vector_type> > m_spPrecond;

	size_t m_maxIterations;
	double m_dPrecision;
	size_t m_iPINVIT;
	std::vector<double> lambda;
	std::vector<bool> vbDirichlet;


	bool m_bPrintProjectedEigenvalues;
	bool m_bPrintProjectedEigenvectors;
	bool m_bPrintProjectedEigenproblem;
	bool m_bPrintUsedTestvectors;
	bool m_bUseAdditionalCorrections;
	bool m_bDebugCalcProjectedEigenvalues;
	size_t m_additionalEigenvectorsToKeep;
	size_t m_currentAdditionalEigenvectors;
	size_t m_currentAdditionalCorrections;

public:
	PINVIT()
	{
		m_pA = NULL;
		m_pB = NULL;
		m_iPINVIT = 3;
		m_dMinimumDefectToCalcCorrection = 1e-8;
		m_dPrecision = 1e-8;

		m_bPrintProjectedEigenvalues = false;
		m_bPrintProjectedEigenvectors = false;
		m_bPrintProjectedEigenproblem = false;
		m_bPrintUsedTestvectors = false;
		m_bUseAdditionalCorrections = false;
		m_bDebugCalcProjectedEigenvalues = false;

		m_additionalEigenvectorsToKeep = 0;
		m_currentAdditionalEigenvectors = 0;
	}

	/**
	 * adds a vector which should be used for eigenvalue computation
	 * @param vec
	 */
	void add_vector(vector_type &vec)
	{
		px.push_back(&vec);
	}

	/**
	 * set the preconditioner (or Linear Iterator)
	 * e.g. Gauss-Seidel, AMG/GMG, Jacobi, ILU, ...
	 * @param precond
	 */
	void set_preconditioner(SmartPtr<ILinearIterator<vector_type> > precond)
	{
		m_spPrecond = precond;
	}

	void set_linear_operator_A(SmartPtr<ILinearOperator<vector_type> > loA)
	{
		m_pA = loA;
		// get dirichlet nodes
		SmartPtr<MatrixOperator<matrix_type, vector_type> > pOp =
				m_pA.template cast_dynamic<MatrixOperator<matrix_type, vector_type> >();
		matrix_type &A = pOp->get_matrix();
		vbDirichlet.resize(A.num_rows());
		for(size_t i=0; i<A.num_rows(); i++)
			vbDirichlet[i] = A.is_isolated(i);
	}

	void set_linear_operator_B(matrix_operator_type &B)
	{
		m_pB = &B;
	}

	void set_max_iterations(size_t maxIterations)
	{
		m_maxIterations = maxIterations;
	}

	void set_precision(double precision)
	{
		m_dPrecision = precision;
		m_dMinimumDefectToCalcCorrection = precision;
	}

	void set_additional_eigenvectors_to_keep(size_t i)
	{
		m_additionalEigenvectorsToKeep = i;
	}

	void set_debug_calc_projected_eigenvalues(bool b)
	{
		m_bDebugCalcProjectedEigenvalues = b;
	}

	void set_use_additional_corrections(bool b)
	{
		m_bUseAdditionalCorrections = b;
	}

	/**
	 * iPINVIT=1  -> Preconditioned Inverse Block Iteration [Neymeyr]
	 * iPINVIT=2  -> Preconditioned Block Gradient Method
	 * iPINVIT=3  -> LOBPCG (locally optimal block preconditioned gradient) [Knyazew]
	 * iPINVIT>=4 -> gerneralized methods.
	 * @param iPINVIT
	 */
	void set_pinvit(size_t iPINVIT)
	{
		m_iPINVIT = iPINVIT;
		UG_ASSERT(iPINVIT >=1 && iPINVIT <= 3, "i has to be >= 1 and <= 3, but is " << iPINVIT);
	}

	size_t num_eigenvalues()
	{
		return px.size();
	}

	double get_eigenvalue(size_t i)
	{
		if(lambda.size() != px.size()) return 0.0;
		return lambda[i];
	}

	vector_type &get_eigenvector(size_t i)
	{
		return *px[i];
	}

	void set_print_projected_eigenvectors(bool b)
	{
		m_bPrintProjectedEigenvectors = b;
	}
	void set_print_projected_eigenvalues(bool b)
	{
		m_bPrintProjectedEigenvalues = b;
	}

	void set_print_projected_eigenproblem(bool b)
	{
		m_bPrintProjectedEigenproblem = b;
	}

	void set_print_used_testvectors(bool b)
	{
		m_bPrintUsedTestvectors = b;
	}

	void print_projected_eigenvectors(DenseMatrix<VariableArray2<double> > &r_ev, std::vector<std::complex<double> > &r_lambda, std::vector<string> &vTestVectorDescription)
	{
		UG_LOG("evs: \n");
		for(size_t c=0; c < r_ev.num_cols(); c++)
		{
			UG_LOG("eigenvalue [" << c << "] =  " << r_lambda[c] << ", vector:\n");
			std::vector<double> tmpEV(r_ev.num_rows());
			for(size_t r=0; r<r_ev.num_rows(); r++)
				tmpEV[r] = r_ev(r, c);
			std::vector<size_t> s = GetSortedIndices(tmpEV, absCompare);


			for(size_t i=0; i<r_ev.num_rows(); i++)
			{
				//if(r_ev.num_rows() > 3 && abs(r_ev(s[i], c))/abs(r_ev(s[r_ev.num_rows()-1], c)) < 0.01) continue;
				size_t j = s[i];
				UG_LOG(std::setw(14) << r_ev(j, c) << "   " << vTestVectorDescription[j] << "\n");
			}
			UG_LOG("\n\n");
		}


	}

	/**
	 * perform the calculation
	 * @return
	 */
	int apply()
	{
		PINVIT_PROFILE_FUNC()
		UG_LOG("Eigensolver\n");
		DenseMatrix<VariableArray2<double> > rA;
		DenseMatrix<VariableArray2<double> > rB;
		DenseMatrix<VariableArray2<double> > r_ev;
		std::vector<std::complex<double> > r_lambda;

		typedef typename vector_type::value_type value_type;
		vector_type defect;
		CloneVector(defect, *px[0]);
		size_t nEigenvalues = px.size();

		m_currentAdditionalEigenvectors = 0;

		//size_t size = px[0]->size();
		/*
		ParallelMatrix<SparseMatrix<double> > B;
		B.resize(size, size);
		for(size_t i=0; i<size; i++)
			B(i,i) = 0.00390625;
		B.set_storage_type(PST_ADDITIVE);*/

				std::vector<vector_type> vAdditional;
		vAdditional.resize(m_additionalEigenvectorsToKeep);
		size_t N = px[0]->size();

		for(size_t i=0; i<m_additionalEigenvectorsToKeep; i++)
		{
			CloneVector(vAdditional[i], *px[0]);
			vAdditional[i].resize(N);
		}

		vector_type tmp;
		CloneVector(tmp, *px[0]);
		std::vector<vector_type> vCorr;

		std::vector<vector_type> vOldX;

		lambda.resize(nEigenvalues);
		vCorr.resize(nEigenvalues);
		vOldX.resize(nEigenvalues);

		for(size_t i=0; i<nEigenvalues; i++)
		{
			UG_ASSERT_EQUAL(N, px[i]->size(), "all vectors must have same size");

			CloneVector(vCorr[i], *px[0]);
			vCorr[i].resize(N);
			CloneVector(vOldX[i], *px[0]);
			vOldX[i].resize(N);

			//PrintStorageType(*px[i]);
			//PrintStorageType(vCorr[i]);
			//PrintStorageType(vOldX[i]);
		}

		std::vector<vector_type *> pTestVectors;


		std::vector<double> vDefectNorm(nEigenvalues, m_dPrecision*10);
		std::vector<double> oldXnorm(nEigenvalues);

		std::vector<std::string> vTestVectorDescription;

		m_spPrecond->init(m_pA);
#ifdef UG_PARALLEL
		pcl::ProcessCommunicator pc;
#endif


		for(size_t iteration=0; iteration<m_maxIterations; iteration++)
		{

			// 0. normalize
			normalize_approximations();

			//  2. compute rayleigh quotient, residuum, apply preconditioner, compute corrections norm
			size_t nrofconverged=0;


			size_t currentAdditionalCorrections = 0;
			size_t numCorrections=0;
			for(size_t i=0; i<nEigenvalues; i++)
			{
				compute_rayleigh_and_defect(*px[i], lambda[i], defect, vDefectNorm[i]);
				if(vDefectNorm[i] < m_dPrecision)
				{
					nrofconverged++;
					if(m_bUseAdditionalCorrections && currentAdditionalCorrections < m_currentAdditionalEigenvectors)
					{
						double additionalLambda, additionalDefect;
						compute_rayleigh_and_defect(vAdditional[currentAdditionalCorrections],
								additionalLambda, defect, additionalDefect);
						currentAdditionalCorrections++;
						if(additionalDefect > m_dPrecision && additionalDefect < 1e5)
							calculate_correction(vCorr[numCorrections++], defect);
					}
				}
				else
					calculate_correction(vCorr[numCorrections++], defect);
			}




			// output
			print_eigenvalues_and_defect(iteration, vDefectNorm, oldXnorm, lambda);

			if(nrofconverged==nEigenvalues)
			{
				UG_LOG("all eigenvectors converged\nEigenvalues");
				return true;
			}

			// 5. add Testvectors
			//UG_LOG("5. add Testvectors\nEigenvalues");

			get_testvectors(iteration, vCorr, numCorrections, vOldX, vAdditional, pTestVectors, vTestVectorDescription, vDefectNorm);

			for(size_t i=0; i<nEigenvalues; i++)
			{ 	write_debug(iteration, i, *px[i], defect, vCorr[i], vOldX[i], vDefectNorm[i] < m_dPrecision); }

			/*for(size_t i=0; i<vTestVectorDescription.size(); i++)
			{	UG_LOG(vTestVectorDescription[i] << "\nEigenvalues");	} */

			// 5. compute reduced Matrices rA, rB

			get_projected_eigenvalue_problem(rA, rB, pTestVectors, vTestVectorDescription);

			// 6. solve reduced eigenvalue problem
			size_t iNrOfTestVectors = pTestVectors.size();
			r_ev.resize(iNrOfTestVectors, iNrOfTestVectors);
			r_lambda.resize(iNrOfTestVectors);

			// solve rA x = lambda rB, --> r_ev, r_lambda
			GeneralizedEigenvalueProblemComplex(rA, r_ev, r_lambda, rB, true);

			if(m_bPrintProjectedEigenvalues)
				print_projected_eigenvectors(r_ev, r_lambda, vTestVectorDescription);


			if(m_bDebugCalcProjectedEigenvalues)
				debug_calc_projected_eigenvalues(r_ev, pTestVectors, iteration, false);
			set_new_approximations_and_save_old(r_ev, pTestVectors, vOldX, vAdditional);

			assert_real_positive(r_lambda);
		}

		UG_LOG("not converged after" << m_maxIterations << " steps.\nEigenvalues");
		return false;
	}

	/**
	 * debug_calc_projected_eigenvalues
	 * @param r_ev
	 * @param pTestVectors
	 * @param iteration
	 * @param bWrite
	 */
	void debug_calc_projected_eigenvalues(DenseMatrix<VariableArray2<double> > &r_ev, std::vector<vector_type *> &pTestVectors, int iteration, bool bWrite)
	{
		PROFILE_FUNC_GROUP("debug");
		if(debug_writer() == NULL) return;

		vector_type t; t.resize(px[0]->size());
		vector_type defect; defect.resize(px[0]->size());
		CloneVector(t, *px[0]);

		for(size_t r=0; r<pTestVectors.size(); r++)
		{
#ifdef UG_PARALLEL
			t.set_storage_type(PST_CONSISTENT);
			defect.set_storage_type(PST_ADDITIVE);
#endif
			for(size_t i=0; i<t.size(); i++)
			{
				t[i] = 0.0;
				if(!vbDirichlet[i])
				{
					for(size_t c=0; c<pTestVectors.size(); c++)
						t[i] += r_ev(c, r) * (*pTestVectors[c])[i];
				}
			}

			t *= 1.0/B_norm(t);
#ifdef UG_PARALLEL
			t.change_storage_type(PST_CONSISTENT);
#endif
			m_pA->apply(defect, t);


			double lambda = t.dotprod(defect);

			if(m_pB)
			{
				// todo: replace with MatMultAdd
				//MatMultAddDirect(defect, 1.0, defect, -lambda[i], *m_pB, *px[i]);
#ifdef UG_PARALLEL
				defect.change_storage_type(PST_ADDITIVE);
				t.change_storage_type(PST_CONSISTENT);
#endif
				MatMultAddDirect(defect, 1.0, defect, -lambda, *m_pB, t);
			}
			else
				VecScaleAdd(defect, 1.0, defect, -lambda, t);


			UG_LOG(r << " lambda: " << std::setw(14) << lambda << " defect: " << std::setw(14) << defect.norm() << "\n");

			if(bWrite)
				write_debug(t, ("pinvit_it_" + ToString(iteration) + "_pev_" + ToString(r)).c_str());
		}
	}


	void set_new_approximations_and_save_old(DenseMatrix<VariableArray2<double> > &r_ev, std::vector<vector_type *> &pTestVectors, std::vector<vector_type> &vOldX,
			std::vector<vector_type> &vAdditional)
	{
#ifdef UG_PARALLEL
		for(size_t i=0; i<pTestVectors.size(); i++)
			pTestVectors[i]->change_storage_type(PST_UNIQUE);
		for(size_t i=0; i<px.size(); i++)
			px[i]->change_storage_type(PST_UNIQUE);
#endif
		// assume r_lambda is sorted

		size_t size = px[0]->size();
		UG_ASSERT_EQUAL(pTestVectors.size(), r_ev.num_rows(), "");

		size_t numCalc = std::min(px.size()+m_additionalEigenvectorsToKeep, r_ev.num_cols()); //px.size()
		m_currentAdditionalEigenvectors = numCalc-px.size();

		std::vector<typename vector_type::value_type> x_tmp(numCalc);
		for(size_t i=0; i<size; i++)
		{
			if(vbDirichlet[i]) continue;

			// since vx can be part of the Testvectors, temporary safe result in x_tmp.
			for(size_t r=0; r < numCalc; r++)
			{
				x_tmp[r] = 0.0;
				for(size_t c=0; c<pTestVectors.size(); c++)
					x_tmp[r] += r_ev(c, r) * (*pTestVectors[c])[i];
			}

			// now overwrite
			for(size_t r=0; r<px.size(); r++)
			{
				// save old
				/*for(int k=0; k<m_nrOfOld; k++)
					vOldX[k][r][i] = vOldX[k][r][i];
				vOldX[0][r][i] = (*px[r])[i];*/
				vOldX[r][i] = (*px[r])[i];
				// store new
				(*px[r])[i] = x_tmp[r];
			}

			// store additional
			for(size_t r=px.size(); r<numCalc; r++)
				vAdditional[r-px.size()][i] = x_tmp[r];

		}

	}

	void assert_real_positive(const std::vector<std::complex<double> > &r_lambda)
	{

		//#define UG_ASSERT2(cond, txt) {if(!cond) { UG_LOG("Warning: " << txt << "\n"); }}

		for(size_t i=0; i<r_lambda.size(); i++) // px.size() or r_lambda.size()
		{
			UG_ASSERT(r_lambda[i].imag() < 1e-8, "eigenvalue " << i << " is imaginary (" << r_lambda[i] << ")\n");
			UG_ASSERT(r_lambda[i].real() > 1e-8, "eigenvalues " << i << "<= 0\n");
		}

		for(size_t i=0; i<r_lambda.size(); i++) // px.size() or r_lambda.size()
		{
			if(r_lambda[i].imag() >= 1e-8)
				{UG_LOG("WARNING: eigenvalue " << i << " is imaginary (" << r_lambda[i] << ")\n"); }
			if(r_lambda[i].real() <= 1e-8)
				{UG_LOG("WARNING: eigenvalues " << i << "<= 0\n");}
		}

		if(m_bPrintProjectedEigenvalues && m_bPrintProjectedEigenvectors == false)
		{
			for(size_t i=0; i<r_lambda.size(); i++)
				UG_LOG(i << ".: " << r_lambda[i] << "\n");
			UG_LOG("\n");
		}
	}

private:
	void write_debug(int iteration, int i, vector_type &x, vector_type &defect, vector_type &corr, vector_type &oldX, bool bConverged)
	{
		PROFILE_FUNC_GROUP("debug")
		write_debug(x, ("pinvit_it_" + ToString(iteration) + "_ev_" + ToString(i)).c_str());
		write_debug(defect, ("pinvit_it_" + ToString(iteration) + "_defect_" + ToString(i)).c_str());
		if(!bConverged)
			write_debug(corr, ("pinvit_it_" + ToString(iteration) + "_corr_" + ToString(i)).c_str());
		write_debug(oldX, ("pinvit_it_" + ToString(iteration) + "_old_" + ToString(i)).c_str());
	}

	double B_norm(vector_type &x)
	{
		PINVIT_PROFILE_FUNC()
		if(m_pB != NULL)
			return EnergyNorm(x, *m_pB);
		else
			return x.norm();
	}

	void normalize_approximations()
	{
		PINVIT_PROFILE_FUNC()
		for(size_t i=0; i< px.size(); i++)
			(*px[i]) *= 1/ (B_norm(*px[i]));
	}

	void calculate_correction(vector_type &corr, vector_type &defect)
	{
#ifdef UG_PARALLEL
		defect.change_storage_type(PST_ADDITIVE);
		corr.set_storage_type(PST_CONSISTENT);
#endif
		corr *= 0.0;
		// d. apply preconditioner
		if(1)
		{
			m_spPrecond->apply(corr, defect);
		}
		else
		{
			m_spPrecond->apply_update_defect(corr, defect);
			UG_LOG(defect.norm() << "\n");
		}
		corr *= 1/ B_norm(corr);
#ifdef UG_PARALLEL
		corr.change_storage_type(PST_UNIQUE);
#endif
	}
	/**
	 * For a given eigenvalue approximation, computes the
	 * rayleigh quotient, the defect, the norm of the defect, and the correction calculated by the preconditioner
	 * @param[in]  x			current normalized eigenvalue approximation (<x,x> = 1)
	 * @param[out] lambda		lambda = <x, Ax> / <x, x>
	 * @param[out] defect		defect = lambda x - Ax
	 * @param[out] vDefectNorm 	vDefectNorm = | defect |_2
	 * @param[out] vCorr		P defect
	 */
	void compute_rayleigh_and_defect(vector_type &x, double &lambda, vector_type &defect, double &defectNorm)
	{
		PINVIT_PROFILE_FUNC()
// a. compute rayleigh quotients
		// lambda = <x, Ax>/<x,x>
		// todo: replace with MatMult
//				UG_LOG("m_pA has storage type "); PrintStorageType(*m_pA); UG_LOG(", and vector px[" << i << "] has storage type"); PrintStorageType(*px[i]); UG_LOG("\n");
		// px can be set to unique because of norm

#ifdef UG_PARALLEL
		x.change_storage_type(PST_CONSISTENT);
		defect.set_storage_type(PST_ADDITIVE);
#endif
		m_pA->apply(defect, x);


#ifdef UG_PARALLEL
		defect.change_storage_type(PST_UNIQUE);
		x.change_storage_type(PST_UNIQUE);
#endif
		lambda = x.dotprod(defect); // / <px[i], px[i]> = 1.0.
		//UG_LOG("lambda[" << i << "] = " << lambda << "\n");

// b. calculate residuum
		// defect = A px[i] - lambda[i] B px[i]
		if(m_pB)
		{
			// todo: replace with MatMultAdd
			//MatMultAddDirect(defect, 1.0, defect, -lambda[i], *m_pB, *px[i]);
#ifdef UG_PARALLEL
			defect.change_storage_type(PST_ADDITIVE);
			x.change_storage_type(PST_CONSISTENT);
#endif
			MatMultAddDirect(defect, 1.0, defect, -lambda, *m_pB, x);
		}
		else
			VecScaleAdd(defect, 1.0, defect, -lambda, x);

// c. check if converged


#ifdef UG_PARALLEL
		defect.change_storage_type(PST_UNIQUE);
#endif
		defectNorm = defect.norm();
	}

	/**
	 * prints the current eigenvalues and convergence status
	 * @param[in] 		iteration		iteration number
	 * @param[in] 		vDefectNorm		vector of defect norms
	 * @param[in,out] 	vOldDefectNorm	vector of defect norms from previous iteration
	 * @param[in] 		vLambda			vector of eigenvalue approximations
	 */
	void print_eigenvalues_and_defect(int iteration, const std::vector<double> &vDefectNorm,
			std::vector<double> &vOldDefectNorm, const std::vector<double> &vLambda)
	{
		PINVIT_PROFILE_FUNC()
		UG_LOG("=====================================================================================\n");
		UG_LOG("iteration " << iteration << "\n");

		for(size_t i=0; i<vLambda.size(); i++)
		{
			UG_LOG(i << " lambda: " << std::setw(14) << vLambda[i] << " defect: " <<
					std::setw(14) << vDefectNorm[i]);
			if(iteration != 0) { UG_LOG(" reduction: " << std::setw(14) << vDefectNorm[i]/vOldDefectNorm[i]); }
			if(vDefectNorm[i] < m_dPrecision) { UG_LOG(" (converged)"); }
			UG_LOG("\n");
			vOldDefectNorm[i] = vDefectNorm[i];
		}
		UG_LOG("\n");
	}

	/**
	 * depending on the PINVIT-method, this function calculates the used testvectors
	 * - for PINVIT(1), projected space is  L^k = span_i < c^k_i - x^{k}_i>,
	 *   that is (current eigenvalue - correction)
	 * - PINVIT(s) for s>=2:
	 *   L^k = span_i < x^{k-s+2}_i , .. x^{k}_i, c^k_i>
	 *   that is the space spanned by the current eigenvalue, its correction, and the s-2 previous eigenvalue approximations
	 *
	 *  if an eigenvalue is converged, we don't calculate a correction for this and previous approximations will be
	 *  not too much different, so we also won't add previous approximations
	 * @param[in]  iteration				iteration number
	 * @param[in]  vCorr					correction for eigenvector approximation
	 * @param[in]  vOldX					previous eigenvector approximations
	 * @param[out] pTestVectors				vector in which we store the used test vectors for the projected eigenvalue problem
	 * @param[out] vTestVectorDescription	description of the vectors (ev, corr or oldEv)
	 * @param[in]  vDefectNorm				norm of the defects
	 */
	void get_testvectors(int iteration, std::vector<vector_type> &vCorr, size_t numCorrections, std::vector<vector_type> &vOldX, std::vector<vector_type> &vAdditional,
			std::vector<vector_type *> &pTestVectors, std::vector<std::string> &vTestVectorDescription,
			const std::vector<double> &vDefectNorm)
	{
		PINVIT_PROFILE_FUNC()
		pTestVectors.clear();
		vTestVectorDescription.clear();
		if(m_iPINVIT == 1)
		{
			UG_ASSERT(0, "todo");
			for(size_t i=0; i < px.size(); i++)
			{
				if(vDefectNorm[i] >= m_dPrecision)
				{
					VecScaleAdd(vCorr[i], -1.0, vCorr[i], 1.0, *px[i]);
					vTestVectorDescription.push_back(std::string("ev - vCorr [") + ToString(i) + std::string("]") );
				}
			}
		}
		else
		{
			for(size_t i=0; i < px.size(); i++)
			{
				pTestVectors.push_back(px[i]);
				vTestVectorDescription.push_back(std::string("eigenvector [") + ToString(i) + std::string("]") );

				if(vDefectNorm[i] >= m_dPrecision && m_iPINVIT >= 3)
				{
					if(iteration != 0)
					{
						pTestVectors.push_back(&vOldX[i]);
						vTestVectorDescription.push_back(std::string("old eigenvalue[") + ToString(i) + std::string("]") );
					}

					/*if(iteration == 0)
					{
						for(size_t j=0; j<px[i]->size(); j++)
							vOldX[i][j] = (*px[i])[j] * urand(-1.0, 1.0);
					}*/
				}
			}

			for(size_t i=0; i<numCorrections; i++)
			{
				pTestVectors.push_back(&vCorr[i]);
				if(vDefectNorm[i] >= m_dPrecision)
					vTestVectorDescription.push_back(std::string("correction [") + ToString(i) + std::string("]") );
				else
					vTestVectorDescription.push_back(std::string("additional correction [") + ToString(i) + std::string("]") );
			}

		}
		for(size_t i=0; i<m_currentAdditionalEigenvectors; i++)
		{
			pTestVectors.push_back(&vAdditional[i]);
			vTestVectorDescription.push_back(std::string("additional [") + ToString(i) + std::string("]") );
		}
	}

	/**
	 * save current eigenvector approximation into vector old
	 * @param[out] old vector to save approximations to
	 */
	void save_old_approximations( std::vector<vector_type> &old)
	{
		PINVIT_PROFILE_FUNC()
		for(size_t i=0; i<px.size(); i++)
			old[i] = *px[i];
	}

	/**
	 * Calculates a maximal set of rows which are linear independent
	 * @param[in]  mat					the input matrix
	 * @param[out] bLinearIndependent	output vector (true if linear independent)
	 */
	void get_linear_independent_rows(DenseMatrix<VariableArray2<double> > mat, std::vector<bool> &bLinearIndependent)
	{
		PINVIT_PROFILE_FUNC()
		// Remove linear depended vectors
		bLinearIndependent.resize(mat.num_rows(), true);
		for(size_t i=0; i<mat.num_rows(); i++)
		{
			for(size_t j=0; j<i; j++)
			{
				if(!bLinearIndependent[i]) continue;
				double val = mat(i, j)/mat(j,j);
				mat(i,j) = 0;
				for(size_t k=j+1; k<mat.num_rows(); k++)
					mat(i,k) -= val*mat(j, k);
			}
			if(mat(i,i) < 1e-12) bLinearIndependent[i] = false;
			else bLinearIndependent[i] = true;
		}
	}

	/**
	 * remove all entries with vbUse[i]==false from vector i
	 * @param[in, out] 	v 		vector to contain result
	 * @param[in] 		vbUse	if vbUse[i] is true, add it to new vector
	 */
	template<typename T>
	void remove_unused(std::vector<T> &v, const std::vector<bool> vbUse)
	{
		PINVIT_PROFILE_FUNC()
		std::vector<T> tmp = v;
		v.clear();
		for(size_t i=0; i<tmp.size(); i++)
			if(vbUse[i])
				v.push_back(tmp[i]);

	}

	void print_used_testvectors(std::vector<std::string> &vTestVectorDescription, std::vector<bool> bUse)
	{
		UG_LOG("used testvectors:\n");
		for(size_t i=0; i<vTestVectorDescription.size(); i++)
			if(bUse[i]) { UG_LOG(vTestVectorDescription[i] << "\n"); }
		UG_LOG("unused testvectors:\n");
		for(size_t i=0; i<vTestVectorDescription.size(); i++)
			if(!bUse[i]) { UG_LOG(vTestVectorDescription[i] << "\n"); }
	}

	/**
	 * Calculate projected eigenvalue problem on space spanned by testvectors in pTestVectors
	 * 1. calculate W as a subset of the testvectors so that those are linear B-independent
	 * 2. rA = W^T A W
	 * 3. rB = W^T B W
	 * @param[out] rA							reduced eigenvalue problem matrix A
	 * @param[out] rB							reduced eigenvalue problem matrix B
	 * @param[in, out] pTestVectors				the testvectors, out: the used testvectors
	 * @param[in, out] vTestVectorDescription 	their description
	 */
	void get_projected_eigenvalue_problem(DenseMatrix<VariableArray2<double> > &rA,
			DenseMatrix<VariableArray2<double> > &rB, std::vector<vector_type *> &pTestVectors,
			std::vector<std::string> &vTestVectorDescription)
	{
		PINVIT_PROFILE_FUNC()
		// 1. calculate W as a subset of the testvectors so that those are linear B-independent

		size_t iNrOfTestVectors = pTestVectors.size();
		rA.resize(iNrOfTestVectors, iNrOfTestVectors);
		rB.resize(iNrOfTestVectors, iNrOfTestVectors);

		if(m_pB)
			MultiEnergyProd(*m_pB, &pTestVectors[0], rB, iNrOfTestVectors);
		else
			MultiScalProd(&pTestVectors[0], rB, iNrOfTestVectors);


		// Remove linear depended vectors
		std::vector<bool> bUse;
		get_linear_independent_rows(rB, bUse);


		if(m_bPrintUsedTestvectors)
			print_used_testvectors(vTestVectorDescription, bUse);

		// save used testvectors
		remove_unused(pTestVectors, bUse);
		remove_unused(vTestVectorDescription, bUse);

		iNrOfTestVectors = pTestVectors.size();

		// 2. & 3. compute reduced Matrices rA, rB
		rA.resize(iNrOfTestVectors, iNrOfTestVectors);
		rB.resize(iNrOfTestVectors, iNrOfTestVectors);

		if(m_pB)
			MultiEnergyProd(*m_pB, &pTestVectors[0], rB, iNrOfTestVectors);
		else
			MultiScalProd(&pTestVectors[0], rB, iNrOfTestVectors);

		MultiEnergyProd(*m_pA, &pTestVectors[0], rA, iNrOfTestVectors);

		if(m_bPrintProjectedEigenproblem)
		{
			PrintMaple(rA, "rA");
			PrintMaple(rB, "rB");
		}


	}


};

} // namespace ug


#endif // __H__UG__LIB_ALGEBRA__PINVIT_H__
