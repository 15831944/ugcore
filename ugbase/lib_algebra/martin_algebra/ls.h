/*
 *  ls.h
 *  flexamg
 *
 *  Created by Martin Rupp on 11.12.09.
 *  Copyright 2009 . All rights reserved.
 *
 */

//!
//! Linear Solver
//! Performs maxit steps of iteration x -> x + P (b-Ax), where P is a preconditioner
// this one uses preconditioner::iterate
template<typename mat_type>
void LinearSolver(typename matrix<mat_type>::Vector_type &x, const matrix<mat_type> &A, 
				  const typename matrix<mat_type>::Vector_type &b, preconditioner<mat_type> &P, int maxit)
{
	typedef typename matrix<mat_type>::Vector_type Vector_type;
	
	P.init(A);
	stopwatch SW;
	SW.start();
	
	
	double res=1, oldres = 0;
	int i;
	
	//	Vector_type
	for(i=0; i< maxit && (res > 0.00001); i++)
	{
		P.iterate(x, b);
		res = norm(b-A*x);		
		cout << "res: " << res << " conv.: " << res/oldres << endl;
		cout.flush();
		oldres = res;
	}
	cout << i << " Iterations." << endl;
	cout << "res: " << norm(b-A* x) << endl;
	
	SW.printTimeDiff();
}


//!
//! Linear Solver
//! Performs maxit steps of iteration x -> x + P (b-Ax), where P is a preconditioner
// this one uses preconditioner::precond
template<typename mat_type>
void LinearSolver2(typename matrix<mat_type>::Vector_type &x, const matrix<mat_type> &A, 
				   const typename matrix<mat_type>::Vector_type &b, preconditioner<mat_type> &P, int maxit)
{
	typedef typename matrix<mat_type>::Vector_type Vector_type;
	
	P.init(A);
	stopwatch SW;
	SW.start();
	
	Vector_type r("r"), c("c");
	r.create(A.getLength());
	c.create(A.getLength());
	r = b-A*x;
	double res=1, oldres = norm(r);	
	int i;
	//	Vector_type
	for(i=0; i< maxit && (res > 0.00001); i++)
	{
		P.precond(c, r);
		x += c;
		r = b-A*x;
		res = norm(r);
		cout << "res: " << res << " conv.: " << res/oldres << endl;
		cout.flush();
		oldres = res;
		
	}
	cout << i << " Iterations." << endl;
	cout << "res: " << norm(b-A*x) << endl;
	
	SW.printTimeDiff();
}

//!
//! CG-Solver
//! CG Solver on system Ax = b with Preconditioner P.
template<typename mat_type>
void CG(typename matrix<mat_type>::Vector_type &x, const matrix<mat_type> &A, const typename matrix<mat_type>::Vector_type &b, preconditioner<mat_type> &P, int maxit)
{
	typedef typename matrix<mat_type>::Vector_type Vector_type;
	
	P.init(A);
	
	
	Vector_type r(x.getLength(), "CG:r");
	r = b - A*x;
	
	Vector_type d(x.getLength(), "CG:d");
	
	d = P.precond(r);
	
	Vector_type z(x.getLength(), "CG:z");
	z = d;
	
	Vector_type t(x.getLength(), "CG:t");
	int i=0;
	double alpha, rz_new, rz_old;
	rz_old = r*z;
	double res, oldres;
	
	res = norm(r);
	cout << "[0] res: " << res << endl;
	
	for(i=1; i <= maxit && (res > 0.00001); i++)
	{
		
		t = A*d;
		alpha = rz_old/(d*t);
		x += alpha * d;
		r -= alpha * t;
		
		z = P.precond(r);
		
		rz_new = r*z;
		
		d = z + (rz_new/rz_old) *d;
		
		rz_old = rz_new;
		
		oldres = res;
		res = norm(r);
		//if(i %10 == 0)
		cout << "[" << i << "] res: " << res << " conv.: " << res/oldres << endl;		
		cout.flush();
	}
	//if(--i %10 != 0) 
	cout << "[" << i << "] res: " << res << " conv.: " << res/oldres << endl;
	cout << i << " Iterations." << endl;
}
