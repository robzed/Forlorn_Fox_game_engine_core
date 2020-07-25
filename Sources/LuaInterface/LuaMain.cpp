/*
 *  LuaMain.cpp
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

#include "GameConfig.h"
#include "LuaMain.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "Utilities.h"
#include <iostream>
#include "luasocket.h"
#include "mime.h"
#include "lfs.h"

void stackDump (lua_State *L)
{
	int i;
	int top = lua_gettop(L);
	Utilities::debugMessage("%i Elements on stack\n", top);
	for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

			case LUA_TSTRING:  /* strings */
				Utilities::debugMessage("`%s'", lua_tostring(L, i));
				break;

			case LUA_TBOOLEAN:  /* booleans */
				Utilities::debugMessage(lua_toboolean(L, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:  /* numbers */
				Utilities::debugMessage("%g", lua_tonumber(L, i));
				break;

			default:  /* other values */
				Utilities::debugMessage("%s", lua_typename(L, t));
				break;

        }
        Utilities::debugMessage("  ");  /* put a separator */
	}
	Utilities::debugMessage("\n");  /* end the listing */
}


//
// Some of this copied from lua.c
//
// Also I found this after writing the below.. http://csl.name/lua/
//
LuaMain::LuaMain()
: L(0)
{
	L = luaL_newstate();  /* create state */
	if (L == NULL) {
		// Should this be a fatal error? or what?
		Utilities::fatalError("Forlorn Fox cannot create state: not enough memory");
		//l_message(argv[0], "cannot create state: not enough memory");
		//return EXIT_FAILURE;
	}

	//effectively option '-E'?
	lua_pushboolean(L, 1);  /* signal for libraries to ignore env. vars. */
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");

	// LuaCLI wants a guaranteed LuaState. We pass a reference in here.
	// This guaranteed is ensured by the NULL check above.
	LuaCLI = new LuaCommandLineInterpreter(*L);
	//register_core_functions();

}

LuaMain::~LuaMain()
{
	delete LuaCLI;
	lua_close(L);
}

void LuaMain::library_init()
{
	/* call 'set_up_standard_libraries' in protected mode */
	lua_pushcfunction(L, &set_up_standard_libraries);
	int status = docall(0, 0, true, "standard libraries");
	fatal_if_lua_errror(status, " on library load");

	// load the gulp library table
	//get_gulp_table_onto_stack();
	//lua_pop(L, 1);	// drop the table

	open_socket_lib();
	open_mime_lib();
	open_lsqlite3_lib();

    luaL_requiref(L, "lfs", luaopen_lfs, 1);
    lua_pop(L, 1);	// drop the module
}

void LuaMain::open_socket_lib()
{
	luaL_requiref(L, "socket.core", luaopen_socket_core, 1);
	lua_pop(L, 1);	// drop the module
	//run_lua_file(LoadPath("scripts/socket.lua").c_str());
}

void LuaMain::open_mime_lib()
{
	luaL_requiref(L, "mime.core", luaopen_mime_core, 1);
	lua_pop(L, 1);	// drop the module
	//run_lua_file(LoadPath("scripts/mime.lua").c_str());
}

extern int luaopen_lsqlite3(lua_State *L);
void LuaMain::open_lsqlite3_lib()
{
	luaL_requiref(L, "lsqlite3", luaopen_lsqlite3, 1);
	lua_pop(L, 1);	// drop the module
}



void LuaMain::return_namespace_table(const std::string name)
{
	// similar to luax_insistglobal in love 2d
	lua_getglobal(L, name.c_str());

	if (not lua_istable(L, -1))
	{
		lua_pop(L, 1); // Pop the non-table.
		lua_newtable(L);
		lua_pushvalue(L, -1);		// dup :-)
		lua_setglobal(L, name.c_str());
	}

}

int LuaMain::set_up_standard_libraries(lua_State *L)
{
	/* open standard libraries */
	luaL_checkversion(L);
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	// we don't call the init file (as per the normal interpreter)

	return 0; // no Lua results
}



//static lua_State *globalL = NULL;
//
//
//static void lstop (lua_State *L, lua_Debug *ar) {
//	(void)ar;  /* unused arg. */
//	lua_sethook(L, NULL, 0, 0);
//	luaL_error(L, "interrupted!");
//}
//
//
//static void laction (int i) {
//	signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
//						 terminate process (default action) */
//	lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
//}


int LuaMain::traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else if (!lua_isnoneornil(L, 1)) {  /* is there an error object? */
		if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
			lua_pushliteral(L, "(no error message)");
	}
	return 1;
}


int LuaMain::docall (lua_State *L, int narg, int nres) {
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */
	//	globalL = L;  /* to be available to 'laction' */
	//	signal(SIGINT, laction);
	status = lua_pcall(L, narg, nres, base);
	//	signal(SIGINT, SIG_DFL);
	//stackDump(L);
	lua_remove(L, base);  /* remove traceback function */
	return status;
}


void LuaMain::open_console()
{
	//std::cout << "Console Opened2" << std::endl;
	threaded_stdin.start();
	LuaCLI->print_version();
	LuaCLI->print_prompt();
}

void LuaMain::process_console()
{
	if(threaded_stdin.is_line_maybe_ready())
	{
		std::string line = threaded_stdin.get_line();
		if(line != "")
		{
			// all lines have '\n' appended to them
			//Utilities::debugMessage("KEYS: %s", line.c_str());
			LuaCLI->process_line(line);
			LuaCLI->print_prompt();
		}
	}
}

int LuaMain::do_string(const std::string& str, bool print_errors, const std::string name)
{
	int status = luaL_loadbuffer(L, str.c_str(), str.length(), name.c_str());
	if (status == LUA_OK)
	{
		status = docall(0, 0, print_errors, name.c_str());
	}
	return status;
}


int LuaMain::docall(int narg, int nres, bool print_errors, const char* name)
{
	int status = docall(L, narg, nres);
	fetch_error_string(status, true);
	if(print_errors) { print_decode_lua_error(status, std::string("in ")+name); }
	return status;
}

int LuaMain::run_lua_file(const char* filename, bool print_errors, bool ignore_file_missing_errors)//, int nargs, int nres)
{
	// luaL_dofile
	// int luaL_dofile (lua_State *L, const char *filename);
	// Loads and runs the given file. It is defined as the following macro:
	//
	// (luaL_loadfile(L, filename) || lua_pcall(L, 0, LUA_MULTRET, 0))
	// It returns false if there are no errors or true in case of errors.
	//return luaL_dofile(L, filename);

	int err = luaL_loadfile(L, filename);
	if(err == LUA_OK)
	{
		//Utilities::debugMessage("Loaded lua file okay %s", filename);

		err = docall(0, 0, print_errors, filename);    // no results expected
		//err = lua_pcall(L, 0, 0, 0);				// no results expected
		//err = lua_pcall(L, 0, LUA_MULTRET, 0);
		/*if(err != LUA_OK and print_errors)
		{
			std::string msg("when running ");
			msg += filename;
			fetch_error_string(err);
			print_decode_lua_error(err, msg);
		}*/
	}
	else
	{
		if(print_errors)
		{
			if(err != LUA_ERRFILE or not ignore_file_missing_errors)
			{
				std::string msg("when loading ");
				msg += filename;
				fetch_error_string(err, false);
				print_decode_lua_error(err, msg);
			}
		}
	}

	return err;
}

void LuaMain::print_decode_lua_error(int error_code, std::string doing_what)
{
	if(error_code != LUA_OK)
	{
		//Utilities::fatalError("Lua error: ");
		Utilities::fatalError("Lua error: %s ", last_error_str.c_str());
	}
}


void LuaMain::fatal_if_lua_errror(int status, std::string additional_text)
{
	if(status != LUA_OK)
	{
		Utilities::fatalError(last_error_str + additional_text);
	}
}


// copied from report() and finalreport() in lua.c
void LuaMain::fetch_error_string(int err, bool stack_trace)
{
	if (err != LUA_OK)
	{
		const char *msg = (lua_type(L, -1) == LUA_TSTRING) ? lua_tostring(L, -1)
		: NULL;
		if (msg == NULL) msg = "(error object is not a string)";
		last_error_str = msg;
		last_error_int = err;

		// get rid of string off stack
		lua_pop(L, 1);

		/* force a complete garbage collection in case of errors */
		lua_gc(L, LUA_GCCOLLECT, 0);
	}
}



//void LuaMain::register_core_functions()
//{
	// to set up a library
	// http://www.lua.org/pil/26.2.html
//
//
//}

int LuaMain::get_last_error()
{
	return last_error_int;
}


LuaCommandLineInterpreter* LuaMain::get_CLI()
{
    return LuaCLI;
}

