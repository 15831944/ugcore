/*
 * scale_add_linker_impl.h
 *
 *  Created on: 04.07.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__SCALE_ADD_LINKER_IMPL__
#define __H__UG__LIB_DISC__SPATIAL_DISC__SCALE_ADD_LINKER_IMPL__

#include "scale_add_linker.h"
#include "lib_disc/spatial_disc/ip_data/data_linker_traits.h"

namespace ug{

////////////////////////////////////////////////////////////////////////////////
//	ScaleAddLinker
////////////////////////////////////////////////////////////////////////////////

template <typename TData, int dim, typename TDataScale>
ScaleAddLinker<TData,dim,TDataScale>::
ScaleAddLinker(const ScaleAddLinker& linker)
{
	if(linker.m_vpIPData.size() != linker.m_vpScaleData.size())
		UG_THROW("ScaleAddLinker: number of scaling factors and data mismatch.");

	for(size_t i = 0; i < linker.m_vpIPData.size(); ++i)
	{
		this->add(linker.m_vpScaleData[i], linker.m_vpIPData[i]);
	}
}


template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::
add(SmartPtr<IPData<TDataScale, dim> > scale, SmartPtr<IPData<TData, dim> > data)
{
//	current number of inputs
	const size_t numInput = base_type::num_input() / 2;

//	resize scaling
	m_vpIPData.resize(numInput+1, NULL);
	m_vpDependData.resize(numInput+1, NULL);
	m_vpScaleData.resize(numInput+1, NULL);
	m_vpScaleDependData.resize(numInput+1, NULL);

//	remember ipdata
	UG_ASSERT(data.valid(), "Null Pointer as Input set.");
	m_vpIPData[numInput] = data;
	m_vpDependData[numInput] = data.template cast_dynamic<DependentIPData<TData, dim> >();

//	remember ipdata
	UG_ASSERT(scale.valid(), "Null Pointer as Scale set.");
	m_vpScaleData[numInput] = scale;
	m_vpScaleDependData[numInput] = scale.template cast_dynamic<DependentIPData<TDataScale, dim> >();

//	increase number of inputs by one and set inputs at base class
	base_type::set_num_input(2*numInput+2);
	base_type::set_input(2*numInput, data);
	base_type::set_input(2*numInput+1, scale);
}

template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::
add(number scale, SmartPtr<IPData<TData, dim> > data)
{
	add(CreateConstUserData<dim>(scale, TDataScale()), data);
}

template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::
add(SmartPtr<IPData<TDataScale, dim> > scale, number data)
{
	add(scale, CreateConstUserData<dim>(data, TData()));
}

template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::
add(number scale, number data)
{
	add(CreateConstUserData<dim>(scale, TDataScale()),
	    CreateConstUserData<dim>(data, TData()));
}


template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::
evaluate (TData& value,
          const MathVector<dim>& globIP,
          number time, int si) const
{
	//	reset value
	value = 0.0;

	TData valData;
	TDataScale valScale;

//	add contribution of each summand
	for(size_t c = 0; c < m_vpIPData.size(); ++c)
	{
		(*m_vpIPData[c])(valData, globIP, time, si);
		(*m_vpScaleData[c])(valScale, globIP, time, si);

		linker_traits<TData, TDataScale>::
		mult_add(value, valData, valScale);
	}
}

template <typename TData, int dim, typename TDataScale>
template <int refDim>
void ScaleAddLinker<TData,dim,TDataScale>::
evaluate (TData& value,
          const MathVector<dim>& globIP,
          number time, int si,
          LocalVector& u,
          GeometricObject* elem,
          const MathVector<dim> vCornerCoords[],
          const MathVector<refDim>& locIP) const
{
	//	reset value
	value = 0.0;

	TData valData;
	TDataScale valScale;

//	add contribution of each summand
	for(size_t c = 0; c < m_vpIPData.size(); ++c)
	{
		(*m_vpIPData[c])(valData, globIP, time, si, u, elem, vCornerCoords, locIP);
		(*m_vpScaleData[c])(valScale, globIP, time, si, u, elem, vCornerCoords, locIP);

		linker_traits<TData, TDataScale>::
		mult_add(value, valData, valScale);
	}
}

template <typename TData, int dim, typename TDataScale>
template <int refDim>
void ScaleAddLinker<TData,dim,TDataScale>::
evaluate(TData vValue[],
         const MathVector<dim> vGlobIP[],
         number time, int si,
         LocalVector& u,
         GeometricObject* elem,
         const MathVector<dim> vCornerCoords[],
         const MathVector<refDim> vLocIP[],
         const size_t nip,
         const MathMatrix<refDim, dim>* vJT) const
{
	//	reset value
	for(size_t ip = 0; ip < nip; ++ip)
		vValue[ip] = 0.0;

	std::vector<TData> vValData(nip);
	std::vector<TDataScale> vValScale(nip);

//	add contribution of each summand
	for(size_t c = 0; c < m_vpIPData.size(); ++c)
	{
		(*m_vpIPData[c])(&vValData[0], vGlobIP, time, si, u,
						elem, vCornerCoords, vLocIP, nip, vJT);
		(*m_vpScaleData[c])(&vValScale[0], vGlobIP, time, si, u,
							elem, vCornerCoords, vLocIP, nip, vJT);

		for(size_t ip = 0; ip < nip; ++ip)
			linker_traits<TData, TDataScale>::
			mult_add(vValue[ip], vValData[ip], vValScale[ip]);
	}
}

template <typename TData, int dim, typename TDataScale>
void ScaleAddLinker<TData,dim,TDataScale>::compute(bool bDeriv)
{
//	check that size of Scalings and inputs is equal
	UG_ASSERT(m_vpIPData.size() == m_vpScaleData.size(), "Wrong num Scales.");

//	compute value
	for(size_t s = 0; s < this->num_series(); ++s)
		for(size_t ip = 0; ip < this->num_ip(s); ++ip)
		{
		//	reset value
			this->value(s,ip) = 0.0;

		//	add contribution of each summand
			for(size_t c = 0; c < m_vpIPData.size(); ++c)
			{
				linker_traits<TData, TDataScale>::
				mult_add(this->value(s, ip),
				         input_value(c, s, ip),
				         scale_value(c, s, ip));
			}
		}

//	check if derivative is required
	if(!bDeriv || this->zero_derivative()) return;

//	check sizes
	UG_ASSERT(m_vpDependData.size() == m_vpScaleDependData.size(),
	          	  	  	  	  	  	  	  	  	  "Wrong num Scales.");

//	clear all derivative values
	this->clear_derivative_values();

//	loop all inputs
	for(size_t c = 0; c < m_vpIPData.size(); ++c)
	{
	//	check if input has derivative
		if(!m_vpIPData[c]->zero_derivative())
		{
			for(size_t s = 0; s < this->num_series(); ++s)
				for(size_t ip = 0; ip < this->num_ip(s); ++ip)
				{
				//	loop functions
					for(size_t fct = 0; fct < input_num_fct(c); ++fct)
					{
					//	get common fct id for this function
						const size_t commonFct = input_common_fct(c, fct);

					//	loop dofs
						for(size_t sh = 0; sh < this->num_sh(fct); ++sh)
						{
							linker_traits<TData, TDataScale>::
							mult_add(this->deriv(s, ip, commonFct, sh),
							         input_deriv(c, s, ip, fct, sh),
							         scale_value(c, s, ip));
						}
					}
				}
		}

	//	check if scaling has derivative
		if(!m_vpScaleData[c]->zero_derivative())
		{
			for(size_t s = 0; s < this->num_series(); ++s)
				for(size_t ip = 0; ip < this->num_ip(s); ++ip)
				{
				//	loop functions
					for(size_t fct = 0; fct < scale_num_fct(c); ++fct)
					{
					//	get common fct id for this function
						const size_t commonFct = scale_common_fct(c, fct);

					//	loop dofs
						for(size_t sh = 0; sh < this->num_sh(fct); ++sh)
						{
							linker_traits<TData, TDataScale>::
							mult_add(this->deriv(s, ip, commonFct, sh),
									 input_value(c, s, ip),
									 scale_deriv(c, s, ip, fct, sh));
						}
					}
				}
		}
	}
}


} // end namespace ug

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__SCALE_ADD_LINKER_IMPL__ */
