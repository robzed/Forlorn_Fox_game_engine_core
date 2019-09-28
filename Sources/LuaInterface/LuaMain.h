/*
 *  LuaMain.h
 *  This module creates a single Lua interpreter.
 *
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 25/11/2013.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2011-2013 Rob Probin and Tony Park
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 * -----------------------------------------------------------------------------
 * (This is the zlib License)
 *
 */

#ifndef LUA_MAIN_H
#define LUA_MAIN_H

#include "GameConfig.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "LuaBridge.h"

#include <string>
#include "StdinThread.h"
#include "LuaCommandLineInterpreter.h"

class GameApplication;

// Ironically this ended up as half-app specific and half generic LuaState.
// Some of the LuaState stuff is done by LuaComandLineInterface
// But ignore that for now, it does the job.
class LuaMain {
public:
	LuaMain();
	~LuaMain();

	void library_init();
	void open_socket_lib();
	void open_mime_lib();

    int run_lua_file(const char* filename, bool print_errors=true, bool ignore_file_missing_errors=false); // , int nargs=0, int nres=0);
	int do_string(const std::string& str, bool print_errors, const std::string name);
	void open_console();
	void process_console();

	// support functions
	static int docall(lua_State *L, int narg, int nres);
	int docall(int narg=0, int nres=0, bool print_errors=true, const char* name="?");

	void return_namespace_table(const std::string name);
	int get_last_error();

	// implicit conversion case
	operator lua_State*() { return L; }
    lua_State* get_internal_state() { return L; }
    LuaCommandLineInterpreter* get_CLI();
    void fatal_if_lua_errror(int status, std::string additional_text);
private:
	// disable copy and assignment constructors for the moment
	// from comments http://csl.name/lua/
	LuaMain(LuaMain const&); // =delete
	LuaMain& operator=(LuaMain const&);	//=delete
//	LuaMain(Lua_State&&) = default;

	static int set_up_standard_libraries(lua_State *L);
	void register_core_functions();
	void fetch_error_string(int err, bool stack_trace);

	void print_decode_lua_error(int error_code, std::string doing_what);

	static int traceback(lua_State *L);

	// data
	lua_State *L;

	// error stuff
	std::string last_error_str;
	int last_error_int;

	//
	StdinThread threaded_stdin;
	LuaCommandLineInterpreter* LuaCLI;
};

void stackDump (lua_State *L);

#endif
