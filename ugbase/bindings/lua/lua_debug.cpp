// created by Martin Rupp
// martin.rupp@gcsc.uni-frankfurt.de
// y12 m03 d17

//////////////////////////////////////////////////////////////////////////////////////////////////////
//extern libraries
#include <cassert>
#include <cstring>
#include <string>
#include <stack>

//////////////////////////////////////////////////////////////////////////////////////////////////////
// ug libraries
#include "ug.h"
#include "lua_util.h"
#include "common/os_dependent/file_util.h"
#include "bindings_lua.h"
#include "bridge/bridge.h"
#include "registry/class_helper.h"
#include "info_commands.h"
#include "lua_user_data.h"
#include "registry/registry.h"

#include "lua_debug.h"
#include "common/profiler/dynamic_profiling.h"

extern "C" {
#include "externals/lua/lstate.h"
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

namespace ug
{

namespace bridge
{
string GetFileLines(const char *filename, size_t fromline, size_t toline, bool includeLineNumbers=false);
}
namespace script
{

//////////////////////////////////////////////////////////////////////////////////////////////////////
// globals


static bool bDebugging = false;
static int debugMode  = DEBUG_CONTINUE;
static bool bProfiling = false;
static std::map<std::string, std::map<int, bool> > breakpoints;
static debug_return (*pDebugShell)() = NULL;
static std::string lastsource;
static int lastline = -1;
static int currentDepth = -1;
#ifdef UG_PROFILER
static bool bEndProfiling=false;
static bool bProfileLUALines=true;
static int profilingDepth=0;
static int profilingEndDepth=0;
static std::map<const char*, std::map<int, pDynamicProfileInformation> >pis;
#endif


bool hookset = false;
//////////////////////////////////////////////////////////////////////////////////////////////////////

extern stack<string> stkPathes;

//////////////////////////////////////////////////////////////////////////////////////////////////////
void LuaCallHook(lua_State *L, lua_Debug *ar);

//////////////////////////////////////////////////////////////////////////////////////////////////////

void FinalizeLUADebug()
{
	breakpoints.clear();
#ifdef UG_PROFILER
	pis.clear();
#endif
	lastsource.clear();
}

int SetDebugShell(debug_return (*s)())
{
	pDebugShell=s;
	return 0;
}

void CheckHook()
{
	if(bDebugging == false && bProfiling == false)
	{
		if(hookset)
		{
			lua_sethook (GetDefaultLuaState(), NULL, 0, 0);
			hookset=false;
		}
	}
	else
	{
		if(!hookset)
		{
			lua_sethook (GetDefaultLuaState(), LuaCallHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
			hookset=true;
		}
	}

}

void AddBreakpoint(const char*source, int line)
{
	if(pDebugShell==NULL)
	{
		UG_LOG("No Debug Shell set!\n");
		return;
	}
	string file;
	const char *s=NULL;
	if(!stkPathes.empty())
	{
		file =stkPathes.top();
		file.append("/").append(source);
		if(FileExists(file.c_str()))
			s = file.c_str();
	}
	if(FileExists(source)) s = source;
	if(s)
	{
		breakpoints[s][line]=true;
		bDebugging = true;
		CheckHook();
		UG_LOG("breakpoint at " << s << ":" << line << "\n")
	}
	else
	{
		UG_LOG("file " << s << " not found\n");
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintBreakpoints()
{
	std::map<std::string, std::map<int, bool> >::iterator it1;
	std::map<int, bool>::iterator it2;
	for(it1 = breakpoints.begin(); it1 != breakpoints.end(); ++it1)
	{
		std::map<int, bool> &m = (*it1).second;
		for(it2 = m.begin(); it2 != m.end(); ++it2)
		{
			UG_LOG((*it1).first << ":" << (*it2).first << ((*it2).second?" enabled":" disabled") << "\n")
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void breakpoint()
{
	if(pDebugShell==NULL)
	{
		UG_LOG("Breakpoint reached, no Debug Shell set.\n");
		return;
	}
	debug_return r=pDebugShell();
	if(r == DEBUG_EXIT)
		UGForceExit();
	else if(r == DEBUG_CONTINUE)
	{
		debugMode = DEBUG_CONTINUE;
		bDebugging = breakpoints.size() > 0;
		CheckHook();
		return;
	}
	else if(r == DEBUG_NEXT || r == DEBUG_STEP || r == DEBUG_FINISH)
	{
		debugMode=r;
		bDebugging=true;
		CheckHook();
		return;
	}
}

int getDepth()
{
	lua_State *L = GetDefaultLuaState();
	int depth=0;
	lua_Debug entry;
	for(int i = 0; lua_getstack(L, i, &entry); i++)
	{
		lua_getinfo(L, "Sln", &entry);
		if(entry.currentline >= 0)
			depth++;
	}
	return depth;
}

void breakpoint_in_script()
{
	lua_Debug entry;
	lua_State *L = GetDefaultLuaState();
	currentDepth =getDepth();
	for(int depth = 0; lua_getstack(L, depth, &entry); depth++)
	{
		lua_getinfo(L, "Sln", &entry);
		if(entry.currentline >= 0)
		{
			lastsource = entry.source+1;
			lastline = entry.currentline;
			breakpoint();
			return;
		}
	}

	breakpoint();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void luaDebug(lua_State *L, const char *source, int line)
{
	if(source == NULL || line < 0 || pDebugShell==NULL) return;
	if(source[0]=='@') source++;

	bool bfound=false;
	if(debugMode == DEBUG_NEXT || debugMode == DEBUG_FINISH)
	{
		int d = getDepth();
		if( ((debugMode == DEBUG_NEXT && d <= currentDepth)
				|| (debugMode == DEBUG_FINISH && d < currentDepth))
				&& (lastsource.compare(source)==0 && lastline == line) == false)
		{
			lastsource = source;
			lastline = line;
			currentDepth = d;
			bfound =true;
		}
	}
	else if(debugMode == DEBUG_STEP)
	{
		if((lastsource.compare(source)==0 && lastline == line) == false)
		{
			lastsource = source;
			lastline = line;
			currentDepth = getDepth();
			bfound =true;
		}
	}


	if(!bfound && breakpoints.size() > 0)
	{
		lua_Debug entry;
		for(int depth = 0; lua_getstack(L, depth, &entry); depth++)
		{
			lua_getinfo(L, "Sln", &entry);
			if(entry.currentline >= 0)
			{
				std::map<int, bool> &m = breakpoints[entry.source+1];
				std::map<int, bool>::iterator it = m.find(entry.currentline);
				//UG_LOG(entry.source+1 << ":" << entry.currentline << "\n");
				if(it != m.end() && (*it).second == true &&
						(lastline != entry.currentline || lastsource.compare(entry.source+1)!=0))
				{
					lastsource = entry.source+1;
					lastline = entry.currentline;
					bfound=true;
					currentDepth = getDepth();
					break;
				}
			}
			if(bfound) break;
		}


	}

	if(!bfound) return;

	breakpoint();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void LuaCallHook(lua_State *L, lua_Debug *ar)
{
#if 0
	/*UG_LOG("------------------------\n");
	{
	    lua_Debug entry;
	    for(int depth = 0; lua_getstack(L, depth, &entry); depth++)
		{
	    	int status = lua_getinfo(L, "Sln", &entry);
	    	if(entry.currentline <0) continue;
	    	if(entry.short_src && entry.currentline>0)
	    	{
				UG_LOG(entry.short_src << ":" << entry.currentline);
				UG_LOG(" " << ug::bridge::GetFileLine(entry.short_src, entry.currentline));
	    	}
	    	if(entry.what)
	    	{
	    		UG_LOG(" " << entry.what);
	    	}
	    	if(entry.name)
	    	{
	    		UG_LOG(" " << entry.what);
	    	}
	    	UG_LOG(entry.event << "\n");
	    	UG_LOG("\n");
	    }
	}*/
#endif
	//fill up the debug structure with information from the lua stack
	lua_Debug entry;
	lua_getinfo(L, "Sln", ar);
	//if(ar->what[0] == 'L' || ar->what[0] == 'C')
	{
		if(ar->event == LUA_HOOKCALL || (bDebugging && ar->event ==LUA_HOOKLINE))
		{
#ifdef UG_PROFILER
			if(bEndProfiling)
			{
				profilingEndDepth++;
				if(bDebugging==false)
					return;
			}
#endif
			if(bDebugging)
			{
				const char *source = "unknown";
				int line = 0;
				if(ar->currentline < 0)
				{
					for(int depth = 0; lua_getstack(L, depth, &entry); depth++)
					{
						lua_getinfo(L, "Sln", &entry);
						if(entry.currentline >= 0)
						{
							source = entry.source;
							line = entry.currentline;
							break;
						}
					}
				}
				else
				{
					source = ar->source;
					line = ar->currentline;
				}
				luaDebug(L, source, line);
			}

#ifdef UG_PROFILER
			if(bProfiling && bEndProfiling==false && ar->event == LUA_HOOKCALL)
			{
				const char *source = ar->source;
				int line = ar->currentline;

				if(line < 0 && bProfileLUALines && lua_getstack(L, 1, &entry))
				{
					lua_getinfo(L, "Sln", &entry);
					source = entry.source;
					line = entry.currentline;
				}
				if(line >= 0)
				if(ar->what[0] == 'L' || ar->what[0] == 'C')
				{
					// be sure that this is const char*

					if(ar->event == LUA_HOOKCALL)
					{
						if(profilingDepth==0)
						{
							pDynamicProfileInformation &pi = pis[source][line];
							if(pi == NULL)
							{
								char buf[255] = "LUAunknown ";
								if(source[0]=='@') source++;
								if(strncmp(source, "./../scripts/", 13)==0)
									sprintf(buf, "!%s:%d ", source+13, line);
								else
									sprintf(buf, "@%s:%d ", source, line);
								const char*p = ug::bridge::GetFileLine(source, line).c_str();
								strncat(buf, p+strspn(p, " \t"), 254);

								pi = new DynamicProfileInformation;
								pi->set_name_and_copy(buf);
								// UG_LOG(buf);
							 }


							 pi->beginNode();
							 //UG_LOG(source << ":" << line << ". Profiling depth is " << profilingDepth << "\n");
						}
						profilingDepth++;
					}
				}
			}
#endif
		}
		else if(ar->event == LUA_HOOKRET)
		{
#if UG_PROFILER
			int line = ar->currentline;
			if(line < 0 && bProfileLUALines && lua_getstack(L, 1, &entry))
			{
				lua_getinfo(L, "Sln", &entry);
				line = entry.currentline;
			}
			if(bProfiling && line >= 0)
			{
				if(profilingEndDepth>0)
					profilingEndDepth--;
				//UG_ASSERT(pis[ar->source][ar->linedefined]->isCurNode(), "profiler nodes not matching. forgot a PROFILE_END?");
				else
				{
					if(profilingDepth>0)
					{
						profilingDepth--;
						if(profilingDepth==0)
							DynamicProfileInformation::endCurNode();
					}
					if(bEndProfiling && profilingDepth==0)
					{
						UG_LOG("Profiling ended.\n");
						bProfiling=false;
						bEndProfiling=false;
						CheckHook();
					}
				}
			}
#endif
		}

	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void ProfileLUA(bool b)
{
#ifdef UG_PROFILER
	if(bProfiling==false && b==true)
	{
		bEndProfiling=false;
		bProfiling=true;
		CheckHook();
	}
	else if(bProfiling == true && b==false)
	{
		bEndProfiling=true;
	}
#else
	UG_LOG("No profiler available.\n");
#endif
}

void DebugList()
{
	lua_Debug entry;
	int depth=getDepth();
	lua_State *L = GetDefaultLuaState();
	for(int i = 0; lua_getstack(L, i, &entry); i++)
	{
		lua_getinfo(L, "Sln", &entry);
		if(entry.currentline <0) continue;
		if(depth==currentDepth)
		{
			lastsource = entry.source+1;
			lastline = entry.currentline;
			UG_LOG(entry.source+1 << ":" << entry.currentline << "\n");
			if(entry.currentline-3<0)
				{UG_LOG(ug::bridge::GetFileLines(entry.source+1, 0, entry.currentline+5, true) << "\n");}
			else
				{UG_LOG(ug::bridge::GetFileLines(entry.source+1, entry.currentline-3, entry.currentline+5, true) << "\n");}
			UG_LOG("\n");
		}
		depth--;
	}
	// todo
}
void DebugBacktrace()
{
	ug::bridge::lua_stacktrace(GetDefaultLuaState());
}



void UpdateDepth()
{
	lua_Debug entry;
	int depth=getDepth();
	lua_State *L = GetDefaultLuaState();
	for(int i = 0; lua_getstack(L, i, &entry); i++)
	{
		lua_getinfo(L, "Sln", &entry);
		if(entry.currentline <0) continue;
		if(depth==currentDepth)
		{
			lastsource = entry.source+1;
			lastline = entry.currentline;
			UG_LOG(entry.source+1 << ":" << entry.currentline);
			UG_LOG(" " << ug::bridge::GetFileLine(entry.short_src, entry.currentline));
			UG_LOG("\n");
		}
		depth--;
	}
}
void DebugUp()
{
	if(currentDepth>0)
	{
		currentDepth--;
		UpdateDepth();
	}
	else
		{UG_LOG("already at base level.\n");}
}
void DebugDown()
{
	if(currentDepth < getDepth())
	{
		currentDepth++;
		UpdateDepth();
	}
	else
	{UG_LOG("already at max level.\n");}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
bool RegisterLuaDebug(ug::bridge::Registry &reg)
{
	reg.add_function("breakpoint", &AddBreakpoint, "/ug4/lua");
	reg.add_function("breakpoint", &breakpoint_in_script, "/ug4/lua");
	reg.add_function("print_breakpoints", &PrintBreakpoints, "/ug4/lua");

	reg.add_function("ProfileLUA", &ProfileLUA, "/ug4/lua");
	return true;
}


}
}
