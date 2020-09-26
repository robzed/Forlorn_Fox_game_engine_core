/*
 *  PresentationMaze.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 19/07/2011.
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

#include "PresentationMaze.h"
#include <stdio.h>
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include <stdlib.h>
#include <map>
#include <iostream>
#include "Utilities.h"
#include "MyGraphics_render.h"	// for glyph set definitions

#include "lua.h"
#include "lauxlib.h"
#include "LuaBridge.h"
#include "Utilities.h"

#include "Clickable.h"

#include <cmath>

const auto hex_horizontal_offset_ratio = 0.75;
const auto hex_vertical_offset_ratio = std::sin((60 / 180.0) * ((double) M_PI));   // ~0.866
const auto square_horizontal_offset_ratio = 1.0;
const auto square_vertical_offset_ratio = 1.0;


PresentationMaze::PresentationMaze(double min_glyphs_horizontally, double min_glyphs_vertically)
: mobs_draw_list(this)
, current_line_max(-1)
, current_column_max(-1)
, offset_line(0)
, offset_column(0)
, available_level_width(min_glyphs_horizontally)
, available_level_height(min_glyphs_vertically)
, ideal_available_level_width(min_glyphs_horizontally)
, ideal_available_level_height(min_glyphs_vertically)
, clickable(false)
, mDown(false)
, click_callback(gulp_cpp->get_ui_lua_state())
, drag_callback(gulp_cpp->get_ui_lua_state())
, click_self(gulp_cpp->get_ui_lua_state())
, mHexRendering(false)
, mHorizontalOffsetRatio(square_horizontal_offset_ratio)
, mVerticalOffsetRatio(square_vertical_offset_ratio)
{
	//std::cout << "Constructing PresentationMaze " << this << std::endl;

	set_default_maze_colours(BRIGHT_WHITE, zx_spectrum_black);

	for(int line = 0; line < MazeConstants::maze_height_max; line++)
	{
		for(int column = 0; column < MazeConstants::maze_width_max; column++)
		{
			view_layer[line][column] = MazeConstants::top_layer;
			current_maze_element_pointers[line][column] = 0;
			transparency[line][column].SetTransparent();
		}
	}

	set_rect(50,50, 1000-50, 600-50);
}

PresentationMaze::~PresentationMaze()
{
	delete_all_cmep();
	//std::cout << "Destroying PresentationMaze " <<this << std::endl;

	gulp_cpp->remove_clickable_target(this);
}


void PresentationMaze::delete_all_cmep()
{
	// We need to keep this array in c++, because the base map elements
	// belong to c++... but some other elements in the maze draw lists
	// may belong to lua. Therefore, we can't just delete everything in
	// in the maze draw lists, because we'd trip over the lua garbage
	// collection. We need to just delete the things that belong to
	// us, and not the things that belong to lua, so we keep the things
	// we created in a big array.
	for(int line=0; line<MazeConstants::maze_height_max; line++)
	{
		for(int column=0; column<MazeConstants::maze_height_max; column++)
		{
			delete current_maze_element_pointers[line][column];
			current_maze_element_pointers[line][column] = 0;
		}
	}
}

double PresentationMaze::GetAvailableLevelWidth()
{
	// is it correct to return a divided result here?  NO

	return available_level_width;
}

double PresentationMaze::GetAvailableLevelHeight()
{
	return available_level_height;
}

int PresentationMaze::GetCellSize()
{
	return viewport.cell_size;
}

void PresentationMaze::load_current_maze(lua_State* L)
{
	// first clear up all the old maze data
	delete_all_cmep();

	// tests for parameter error, if so, raise error.
	luaL_checktype(L, -1, LUA_TTABLE);

	current_line_max = luaL_len(L, -1) - 1;
	// cap the number of lines
	if(current_line_max >= MazeConstants::maze_height_max)
	{
		current_line_max = MazeConstants::maze_height_max-1;
	}
	if(current_line_max < 0)	// can only be -1
	{
		current_column_max = -1;
		current_line_max = -1;
		return;
	}

	// now load the data
	for(int line = 0; line <= current_line_max; line++)
	{
		lua_pushinteger(L, line+1);	// could use lua_next but we have loop already
		lua_gettable(L, -2);		// result is a line table
		if(lua_type(L, -1) != LUA_TTABLE)
		{
			// less proper lines if one is not a table
			current_line_max = line-1;
			return;
		}

		// width set by length of first line..
		current_column_max = luaL_len(L, -1) - 1;
		// cap the number of columns
		if(current_column_max >= MazeConstants::maze_width_max)
		{
			current_column_max = MazeConstants::maze_width_max-1;
		}
		for(int column = 0; column <= current_column_max; column++)
		{
			lua_pushinteger(L,  column+1);
			lua_gettable(L, -2);

            // If this is a table then it's an animated glyph list
            if(lua_type(L, -1) == LUA_TTABLE)
            {
                // Create the element
                MazeDrawListAnimatedElement* element = new MazeDrawListAnimatedElement(get_maze_draw_list(line, column), 0);
                if (!element)
                {
                    // Error failed to create object
                    return;
                }
                
                lua_pushnil(L); // push nil on to the stack
            
                bool first = true; // First entry is the interval
                
                // For all entries in the table load the glyphs
                while(lua_next(L, -2 ))
                {
                    // Check the read item is a number
                    if(lua_isnumber(L, -1 ))
                    {
                        // First item in the table is the interval
                        if (true == first)
                        {
                            element->setInterval(luaL_optint(L, -1, ' '));
                            first = false;
                        }
                        else
                        {
                            element->add_glyph(luaL_optint(L, -1, ' '));
                        }
                    }
                    
                    // Finished with number for pop
                    lua_pop(L, 1);
                }
                
                // All loaded so store on to the current_maze_element_pointers
                current_maze_element_pointers[line][column] = element;
            }
            else
            {
                // If the lines aren't big enough they get padded with Unicode space.
                // If the table doesn't contain nil or integer at that position, will raise error.
                // Maybe should it convert it?
            	int glyph = luaL_optint(L, -1, ' ');
            	if(glyph != 0x20)
            	{
            		current_maze_element_pointers[line][column] = new MazeDrawListElement(get_maze_draw_list(line, column), glyph, glyph==0x20?0:100);
            	}
            }

			lua_pop(L, 1);	// drop the integer, we've used it
		}

		lua_pop(L, 1);	// drop the line table, we've used it
	}

}

int PresentationMaze::calculate_cell_size()
{

	// calculate the cell_size for the viewport
	// based on the minimum lines / columns, calculate the glyph size
	int cell_size = 1000000;	// very large

	if (viewport.rect.h / ideal_available_level_height < cell_size)
	{
		cell_size = viewport.rect.h / ideal_available_level_height;
	}

	if (viewport.rect.w / ideal_available_level_width < cell_size)
	{
		cell_size = viewport.rect.w / ideal_available_level_width;
	}

	if(mHexRendering) cell_size /= hex_vertical_offset_ratio;

	// limit the maximum zoom
	if (cell_size >= 1000000)
	{
		// make sure we change cell size at least a wee bit
		cell_size = viewport.cell_size + 2;		// 2 to account for correction below
		ideal_available_level_height = (double)viewport.rect.h / (double)cell_size;
		ideal_available_level_width = (double)viewport.rect.w / (double)cell_size;
	}

	if (cell_size % 2) cell_size--;

	// limit the minimum zoom
	if (cell_size < cell_size_lower_limit)
	{
		// limit how small the zoom can go
		cell_size = cell_size_lower_limit;
	}

	return cell_size;
}

void PresentationMaze::update_viewport_and_dimensions()
{
	//Utilities::debugMessage("old available_level_height %f", available_level_height);

	// calculate available level width to provide lua with this information about the maze
	available_level_width = (double)viewport.rect.w / (double)viewport.cell_size;
	available_level_height = (double)viewport.rect.h / (double)viewport.cell_size;

	//Utilities::debugMessage("available_level_height %f", available_level_height);

	// set the mobs draw list viewport to match the maze's viewport
	mobs_draw_list.set_viewport(viewport);
}

void PresentationMaze::set_rect(int left, int top, int right, int bottom)
{
	// don't take the piss
	if (top >= bottom || left >= right) return;

	// set up the rect for the maze render
	viewport.rect.x = left;
	viewport.rect.y = top;
	viewport.rect.w = right-left;
	viewport.rect.h = bottom-top;

	viewport.cell_size = calculate_cell_size();
	update_viewport_and_dimensions();
}

void PresentationMaze::zoom(bool zoom_in)
{
	double zoom_factor = 1.125;
	int cell_size = viewport.cell_size;
	if(cell_size <= cell_size_lower_limit and not zoom_in)
	{
		viewport.cell_size = cell_size_lower_limit;
		return;
	}

	while(cell_size == viewport.cell_size)
	{
		ideal_available_level_height = ideal_available_level_height * (zoom_in ? (1/zoom_factor) : zoom_factor);
		ideal_available_level_width  = ideal_available_level_width  * (zoom_in ? (1/zoom_factor) : zoom_factor);
		cell_size = calculate_cell_size();
	}

	viewport.cell_size = cell_size;

	update_viewport_and_dimensions();
}

void PresentationMaze::set_click_callback(luabridge::LuaRef callback, luabridge::LuaRef self)
{
	clickable = callback.isFunction();

	if(clickable)
	{
	    callback.force_to_main_state();
		gulp_cpp->add_clickable_target(this);
	}
	else
	{
		gulp_cpp->remove_clickable_target(this);
	}

	// set the callback...!
	click_callback = callback;
	click_self = self;

}


bool PresentationMaze::check_for_click(int x, int y, bool down, bool drag)
{
	// x and y are windows coordinates, check if they are within our rectangle
	if(x < viewport.rect.x) return false;
	if(y < viewport.rect.y) return false;
	if(x > viewport.rect.x+viewport.rect.w) return false;
	if(y > viewport.rect.y+viewport.rect.h) return false;

	// could add 'mouseover' support (e.g. for tooltips) here
	// but for now ignore drag if button is not down
	if(not down) drag = false;

	// click happens when button is released
	bool click = mDown and not down;

	// store for next time
	mDown = down;

	// adjust x and y to our rect
	x -= viewport.rect.x;
	y -= viewport.rect.y;

	pos_t cx = (pos_t)x / viewport.cell_size;
	pos_t cy = (pos_t)y / viewport.cell_size;

	// action the click
	bool consumed = false;

	if(drag && drag_callback.isFunction())
	{
        consumed = drag_callback(click_self, cx, cy);
	}

	if(click && click_callback.isFunction())
	{
        consumed = click_callback(click_self, cx, cy);
    }

	return consumed;
}


void PresentationMaze::check(int line, int column)
{
	if(column < 0 || column > current_column_max)
	{
		Utilities::debugMessage("Column out of range - actual=%d max=%d", column, current_column_max);
		Utilities::fatalError("PresentationMaze::check", 42100);
	}
	if(line < 0 || line > current_line_max)
	{
		Utilities::debugMessage("Line out of range - actual=%d max=%d", line, current_line_max);
		Utilities::fatalError("PresentationMaze::check", 42101);
	}
}

void PresentationMaze::print(MyGraphics* gr)
{
	const int more_than_maze_lines = 1000;
	print_selected(gr, 0, more_than_maze_lines);
}

void PresentationMaze::print_selected(MyGraphics* gr, int start_line, int lines_to_print)
{
	// hack for escape - tiles are 10 cells wide, and only contained in every 10th
	// element of the presentation maze, so need to offset printing by tile size
	// in order to ensure we print partial tiles to top and left of screen
	// in other games this would be 1
	// access to set this from lua would be good - or it might be possible to infer it,
	// or even better the tiles in Escape should be 1x1 in the presentation maze structure,
	// but still 10x10 otherwise
	int tile_size = 1;

	gr->set_viewport(viewport);

	int integer_part_of_offset_line = (int)offset_line;
	pos_t fractional_part_of_offset_line = offset_line - (int)offset_line;

	// tile_size hack for escape
	pos_t line = 0 - fractional_part_of_offset_line - tile_size;

	// int cast in the following line is REQUIRED to avoid errors where offset_line is close to zero,
	// even though all the other values, and the result, are ints...!
	int map_start_line = start_line + integer_part_of_offset_line - tile_size;

	if(map_start_line > current_line_max)
	{
		return;
	}
	if(map_start_line < 0)
	{
		if((map_start_line + lines_to_print) < 0)
		{
			return;
		}

		line = line - map_start_line;
		map_start_line = 0;
		start_line = 0;
	}

	line += start_line;

	// only print lines on the map
	if((lines_to_print + map_start_line) > current_line_max+1)
	{
		lines_to_print = current_line_max - map_start_line + 1;
	}

	double available_height = GetAvailableLevelHeight();
	if(mHexRendering) available_height /= mVerticalOffsetRatio;

	pos_t half_vertical_cell = mVerticalOffsetRatio / 2;

	while(lines_to_print and line < available_height and map_start_line <= current_line_max and map_start_line >= 0)
	{

		// Calculate the left hand edge of the screen
        // Remember offset column refers to cell to 0 that is inset in to screen as per old Spectrum

		// tile_size hack for escape
		int map_start_column = (int)offset_column - tile_size;

        // For non widescreen this is simply the fractonal part of the scrolling.
        // For widescreen we need to take account that the screen start is negative
        pos_t column = -offset_column + (int)offset_column - tile_size;

        // Cannot negative map coordinates
        if (map_start_column < 0)
        {
            column -= map_start_column;
            map_start_column = 0;
        }

        while(map_start_column <= current_column_max and map_start_column >= 0)
		{
        	render_map_data(*gr, map_start_line, map_start_column, line+((mHexRendering && (map_start_column % 2)) ? half_vertical_cell : 0), column, 0, view_layer[map_start_line][map_start_column]);
            column += mHorizontalOffsetRatio;
            map_start_column ++;
        }
 		
        line += mVerticalOffsetRatio;
		map_start_line ++;
		lines_to_print--;
	}

	mobs_draw_list.render_complex(gr, offset_line, offset_column, this, &view_layer);

}

DrawList* PresentationMaze::get_mobs_draw_list()
{
	return &mobs_draw_list;
}

void PresentationMaze::set_render_option(int ro_in)
{
	render_option ro = static_cast<render_option>(ro_in);

	if(ro == do_not_render_empty_draw_list || ro == render_empty_draw_list_as_space)
	{
		for(int line = 0; line < MazeConstants::maze_height_max; line++)
		{
			for(int column = 0; column < MazeConstants::maze_width_max; column++)
			{
				maze_draw_list[line][column].set_render_option(ro);
			}
		}
	}
	else if(ro == hex_rendering)
	{
		mHexRendering = true;
		mHorizontalOffsetRatio = hex_horizontal_offset_ratio;
		mVerticalOffsetRatio = hex_vertical_offset_ratio;
	}
	else if(ro == square_rendering)
	{
		mHexRendering = false;
		mHorizontalOffsetRatio = square_horizontal_offset_ratio;
		mVerticalOffsetRatio = square_vertical_offset_ratio;
	}

}

void PresentationMaze::render_map_data(MyGraphics& gr, int map_line, int map_column, pos_t screen_line, pos_t screen_column, int start_layer, int end_layer, bool overdraw)
{
	gr.set_fg_colour(maze_foreground[map_line][map_column]);
	gr.set_bg_fullcolour(maze_background[map_line][map_column]);


	// Need overdraw for hex tiles to work
	// and for some reason as yet unknown, overdraw breaks the older maps
	// do for now, pass mHexRendering in as the overdraw flag
	(maze_draw_list[map_line][map_column]).render(gr, screen_line, screen_column, start_layer, end_layer, overdraw || mHexRendering);
}


void PresentationMaze::set_view_layer(int player, int line, int column, int layer)
{
	if(player != 0)
	{
		Utilities::debugMessage("Multiplayer not supported in set_view_layer()");
		Utilities::fatalError("Death has stalked your program once again", 42200);
	}

	view_layer[line][column] = layer;
}


int PresentationMaze::get_glyph(int line, int column)
{
	check(line, column);
	if(current_maze_element_pointers[line][column] == 0)
	{
		return 0x20;	// a space
	}

	return current_maze_element_pointers[line][column]->get_glyph();
}

void PresentationMaze::set_glyph(int line, int column, int glyph, int layer, int cell_width, int cell_height)
{
	check(line, column);

	if(current_maze_element_pointers[line][column] == nullptr)
	{
		current_maze_element_pointers[line][column] = new MazeDrawListElement(get_maze_draw_list(line, column), glyph, glyph==0x20?0:100);
	}
	else
	{
		current_maze_element_pointers[line][column]->update_glyph(glyph);
	}

	current_maze_element_pointers[line][column]->update_layer(layer);
	current_maze_element_pointers[line][column]->set_cell_width(cell_width);
	current_maze_element_pointers[line][column]->set_cell_height(cell_height);

}

void PresentationMaze::update_glyph(int line, int column, int glyph)
{
	check(line, column);

	if(current_maze_element_pointers[line][column] != nullptr)
	{
		current_maze_element_pointers[line][column]->update_glyph(glyph);
	}
}

void PresentationMaze::set_rotation(int line, int column, double angle)
{
	check(line, column);
	current_maze_element_pointers[line][column]->update_angle(angle);
}


void PresentationMaze::set_maze_colours(int line, int column, simple_colour_t fg, SDL_Colour& bg)
{
	maze_background[line][column] = bg;
	maze_foreground[line][column] = fg;
}

void PresentationMaze::set_default_maze_colours(simple_colour_t fg, SDL_Colour& bg)
{
	for(int line = 0; line < MazeConstants::maze_height_max; line++)
	{
		for(int column = 0; column < MazeConstants::maze_width_max; column++)
		{
			maze_background[line][column] = bg;
			maze_foreground[line][column] = fg;
		}
	}
}

void PresentationMaze::set_offset(pos_t line, pos_t column)
{
	offset_line = line;
	offset_column = column;
}

int PresentationMaze::width()
{
	return ((current_column_max+1));
}

int PresentationMaze::height()
{
	return ((current_line_max+1));
}

void PresentationMaze::set_wall_transparency(int line, int column, unsigned int direction_map)
{
	// direction map format come from MapUtils::calculate_wall_direction_map()
	// 0x08=N 0x04=E 0x02=S 0x01=W

	// assume transparent in all directions
	transparency[line][column].SetTransparent();

	if(direction_map & 0x08)
	{
		// there is a wall to the north of this cell - no transparency to north
		transparency[line][column].SetOpaque(NORTH, SOUTH);
		transparency[line][column].SetOpaque(NORTH, EAST);
		transparency[line][column].SetOpaque(NORTH, WEST);

		// wall is a bump - no EAST - WEST visibility either
		transparency[line][column].SetOpaque(EAST, WEST);
	}

	if(direction_map & 0x04)
	{
		// there is a wall to the east of this cell - no transparency to east
		transparency[line][column].SetOpaque(EAST, SOUTH);
		transparency[line][column].SetOpaque(NORTH, EAST);
		transparency[line][column].SetOpaque(EAST, WEST);

		// wall is a bump - no NORTH - SOUTH visibility either
		transparency[line][column].SetOpaque(NORTH, SOUTH);
	}

	if(direction_map & 0x02)
	{
		// there is a wall to the south of this cell - no transparency to south
		transparency[line][column].SetOpaque(NORTH, SOUTH);
		transparency[line][column].SetOpaque(SOUTH, EAST);
		transparency[line][column].SetOpaque(SOUTH, WEST);

		// wall is a bump - no EAST - WEST visibility either
		transparency[line][column].SetOpaque(EAST, WEST);
	}

	if(direction_map & 0x01)
	{
		// there is a wall to the west of this cell - no transparency to west
		transparency[line][column].SetOpaque(WEST, SOUTH);
		transparency[line][column].SetOpaque(NORTH, WEST);
		transparency[line][column].SetOpaque(EAST, WEST);

		// wall is a bump - no NORTH - SOUTH visibility either
		transparency[line][column].SetOpaque(NORTH, SOUTH);
	}
}

//#define LOS_DEBUG
//#define LOS_DEBUG_COUT

bool PresentationMaze::line_of_sight(pos_t line1, pos_t column1, pos_t line2, pos_t column2)
{
	bool los = true;

	pos_t slope, inv_slope;

	// calculate the slope of the line between the points
	if(column1 == column2)
	{
		// avoid divide by zero
		slope = 1000000;
		inv_slope = 0;
	}
	else if(line1 == line2)
	{
		slope = 0;
		inv_slope = 1000000;
	}
	else
	{
		slope = (line1-line2) / (column1-column2);
		inv_slope = 1 / slope;
	}

#if __cplusplus >= 201103L
	auto column_fn = [&] (pos_t y) { return (((y-line1) * inv_slope) + column1); };
	auto line_fn  = [&] (pos_t x) { return (((x-column1) * slope) + line1); };
#else
	#define column_fn(y) (((y-line1) * inv_slope) + column1)
	#define line_fn(x) (((x-column1) * slope) + line1)
#endif

	typedef std::pair<int,int> cell_ref;


	int loop_start, loop_end, line, column;
	std::map<cell_ref,unsigned int> cells;
	std::map<cell_ref,unsigned int>::iterator it;

    // loop through the integer lines between source and target
    // always go from lowest to highest
    // +0.0001 so that if we are right on an integer line, don't consider the visibility of that line
    if(line1 > line2)
    {
        loop_start = ceil(line2+0.0001);
        loop_end = floor(line1);
    }
    else
    {
        loop_start = ceil(line1+0.0001);
        loop_end = floor(line2);
    }

    for(line=loop_start; line<=loop_end; line++)
    {

         // for each line, calculate the column, and add or update the
         // cell table for the two cells that border the line where the
         // LOS crosses the columns

         column = column_fn(line);
         column = floor(column);

         cells[cell_ref(line, column)] |= NORTH;
         cells[cell_ref(line-1, column)] |= SOUTH;

#ifdef LOS_DEBUG
         set_maze_colours(line, column, BLACK, BRIGHT_WHITE);
         set_maze_colours(line-1, column, BLACK, BRIGHT_YELLOW);
#endif
    }

    //-- loop through the integer columns between source and target
    //-- always go from lowest to highest
    //-- +0.0001 so that if we are right on an integer column, don't consider the visibility of that column
    if(column1 > column2)
    {
        loop_start = ceil(column2+0.0001);
        loop_end = floor(column1);
    }
    else
    {
        loop_start = ceil(column1+0.0001);
        loop_end = floor(column2);
    }

    for(column=loop_start; column<=loop_end; column++)
    {
        // for each column, calculate the line, and add or update the
        // cell table for the two cells that border the column where the
        // LOS crosses the line

        line = line_fn(column);
        line = floor(line);

        cells[cell_ref(line, column)] |= WEST;
        cells[cell_ref(line, column-1)] |= EAST;

#ifdef LOS_DEBUG
        set_maze_colours(line, column, BLACK, BRIGHT_RED);
        set_maze_colours(line, column-1, BLACK, BRIGHT_GREEN);
#endif
    }

    // now iterate over cells, checking visibility in the directions selected
    it = cells.begin();
    while(it != cells.end() && los)
    {
    	int vis_map = it->second;
    	int line = it->first.first;
    	int column = it->first.second;

    	switch(vis_map)
    	{
			case NORTH+SOUTH: los = transparency[line][column].IsTransparent(NORTH,SOUTH); break;
			case NORTH+EAST: los = transparency[line][column].IsTransparent(NORTH,EAST); break;
			case NORTH+WEST: los = transparency[line][column].IsTransparent(NORTH,WEST); break;
			case SOUTH+EAST: los = transparency[line][column].IsTransparent(SOUTH,EAST); break;
			case SOUTH+WEST: los = transparency[line][column].IsTransparent(SOUTH,WEST); break;
			case EAST+WEST: los = transparency[line][column].IsTransparent(EAST,WEST); break;
    	}

#ifdef LOS_DEBUG_COUT
    	std::cout << "1="<< line1<<","<<column1<<"  2="<<line2<<","<<column2<<" "<< line << " " << column << " " << vis_map << " " << los << std::endl;
#endif

    	it++;
    }

    return los;
}

inline unsigned int make_transparent_map(direction_t from, direction_t to)
{
								//     0x01  0x02        0x04                     0x08
	unsigned int from_table[9] = {  0,    0,    4,    0,    8,    0,    0,    0,    12 };
	return (to << from_table[from]);
}

void Transparency::SetTransparent()
{
	transparent_map = 0xFFFF;		// transparent in all directions
}

void Transparency::SetOpaque()
{
	transparent_map = 0x00;		// opaque in all directions
}

void Transparency::SetTransparent(direction_t direction)
{
	SetOneWayTransparent(direction, direction);
}

void Transparency::SetTransparent(direction_t from, direction_t to)
{
	SetOneWayTransparent(from, to);
	SetOneWayTransparent(to, from);
}

void Transparency::SetOneWayTransparent(direction_t from, direction_t to)
{
	unsigned int direction_map = make_transparent_map(from, to);
	transparent_map |= direction_map;

}

void Transparency::SetOpaque(direction_t direction)
{
	SetOpaque(direction, direction);
}

void Transparency::SetOpaque(direction_t from, direction_t to)
{
	// we want the 'to' bit in the 'from' set
	unsigned int direction_map = make_transparent_map(from, to);
	transparent_map &= ~direction_map;

	// and the other direction too
	direction_map = make_transparent_map(to, from);
	transparent_map &= ~direction_map;
}

bool Transparency::IsTransparent(direction_t direction)
{
	return IsTransparent(direction, direction);
}

bool Transparency::IsTransparent(direction_t from, direction_t to)
{
	// we want the 'to' bit in the 'from' set
	unsigned int direction_map = make_transparent_map(from, to);
	return direction_map & transparent_map;
}

