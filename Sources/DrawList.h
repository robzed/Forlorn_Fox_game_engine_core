/*
 * DrawList.h
 *
 *  Created on: 10 Jun 2014
 *      Author: Tony Park
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

#ifndef DRAWLIST_H_
#define DRAWLIST_H_

#include <list>
#include <vector>
#include "MyGraphics.h"
#include "MazeConstants.h"
#include "Clickable.h"
class DrawList;
class PresentationMaze;

#ifdef RENDER_DEBUG
extern bool dl_render_debug;
#endif

struct DrawListElement
{
private:
	// lets not have these copy constructed or assigned
	DrawListElement(const DrawListElement&);
	DrawListElement& operator=(const DrawListElement&);

	bool listed;
    DrawList* draw_list;

public:
    struct compound_glyph
	{
		int glyph;
		int sublayer;
		float line_offset;
		float column_offset;
		bool visible;
		compound_glyph()              : glyph(0), sublayer(0), line_offset(0.0), column_offset(0.0), visible(false) {};
		compound_glyph(int g, int s)  : glyph(g), sublayer(s), line_offset(0.0), column_offset(0.0), visible(true) {};
		compound_glyph(int g, bool v) : glyph(g), sublayer(0), line_offset(0.0), column_offset(0.0), visible(v) {};
		compound_glyph(int g, int s, bool v) : glyph(g), sublayer(s), line_offset(0.0), column_offset(0.0), visible(v) {};
		compound_glyph(int g, int s, bool v, float l, float c) : glyph(g), sublayer(s), line_offset(l), column_offset(c), visible(v) {};
	};

    std::list<compound_glyph> glyphs;							// what we are printing (ordered by layer
    std::vector<std::list<compound_glyph>::iterator> gl_its;	// allow direct access to list elements
	pos_t line;	// where we are printing it
	pos_t column;
	int layer;		// above or below overlapping glyphs?

	SDL_Colour fg_colour;		// colour to use where the glyph says 'multicolour'
	SDL_Colour bg_colour;			// colour to use for the background
	bool bg_transparent;

	double angle;
	bool dim;

	double size_ratio;
	int height;
	int width;

	bool clickable;
	bool mDown;
	luabridge::LuaRef click_callback;
	luabridge::LuaRef drag_callback;
	luabridge::LuaRef click_self;
	luabridge::LuaRef click_arg;

private:
    DrawListElement(
			DrawList* draw_list,
			int glyph,
			pos_t line,
			pos_t column,
			int layer,
			simple_colour_t fg_colour,
			SDL_Colour bg_colour,
			bool bg_transparent);

    void sort_compound_glyphs();

public:
    
	DrawListElement(
			DrawList* draw_list,
			int glyph,
			pos_t line,
			pos_t column,
			int layer,
			simple_colour_t fg_colour);

	~DrawListElement();
	void show(bool suppress_clickable =  false);
	void hide();
	bool is_visible();
	void list_died();

	void draw(MyGraphics& gr, pos_t line_offset, pos_t column_offset);

	void set_glyph(int g);
	int get_glyph();
    int new_compound_glyph(int glyph, int layer, bool v, pos_t line_offset, pos_t column_offset);		// returns index
	void set_compound_glyph(int index, int glyph);
	void set_compound_glyph_layer(int index, int layer);
	void set_compound_glyph_offsets(int index, pos_t l, pos_t c);
	void hide_compound_glyph(int index);
	void show_compound_glyph(int index);

	void set_line(pos_t l) { line = l; }
	void set_column(pos_t c) {column = c; }
	pos_t get_line() { return line; }
	pos_t get_column() { return column; }
	void set_layer(int l);
	int get_layer() { return layer; }
	void set_fg_colour(simple_colour_t c) { fg_colour = get_rgb_from_simple_colour(c); }
	void set_fg_fullcolour(SDL_Color c) { fg_colour = c; }
	void set_bg_colour(SDL_Color c) { bg_colour = c; }
	void set_bg_transparent() { bg_transparent = true; }
	void set_bg_opaque() { bg_transparent = false; }
	void set_bg_transparency(bool t) { bg_transparent = t; }
	void set_dim() { dim = true; }
	void set_bright() { dim = false; }
	void set_size_ratio(double s) { size_ratio = s; }
	double get_size_ratio() { return size_ratio; };

	void set_height(int h) { height = h; }
	int get_height() { return height; }
	void set_width(int w) {width = w; }
	int get_width() { return width; }

	bool is_clickable() { return clickable; }
	bool click(bool down, bool drag);

	void set_click_callback(luabridge::LuaRef callback, luabridge::LuaRef self, luabridge::LuaRef arg);
	void set_draw_list(DrawList* dl, pos_t l, pos_t c, bool suppress_clickable = false);
	DrawList* get_draw_list() { return draw_list; };
	pos_t get_line_in_pixels();
	pos_t get_column_in_pixels();

	void set_angle(double a) { angle = a; }
	double get_angle() { return angle; }
};

typedef std::list<DrawListElement*>::iterator dl_iterator;


class DrawListOwner
{
public:
	virtual pos_t get_line_offset() = 0;
	virtual pos_t get_column_offset() = 0;
	virtual ~DrawListOwner() {};
};

class DrawList : public Clickable
{
public:
	DrawList();
	virtual ~DrawList();

	DrawList(DrawListOwner* mpParent);

    // to try ensure only valid pointers are passed around
    unsigned long dl_magic;

private:
	// lets not have these copy constructed or assigned, could be expensive
	DrawList(const DrawList&);
	DrawList& operator=(const DrawList&);

	Viewport viewport;

	DrawListOwner* mpParent;

public:


	void render_simple(MyGraphics* gr);
	void render_complex(MyGraphics* gr, pos_t offset_line, pos_t offset_column, PresentationMaze* maze, int (*view_layer)[MazeConstants::maze_height_max][MazeConstants::maze_width_max]);
	void insert_element(DrawListElement*, bool clickable);
	void remove_element(DrawListElement*);

	void set_size(int s) { if(s<1) s=1; viewport.cell_size = s; }
	int get_size() { return viewport.cell_size; }
	void set_draw_location_mode(Viewport::draw_mode_t m) { viewport.draw_mode = m; }
	// work round luabridge not supporting enums
	void set_draw_location_mode_from_lua(int m) { viewport.draw_mode = (Viewport::draw_mode_t)m; }
	Viewport::draw_mode_t get_draw_location_mode() { return viewport.draw_mode; }
	void set_viewport(Viewport& v) { viewport = v; }

	void set_rect(int left, int top, int right, int bottom);
	bool check_for_click(int x, int y, bool down, bool drag);

	pos_t get_line_offset();
	pos_t get_column_offset();

	pos_t get_line_in_pixels(pos_t line);
	pos_t get_column_in_pixels(pos_t column);

	// copy enum values into consts for exposure to lua
	const int PIXEL_BASED = Viewport::pixel_based;
	const int CELL_BASED = Viewport::cell_based;

private:
	std::list<DrawListElement*> draw_list;


	dl_iterator find_layer(int layer);


	int element_count;		// for debug

};

#endif /* DRAWLIST_H_ */
