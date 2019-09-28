/*
 *  LuaCommandLineInterpreter.h
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

#ifndef LUA_COMMAND_LINE_INTERPRETER_H
#define LUA_COMMAND_LINE_INTERPRETER_H

#include "lua.h"
#include <string>

class LuaCommandLineInterpreter {
public:
	LuaCommandLineInterpreter(lua_State& lua_state);
	~LuaCommandLineInterpreter();
	void process_line(const std::string& line);

	// support functions
	void print_version();
	void print_prompt();
private:
	// member functions
	void transform_equals_at_start(std::string& line);
	//void remove_newline(std::string& line);
	void ensure_ends_with_newline(std::string& line);
	bool incomplete(int status);
	static void l_message(const char *pname, const char *msg);
	int report(int status);
	
	// data
	lua_State* L;		// we don't own this!
	bool firstline;
	std::string incomplete_buffer;
	const char *progname;
};



#endif

