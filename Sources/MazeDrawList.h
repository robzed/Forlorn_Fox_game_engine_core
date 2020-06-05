/*
 * MazeDataList.h
 *
 *  Created on: 30 Jun 2014
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

#ifndef MAZEDATALIST_H_
#define MAZEDATALIST_H_

#include "MyGraphics.h"
#include <list>
#include <vector>

// here and not in PresentationMaze.h to avoid circular includes
// should really stick this in its own header file
enum render_option
{
	do_not_render_empty_draw_list,
	render_empty_draw_list_as_space,
	hex_rendering,
	square_rendering,
};

class MazeDrawListElement;
typedef std::list<MazeDrawListElement*>::iterator mdl_iterator;

class MazeDrawList
{
public:
	MazeDrawList();
	virtual ~MazeDrawList();

private:
	// lets not have these copy constructed or assigned, could be expensive
	MazeDrawList(const MazeDrawList&);
	MazeDrawList& operator=(const MazeDrawList&);
	mdl_iterator find_layer(int layer);

public:
    // to try ensure only valid pointers are passed around
    unsigned long mdl_magic;
    
	void insert_element(MazeDrawListElement*);
	void remove_element(MazeDrawListElement*);
	void render(MyGraphics& gr, pos_t line, pos_t column, int start_layer, int end_layer, bool overdraw = false);

	void check_integrity();

	void set_render_option(render_option ro);

private:
	std::list<MazeDrawListElement*> maze_draw_list;
	bool _render_empty_draw_list_as_space;
};


class MazeDrawListElement
{
public:
	MazeDrawListElement(MazeDrawList* mdl, int glyph, int layer, double angle = 0.0);
	virtual ~MazeDrawListElement();
	void hide();
	void show();
	void update_glyph(int glyph);
	void update_layer(int layer);
	void update_angle(double angle);
	virtual int get_glyph();
	int get_layer();
	double get_angle();
	void owner_died();
	void set_cell_height(int h) { cell_height = h;} ;
	void set_cell_width(int w) { cell_width = w;};
	int get_cell_height() { return cell_height; };
	int get_cell_width() { return cell_width; };


private:
	// lets not have these copy constructed or assigned
	// nor allow a null constructor
	MazeDrawListElement();// = delete;
	MazeDrawListElement(const MazeDrawListElement&);
	MazeDrawListElement& operator=(const MazeDrawListElement&);

	MazeDrawList* mdl;
    int glyph;
	int layer;
	double angle;
	int cell_height;
	int cell_width;

};



class MazeDrawListAnimatedElement : public MazeDrawListElement
{
public:
    MazeDrawListAnimatedElement(MazeDrawList* mdl, int layer);
    virtual ~MazeDrawListAnimatedElement();
    void update_glyph_list(lua_State* L);
    int get_glyph();
    void add_glyph(int glyph);
    bool setInterval(double seconds);
    
private:
    // lets not have these copy constructed or assigned
    // nor allow a null constructor
    MazeDrawListAnimatedElement();// = delete;
    MazeDrawListAnimatedElement(const MazeDrawListAnimatedElement&);
    MazeDrawListAnimatedElement& operator=(const MazeDrawListAnimatedElement&);
    
    std::vector<int> glyphs;
    double interval;
    double current_dt;
    size_t current_glyph_index;
    
};
#endif /* MAZEDATALIST_H_ */
