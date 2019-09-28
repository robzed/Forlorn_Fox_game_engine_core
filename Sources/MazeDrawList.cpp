/*
 * MazeDataList.cpp
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

#include "MazeDrawList.h"
#include "Debug.h"
#include "Utilities.h"
#include <iostream>

const unsigned long MDL_MAGIC = 0x12344321;

MazeDrawListElement::MazeDrawListElement(MazeDrawList* mdl_in, int g, int l, double a)
: mdl(mdl_in), glyph(g), layer(l), angle(a), cell_height(1), cell_width(1)
{
    if(mdl->mdl_magic != MDL_MAGIC)
    {
        Utilities::fatalError("MazeDrawListElement got something without correct magic. Aborting!");
    }
	mdl->insert_element(this);
	debug.inc_md_count();
}

MazeDrawListElement::~MazeDrawListElement()
{
	if(mdl) mdl->remove_element(this);
	debug.dec_md_count();
}

void MazeDrawListElement::hide()
{
	if(mdl) mdl->remove_element(this);
}

void MazeDrawListElement::show()
{
	if(mdl) mdl->insert_element(this);
}


void MazeDrawListElement::update_glyph(int g)
{
	glyph = g;
}

void MazeDrawListElement::update_angle(double a)
{
	angle = a;
}

void MazeDrawListElement::update_layer(int l)
{
	if(mdl)
	{
		mdl->remove_element(this);
		layer = l;
		mdl->insert_element(this);
	}
}

void MazeDrawListElement::owner_died()
{
	// set the owner as null
	mdl = 0;
}

int MazeDrawListElement::get_layer()
{
	return layer;
}

double MazeDrawListElement::get_angle()
{
	return angle;
}

int MazeDrawListElement::get_glyph()
{
	return glyph;
}

// --- Animated ---


MazeDrawListAnimatedElement::MazeDrawListAnimatedElement(MazeDrawList* mdl_in, int l)
: MazeDrawListElement(mdl_in, 0, l), interval(0), current_dt(0), current_glyph_index(0)
{

}

MazeDrawListAnimatedElement::~MazeDrawListAnimatedElement()
{

}

void MazeDrawListAnimatedElement::update_glyph_list(lua_State* L)
{
    // tests for parameter error, if so, raise error.
    luaL_checktype(L, -1, LUA_TTABLE);
 
    // Get the number of glyphs
    Uint32 number_of_glyphs = luaL_len(L, -1);
    
    // Empy the vector list
    glyphs.clear();
    
    for (Uint32 glyph_index = 1; glyph_index <= number_of_glyphs; ++glyph_index)
    {
        lua_pushinteger(L, glyph_index);
        lua_gettable(L, -2);		// replaces key with value
        
        if (lua_isnumber(L, -1))
        {
            glyphs.push_back(lua_tonumber(L, -1));
        }
        
        // Finished with this so pop from list
        lua_pop(L, 1);
    }
    
    // Check new number of glyphs is not off the end of the list
    if (glyphs.size() <= current_glyph_index)
    {
        current_glyph_index = 0;
    }
}

void MazeDrawListAnimatedElement::add_glyph(int glyph)
{
    glyphs.push_back(glyph);
}

int MazeDrawListAnimatedElement::get_glyph()
{
   uint32_t total_dt = SDL_GetTicks();
    
   if(current_dt != total_dt)
   {
       while((total_dt - current_dt) > interval)
       {
           current_glyph_index += 1;
           
           if(current_glyph_index >= glyphs.size())
           {
               current_glyph_index = 0;
           }
           current_dt += interval;
       }
   }
   return glyphs.at(current_glyph_index);
}

bool MazeDrawListAnimatedElement::setInterval(double seconds)
{
    // Could add range check
    interval = seconds;
    return true;
}


MazeDrawList::MazeDrawList()
: mdl_magic(MDL_MAGIC)
{
}

MazeDrawList::~MazeDrawList()
{
	mdl_iterator mdl = maze_draw_list.begin();

	while(mdl != maze_draw_list.end())
	{
		(*mdl)->owner_died();
		mdl++;
	}
    // to ensure only valid pointers are passed around
    mdl_magic = 0;
}

void MazeDrawList::render(MyGraphics& gr, pos_t line, pos_t column, int start_layer, int end_layer, bool overdraw)
{

	mdl_iterator dl = maze_draw_list.begin();

	if(overdraw)
		gr.set_bg_transparent();
	else
		gr.set_bg_opaque();

	while(dl != maze_draw_list.end())
	{
		MazeDrawListElement* mdl = *dl;

		int layer = mdl->get_layer();

		if(layer > end_layer) break;

		if(layer >= start_layer)
		{
			gr.print(line, column, mdl->get_glyph(), mdl->get_angle(), mdl->get_cell_width(), mdl->get_cell_height());
		}

		gr.set_bg_transparent();
		dl++;
	}

	gr.set_bg_opaque();
}

void MazeDrawList::check_integrity()
{
	mdl_iterator dl = maze_draw_list.begin();


	while(dl != maze_draw_list.end())
	{
		if(*dl==0)
		{
	        Utilities::fatalError("MazeDrawList has gone pear-shaped");
		}

		dl++;
	}

}

void MazeDrawList::insert_element(MazeDrawListElement* md)
{
	// add dle to the list depending on the render order - i.e. the layer
	mdl_iterator it = find_layer(md->get_layer());
	maze_draw_list.insert(it, md);

	//Utilities::debugMessage("MazeData added to list - length %d", maze_draw_list.size());
}

void MazeDrawList::remove_element(MazeDrawListElement* md)
{
	// find the element in the list and remove it
	mdl_iterator mdl = maze_draw_list.begin();
	bool found = false;

	while(mdl != maze_draw_list.end() && !found)
	{
		if(*mdl == md)
		{
			found = true;
		}
		else
		{
			mdl++;
		}
	}

	if(found)
	{
		// remove from the list
		maze_draw_list.erase(mdl);
		//Utilities::debugMessage("MazeData removed from list - length %d", maze_draw_list.size());
	}
	else
	{
		//Utilities::debugMessage("Didn't find MazeData in maze_data_list in remove_element()");
	}

}

mdl_iterator MazeDrawList::find_layer(int layer)
{
	// walk the list looking for this layer
	mdl_iterator mdl = maze_draw_list.begin();

	while(mdl != maze_draw_list.end())
	{
		if((*mdl)->get_layer() >= layer) return mdl;
		mdl++;
	}

	return mdl;
}
