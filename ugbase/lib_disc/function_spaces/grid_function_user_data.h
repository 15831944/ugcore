/*
 * grid_function_user_data.h
 *
 *  Created on: 03.07.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_USER_DATA__
#define __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_USER_DATA__

#include "common/common.h"

#include "lib_disc/common/subset_group.h"
#include "lib_disc/common/function_group.h"
#include "lib_disc/common/groups_util.h"
#include "lib_disc/quadrature/quadrature.h"
#include "lib_disc/local_finite_element/local_shape_function_set.h"
#include "lib_disc/spatial_disc/ip_data/ip_data.h"


namespace ug{

template <typename TData, int dim, typename TImpl>
class StdGridFunctionData
	: 	public IPData<TData,dim>
{
	public:
		////////////////
		// one value
		////////////////
		virtual void operator() (TData& value,
		                         const MathVector<dim>& globIP,
		                         number time, int si) const
		{
			UG_THROW("StdGridFunctionData: Need element.");
		}

		virtual void operator() (TData& value,
		                         const MathVector<dim>& globIP,
		                         number time, int si,
		                         LocalVector& u,
		                         GeometricObject* elem,
		                         const MathVector<dim> vCornerCoords[],
		                         const MathVector<1>& locIP) const
		{
			getImpl().template evaluate<1>(&value,&globIP,time,si,u,elem,vCornerCoords,&locIP, 1, NULL);
		}

		virtual void operator() (TData& value,
		                         const MathVector<dim>& globIP,
		                         number time, int si,
		                         LocalVector& u,
		                         GeometricObject* elem,
		                         const MathVector<dim> vCornerCoords[],
		                         const MathVector<2>& locIP) const
		{
			getImpl().template evaluate<2>(&value,&globIP,time,si,u,elem,vCornerCoords,&locIP, 1, NULL);
		}

		virtual void operator() (TData& value,
		                         const MathVector<dim>& globIP,
		                         number time, int si,
		                         LocalVector& u,
		                         GeometricObject* elem,
		                         const MathVector<dim> vCornerCoords[],
		                         const MathVector<3>& locIP) const
		{
			getImpl().template evaluate<3>(&value,&globIP,time,si,u,elem,vCornerCoords,&locIP, 1, NULL);
		}

		////////////////
		// vector of values
		////////////////

		virtual void operator() (TData vValue[],
		                         const MathVector<dim> vGlobIP[],
		                         number time, int si, const size_t nip) const
		{
			UG_THROW("StdGridFunctionData: Need element.");
		}


		virtual void operator()(TData vValue[],
		                        const MathVector<dim> vGlobIP[],
		                        number time, int si,
		                        LocalVector& u,
		                        GeometricObject* elem,
		                        const MathVector<dim> vCornerCoords[],
		                        const MathVector<1> vLocIP[],
		                        const size_t nip,
		                        const MathMatrix<1, dim>* vJT = NULL) const
		{
			getImpl().template evaluate<1>(vValue,vGlobIP,time,si,u,elem,
			                               vCornerCoords,vLocIP,nip, vJT);
		}

		virtual void operator()(TData vValue[],
		                        const MathVector<dim> vGlobIP[],
		                        number time, int si,
		                        LocalVector& u,
		                        GeometricObject* elem,
		                        const MathVector<dim> vCornerCoords[],
		                        const MathVector<2> vLocIP[],
		                        const size_t nip,
		                        const MathMatrix<2, dim>* vJT = NULL) const
		{
			getImpl().template evaluate<2>(vValue,vGlobIP,time,si,u,elem,
			                               vCornerCoords,vLocIP,nip, vJT);
		}

		virtual void operator()(TData vValue[],
		                        const MathVector<dim> vGlobIP[],
		                        number time, int si,
		                        LocalVector& u,
		                        GeometricObject* elem,
		                        const MathVector<dim> vCornerCoords[],
		                        const MathVector<3> vLocIP[],
		                        const size_t nip,
		                        const MathMatrix<3, dim>* vJT = NULL) const
		{
			getImpl().template evaluate<3>(vValue,vGlobIP,time,si,u,elem,
			                               vCornerCoords,vLocIP,nip, vJT);
		}

		virtual void compute(LocalVector* u, GeometricObject* elem, bool bDeriv = false)
		{
			UG_THROW("Not implemented.");
		}

	protected:
	///	access to implementation
		TImpl& getImpl() {return static_cast<TImpl&>(*this);}

	///	const access to implementation
		const TImpl& getImpl() const {return static_cast<const TImpl&>(*this);}
};


template <typename TGridFunction>
class GridFunctionNumberData
	: public StdGridFunctionData<number, TGridFunction::dim,
	  	  	  	  	  	  	  	  GridFunctionNumberData<TGridFunction> >
{
	public:
	//	world dimension of grid function
		static const int dim = TGridFunction::dim;

	private:
	// grid function
		SmartPtr<TGridFunction> m_spGridFct;

	//	component of function
		size_t m_fct;

	//	local finite element id
		LFEID m_lfeID;

	public:
	/// constructor
		GridFunctionNumberData(SmartPtr<TGridFunction> spGridFct, const char* cmp)
			: m_spGridFct(spGridFct)
		{
		//	get function id of name
			m_fct = spGridFct->fct_id_by_name(cmp);

		//	check that function exists
			if(m_fct >= spGridFct->num_fct())
				UG_THROW("GridFunctionNumberData: Function space does not contain"
						" a function with name " << cmp << ".");

		//	local finite element id
			m_lfeID = spGridFct->local_finite_element_id(m_fct);
		};

		template <int refDim>
		inline void evaluate(number vValue[],
		                     const MathVector<dim> vGlobIP[],
		                     number time, int si,
		                     LocalVector& u,
		                     GeometricObject* elem,
		                     const MathVector<dim> vCornerCoords[],
		                     const MathVector<refDim> vLocIP[],
		                     const size_t nip,
		                     const MathMatrix<refDim, dim>* vJT = NULL) const
		{
		//	reference object id
			const ReferenceObjectID roid = elem->reference_object_id();

		//	get trial space
			try{
			const LocalShapeFunctionSet<refDim>& rTrialSpace =
					LocalShapeFunctionSetProvider::get<refDim>(roid, m_lfeID);

		//	memory for shapes
			std::vector<number> vShape;

		//	loop ips
			for(size_t ip = 0; ip < nip; ++ip)
			{
			//	evaluate at shapes at ip
				rTrialSpace.shapes(vShape, vLocIP[ip]);

			//	get multiindices of element
				std::vector<MultiIndex<2> > ind;
				m_spGridFct->multi_indices(elem, m_fct, ind);

			// 	compute solution at integration point
				vValue[ip] = 0.0;
				for(size_t sh = 0; sh < vShape.size(); ++sh)
				{
					const number valSH = BlockRef((*m_spGridFct)[ind[sh][0]], ind[sh][1]);
					vValue[ip] += valSH * vShape[sh];
				}
			}

			}catch(UGError_LocalShapeFunctionSetNotRegistered& ex){
				UG_THROW("GridFunctionNumberData: "<< ex.get_msg()<<", Reference Object: "
				         <<roid<<", Trial Space: "<<m_lfeID<<", refDim="<<refDim);
			}
		}
};

template <typename TGridFunction>
class GridFunctionVectorData
	: public StdGridFunctionData<MathVector<TGridFunction::dim>, TGridFunction::dim,
	  	  	  	  	  	  	  	  GridFunctionVectorData<TGridFunction> >
{
	public:
	//	world dimension of grid function
		static const int dim = TGridFunction::dim;

	private:
	// grid function
		SmartPtr<TGridFunction> m_spGridFct;

	//	component of function
		size_t m_vfct[dim];

	//	local finite element id
		LFEID m_vlfeID[dim];

	public:
	/// constructor
		GridFunctionVectorData(SmartPtr<TGridFunction> spGridFct, const char* cmp)
			: m_spGridFct(spGridFct)
		{
		//	create function group of this elem disc
			try{
			//	get strings
				std::string fctString = std::string(cmp);

			//	tokenize strings and select functions
				std::vector<std::string> tokens;
				TokenizeString(fctString, tokens, ',');

				for(size_t i = 0; i < tokens.size(); ++i)
						RemoveWhitespaceFromString(tokens[i]);

				if((int)tokens.size() != dim)
					UG_THROW("GridFunctionVectorData: Needed "<<dim<<" components "
					         "in symbolic function names, but given: "<<cmp);

			//	get function id of name
				for(int i = 0; i < dim; ++i){
					m_vfct[i] = spGridFct->fct_id_by_name(tokens[i].c_str());
					m_vlfeID[i] = spGridFct->local_finite_element_id(m_vfct[i]);
				}

			}UG_CATCH_THROW("GridFunctionVectorData: Cannot find  some symbolic function "
							"name in '"<<cmp<<"'.");
		};

		template <int refDim>
		inline void evaluate(MathVector<dim> vValue[],
							 const MathVector<dim> vGlobIP[],
							 number time, int si,
							 LocalVector& u,
							 GeometricObject* elem,
							 const MathVector<dim> vCornerCoords[],
							 const MathVector<refDim> vLocIP[],
							 const size_t nip,
							 const MathMatrix<refDim, dim>* vJT = NULL) const
		{
		//	reference object id
			const ReferenceObjectID roid = elem->reference_object_id();

		//	memory for shapes
			std::vector<number> vShape;

		//	loop ips
			try{
			for(size_t ip = 0; ip < nip; ++ip)
			{
				for(int d = 0; d < dim; ++d)
				{
					const LocalShapeFunctionSet<refDim>& rTrialSpace =
							LocalShapeFunctionSetProvider::get<refDim>(roid, m_vlfeID[d]);

				//	evaluate at shapes at ip
					rTrialSpace.shapes(vShape, vLocIP[ip]);

				//	get multiindices of element
					std::vector<MultiIndex<2> > ind;
					m_spGridFct->multi_indices(elem, m_vfct[d], ind);

				// 	compute solution at integration point
					vValue[ip] = 0.0;
					for(size_t sh = 0; sh < vShape.size(); ++sh)
					{
						const number valSH = BlockRef((*m_spGridFct)[ind[sh][0]], ind[sh][1]);
						vValue[ip][d] += valSH * vShape[sh];
					}
				}
			}

			}UG_CATCH_THROW("GridFunctionVectorData: Reference Object: "
						 <<roid<<", refDim="<<refDim);
		}
};


template <typename TGridFunction>
class GridFunctionGradientData
	: public StdGridFunctionData<MathVector<TGridFunction::dim>, TGridFunction::dim,
	  	  	  	  	  	  	  	  GridFunctionGradientData<TGridFunction> >
{
	public:
	//	world dimension of grid function
		static const int dim = TGridFunction::dim;

	private:
	// grid function
		SmartPtr<TGridFunction> m_spGridFct;

	//	component of function
		size_t m_fct;

	//	local finite element id
		LFEID m_lfeID;

	public:
	/// constructor
		GridFunctionGradientData(SmartPtr<TGridFunction> spGridFct, const char* cmp)
			: m_spGridFct(spGridFct)
		{
		//	get function id of name
			m_fct = spGridFct->fct_id_by_name(cmp);

		//	check that function exists
			if(m_fct >= spGridFct->num_fct())
				UG_THROW("GridFunctionGradientData: Function space does not contain"
						" a function with name " << cmp << ".");

		//	local finite element id
			m_lfeID = spGridFct->local_finite_element_id(m_fct);
		};

		template <int refDim>
		inline void evaluate(MathVector<dim> vValue[],
							 const MathVector<dim> vGlobIP[],
							 number time, int si,
							 LocalVector& u,
							 GeometricObject* elem,
							 const MathVector<dim> vCornerCoords[],
							 const MathVector<refDim> vLocIP[],
							 const size_t nip,
							 const MathMatrix<refDim, dim>* vJT = NULL) const
		{
		//	must pass vJT
			UG_ASSERT(vJT != NULL, "Jacobian transposed needed.");

		//	reference object id
			const ReferenceObjectID roid = elem->reference_object_id();

		//	get trial space
			try{
			const LocalShapeFunctionSet<refDim>& rTrialSpace =
					LocalShapeFunctionSetProvider::get<refDim>(roid, m_lfeID);

		//	storage for shape function at ip
			std::vector<MathVector<refDim> > vLocGrad;
			MathVector<refDim> locGrad;

		//	Reference Mapping
			MathMatrix<dim, refDim> JTInv;

		//	loop ips
			for(size_t ip = 0; ip < nip; ++ip)
			{
			//	evaluate at shapes at ip
				rTrialSpace.grads(vLocGrad, vLocIP[ip]);

			//	get multiindices of element
				std::vector<MultiIndex<2> > ind;
				m_spGridFct->multi_indices(elem, m_fct, ind);

			//	compute grad at ip
				VecSet(locGrad, 0.0);
				for(size_t sh = 0; sh < vLocGrad.size(); ++sh)
				{
					const number valSH = BlockRef((*m_spGridFct)[ind[sh][0]], ind[sh][1]);
					VecScaleAppend(locGrad, valSH, vLocGrad[sh]);
				}

				Inverse(JTInv, vJT[ip]);
				MatVecMult(vValue[ip], JTInv, locGrad);
			}
			}catch(UGError_LocalShapeFunctionSetNotRegistered& ex){
				UG_THROW("GridFunctionGradientData: "<< ex.get_msg()<<", Reference Object: "
				         <<roid<<", Trial Space: "<<m_lfeID<<", refDim="<<refDim);
			}
		}
};

} // end namespace ug

#endif /* __H__UG__LIB_DISC__FUNCTION_SPACE__GRID_FUNCTION_USER_DATA__ */
