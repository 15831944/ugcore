// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// y10 m01 d20

#ifndef __H__UG__UG_SCRIPT__
#define __H__UG__UG_SCRIPT__
#include <vector>
#include <string>

extern "C" {
#include "externals/lua/lua.h"
#include "externals/lua/lauxlib.h"
#include "externals/lua/lualib.h"
}

#include "common/common.h"
#include "common/util/path_provider.h"




namespace ug
{

namespace script
{

///	Error class thrown if an error occurs during parsing.
class LuaError : public UGError
{
	public:
		LuaError(const char* msg) : UGError(msg)	{}
};



///	loads and parses a file. Several paths are tried if the file is not found.
/**	Throws an instance of LuaError, if a parse error occurs.
 * This method first tries to load the file specified with filename relative
 * to the path of the currently parsed file (if LoadUGScript is called from
 * within a load-script). If this failed, the file is tried to be loaded
 * with the raw specified filename. If this fails too, the method tries to
 * load the file from ugs scripting directory.
 *
 * Note that this method pushes the path of the currently parsed script to
 * PathProvider when parsing starts, and pops it when parsing is done.*/
UG_API bool LoadUGScript(const char* filename, bool bNoMPI=false);

///	returns the default lua state
/**	When called for the first time, or after ReleaseDefaultLuaState,
 * a new state is created and the methods and classes in ugs default registry
 * (ug::bridge::GetUGRegistry) are registered. Furthermore a callback
 * is registered, which registers new methods whenever
 * Registry::registry_changed() is called on the default registry.*/
UG_API lua_State* GetDefaultLuaState();

/// Releases the lua-state returned by GetDefaultLuaState().
/**	This method is useful, if you want to restart scripting from scratch.*/
UG_API void ReleaseDefaultLuaState();

///	parses and executes a buffer
/**	Throws an instance of LuaError, if a parse error occurs.*/
UG_API bool ParseBuffer(const char* buffer, const char *bufferName="buffer");

///	parses and executes a file
/**	Throws an instance of LuaError, if a parse error occurs.*/
UG_API bool ParseFile(const char* filename);

/// UGLuaPrint. Redirects LUA prints to UG_LOG
UG_API int UGLuaPrint(lua_State *L);

/// UGLuaWrite. prints LUA output to UG_LOG without adding std::endl automatically
UG_API int UGLuaWrite(lua_State *L);

/**
 * searches for the filename
 * - relative to current script
 * - as absolute filename
 * - in PathProvider::get_path(SCRIPT_PATH) (ug4/scripts)
 * - in PathProvider::get_path(APPS_PATH) (ug4/apps)
 * - in PathProvider::get_path(ROOT_PATH) (ug4)
 * @param filename in: relative filename to paths above. out: absolute filename (if found)
 * @return true if found, else false
 */
UG_API bool GetAbsoluteUGScriptFilename(const std::string &filename, std::string &absoluteFilename);

}//	end of namespace
}//	end of namespace


#endif
