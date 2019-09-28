/*
 * DrawList.cpp
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

#include "DrawList.h"
#include "PresentationMaze.h"
#include "Utilities.h"
#include "Debug.h"
#include <iostream>

#include "GameApplication.h"

const unsigned long DL_MAGIC = 0x13218887;

#ifdef RENDER_DEBUG
bool dl_render_debug = false;
#endif

DrawListElement::DrawListElement(DrawList* dl,
		int g, pos_t l,	pos_t c, int la, simple_colour_t fg,
		SDL_Colour bg, bool bgt)
: listed(false)
, draw_list(dl)
, line(l)
, column(c)
, layer(la)
, fg_colour(get_rgb_from_simple_colour(fg))
, bg_colour(bg)
, bg_transparent(bgt)
, angle(0.0)
, dim(false)
, size_ratio(1.0)
, height(1)
, width(1)
, clickable(false)
, mDown(false)
, click_callback(gulp_cpp->get_ui_lua_state())
, drag_callback(gulp_cpp->get_ui_lua_state())
, click_self(gulp_cpp->get_ui_lua_state())
, click_arg(gulp_cpp->get_ui_lua_state())
{
	debug.inc_dle_count();
	//Utilities::debugMessage("Constructing a DLE");
    
    if(draw_list->dl_magic != DL_MAGIC)
    {
        Utilities::fatalError("DrawListElement got something without correct magic. Aborting!");
    }

    glyphs.push_back(compound_glyph(g, true));
    gl_its.push_back(glyphs.begin());
}

DrawListElement::DrawListElement(DrawList* dl,
		int g, pos_t l,	pos_t c, int la,
		simple_colour_t fg)
: listed(false)
, draw_list(dl)
, line(l)
, column(c)
, layer(la)
, fg_colour(get_rgb_from_simple_colour(fg))
, bg_colour(SDL_Color())
, bg_transparent(true)
, angle(0.0)
, dim(false)
, size_ratio(1.0)
, height(1)
, width(1)
, clickable(false)
, mDown(false)
, click_callback(gulp_cpp->get_ui_lua_state())
, drag_callback(gulp_cpp->get_ui_lua_state())
, click_self(gulp_cpp->get_ui_lua_state())
, click_arg(gulp_cpp->get_ui_lua_state())
{
	debug.inc_dle_count();
	//Utilities::debugMessage("Constructing a DLE (no colour info)");

    if(draw_list->dl_magic != DL_MAGIC)
    {
        Utilities::fatalError("DrawListElement got something without correct magic. Aborting!");
    }

    glyphs.push_back(compound_glyph(g, true));
    gl_its.push_back(glyphs.begin());
}

DrawListElement::~DrawListElement()
{
	hide();
	//Utilities::debugMessage("Deleting a DLE");
	debug.dec_dle_count();
}

bool DrawListElement::is_visible()
{
	return listed;
}

void DrawListElement::show(bool suppress_clickable)
{
	// itsert ourselves into the list we are associated with
	if(draw_list and not listed)
	{
		draw_list->insert_element(this, clickable and not suppress_clickable);
		listed = true;
	}
}

void DrawListElement::set_draw_list(DrawList* dl, pos_t l, pos_t c, bool suppress_clickable)
{
	// remove element from the current draw list (if necessary)
	hide();

	draw_list = dl;
	set_line(l);
	set_column(c);

	show(suppress_clickable);
}

void DrawListElement::set_click_callback(luabridge::LuaRef callback, luabridge::LuaRef self, luabridge::LuaRef arg)
{
	bool new_clickable = false;

	if(callback.isFunction())
	{
		new_clickable = true;
	}

	// set the callback...!
    callback.force_to_main_state();
	click_callback = callback;
	click_self = self;
	click_arg = arg;

	// if we're already visible and changing click state, then we need to
	if(clickable != new_clickable)
	{
		// if we are already in a draw list (i.e. visible), then we must remove ourselves (hide()),
		// before we update our state, then add ourselves again after the state is updated
		bool visible = is_visible();
		if(visible) hide();
		clickable = new_clickable;
		if(visible) show();
	}
}

bool DrawListElement::click(bool down, bool drag)
{
	// could add 'mouseover' support (e.g. for tooltips) here
	// but for now ignore drag if button is not down
	if(not down) drag = false;

	// click happens when button is released
	bool click = mDown and not down;

	// store for next time
	mDown = down;

	bool consumed = false;

	if(drag && drag_callback.isFunction())
	{
        consumed = drag_callback(click_self, click_arg);
	}

	if(click && click_callback.isFunction())
	{
        consumed = click_callback(click_self, click_arg);
    }

	return consumed;
}

void DrawListElement::hide()
{
	// find the element in the list and remove it
	if(listed)
	{
		if(draw_list) draw_list->remove_element(this);
	}

	listed = false;
}

void DrawListElement::list_died()
{
	draw_list = 0;
	listed = false;

	//Utilities::debugMessage("DrawList died");

}

void DrawListElement::set_layer(int l)
{
	// no need to hide and show to set layer
	// because list is sorted pre-render, doesn't need to be kept ordered

	//hide();
	layer = l;
	//show();
}


void DrawListElement::set_glyph(int g)
{
	gl_its[0]->glyph = g;
}

int DrawListElement::get_glyph()
{
	return gl_its[0]->glyph;
}

void DrawListElement::sort_compound_glyphs()
{
	glyphs.sort([] (compound_glyph a, compound_glyph b) {
		return (a.sublayer < b.sublayer);
	});
}
int DrawListElement::new_compound_glyph(int g, int l, bool v, pos_t line_offset, pos_t column_offset)
{
    int index = static_cast<int>(gl_its.size()); // Stop warning: should never be bigger than (2^31)-1

	glyphs.push_back(compound_glyph(g,l, v, line_offset, column_offset));
	gl_its.push_back(--glyphs.end());

    sort_compound_glyphs();

	return index;
}
void DrawListElement::set_compound_glyph(int index, int g)
{
	gl_its[index]->glyph = g;
}
void DrawListElement::set_compound_glyph_layer(int index, int l)
{
	gl_its[index]->sublayer = l;
    sort_compound_glyphs();
}
void DrawListElement::set_compound_glyph_offsets(int index, pos_t l, pos_t c)
{
	gl_its[index]->line_offset = l;
	gl_its[index]->column_offset = c;
}

void DrawListElement::hide_compound_glyph(int index)
{
	gl_its[index]->visible = false;
}

void DrawListElement::show_compound_glyph(int index)
{
	gl_its[index]->visible = true;
}

void DrawListElement::draw(MyGraphics& gr, pos_t line_offset, pos_t column_offset)
{
	auto it = glyphs.begin();
	while(it != glyphs.end())
	{
		compound_glyph& gl = *it;
		if(gl.visible)
		{
			if(dim) gr.set_dim_alpha();

			gr.print(line-line_offset+gl.line_offset, column-column_offset+gl.column_offset, fg_colour, bg_colour, gl.glyph, angle, size_ratio, width, height);

#ifdef RENDER_DEBUG
			if (dl_render_debug)
			{
				std::cout << "Rendered at " << line-line_offset+gl.line_offset << " (" << line << "+" << line_offset << "+" << gl.line_offset << "), " << column-column_offset+gl.column_offset << " (" << column << "+" << column_offset << "+" << gl.column_offset << ")" << std::endl;
			}
#endif


			if(dim) gr.set_full_alpha();
		}
		it++;
	}
}

pos_t DrawListElement::get_line_in_pixels()
{
	if(draw_list)
	{
		return draw_list->get_line_in_pixels(line);
	}

	// not in a list... no idea where it is..!
	return 0;
}

pos_t DrawListElement::get_column_in_pixels()
{
	if(draw_list)
	{
		return draw_list->get_column_in_pixels(column);
	}

	// not in a list... no idea where it is..!
	return 0;
}


DrawList::DrawList()
: dl_magic(DL_MAGIC)
, mpParent(nullptr)
, element_count(0)
{
	// TODO Auto-generated constructor stub

}

DrawList::DrawList(DrawListOwner* parent)
: dl_magic(DL_MAGIC)
, mpParent(parent)
, element_count(0)
{
	// TODO Auto-generated constructor stub

}

DrawList::~DrawList()
{
    //Utilities::debugMessage("Destroy DrawList");

	gulp_cpp->remove_clickable_target(this);

	dl_iterator dl = draw_list.begin();

	while(dl != draw_list.end())
	{
		(*dl)->list_died();
		dl++;
	}
    dl_magic = 0;
}

void DrawList::set_rect(int left, int top, int right, int bottom)
{
	// don't take the piss
	if (top >= bottom || left >= right) return;

	// set up the clip rect for the maze
	viewport.rect.x = left;
	viewport.rect.y = top;
	viewport.rect.w = right-left;
	viewport.rect.h = bottom-top;
}

pos_t DrawList::get_line_offset()
{
	pos_t offset = viewport.rect.y;

	if(mpParent)
	{
		offset -= mpParent->get_line_offset() * get_size();
	}

	return offset;
}

pos_t DrawList::get_line_in_pixels(pos_t line)
{
	//std::cout << line << " " << size << " " << cell_size << " " << get_line_offset() << " " << (line * get_size()) + get_line_offset() << std::endl;

	return (line * get_size()) + get_line_offset();
}

pos_t DrawList::get_column_offset()
{
	pos_t offset = viewport.rect.x;

	if(mpParent)
	{
		offset -= mpParent->get_column_offset() * get_size();
	}

	return offset;
}

pos_t DrawList::get_column_in_pixels(pos_t column)
{
	return (column * get_size()) + get_column_offset();
}

void DrawList::render_simple(MyGraphics* gr)
{
    render_complex(gr, 0, 0, nullptr, nullptr);
}

void DrawList::render_complex(MyGraphics* gr, pos_t offset_line, pos_t offset_column, PresentationMaze* maze, int (*view_layer)[MazeConstants::maze_height_max][MazeConstants::maze_width_max])
{
	draw_list.sort([] (DrawListElement* a, DrawListElement* b) {
		if(a->layer < b->layer) return true;
		if(a->layer > b->layer) return false;
		return (a->line < b->line);
	});

    if(not gr)
    {
        Utilities::fatalError("MyGraphics nullptr in render_complex");
    }

	gr->set_viewport(viewport);

	dl_iterator dl = draw_list.begin();

	while(dl != draw_list.end())
	{
		if((*dl)->bg_transparent)
			gr->set_bg_transparent();
		else
			gr->set_bg_opaque();

		bool element_onscreen = true;
		//pos_t screen_line = (*dl)->line-offset_line;
		//pos_t screen_column = (*dl)->column-offset_column;

		//if(screen_line < -1 || screen_line > GetAvailableLevelHeight()+1) element_onscreen = false;
		//if(screen_column < (current_column_start-1) || screen_column > current_column_end) element_onscreen = false;

		if(element_onscreen)
		{
			// print the element(s)
			(*dl)->draw(*gr, offset_line, offset_column);

			// print the map data that would be above the element
			// not for stuff printed in pixel_based draw mode, since we
			// can't easily know which bits of the map overlap it
			if(maze && view_layer && viewport.draw_mode == Viewport::cell_based)
			{
				// work out the set of up to 4 maze elements that (*dl) is overlapping
				// strictly speaking we should check the size of all the compound
				// glyphs and make sure we draw all the elements required...

				// but to fix issues with tag markers not layering correctly,
				// we'll go to 4x4 = 16 glyphs over the top of mobs

				// this is a bit hacky, not very efficient and not flexible
				// (it won;t cope with bigger compound glyphs automatically)
				// but will do the job for now

				int int_line = (*dl)->line;		// cast away the decimal part
				int int_column = (*dl)->column;

				int top_line_layer_adjust = 1;

				// loop from 0 to 1 if the glyph isn't directly in line with the maze elements
				//for(int l=int_line; l<=int_line+((int_line==(*dl)->line)?0:1); l++)
				for(int l=int_line-1; l<=int_line+2; l++)
				{
					// loop from 0 to 1 if the glyph isn't directly in column with the maze elements
					//for(int c=int_column; c<=int_column+((int_column==(*dl)->column)?0:1); c++)
					for(int c=int_column-1; c<=int_column+2; c++)
					{
						if( (l >= 0) && (c >= 0) && (l < maze->height()) && (c < maze->width()))
						{
							MazeDrawList* mdl = maze->get_maze_draw_list(l, c);
							int vl = (*view_layer)[l][c];
							mdl->render(*gr, l-offset_line, c-offset_column, (*dl)->layer + top_line_layer_adjust, vl, true);
						}
					}

					if (l == int_line)	top_line_layer_adjust = 0;
				}
			}
		}

		dl++;
	}

	gr->set_bg_opaque();

}

void DrawList::insert_element(DrawListElement* dle, bool clickable_element)
{
	// add dle to the list depending on the render order - i.e. the layer

	// list is sorted pre-render, no need to find layer
	// dl_iterator it = find_layer(dle->layer);
	// draw_list.insert(it, dle);

	//if(dle->get_size()==0 && size) dle->set_size(size);

	draw_list.push_back(dle);

	// if we have a clickable element, make the list clickable
	if(clickable_element)
	{
		gulp_cpp->add_clickable_target(this, true);
	}


	//Utilities::debugMessage("DLE added to list - length %d", draw_list.size());
}

void DrawList::remove_element(DrawListElement* dle)
{
	// find the element in the list and remove it
	dl_iterator dl = draw_list.begin();
	bool found = false;

	while(dl != draw_list.end() && !found)
	{
		if(*dl == dle)
		{
			found = true;
		}
		else
		{
			dl++;
		}
	}

	if(found)
	{
		// remove from the list
		draw_list.erase(dl);
		//Utilities::debugMessage("DLE removed from list - length %d", draw_list.size());
	}
	else
	{
		Utilities::debugMessage("Didn't find DLE in draw_list in remove_element()");
	}

}

bool DrawList::check_for_click(int x, int y, bool down, bool drag)
{
	// x and y are windows coordinates, check if they are within our rectangle
	if(x < viewport.rect.x) return false;
	if(y < viewport.rect.y) return false;
	if(x > viewport.rect.x+viewport.rect.w) return false;
	if(y > viewport.rect.y+viewport.rect.h) return false;

	// adjust x and y to our rect
	x -= get_column_offset();
	y -= get_line_offset();

	pos_t cx = (pos_t)x / viewport.cell_size;
	pos_t cy = (pos_t)y / viewport.cell_size;

	// now check each draw list element
	dl_iterator dl = draw_list.begin();

	while(dl != draw_list.end())
	{
		if((*dl)->is_clickable())
		{
			pos_t column = (*dl)->get_column();
			pos_t line = (*dl)->get_line();
			pos_t size_ratio = (*dl)->get_size_ratio();
			pos_t width = (*dl)->get_width();
			pos_t height = (*dl)->get_height();

			//std::cout << cx << " " << column << " " << cy << " " << line << " " << size_ratio << " " << width << " " << height << std::endl;

			if((cx > column) and (cx < column+(width*size_ratio)) and (cy > line) and (cy < line + (height*size_ratio)))
			{
				if((*dl)->click(down, drag)) return true;
			}
		}

		dl++;
	}

	return false;
}

/*
dl_iterator DrawList::find_layer(int layer)
{
	// walk the list looking for this layer
	dl_iterator dl = draw_list.begin();

	while(dl != draw_list.end())
	{
		if((*dl)->layer >= layer) return dl;
		dl++;
	}

	return dl;
}
*/
