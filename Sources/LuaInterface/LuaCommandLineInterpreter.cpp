/*
 *  LuaCommandLineInterpreter.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 08/12/2013.
 *  Copyright 2013 Rob Probin. All rights reserved.
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

#include "LuaCommandLineInterpreter.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

//#define lua_c

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "LuaMain.h"

//
// Some of this copied from lua.c
//
#if !defined(LUA_PROMPT)
#define LUA_PROMPT		"> "
#define LUA_PROMPT2		">> "
#endif


LuaCommandLineInterpreter::LuaCommandLineInterpreter(lua_State& lua_state)
: L(&lua_state)
, firstline(true)
, progname(0)		// no program name, not needed during interpreter (not used by dotty)
{
}

LuaCommandLineInterpreter::~LuaCommandLineInterpreter()
{
}

int LuaCommandLineInterpreter::report(int status) 
{
	if (status != LUA_OK && !lua_isnil(L, -1))
	{
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error object is not a string)";
		l_message(progname, msg);
		lua_pop(L, 1);
		/* force a complete garbage collection in case of errors */
		lua_gc(L, LUA_GCCOLLECT, 0);
	}
	return status;
}




void LuaCommandLineInterpreter::l_message (const char *pname, const char *msg)
{
	if (pname) luai_writestringerror("%s: ", pname);
	luai_writestringerror("%s\n", msg);
}



//void LuaCommandLineInterpreter::remove_newline(std::string& line)
//{
//	if(line.length() > 0 and *line.rbegin() == '\n')
//	{
//		line.resize(line.size() - 1);
//	}
//}

void LuaCommandLineInterpreter::ensure_ends_with_newline(std::string& line)
{
	if(line.length() > 0 and *line.rbegin() != '\n')
	{
		line += '\n';
	}
}

void LuaCommandLineInterpreter::transform_equals_at_start(std::string& line)
{
	/* first line starts with `=' ? */
	if (line.length() > 0 and line[0] == '=')  
	{
		line = std::string("return ") + line.substr(1);
	}
}


/* the next function is called unprotected, so it must avoid errors */
//static void finalreport (lua_State *L, int status) {
//	if (status != LUA_OK) {
//		const char *msg = (lua_type(L, -1) == LUA_TSTRING) ? lua_tostring(L, -1)
//		: NULL;
//		if (msg == NULL) msg = "(error object is not a string)";
//		l_message(progname, msg);
//		lua_pop(L, 1);
//	}
//}

///* mark in error messages for incomplete statements */
#define EOFMARK "<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)
bool LuaCommandLineInterpreter::incomplete(int status)
{
	if (status == LUA_ERRSYNTAX) {
		size_t lmsg;
		const char *msg = lua_tolstring(L, -1, &lmsg);
		if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
			lua_pop(L, 1);
			return 1;
		}
	}
	return 0;  /* else... */
}

//
// Does this need to be called protected (e.g. for lots of tolstring)???
//
void LuaCommandLineInterpreter::process_line(const std::string& line_in)
{
	std::string line;
	//remove_newline(ln);
	if (firstline)
	{
		line = line_in;
		transform_equals_at_start(line);
	}
	else
	{
		line = incomplete_buffer + line_in;
	}
	// ensure stack is empty
	lua_settop(L, 0);
	int status = luaL_loadbuffer(L, line.c_str(), line.length(), "=stdin");
	if(incomplete(status))
	{
		ensure_ends_with_newline(line);
		incomplete_buffer = line;
		firstline = false;
	}
	else
	{
		// we've got a complete line...
		if (status == LUA_OK) { status = LuaMain::docall(L, 0, LUA_MULTRET); }
		report(status);
		if (status == LUA_OK && lua_gettop(L) > 0) {  /* any result to print? */
			luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
			lua_getglobal(L, "print");
			lua_insert(L, 1);
			if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != LUA_OK)
			{
				l_message(progname, lua_pushfstring(L,
													"error calling " LUA_QL("print") " (%s)",
													lua_tostring(L, -1)));
//				std::cerr << "error calling " LUA_QL("print") " (" << lua_tostring(L, -1) << ")\n";
			}
		}
		
		// clean up
		lua_settop(L, 0);  /* clear stack */
		incomplete_buffer = "";
		firstline = true;
	}
	
}

void LuaCommandLineInterpreter::print_version()
{
	std::cout << LUA_COPYRIGHT << std::endl;
}


void LuaCommandLineInterpreter::print_prompt()
{
	const char *p = NULL;
	if(L)
	{
		lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2");
		p = /*(lua_type(L, -1) == LUA_TSTRING) ?*/ lua_tostring(L, -1) /*: NULL*/;
	}
	if (p == NULL) p = (firstline ? LUA_PROMPT : LUA_PROMPT2);
	
	std::cout << p << std::flush;
	lua_pop(L, 1);	// drop _PROMPT
}

