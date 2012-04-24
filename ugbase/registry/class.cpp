/*
 * class.cpp
 *
 *  Created on: 26.09.2011
 *      Author: andreasvogel
 */

#include "class.h"

namespace ug{
namespace bridge{

////////////////////////////////////////////////////////////////////////////////
// ExportedConstructor
////////////////////////////////////////////////////////////////////////////////

ExportedConstructor::
ExportedConstructor(ProxyFunc pf,
                    const std::string& className, const std::string& options,
                    const std::string& paramInfos,
                    const std::string& tooltip, const std::string& help)
: m_proxy_func(pf), m_className(className),
  m_options(options), m_paramInfos(paramInfos), m_tooltip(tooltip), m_help(help)
{
#ifdef PROFILE_BRIDGE
	m_profname=m_className;
	m_profname.append("(...)");
	Shiny::ProfileZone pi = {NULL, Shiny::ProfileZone::STATE_HIDDEN, m_profname.c_str(),{ { 0, 0 }, { 0, 0 }, { 0, 0 } }};
	profileInformation = pi;
	profilerCache =	&Shiny::ProfileNode::_dummy;
#endif

//	Tokenize string for parameters into infos per one parameter (separated by '#')
	std::vector<std::string> vParamInfoTmp;
	tokenize(m_paramInfos, vParamInfoTmp, '#');
	m_vvParamInfo.resize(vParamInfoTmp.size());

//	Tokenize each info-string of one parameter into single infos (separated by '|')
	for(size_t i = 0; i < vParamInfoTmp.size(); ++i)
		tokenize(vParamInfoTmp[i], m_vvParamInfo[i], '|');
};

bool ExportedConstructor::check_consistency(std::string classname) const
{
//	flag to indicate, that unnamed parameter is found
	bool bUndeclaredParameterFound = false;

//	loop method parameters
	for(int j=0; j<params_in().size(); j++)
	{
		if(!params_in().parameter_named(j))
		{
		//	print error output, indicate parameter by 1, ..., NumParams
			if(!bUndeclaredParameterFound)
			{
				bUndeclaredParameterFound = true;
				UG_LOG("#### Registry ERROR: Unregistered Class used in ");
				UG_LOG("Constructor of class "<<classname.c_str());
				UG_LOG("': Parameter " << j+1);
			}
			else
			{	UG_LOG(", " << j+1);	}
		}
	}

//	check if undeclared parameter has been found
	if(bUndeclaredParameterFound) {UG_LOG("\n"); return false;}

//	everything ok
	return true;
}

void ExportedConstructor::tokenize(const std::string& str,
                                    std::vector<std::string>& tokens,
                                    const char delimiter)
{
	tokens.clear();
	std::stringstream tokenstream;
	tokenstream << str;
	std::string token;

	while ( std::getline (tokenstream, token, delimiter ) )
	{
		tokens.push_back(TrimString(token));
	}
}

////////////////////////////////////////////////////////////////////////////////
// Interface Exported Class
////////////////////////////////////////////////////////////////////////////////

bool IExportedClass::check_consistency() const
{
//	get class name vector of all parents
	const std::vector<const char*>* vClassNames = class_names();

//	check if class name vector correct
	if(vClassNames==NULL)
	{
		UG_LOG("ERROR in 'IExportedClass::check_consistency':"
				" Class name vector of parent classes missing for "
				"class '"<<this->name()<<"'.\n");
		return false;
	}

//	loop all base classes
	for(size_t i = 0; i < (*vClassNames).size(); ++i)
	{
	//	get name of base class
		const char* baseName = (*vClassNames)[i];

	//	check the name
		if(baseName == NULL || *baseName == '\0' || baseName[0] == '[')
		{
			if(i>0){
			UG_LOG("ERROR in 'IExportedClass::check_consistency':"
					" base class "<<i<<" of class '"<<this->name()<<
					"' has not been named.\n");
				return false;
			}
			else{
			UG_LOG("ERROR in 'IExportedClass::check_consistency':"
					" Class '"<<this->name()<<"' has not been named.\n");
				return false;
			}
		}
	}

//	everything ok
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of ExportedClassBaseImpl
////////////////////////////////////////////////////////////////////////////////
ExportedClassBaseImpl::
ExportedClassBaseImpl()
{
}

ExportedClassBaseImpl::
ExportedClassBaseImpl(const ExportedClassBaseImpl& other)
{
}

ExportedClassBaseImpl::
ExportedClassBaseImpl(const std::string& tooltip)
	: m_destructor(NULL), m_tooltip(tooltip), m_constructAsSmartPtr(false)
{
}

ExportedClassBaseImpl::
~ExportedClassBaseImpl()
{
//	delete constructors
	for(size_t i = 0; i < m_vConstructor.size(); ++i)
		delete (m_vConstructor[i].m_constructor);

//  delete methods
	for(size_t i = 0; i < m_vMethod.size(); ++i)
		delete m_vMethod[i];

	for(size_t i = 0; i < m_vConstMethod.size(); ++i)
		delete m_vConstMethod[i];
}

const std::string& ExportedClassBaseImpl::
tooltip() const
{
	return m_tooltip;
}

size_t ExportedClassBaseImpl::
num_methods() const
{
	return m_vMethod.size();
}

size_t ExportedClassBaseImpl::
num_const_methods() const
{
	return m_vConstMethod.size();
}

const ExportedMethod& ExportedClassBaseImpl::
get_method(size_t i) const
{
	return *m_vMethod.at(i)->get_overload(0);
}

const ExportedMethod& ExportedClassBaseImpl::
get_const_method(size_t i) const
{
	return *m_vConstMethod.at(i)->get_overload(0);
}

size_t ExportedClassBaseImpl::
num_overloads(size_t funcInd) const
{
	return m_vMethod.at(funcInd)->num_overloads();
}

size_t ExportedClassBaseImpl::
num_const_overloads(size_t funcInd) const
{
	return m_vConstMethod.at(funcInd)->num_overloads();
}

const ExportedMethod& ExportedClassBaseImpl::
get_overload(size_t funcInd, size_t oInd) const
{
	return *m_vMethod.at(funcInd)->get_overload(oInd);
}

const ExportedMethod& ExportedClassBaseImpl::
get_const_overload(size_t funcInd, size_t oInd) const
{
	return *m_vConstMethod.at(funcInd)->get_overload(oInd);
}

const ExportedMethodGroup& ExportedClassBaseImpl::
get_method_group(size_t ind) const
{
	return *m_vMethod.at(ind);
}

const ExportedMethodGroup& ExportedClassBaseImpl::
get_const_method_group(size_t ind) const
{
	return *m_vConstMethod.at(ind);
}

size_t ExportedClassBaseImpl::
num_constructors() const
{
	return m_vConstructor.size();
}

const ExportedConstructor& ExportedClassBaseImpl::
get_constructor(size_t i) const
{
	return *(m_vConstructor[i].m_constructor);
}

bool ExportedClassBaseImpl::
construct_as_smart_pointer() const
{
	return m_constructAsSmartPtr;
}

void ExportedClassBaseImpl::
set_construct_as_smart_pointer(bool enable)
{
	m_constructAsSmartPtr = enable;
}

bool ExportedClassBaseImpl::
is_instantiable() const
{
	return !m_vConstructor.empty();
}

void ExportedClassBaseImpl::
destroy(void* obj) const
{
	if(m_destructor != NULL)
		(*m_destructor)(obj);
}

bool ExportedClassBaseImpl::
constructor_type_id_registered(size_t typeID)
{
	for(size_t i = 0; i < m_vConstructor.size(); ++i)
		if(typeID == m_vConstructor[i].m_typeID)
			return true;

	return false;
}

bool ExportedClassBaseImpl::
constmethodname_registered(const std::string& name)
{
	for(size_t i = 0; i < m_vConstMethod.size(); ++i)
		if(name == m_vConstMethod[i]->name())
			return true;

	return false;
}

bool ExportedClassBaseImpl::
methodname_registered(const std::string& name)
{
	for(size_t i = 0; i < m_vMethod.size(); ++i)
		if(name == m_vMethod[i]->name())
			return true;

	return false;
}

ExportedMethodGroup* ExportedClassBaseImpl::
get_exported_method_group(const std::string& name)
{
	for(size_t i = 0; i < m_vMethod.size(); ++i)
		if(name == m_vMethod[i]->name())
			return m_vMethod[i];

	return NULL;
}

const ExportedMethodGroup* ExportedClassBaseImpl::
get_exported_method_group(const std::string& name) const
{
	for(size_t i = 0; i < m_vMethod.size(); ++i)
		if(name == m_vMethod[i]->name())
			return m_vMethod[i];

	return NULL;
}

ExportedMethodGroup* ExportedClassBaseImpl::
get_const_exported_method_group(const std::string& name)
{
	for(size_t i = 0; i < m_vConstMethod.size(); ++i)
		if(name == m_vConstMethod[i]->name())
			return m_vConstMethod[i];

	return NULL;
}

const ExportedMethodGroup* ExportedClassBaseImpl::
get_const_exported_method_group(const std::string& name) const
{
	for(size_t i = 0; i < m_vConstMethod.size(); ++i)
		if(name == m_vConstMethod[i]->name())
			return m_vConstMethod[i];

	return NULL;
}

} // end namespace ug
} // end namespace bridge
