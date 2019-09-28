/*
 *  MapUtils.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 13/07/2014.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2014 Rob Probin and Tony Park
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

#ifndef MAP_UTILS_H
#define MAP_UTILS_H

class PresentationMaze;

#include <vector>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "LuaBridge.h"

namespace MapUtils {

	class MapFinder {
	public:
		MapFinder (PresentationMaze* m);
		void add_target(int unicode_value);
		bool find_anywhere();		// can do a GetItem on PresentationMaze instance to find out what...

		// these might be useful for looking for stuff?
		//find_down();
		//find_up();
		//find_left();
		//find_right();
		int line;
		int column;
	private:
		PresentationMaze* maze;
		typedef std::vector<int> tofind_t;
		tofind_t to_find;	// could be a set or a map
	};


	void map_transform(PresentationMaze* maze, luabridge::LuaRef char_code_selector_table, luabridge::LuaRef character_code_matcher_table, luabridge::LuaRef wall_glyph_lookup_table);

};

#endif
