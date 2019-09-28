/*
 *  LuaCppInteface.h
 *  This module provides the patching between Lua and the C++ Engine
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
#ifndef LUA_CPP_INTERFACE_H
#define LUA_CPP_INTERFACE_H

#include "lua.h"
#include <string>
#include "LuaMain.h"

class GameApplication;

// These don't need to be a class, and a namespace seems bit overegging it.
// Functions are therefore fine.
void set_up_basic_ff_libraries(LuaMain*l);
void set_up_ui_ff_libraries(LuaMain*l, GameApplication& app);

void load_main_file(LuaMain* l, const char* file_name);
void load_conf_file(LuaMain* l);
void load_file_vargs_ret(LuaMain* l, const char* file_name, int nargs, int nres);

// returns true if function was called
int run_gulp_function_if_exists(LuaMain* l, const char* function_name, int narg=0, int nres=0);
int run_method_in_gulp_object_if_exists(LuaMain* l, const char* gulp_subtable, const char* function_name, int narg=0, int nres=0);

#define LUA_FUNCTION_NOT_CALLED -12345

// for glyph editor
//void create_and_return_glyph_table(lua_State *L, unsigned int* glyph, int glyph_size);
//void decode_and_drop_glyph_table(lua_State *L, unsigned int* glyph, int glyph_size);


#endif
