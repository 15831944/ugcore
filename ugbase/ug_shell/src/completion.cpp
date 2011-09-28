/**
 * \file completion.cpp
 *
 * \author Martin Rupp
 *
 * \date 14.10.2010
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#include <iostream>
#include <sstream>

#include <dirent.h>
#include <string>
#include <sys/stat.h>

#include "ug.h"

#include "bindings/lua/lua_util.h"
#include "bridge/ug_bridge.h"
#include "bindings/lua/info_commands.h"
#include "bindings/lua/lua_stack_check.h"
#include "registry/registry.h"

extern "C"
{
#include "bindings/lua/externals/lua/lstate.h"
}

using namespace std;
using namespace ug;
using namespace bridge;


/**
 * GetNamespaceCompletitions
 * gets completion of the word in p, e.g. for math.pi (with '.')
 * and puts matching completions in matches.
 * \param buf the buffer to complete
 * \param len the length of buf
 * \param matches put your matches here
 * \param sniplen how much of buf we use (for example, completing "ex" to "example" is 2, completing "examp" to "example" is 5)
 * \return number of added matches
 */
static size_t GetNamespaceCompletitions(char *buf, int len, std::vector<string> &matches, size_t &sniplen, int iPrintCompletionList)
{
	size_t matchesSizeBefore = matches.size();
	char *p = buf+len-1;

	// first we get the word left of the cursor to be completed.
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;

	if(*p != '.')
		return 0;
	p++;

	lua_State* L = script::GetDefaultLuaState();
	LUA_STACK_CHECK(L, 0);

	char *snip = p;
	sniplen = strlen(snip);
	// get word before '.' => name of the object
	p--; UG_ASSERT(*p == '.', ""); p--;
	while(p >= buf && (isalnum(*p) || *p == '_' || *p == '.')) // all?
		p--;
	p++;
	string name;
	name.append(p, snip-p-1);

	GetLuaNamespace(L, name);

	if(lua_istable(L, -1))
	{
		lua_pushnil( L ); // -1
		while( lua_next( L, -2 ))
		{
			const char *name = lua_tostring(L, -2);
			if(strncmp(snip, name, sniplen) == 0)
				matches.push_back(name);

			lua_pop(L, 1);
		}
	}

	lua_pop(L, 1); // pop global

	if(iPrintCompletionList == 2 && matches.size() == 1 && sniplen == matches[0].size())
	{
		UG_LOG("\n");
		bridge::UGTypeInfo(p);
	}
	return matches.size() - matchesSizeBefore;
}

/*
 * GetMemberFunctionCompletitions
 * gets completion of the word in p, based on the classname which is before p,
 * and puts matching completions in matches.
 * \param buf the buffer to complete
 * \param len the length of buf
 * \param matches put your matches here
 * \param sniplen how much of buf we use (for example, completing "ex" to "example" is 2, completing "examp" to "example" is 5)
 * \return number of added matches
 */
static size_t GetMemberFunctionCompletitions(char *buf, int len, std::vector<string> &matches, size_t &sniplen)
{
	size_t matchesSizeBefore = matches.size();
	char *p = buf+len-1;

	// first we get the word left of the cursor to be completed.
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;

	if(*p != ':')
		return false;
	p++;
	sniplen = strlen(p);

	lua_State* L = script::GetDefaultLuaState();
	LUA_STACK_CHECK(L, 0);

	Registry &reg = GetUGRegistry();

	char *snip = p;
	sniplen = strlen(snip);
	// get word before ':' => name of the object
	p--; UG_ASSERT(*p == ':', ""); p--;
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;
	p++;
	string name;
	name.append(p, snip-p-1);

	const std::vector<const char*> *names = GetClassNames(L, name.c_str());
	if(names == NULL) return 0;

	// now we have the class names of the lua object
	for(size_t i=0; i < names->size(); ++i)
	{
		const IExportedClass *c = FindClass(reg, (*names)[i]);
		if(c == NULL) continue;

		// classname found, now add matching completions
		for(size_t k=0; k<c->num_methods(); ++k)
		{
			const char *name = c->get_method(k).name().c_str();
			if(strncmp(snip, name, sniplen) == 0)
				matches.push_back(name);
		}
		for(size_t k=0; k<c->num_const_methods(); ++k)
		{
			const char *name = c->get_const_method(k).name().c_str();
			if(strncmp(snip, name, sniplen) == 0)
				matches.push_back(name);
		}
	}

	return matches.size() - matchesSizeBefore;
}

/**
 * GetGlobalFunctionInfo
 * if buf is something like "function(", we print the function info (like "void function(number a, number b)")
 * \return true if something like "function(" found, else false.
 */
static bool GetGlobalFunctionInfo(char *buf, int len)
{
	char *p = buf+len-1;
	if(*p != '(') return false;

	p--;

	// first we get the word left of the cursor to be completed.
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;

	p++;

	string snip;
	snip.append(p, strlen(p)-1);

	lua_State* L = script::GetDefaultLuaState();
	LUA_STACK_CHECK(L, 0);

	Registry &reg = GetUGRegistry();

	lua_getglobal(L, snip.c_str());
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}

	if(lua_iscfunction(L, -1))
	{
		printf("\n");
		PrintFunctionInfo(reg, snip.c_str());
		printf("\n");
	}
	else if(lua_isfunction(L, -1))
	{
		printf("\n");
		PrintFunctionInfo(L, false);
		printf("\n");
	}
	lua_pop(L, 1);

	return false;
}

/**
 * GetMemberFunctionInfo
 * if buf is something like "instance:function(", we print the function info (like "void class::function(number a, number b)")
 * \return true if something like "function(" found, else false.
 */
static bool GetMemberFunctionInfo(char *buf, int len)
{
	char *p = buf+len-1;
	if(*p != '(') return false;

	p--;

	// first we get the word left of the cursor to be completed.
	while(p >= buf &&
		(isalnum(*p) || *p == '_')) // all?
	{
		p--;
	}

	if(*p != ':')
		return false;
	p++;

	lua_State* L = script::GetDefaultLuaState();
	LUA_STACK_CHECK(L, 0);

	Registry &reg = GetUGRegistry();

	char *psnip = p;
	string snip;
	snip.append(p, strlen(p)-1);

	// get word before ':' => name of the object
	p--; UG_ASSERT(*p == ':', ""); p--;
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;
	p++;
	string name;
	name.append(p, psnip-p-1);

	const std::vector<const char*> *names = GetClassNames(L, name.c_str());
	if(names == NULL) return 0;

	bool bFound = false;
	// now we have the class names of the lua object
	for(size_t i=0; i < names->size(); ++i)
	{
		const IExportedClass *c = FindClass(reg, (*names)[i]);
		if(c == NULL) continue;

		// classname found, now add matching completions
		for(size_t k=0; k<c->num_methods(); ++k)
		{
			if(snip.compare(c->get_method(k).name()) == 0)
			{
				printf("\n");
				PrintFunctionInfo(c->get_method(k), false, (*names)[i]);
				bFound = true;
			}
		}
		for(size_t k=0; k<c->num_const_methods(); ++k)
		{
			if(snip.compare(c->get_const_method(k).name()) == 0)
			{
				printf("\n");
				PrintFunctionInfo(c->get_method(k), false, (*names)[i]);
				bFound = true;
			}

		}
	}

	if(bFound)
		printf("\n");
	return bFound;
}

/**
 * GetGlobalsCompletitions
 * searches in the Lua string table for string that completes the string in buf
 * if they are also globals, we add them to vector<string> matches.
 * \param buf the buffer to complete
 * \param len the length of buf
 * \param matches put your matches here
 * \param sniplen how much of buf we use (for example, completing "ex" to "example" is 2, completing "examp" to "example" is 5)
 * \return number of added matches
 */
static size_t GetGlobalsCompletitions(char *buf, int len, std::vector<string> &matches, size_t &sniplen, int iPrintCompletionList)
{
	size_t matchesSizeBefore = matches.size();
	char *p = buf+len-1;

	// first we get the word left of the cursor to be completed.
	while(p >= buf &&
		(isalnum(*p) || *p == '_')) // all?
	{
		p--;
	}

	p++;
	char *snip = p;
	sniplen = strlen(snip);
	if(sniplen == 0) return 0;

	lua_State* L = script::GetDefaultLuaState();
	LUA_STACK_CHECK(L, 0);

	// iterate through all of lua's global string table
	for(int i=0; i<G(L)->strt.size; i++)
	{
		GCObject *obj;
		for (obj = G(L)->strt.hash[i]; obj != NULL; obj = obj->gch.next)
		{
			// get the string
			TString *ts = rawgco2ts(obj);
			if(ts == NULL) continue;

			const char *luastr = getstr(ts);
			// check is of a global variable
			lua_getglobal(L, luastr);
			if(lua_isnil(L, -1))
			{
				lua_pop(L, 1); // remove global from stack
				continue; // nope
			}
			lua_pop(L, 1); // remove global from stack

			// it is a global variable, now try matching
			if(luastr != NULL && strchr(luastr, ' ') == NULL
					&& strncmp(luastr, snip, sniplen) == 0)
				matches.push_back(luastr);

		}
	}

	if(iPrintCompletionList == 2 && matches.size() == 1 && sniplen == matches[0].size())
	{
		UG_LOG("\n");
		bridge::UGTypeInfo(p);
	}
	return matches.size() - matchesSizeBefore;
}

static size_t GetClassesCompletitions(char *buf, int len, std::vector<string> &matches, size_t &sniplen, int iPrintCompletionList)
{
	// we miss some classes which are not instatiable otherwise
	size_t matchesSizeBefore = matches.size();
	char *p = buf+len-1;

	// first we get the word left of the cursor to be completed.
	while(p >= buf && (isalnum(*p) || *p == '_')) // all?
		p--;
	p++;
	sniplen = strlen(p);

	Registry &reg = GetUGRegistry();
	for(size_t j=0; j<reg.num_classes(); ++j)
	{
		if(strncmp(p, reg.get_class(j).name().c_str(), sniplen) == 0)
			matches.push_back(reg.get_class(j).name());
	}

	if(iPrintCompletionList == 2 && matches.size() == 1 && sniplen == matches[0].size())
	{
		UG_LOG("\n");
		bridge::UGTypeInfo(p);
	}
	return matches.size() - matchesSizeBefore;
}

/**
 * GetPathCompletitions
 * puts in matches completitions of path in buf if possible
 * \param buf the buffer to complete
 * \param len the length of buf
 * \param matches put your matches here
 * \param sniplen how much of buf we use (for example, completing "ex" to "example" is 2, completing "examp" to "example" is 5)
 * \return number of added matches
 */
size_t GetPathCompletitions(char *buf, int len, std::vector<string> &matches, size_t &sniplen)
{
	size_t matchesSizeBefore = matches.size();
	char *p = buf+len-1;

	// first we get the word left of the cursor to be completed.
	// it can be a string or a path
	char *lastSlash = NULL;
	while(p >= buf &&
		(isalnum(*p) || *p == '_'
		|| *p == '/' ||  *p == '.' || *p == '\\' || *p == '-' )) // all?
	{
		if((*p == '/' || *p == '\\') && lastSlash == NULL)
			lastSlash=p;
		p--;
	}

	if(*p != '"')
		return 0;

	// first we look if we can complete a filename/directory
	p++;

	if(strlen(p) == 0) return 0;

	string dirname;
	if(lastSlash)
	{
		// if a slash was found, the string from p to lastSlash
		// is the directory
		dirname.append(p, lastSlash-p+1);
		p = lastSlash+1;
	}
	else
		// otherwise we are searching in the current directory
		dirname = ".";

	sniplen = strlen(p);

	DIR           *d;
	struct dirent *dir;
	d = opendir(dirname.c_str());
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
			if(strncmp(dir->d_name, p, sniplen) == 0)
			{
				string filename(dirname);
				filename.append("/");
				filename.append(dir->d_name);
				struct stat stat_struct;
				stat(filename.c_str(), &stat_struct);

				if(S_ISDIR(stat_struct.st_mode))
				{
					string tmp(dir->d_name);
					tmp.append("/");
					matches.push_back(tmp);
				}
				else if(S_ISREG(stat_struct.st_mode))
					matches.push_back(dir->d_name);
			}
		closedir(d);
	}

	return matches.size() - matchesSizeBefore;
}

/*
 * \brief A function to implement word completion of classes and functions of ugscript
 * When entered Dom<tab>, it completes to Domain2d, for example. If Domain2d and Domain3d
 * are registered, it completes to Domain and shows suggestions Domain2d and Domain3d.
 * currently only works with Linenoise.
 * todo: 	- not all completions must have same snipped length. change
  * \sa linenoiseSetCompletionFunction
  * \param buf buffer from linenoise to complete
  * \param len length of the buf
  * \param buflen maximal length of the buffer buf
  * \param iPrintCompletitionList number of times <tab> has been pushed, so we can display next 5 matches each time <tab> has been pushed.
 */
int CompletionFunction(char *buf, int len, int buflen, int iPrintCompletionList)
{
	vector<string> matches;
	size_t sniplen=0;

	if(iPrintCompletionList == 0 && (GetGlobalFunctionInfo(buf, len) || GetMemberFunctionInfo(buf, len)) )
		return len;

	if( GetPathCompletitions(buf, len, matches, sniplen)  == 0
		&& GetMemberFunctionCompletitions(buf, len, matches, sniplen) == 0
		&& GetNamespaceCompletitions(buf, len, matches, sniplen, iPrintCompletionList) == 0
		&& GetGlobalsCompletitions(buf, len, matches, sniplen, iPrintCompletionList)==0
		&& GetClassesCompletitions(buf, len, matches, sniplen, iPrintCompletionList)==0)
		return len; // no match

	if(matches.size() == 0)
		return len; // no match
	else if(matches.size() == 1)
	{
		// only one match: append
		strcat(buf, matches[0].c_str()+sniplen);
		return strlen(buf);
	}
	else
	{
		// several matches. get longest common substring:
		size_t submatch=0, i;
		for(submatch = 0; ; submatch++)
		{
			if(matches[0][submatch] == 0x00) break;
			for(i = 0; i<matches.size(); i++)
				if(matches[i][submatch] != matches[0][submatch])
					break;
			if(i != matches.size())
				break;
		}

		// append longest common substring
		if(strlen(buf)+submatch > buflen+sniplen)
			submatch = buflen-strlen(buf)-sniplen;
		if(submatch > sniplen)
			strncat(buf, matches[0].c_str()+sniplen, submatch-sniplen);

		// show at most 5 suggested completions
		// todo: remove doubled suggestions
		int first = (iPrintCompletionList-1)*5;
		if(first >= 0 && first < (int)matches.size())
		{
			printf("\x1b[0G ");
			if(first == 0) printf("completions: "); //, p);
			else printf(" ... ");
			printf("%s", matches[first].c_str());
			for(int i=first+1; i < first+5 && i < (int)matches.size(); i++)
				printf(", %s", matches[i].c_str());
			if((int)matches.size() > first+5)
				printf(" ... (%lu more)", matches.size()-first-5);
			printf("\n");
		}
		return strlen(buf);
	}
}
