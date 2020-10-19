/*
 *  PresentationMaze.h
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


#ifndef CURRENT_MAZE_H
#define CURRENT_MAZE_H

#include "MyGraphics.h"
#include "Debug.h"
#include "MazeDrawList.h"
#include "DrawList.h"
#include <list>
#include "MazeConstants.h"
#include "GameApplication.h"
#include <map>

struct lua_State;
//class DrawList;

// don't change these, there is a dependency on the ordering
// of wall_glyph_lookup_table in MapUtils
enum direction_t { NORTH=0x08, SOUTH=0x02, EAST=0x04, WEST=0x01 };


class Transparency
{
private:
	// Bit					 FEDC            BA98            7654            3210
	// Transparent from N to NESW, from S to NESW, from E to NESW, from W to NESW
	// Allows for one way transparency
	unsigned int transparent_map;

public:
	void SetTransparent();
	void SetTransparent(direction_t direction);
	void SetTransparent(direction_t from, direction_t to);
	void SetOneWayTransparent(direction_t from, direction_t to);
	void SetOpaque();
	void SetOpaque(direction_t direction);
	void SetOpaque(direction_t from, direction_t to);

	bool IsTransparent(direction_t direction);
	bool IsTransparent(direction_t from, direction_t to);
};


class PresentationMaze : public DrawListOwner, public Clickable
{
public:
	PresentationMaze(double min_glyphs_horizontally, double min_glyphs_vertically);
	~PresentationMaze();

	void load_current_maze(lua_State* L);
		
	void print(MyGraphics* gr);
	void print_selected(MyGraphics* gr, int start_line, int lines_to_print);
	
	int get_glyph(int line, int column);
	void set_glyph(int line, int column, int value, int layer = 0, int cell_width = 1, int cell_height = 1);
	void update_glyph(int line, int column, int glyph);
	void update_layer(int line, int column, int layer);
	
	void set_rotation(int line, int column, double angle);

	void check(int line, int column);
	void initialise_back_colour();
	void set_default_maze_colours(simple_colour_t fg, SDL_Colour& bg);
	
	void set_maze_colours(int line, int column, simple_colour_t fg, SDL_Colour& bg);

	void set_offset(pos_t line, pos_t column);
	int width();
	int height();

	MazeDrawList* get_maze_draw_list(int line, int column) { return &(maze_draw_list[line][column]); }
	DrawList* get_mobs_draw_list();

	void set_view_layer(int line, int column, int layer);
	
	void set_wall_transparency(int line, int column, unsigned int directions_map);
	bool line_of_sight(pos_t line1, pos_t column1, pos_t line2, pos_t column2);

	void set_rect(int left, int top, int right, int bottom);
	void zoom(bool zoom_in);

	pos_t get_line_offset() { return offset_line; }
	pos_t get_column_offset() { return offset_column; }

	void set_click_callback(luabridge::LuaRef callback, luabridge::LuaRef self);
	bool check_for_click(int x, int y, bool down, bool drag);

	double GetAvailableLevelWidth();
	double GetAvailableLevelHeight();
	int GetCellSize();

	void set_render_option(int ro);

	bool RenderAsHex() { return mHexRendering; }

	void render_map_data(MyGraphics& gr, int map_line, int map_column, pos_t screen_line, pos_t screen_column, int start_layer, int end_layer, bool overdraw = false);

private:
	void delete_all_cmep();
	void update_viewport_and_dimensions();
	int calculate_cell_size();
	
	MazeDrawList maze_draw_list[MazeConstants::maze_height_max][MazeConstants::maze_width_max];

	// needs to be replicated per player for multiple players
	// this is not true in the model where each player is a separate client
	int view_layer[MazeConstants::maze_height_max][MazeConstants::maze_width_max];

	// store pointers to maze list elements so we can delete them
	MazeDrawListElement* current_maze_element_pointers[MazeConstants::maze_height_max][MazeConstants::maze_width_max];


	// list of mobs (and anything else you care to draw this way)
	DrawList mobs_draw_list;

	// colours per map element
	SDL_Colour maze_background[MazeConstants::maze_height_max][MazeConstants::maze_width_max];
	simple_colour_t maze_foreground[MazeConstants::maze_height_max][MazeConstants::maze_width_max];

	// transparency of map elements
	Transparency transparency[MazeConstants::maze_height_max][MazeConstants::maze_width_max];

	int current_line_max;
	int current_column_max;	
	
	pos_t offset_line;
	pos_t offset_column;
	
	double available_level_width;
	double available_level_height;
	double ideal_available_level_width;
	double ideal_available_level_height;

	Viewport viewport;

	bool clickable;
	bool mDown;
	luabridge::LuaRef click_callback;
	luabridge::LuaRef drag_callback;
	luabridge::LuaRef click_self;

	bool mHexRendering;

	double mHorizontalOffsetRatio;
	double mVerticalOffsetRatio;


};

#endif

