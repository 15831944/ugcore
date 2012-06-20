/*
 * local_finite_element_id.cpp
 *
 *  Created on: 16.11.2010
 *      Author: andreasvogel
 */

#include "local_finite_element_id.h"
#include <string>
#include <algorithm> // std::transform
#include <cctype> // std::tolower
#include "common/error.h"

namespace ug{

/// writes the Identifier to the output stream
std::ostream& operator<<(std::ostream& out,	const LFEID& v)
{
	std::stringstream ss;
	if(v.m_order >= 0) ss << v.m_order;
	else if(v.m_order == LFEID::ADAPTIV) ss << "adaptive";
	else ss << "invalid";

	switch(v.m_type)
	{
		case LFEID::LAGRANGE: out << "(Lagrange, " << ss.str() << ")"; break;
		case LFEID::CROUZEIX_RAVIART: out << "(Crouzeix-Raviart, " << ss.str() << ")"; break;
		case LFEID::PIECEWISE_CONSTANT: out << "(Piecewise constant, " << ss.str() << ")"; break;
		case LFEID::DG: out << "(DG, " << ss.str() << ")"; break;
		case LFEID::USER_DEFINED: out << "(User defined, " << ss.str() << ")"; break;
		default: out << "(unknown, " << ss.str() << ")";
	}
	return out;
}

///	returns the LFEID for a combination of Space and order
LFEID ConvertStringToLFEID(const char* type, int order)
{
//	convert to string
	std::string typeStr(type);
	std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

//	compare
	LFEID::SpaceType eType = LFEID::NONE;
	if(typeStr == "lagrange") eType = LFEID::LAGRANGE;
	if(typeStr == "crouzeix-raviart") eType = LFEID::CROUZEIX_RAVIART;
	if(typeStr == "piecewise-constant") eType = LFEID::PIECEWISE_CONSTANT;
	if(typeStr == "dg") eType = LFEID::DG;

	return LFEID(eType, order);
}

///	returns the LFEID for a combination of Space and order
LFEID ConvertStringToLFEID(const char* type)
{
	int order;
//	convert to string
	std::string typeStr(type);
	std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::tolower);

//	compare
	LFEID::SpaceType eType = LFEID::NONE;
	if(typeStr == "lagrange"){
		eType = LFEID::LAGRANGE;
		order = 1;
	}
	if(typeStr == "crouzeix-raviart"){
		eType = LFEID::CROUZEIX_RAVIART;
		order = 1;
	}
	if(typeStr == "piecewise-constant"){
		eType = LFEID::PIECEWISE_CONSTANT;
		order = 0;
	}
	if(typeStr == "dg"){
		// eType = LFEID::DG;
		UG_THROW("Unspecified order for DG approximation space.\n");
	}

	return LFEID(eType, order);
}

/// @}

} // end namespace ug
