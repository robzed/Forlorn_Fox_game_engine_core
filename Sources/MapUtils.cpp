/*
 *  MapUtils.cpp
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

#include "MapUtils.h"
#include "PresentationMaze.h"
#include <iostream>

MapUtils::MapFinder::MapFinder (PresentationMaze* m)
: line(0)
, column(-1)	// account for pre-increment
, maze(m)
{
}

void MapUtils::MapFinder::add_target(int unicode_value)
{
	to_find.push_back(unicode_value);
}

bool MapUtils::MapFinder::find_anywhere()
{
	while(true)
	{
		// increment column whether we find the item or not
		// prevents an infinte loop in lua if we don't alter
		// the mapping for the character, and keep finding it
		// again and again!
		column++;

		if(column >= maze->width())
		{
			column = 0;
			line++;
		}
		if(line >= maze->height())
		{
			return false;
			break;
		}

		int item = maze->get_glyph(line, column);

		for (tofind_t::iterator it = to_find.begin() ; it != to_find.end(); ++it)
		{
			if(item == *it)
			{
				return true;
				break;
			}
		}
	}

}


// calculate_wall_glyph() - this checks surrounding blocks against a set of criteria.
// And returns one of 16 blocks based on this. Nil in the lookup table means
// don't modify.
static int calculate_wall_direction_map(PresentationMaze* maze, int line, int column, luabridge::LuaRef character_code_matcher_table)
{
	unsigned int direction_map = 0;

	if(line <= 0 || character_code_matcher_table[maze->get_glyph(line-1, column)] )
	{
		// set north bit
		direction_map += NORTH;
	}

	if(line >= maze->height()-1 || character_code_matcher_table[maze->get_glyph(line+1, column)] )
	{
		// set south bit
		direction_map += SOUTH;
	}

	if(column <= 0 || character_code_matcher_table[maze->get_glyph(line, column-1)] )
	{
		// set west bit
		direction_map += WEST;
	}
 	if(column >= maze->width()-1 || character_code_matcher_table[maze->get_glyph(line, column+1)] )
	{
		// set east bit
 		direction_map += EAST;
	}

 	return direction_map;
}

// map_transform() - transform a specific type of block based on the surrounding blocks.
//
//
void MapUtils::map_transform(PresentationMaze* maze, luabridge::LuaRef char_code_selector_table, luabridge::LuaRef character_code_matcher_table, luabridge::LuaRef wall_glyph_lookup_table)
{
	for(int line = 0; line < maze->height(); line++)
	{
		for(int column = 0; column < maze->width(); column++)
		{
			int m = maze->get_glyph(line, column);

			// if this unicode matches any of the criteria we are looking for... 
			luabridge::LuaRef lookup_index = char_code_selector_table[m];

			if(!(lookup_index == 0) && !lookup_index.isNil())		// LuaRef does not implement operator !=
			{
				// check the surrounding blocks
				unsigned int direction_map = calculate_wall_direction_map(maze, line, column, character_code_matcher_table);

				//std::cout << lookup_index << ", " << direction_map+1 << std::endl;

				// the following double indexing didn't work, so deref one at a time...
				//luabridge::LuaRef mref = wall_glyph_lookup_table[lookup_index][direction_map+1];

				luabridge::LuaRef temp = wall_glyph_lookup_table[lookup_index];
				luabridge::LuaRef mref = temp[direction_map+1];				// add 1 because we are accessing a lua table

				if(mref.isNumber())	// don't convert to int if it's not a number!
				{
					maze->set_glyph(line, column, mref);

					// set up transparency in various directions for the wall
					maze->set_wall_transparency(line, column, direction_map);
				}
			}
			else
			{
				// make all space like characters transparent
				maze->set_wall_transparency(line, column, 0);

				// update the view layer for everything that is walkable to 0
				maze->update_layer(line, column, 0);
			}

		}
	}
}


