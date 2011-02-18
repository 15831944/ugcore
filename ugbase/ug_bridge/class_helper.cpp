/**
 * \file class_helper.cpp
 *
 * \author Martin Rupp
 *
 * \date 20.10.2010
 *
 * ClassHierarchy implementation, GetClassHierarchy, FindClass
 *
 * Goethe-Center for Scientific Computing 2009-2010.
 */

#include <string>
#include <vector>
#include <algorithm>

#include "class_helper.h"
#include "registry.h"
#include "common/util/string_util.h"

using namespace std;

namespace ug
{
namespace bridge
{
extern Registry& GetUGRegistry();


void ClassHierarchy::insert_class(const IExportedClass &c)
{

	//	get name and visualization options of function
	std::vector<std::string> vGroups;
	TokenizeString(c.group(), vGroups, '/');

	ClassHierarchy *base = this;

	for(vector<std::string>::const_iterator it = vGroups.begin(); it != vGroups.end(); ++it)
	{
		const std::string thename = TrimString(*it);
		if(thename.length() <= 0) continue;
		bool bFound = false;
		for(size_t j=0; j<base->subclasses.size(); j++)
		{
			if(base->subclasses.at(j).name.compare(thename) == 0)
			{
				base = &base->subclasses.at(j);
				bFound = true;
				break;
			}
		}

		if(!bFound)
		{
			ClassHierarchy newclass;
			newclass.name = thename;
			newclass.bGroup = true;
			base->subclasses.push_back(newclass);
			base = &base->subclasses.back();
		}
	}

	const vector<const char *> *pNames = c.class_names();

	if(pNames == NULL) return;

	for(vector<const char*>::const_reverse_iterator rit = pNames->rbegin(); rit != pNames->rend(); ++rit)
	{
		const char *thename = (*rit);
		bool bFound = false;
		for(size_t j=0; j<base->subclasses.size(); j++)
		{
			if(base->subclasses.at(j).name.compare(thename) == 0)
			{
				base = &base->subclasses.at(j);
				bFound = true;
				break;
			}
		}

		if(!bFound)
		{
			ClassHierarchy newclass;
			newclass.name = thename;
			newclass.bGroup = false;
			base->subclasses.push_back(newclass);
			base = &base->subclasses.back();
		}
	}
}

void ClassHierarchy::sort()
{
	std::sort(subclasses.begin(), subclasses.end());
	for(size_t i=0; i<subclasses.size(); i++)
		subclasses[i].sort();
}

ClassHierarchy *ClassHierarchy::find_class(const char *classname)
{
	if(name.compare(classname) == 0)
		return this;
	for(size_t i=0; i < subclasses.size(); i++)
	{
		ClassHierarchy *c = subclasses[i].find_class(classname);
		if(c) return c;
	}
	return NULL;
}

void GetClassHierarchy(ClassHierarchy &hierarchy, const Registry &reg)
{
	hierarchy.subclasses.clear();
	hierarchy.name = "UGBase";
	for(size_t i=0; i<reg.num_classes(); ++i)
		hierarchy.insert_class(reg.get_class(i));
	hierarchy.sort();
}





void PrintClassSubHierarchy(ClassHierarchy &c, int level)
{
	for(int j=0; j<level; j++) UG_LOG("  ");
	UG_LOG(c.name << endl);
	if(c.subclasses.size())
	{
		for(size_t i=0; i<c.subclasses.size(); i++)
			PrintClassSubHierarchy(c.subclasses[i], level+1);
	}
}

bool PrintClassHierarchy(Registry &reg, const char *classname)
{
	const IExportedClass *c = FindClass(reg, classname);
	if(c == NULL)
	{
		UG_LOG("Class name " << classname << " not found\n");
		return false;
	}

	UG_LOG("\nClass Hierarchy of " << classname << "\n");

	int level = 0;
	const std::vector<const char*> *names = c->class_names();
	if(names != NULL && names->size() > 0)
	{
		for(int i = names->size()-1; i>0; i--)
		{
			for(int j=0; j<level; j++) UG_LOG("  ");
			UG_LOG(names->at(i) << endl);
			level++;
		}
	}

	ClassHierarchy hierarchy;
	GetClassHierarchy(hierarchy, reg);
	ClassHierarchy *ch = hierarchy.find_class(classname);
	if(ch)
		PrintClassSubHierarchy(*ch, level);
	else
	{
		for(int j=0; j<level; j++) UG_LOG("  ");
		UG_LOG(classname);
	}

	return true;

}




const IExportedClass *FindClass(Registry &reg, const char* classname)
{
	for(size_t j=0; j<reg.num_classes(); ++j)
		if(strcmp(classname, reg.get_class(j).name()) == 0)
		{
			return &reg.get_class(j);
			break;
		}
	return NULL;
}





/**
 *
 * \brief Gets a description of the i-th parameter of a ParameterStack
 * todo: perhaps this function would be better somewhere else like in parameter_stack.cpp
  */
string ParameterToString(const ParameterStack &par, int i)
{
	switch(par.get_type(i))
	{
	default:
	case PT_UNKNOWN:
		return string("unknown");
	case PT_BOOL:
		return string("bool");

	case PT_INTEGER:
		return string("integer");

	case PT_NUMBER:
		return string("number");

	case PT_STRING:
		return string("string");

	case PT_POINTER:
		return string(par.class_name(i)).append("*");
		break;

	case PT_CONST_POINTER:
		return string("const ").append(par.class_name(i)).append("*");
		break;
	}
}

bool PrintParametersIn(const ExportedFunctionBase &thefunc, const char*highlightclassname = NULL)
{
	UG_LOG("(");
	for(size_t i=0; i < (size_t)thefunc.params_in().size(); ++i)
	{
		if(i>0) UG_LOG(", ");
		bool b=false;
		if(highlightclassname != NULL && thefunc.params_in().class_name(i) != NULL &&
				strcmp(thefunc.params_in().class_name(i), highlightclassname)==0)
			b = true;
		if(b) UG_LOG("[");
		UG_LOG(ParameterToString(thefunc.params_in(), i));
		if(i < thefunc.num_parameter())
			UG_LOG(" " << thefunc.parameter_name(i));
		if(b) UG_LOG("]");
	}
	UG_LOG(")");
	return true;
}

bool PrintParametersOut(const ExportedFunctionBase &thefunc)
{
	if(thefunc.params_out().size() == 1)
	{
		UG_LOG(ParameterToString(thefunc.params_out(), 0));
		//file << " " << thefunc.return_name();
		UG_LOG(" ");
	}
	else if(thefunc.params_out().size() > 1)
	{
		UG_LOG("(");
		for(int i=0; i < thefunc.params_out().size(); ++i)
		{
			if(i>0) UG_LOG(", ");
			UG_LOG(ParameterToString(thefunc.params_out(), i));

		}
		UG_LOG(") ");
	}
	return true;
}

/**
 *
 * Prints parameters of the function thefunc.
 * If highlightclassname != NULL, it highlights parameters which implement the highlightclassname class.
 */
void PrintFunctionInfo(const ExportedFunctionBase &thefunc, bool isConst, const char *classname, const char *highlightclassname)
{
	PrintParametersOut(thefunc);
	if(classname)
		UG_LOG(classname << ":");

	UG_LOG(thefunc.name() << " ");

	PrintParametersIn(thefunc, highlightclassname);

	if(isConst) { UG_LOG(" const"); }
}


const ExportedFunction *FindFunction(Registry &reg, const char *functionname)
{
	for(size_t i=0; i<reg.num_functions(); i++)
	{
		if(strcmp(functionname, reg.get_function(i).name().c_str()) == 0)
			return &reg.get_function(i);
	}
	return NULL;
}

/**
 *
 * searches for a function named functionname in the registry and prints it
 */
bool PrintFunctionInfo(Registry &reg, const char *functionname)
{
	const ExportedFunction *f = FindFunction(reg, functionname);
	if(f)
	{
		PrintFunctionInfo(*f, false);
		return true;
	}
	else
		return false;
}

/**
 *
 * \brief Prints the (const) method of one class
 */
void PrintClassInfo(const IExportedClass &c)
{
	UG_LOG("class " << c.name() << ", " << c.num_methods() << " method(s), " <<
		c.num_const_methods() << " const method(s):" << endl);
	for(size_t k=0; k<c.num_methods(); ++k)
	{
		UG_LOG(" ");
		PrintFunctionInfo(c.get_method(k), false);
		UG_LOG(endl);
	}
	for(size_t k=0; k<c.num_const_methods(); ++k)
	{
		UG_LOG(" ");
		PrintFunctionInfo(c.get_const_method(k), true);
		UG_LOG(endl);
	}
}


/**
 *
 * Searches the classname in the Registry and prints info of the class
 */
bool PrintClassInfo(Registry &reg, const char *classname)
{
	// search registry for that class
	const IExportedClass *c = FindClass(reg, classname);
	if(c)
	{
		PrintClassInfo(*c);
		return true;
	}
	else
		return false;
}


/**
 *
 * \return true, if the class classname is in a parameter in the ParameterStack par
 */
bool IsClassInParameters(const ParameterStack &par, const char *classname)
{
	int i;
	for(i=0; i<par.size(); ++i)
	{
		if(par.get_type(i) != PT_POINTER && par.get_type(i) != PT_CONST_POINTER)
			continue;
		if(par.class_names(i) != NULL && strcmp(par.class_name(i), classname)==0)
			break;
	}

	if(i==par.size()) return false;
	else return true;
}


/**
 *
 * \param reg			Registry
 * \param classname 	the class (and only this class) to print usage in functions/member functions of.
 * \param OutParameters
 */
bool ClassUsageExact(Registry &reg, const char *classname, bool OutParameters)
{
	// check functions
	for(size_t i=0; i<reg.num_functions(); i++)
	{
		const ExportedFunctionBase &thefunc = reg.get_function(i);
		if((!OutParameters && IsClassInParameters(thefunc.params_in(), classname)) ||
				(OutParameters && IsClassInParameters(thefunc.params_out(), classname)))
		{
			UG_LOG(" ");
			PrintFunctionInfo(thefunc, false, classname);
			UG_LOG(endl);
		}
	}

	// check classes
	for(size_t i=0; i<reg.num_classes(); i++)
	{
		const IExportedClass &c = reg.get_class(i);
		for(size_t i=0; i<c.num_methods(); i++)
		{
			const ExportedFunctionBase &thefunc = c.get_method(i);
			if((!OutParameters && IsClassInParameters(thefunc.params_in(), classname)) ||
					(OutParameters && IsClassInParameters(thefunc.params_out(), classname)))
			{
				UG_LOG(" ");
				PrintFunctionInfo(thefunc, false, c.name(), classname);
				UG_LOG(endl);
			}
		}

		for(size_t i=0; i<c.num_const_methods(); i++)
		{
			const ExportedFunctionBase &thefunc = c.get_const_method(i);
			if((!OutParameters && IsClassInParameters(thefunc.params_in(), classname)) ||
					(OutParameters && IsClassInParameters(thefunc.params_out(), classname)))
			{
				UG_LOG(" ");
				PrintFunctionInfo(thefunc, false, c.name(), classname);
				UG_LOG(endl);
			}
		}
	}
	return true;
}


} // namespace bridge
} // namespace ug

