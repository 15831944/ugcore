// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// y10 m01 d20

//extern libraries
#include <cassert>
#include <cstring>
#include <string>
#include <stack>


// ug libraries
#include "ug.h"
#include "lua_util.h"
#include "common/util/path_provider.h"
#include "common/util/file_util.h"
#include "bindings_lua.h"
#include "bridge/bridge.h"
#include "registry/class_helper.h"
#include "info_commands.h"
#include "lua_user_data.h"
#include "registry/registry.h"
#include "lua_debug.h"

#include "common/util/binary_buffer.h"


#ifdef UG_PARALLEL
#include "pcl/pcl.h"
#endif


using namespace std;

namespace ug
{

namespace script
{


#define PL_COULDNT_FIND -2
#define PL_COULDNT_READ -1





/**
 * Reads a file with fopen/fread into a BinaryStream
 * @param filename		filename of file to read
 * @param buf			BinaryBuffer to write data to
 * @param bText 		if true, add terminating zero at end
 * @return
 */
bool ReadFile(const char* filename, BinaryBuffer &buf, bool bText);

/**
 * Reads a file with fopen/fread into a std::vector<char>
 * @param filename		filename of file to read
 * @param buf			vector to write data to
 * @param bText 		if true, add terminating zero at end
 * @return
 */
bool ReadFile(const char* filename, vector<char> &file, bool bText);

bool GetAbsoluteFilename(const string &relativeFilename, string &absoluteFilename)
{
	if(FileExists(relativeFilename.c_str())==false) return false;
	absoluteFilename = relativeFilename;
	return true;
}


bool GetAbsoluteUGScriptFilename(const string &filename, string &absoluteFilename)
{
	PROFILE_FUNC();
	return PathProvider::get_filename_relative_to_current_path(filename, absoluteFilename)
			|| GetAbsoluteFilename(filename, absoluteFilename)
			|| PathProvider::get_filename_relative_to_path(SCRIPT_PATH, filename, absoluteFilename)
			|| PathProvider::get_filename_relative_to_path(APPS_PATH, filename, absoluteFilename)
			|| PathProvider::get_filename_relative_to_path(ROOT_PATH, filename, absoluteFilename);
}

bool LoadUGScript(const char *_filename, bool bDistributedLoad)
{
	PROFILE_FUNC();
	string filename=_filename;
	string absoluteFilename=filename;

	long status=0;
	BinaryBuffer buf;
	std::vector<char> file;

#ifdef UG_PARALLEL
	if(pcl::GetProcRank() == 0 || bDistributedLoad==false)
#endif
	{
		if(GetAbsoluteUGScriptFilename(filename, absoluteFilename) == false)
			status=PL_COULDNT_FIND;
		else if(ReadFile(absoluteFilename.c_str(), file, true) == false)
			status=PL_COULDNT_READ;
		else
			status = 0;
	}

#ifdef UG_PARALLEL
	if(bDistributedLoad)
	{
		if(pcl::GetProcRank() == 0)
		{
			Serialize(buf, status);
			if(status == 0)
			{
				Serialize(buf, absoluteFilename);
				Serialize(buf, file);
			}
		}

		pcl::ProcessCommunicator pc; // = MPI_COMM_WORLD
		pc.broadcast(buf);

		if(pcl::GetProcRank() != 0)
		{
			Deserialize(buf, status);
			if(status == 0)
			{
				Deserialize(buf, absoluteFilename);
				Deserialize(buf, file);
			}
		}
	}
#endif

	if(status == PL_COULDNT_FIND)
	{
		UG_LOG("Couldn't find script " << absoluteFilename << endl);
		return false;
	}
	else if(status == PL_COULDNT_READ)
	{
		UG_LOG("Couldn't read script " << absoluteFilename << endl);
		return false;
	}

	std::string curPath = PathFromFilename(absoluteFilename);
	PathProvider::push_current_path(curPath);
	bool success = ParseBuffer(&file[0], absoluteFilename.c_str());
	//bool success = ParseFile(absoluteFilename.c_str());
	PathProvider::pop_current_path();

	return success;
}


bool ReadFile(const char* filename, BinaryBuffer &buf, bool bText)
{
	PROFILE_FUNC();
	FILE *f = fopen(filename, bText ? "r" : "rb");
	if(f==NULL)	return false;
	fseek(f, 0, SEEK_END);
	long filesize=ftell(f);
	fseek(f, 0, SEEK_SET);

	long actualFilesize=filesize;
	if(bText) actualFilesize++;
	buf.reserve(actualFilesize);

	fread(buf.buffer(), 1, filesize, f);
	if(bText) buf.buffer()[filesize]=0x00;
	buf.set_write_pos(actualFilesize);
	return true;
}

bool ReadFile(const char* filename, vector<char> &file, bool bText)
{
	PROFILE_FUNC();
	FILE *f = fopen(filename, bText ? "r" : "rb");
	if(f==NULL)	return false;
	fseek(f, 0, SEEK_END);
	long filesize=ftell(f);
	fseek(f, 0, SEEK_SET);

	long actualFilesize=filesize;
	if(bText) actualFilesize++;
	file.resize(actualFilesize);

	fread(&file[0], 1, filesize, f);
	if(bText) file[filesize]=0x00;
	return true;
}


bool LoadUGScript_Parallel(const char* filename)
{
	return LoadUGScript(filename, true);
}
bool LoadUGScript_Single(const char* filename)
{
	return LoadUGScript(filename, false);
}

static ug::bridge::Registry* g_pRegistry = NULL;

static void UpdateScriptAfterRegistryChange(ug::bridge::Registry* pReg)
{
	PROFILE_FUNC();
	UG_ASSERT(pReg == g_pRegistry, "static g_pRegistry does not match parameter pReg, someone messed up the registries!");
	
//	this can be called since CreateBindings automatically avoids
//	double registration
	ug::bridge::lua::CreateBindings_LUA(GetDefaultLuaState(),
										*pReg);
}

static lua_State* theLuaState = NULL;
lua_State* GetDefaultLuaState()
{
//	if the state has not already been opened then do it now.
	if(!theLuaState)
	{
		PROFILE_BEGIN(CreateLUARegistry);
		if(!g_pRegistry){
		//	store a pointer to the registry and avoid multiple callback registration
			g_pRegistry = &ug::bridge::GetUGRegistry();
			g_pRegistry->add_callback(UpdateScriptAfterRegistryChange);

			g_pRegistry->add_function("ug_load_script", &LoadUGScript_Parallel, "/ug4/lua",
						"success", "", "ONLY IF ALL CORES INVOLVED! Loads and parses a script and returns whether it succeeded.");
			g_pRegistry->add_function("ug_load_script_single",
					&LoadUGScript_Single, "/ug4/lua",
						"success", "", "Loads and parses a script and returns whether it succeeded.");

			RegisterLuaDebug(*g_pRegistry);

		//	this define makes sure that no methods are referenced that
		//	use the algebra, even if no algebra is included.
			#ifdef UG_ALGEBRA
		//	Register info commands
			RegisterInfoCommands(*g_pRegistry);

		//	Register user functions
			RegisterLuaUserData(*g_pRegistry, "/ug4");

			#endif

			if(!g_pRegistry->check_consistency())
				throw(UGError("Script-Registry not ok."));

		}
		
	//	open a lua state
		theLuaState = lua_open();
	//	open standard libs
		luaL_openlibs(theLuaState);

	//	create lua bindings for registered functions and objects
		ug::bridge::lua::CreateBindings_LUA(theLuaState, *g_pRegistry);
		PROFILE_END();
	}
	
	return theLuaState;
}

/// calls lua_close, which calls delete for all lua objects
void ReleaseDefaultLuaState()
{
	if(theLuaState != NULL)
	{
		lua_close(theLuaState);
		theLuaState = NULL;
	}
	FinalizeLUADebug();
	return;
}


/// error function to be used for lua_pcall
int luaCallStackError( lua_State *L )
{
	//UG_LOG("Error: " << lua_tostring(L, -1) << ". ");
    //UG_LOG("call stack:\n"); ug::bridge::LuaStackTrace(L);
    return 1;
}

/**
 * Parses the content of buffer and executes it in the default lua state
 * @param buffer		the buffer to be executed
 * @param bufferName	name of the buffer (for error messages)
 * @return				true on success, otherwise throw(LuaError)
 */
bool ParseBuffer(const char* buffer, const char *bufferName)
{
	PROFILE_BEGIN(ParseBuffer);
	lua_State* L = GetDefaultLuaState();

	lua_pushcfunction(L, luaCallStackError);

	PROFILE_BEGIN(luaL_loadbuffer);
	int error = luaL_loadbuffer(L, buffer, strlen(buffer), bufferName);
	PROFILE_END();

	if(error == 0)
	{
		PROFILE_BEGIN(lua_pcall);
		error = lua_pcall(L, 0, 0, -2);
	}

	if(error)
	{
//		LOG("PARSE-ERROR: " << lua_tostring(L, -1) << endl);
		string msg = lua_tostring(L, -1);
		lua_pop(L, 1);
		//ug::bridge::LuaStackTrace(L);
		throw(LuaError(msg.c_str()));
//		return false;
	}

	PROFILE_END();
	return true;

}

/**
 * Parses the content of the file "filename" and executes it in the default lua state
 * \note don't use this function on all cores (i/o limited).
 * @param filename		the buffer to be executed
 * @return				true on success, otherwise throw(LuaError)
 */
bool ParseFile(const char* filename)
{
	lua_State* L = GetDefaultLuaState();

	lua_pushcfunction(L, luaCallStackError);

	PROFILE_BEGIN(luaL_loadfile);
	int error = luaL_loadfile(L, filename);
	PROFILE_END();

	if(error == 0)
	{
		PROFILE_BEGIN(lua_pcall);
		error = lua_pcall(L, 0, 0, -2);
		PROFILE_END();
	}

	if(error)
	{
		//LOG("PARSE-ERROR in parse_file(" << filename << "): " << lua_tostring(L, -1) << endl);
		string msg = lua_tostring(L, -1);
		lua_pop(L, 1);
		//ug::bridge::LuaStackTrace(L);
		throw(LuaError(msg.c_str()));
	}
	return true;
}

/// UGLuaPrint. Redirects LUA prints to UG_LOG
int UGLuaPrint(lua_State *L)
{
	int nArgs = lua_gettop(L);
	int i;
	lua_getglobal(L, "tostring");

	for(i=1; i<=nArgs; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);
		if(s) UG_LOG(s);
		lua_pop(L, 1);
	}
	UG_LOG(endl);
	lua_pop(L,1);
	return 0;
}

/// UGLuaWrite. Redirects LUA prints to UG_LOG without adding newline at the end
int UGLuaWrite(lua_State *L)
{
	PROFILE_FUNC();
	int nArgs = lua_gettop(L);
	int i;
	lua_getglobal(L, "tostring");

	for(i=1; i<=nArgs; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);
		if(s) UG_LOG(s);
		lua_pop(L, 1);
	}
	lua_pop(L,1);
	return 0;
}

}}//	end of namespace
