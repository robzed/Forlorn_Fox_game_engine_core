/*
 * glyph_set_utilities.h
 *
 *  Created on: 31 Jan 2014
 *      Author: tony
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2014 Tony Park and Rob Probin
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
 */

#ifndef GLYPH_SET_UTILS_H_
#define GLYPH_SET_UTILS_H_

#include "LuaMain.h"
#include "MyGraphics_render.h"
#include "LuaBridge.h"


bool save_glyph(SDL_Surface* glyph_set_surface, luabridge::LuaRef glyph_set, int glyph_number, luabridge::LuaRef glyph);
luabridge::LuaRef load_glyph(SDL_Surface* glyph_set_surface, luabridge::LuaRef glyph_set, int glyph, lua_State* L);
luabridge::LuaRef get_colour_format(SDL_Surface* glyph_set, lua_State* L);

SDL_Surface* load_glyph_file(luabridge::LuaRef glyph_set);
bool save_glyph_file(SDL_Surface* surface, luabridge::LuaRef glyph_set);

#endif /* GLYPH_SET_UTILS_H_ */
