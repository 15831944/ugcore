
#ifndef __H__LIB_ALGEBRA__OPERATOR__INTERFACE__LINEAR_OPERATOR__
#define __H__LIB_ALGEBRA__OPERATOR__INTERFACE__LINEAR_OPERATOR__

#include "operator.h"

namespace ug{

///////////////////////////////////////////////////////////////////////////////
// Linearized Operator
///////////////////////////////////////////////////////////////////////////////

/// describes a linear mapping X->Y
/**
 * This class is the base class for all linear mappings between two spaces.
 * The domain space and the codomain space are passed as template parameters.
 * The mapping must be linear. For nonlinear mappings see IOperator. The basic
 * usage of this class is to provide the computation of:
 *
 * 		f := L*u,    (resp.  d := J(u) * c in iterative schemes),
 *
 * where f (resp. d) is from the codomain space, u (resp. c) a function of
 * the domain space and L is a linear mapping (resp. J(u) a linearized mapping)
 *
 * This application is splitted into two steps, that have to be called in the
 * correct order:
 *
 * 1. init() or init(u):
 * 		Theses methods initialize the operator for application. One of these
 * 		methods has to be called once before one of the other two methods can be
 * 		invoked. There is no need to init the operator more than once, but
 * 		sometimes - due to parameter change - this is desirable and can be done.
 *
 * 2. apply() or apply_sub():
 * 		These methods can be called when the operator has been initialized
 * 		by a call of init. These function perform the linear mapping, where in
 * 		the case of apply_sub() the result is subtracted from the input function.
 *
 * This splitting has been made, since initialization may be computationally
 * expansive. Thus, the user of this class has the choice when to call this
 * initialization. E.g. when the operator is applied several times, the init
 * of the operator is only needed once.
 *
 * \tparam	X 	Domain space function
 * \tparam	Y	Range space function
 */
template <typename X, typename Y = X>
class ILinearOperator : public IOperator<X,Y>
{
	public:
	///	Domain space
		typedef X domain_function_type;

	///	Range space
		typedef Y codomain_function_type;

	public:
	///	init operator depending on a function u
	/**
	 * This method initializes the operator. Once initialized the 'apply'-method
	 * can be called. The function u is passed here, since the linear operator
	 * may be the linearization of some non-linear operator. Thus, the operator
	 * depends on the linearization point.
	 * If the operator is not a linearization, this method can be implemented
	 * by simply calling init() and forgetting about the linearization point.
	 *
	 * \param[in]	u		function (linearization point)
	 * \returns 	bool	success flag
	 */
		virtual void init(const X& u) = 0;

	///	init operator
	/**
	 * This method initializes the operator. Once initialized the 'apply'-method
	 * can be called.
	 * If the operator is a linearization this function returns false.
	 *
	 * \returns 	bool	success flag
	 */
		virtual void init() = 0;

	///	default implementation for IOperator interface
		virtual void prepare(X& u) {}

	// 	applies the operator
	/**
	 * This method applies the operator, i.e. f = L*u (or d = J(u)*c in
	 * iterative schemes). Note, that the operator must have been initialized
	 * once before this method can be used.
	 *
	 * \param[in]	u		domain function
	 * \param[out]	f		codomain function
	 * \returns		bool	success flag
	 */
		virtual void apply(Y& f, const X& u) = 0;

	// 	applies the operator and subtracts the result from the input
	/**
	 * This method applies the operator and subracts the result from the input
	 * codomain function, i.e. f -= L*u (or d -= J(u)*c in iterative schemes).
	 * Note, that the operator must have been initialized once before this
	 * method can be used.
	 *
	 * \param[in]		u		domain function
	 * \param[in,out]	f		codomain function
	 * \returns			bool	success flag
	 */
		virtual void apply_sub(Y& f, const X& u) = 0;

	/// virtual	destructor
		virtual ~ILinearOperator() {};
};

}
#endif /* __H__LIB_ALGEBRA__OPERATOR__INTERFACE__LINEAR_OPERATOR__ */
