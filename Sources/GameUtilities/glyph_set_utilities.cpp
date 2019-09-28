/*
 * glyph_set_utilities.cpp
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

#include "glyph_set_utilities.h"
#include "LuaCppInterface.h"
#include <iostream>
#include "GameApplication.h"
#include "Utilities.h"
#include "LuaBridge.h"
#include "GameToScreenMapping.h"
#include "SaveDataPathDeveloper.h"
#include "LoadPath.h"
#include "image_loader.h"

// declare helper functions
void rotate_1glyph(SDL_Surface *surface, int offset_x, int offset_y, int size);
void rotate_glyphs(SDL_Surface *surface, int size);
SDL_Surface* double_image_size(SDL_Surface *source);


////////////////////////////////
// EXPORTED FUNCTIONS
////////////////////////////////
luabridge::LuaRef get_colour_format(SDL_Surface* glyph_set, lua_State* L)
{
    if(!glyph_set)
    {
        Utilities::fatalError("get_colour_format has NULL == glyph_set");
    }

	// push some bitmap format data
	luabridge::LuaRef colour_format = luabridge::newTable(L);

	colour_format["Amask"] = (double)glyph_set->format->Amask;
	colour_format["Bmask"] = (double)glyph_set->format->Bmask;
	colour_format["Rmask"] = (double)glyph_set->format->Rmask;
	colour_format["Gmask"] = (double)glyph_set->format->Gmask;
	colour_format["Ashift"] = (double)glyph_set->format->Ashift;
	colour_format["Bshift"] = (double)glyph_set->format->Bshift;
	colour_format["Rshift"] = (double)glyph_set->format->Rshift;
	colour_format["Gshift"] = (double)glyph_set->format->Gshift;

	return colour_format;
}

luabridge::LuaRef load_glyph(SDL_Surface* glyph_set_surface, luabridge::LuaRef glyph_set, int glyph_number, lua_State* L)
{
    if(!glyph_set_surface)
    {
        Utilities::fatalError("load_glyph has NULL == glyph_set_surface");
    }

	luabridge::LuaRef pixel_table = luabridge::newTable(L);

	int characters_per_line = glyph_set["characters_per_line"];
	int glyph_size = glyph_set["glyph_size"];

	int column = glyph_number % characters_per_line;
	int row = glyph_number / characters_per_line;

	for(int x = column * glyph_size; x < (column+1) * glyph_size; x++)
	{
		for(int y = row * glyph_size; y < (row+1) * glyph_size; y++)
		{
			pixel_table.append((double)getpixel(glyph_set_surface, x, y));
		}
	}

	luabridge::LuaRef glyph_table = luabridge::newTable(L);
	glyph_table["size"] = glyph_size;
	glyph_table["pixels"] = pixel_table;

	return glyph_table;
}




bool save_glyph_file(SDL_Surface* surface, luabridge::LuaRef glyph_set)
{
    if(!surface)
    {
        Utilities::fatalError("save_glyph_file has NULL == surface");
    }

	std::string file_name = glyph_set["filename"];
	file_name = "graphics/" + file_name;

	// note: save_PNG is 'safe' to call with 0 as the filename...
    if(-1 == save_PNG(surface, SaveDataPathDeveloper(file_name).c_str()))
    {
        Utilities::debugMessage("Error saving bitmap: %s\n", file_name.c_str());
		// don't bomb out just because the there is an error
        //Utilities::fatalError("Error saving bitmap");
        return false;
    }

    return true;
}

SDL_Surface* load_glyph_file(luabridge::LuaRef glyph_set)
{
	int glyph_size = glyph_set["glyph_size"];

	std::string file_name = glyph_set["filename"];
	file_name = "graphics/" + file_name;

	file_name = LoadPath(file_name).str();

	SDL_Surface *load_surface = load_image(file_name.c_str());

    if (!load_surface)
    {
        Utilities::debugMessage("Error loading bitmap: %s\n", file_name.c_str());
        Utilities::fatalError("Error loading bitmap");
        return 0 /*nullptr*/;
    }
    else if(glyph_size == 8)
	{
		/*
    	This does not work - it would need to double glyph size and crucially,
    	return it doubled to lua, which it can't do right now

		Should just do it from lua if it is necessary

    	SDL_Surface* doubled_surface = double_image_size(load_surface);
		SDL_FreeSurface(load_surface);
		load_surface = doubled_surface;
		*/
	}

    return load_surface;
}


bool save_glyph(SDL_Surface* glyph_set_surface, luabridge::LuaRef glyph_set, int glyph_number, luabridge::LuaRef glyph)
{
    bool changed = false;
    if(!glyph_set_surface)
    {
        Utilities::fatalError("save_glyph has NULL == glyph_set_surface");
    }

	luabridge::LuaRef pixels = glyph["pixels"];
	int glyph_size = glyph["size"];

	int characters_per_line = glyph_set["characters_per_line"];

	int column = glyph_number % characters_per_line;
	int row = glyph_number / characters_per_line;

	int glyph_string_pointer = 1;

	for(int x = column * glyph_size; x < (column+1) * glyph_size; x++)
	{
		for(int y = row * glyph_size; y < (row+1) * glyph_size; y++)
		{
            Uint32 oldpix = getpixel(glyph_set_surface, x, y);
            Uint32 newpix = pixels[glyph_string_pointer++];
            if(oldpix != newpix)
            {
                putpixel(glyph_set_surface, x, y, newpix);
                changed = true;
            }
		}
	}
    return changed;
}


//////////////////////////////////////////////
//HELPER FUNCTIONS
//////////////////////////////////////////////
void rotate_1glyph(SDL_Surface *surface, int offset_x, int offset_y, int size)
{
    if(!surface)
    {
        Utilities::fatalError("rotate_1glyph has NULL == surface");
    }

	int width = size;
	int height = size;

	SDL_LockSurface(surface);

	for(int y = 0; y < (height/2) ; y ++)
	{
		for( int x = 0; x < (width/2); x++)
		{
			// NOTE 3Feb13: These x and y order look screwy, but probably works
			// because it's square.
			Uint32 p1 =  getpixel(surface, x+offset_x, y+offset_y);
			Uint32 p2 =  getpixel(surface, offset_x+(width-(y+1)), x+offset_y);
			Uint32 p3 =  getpixel(surface, (width-(x+1))+offset_x, (height-(y+1))+offset_y);
			Uint32 p4 =  getpixel(surface, y+offset_x, (height-(x+1))+offset_y);

			putpixel(surface, x+offset_x, y+offset_y, p4);
			putpixel(surface, offset_x+(width-(y+1)), x+offset_y, p1);
			putpixel(surface, (width-(x+1))+offset_x, (height-(y+1))+offset_y, p2);
			putpixel(surface, y+offset_x, (height-(x+1))+offset_y, p3);
		}
	}
	SDL_UnlockSurface(surface);
}




void rotate_glyphs(SDL_Surface *surface, int size)
{
    if(!surface)
    {
        Utilities::fatalError("rotate_glyphs has NULL == surface");
    }

	int width = surface->w / size;
	int height = surface->h / size;

	int offset_y = 0;
	for(int y = 0; y < height; y++)
	{
		int offset_x = 0;
		for(int x = 0; x < width; x++)
		{
			rotate_1glyph(surface, offset_x, offset_y, size);
			offset_x += size;
		}
		offset_y += size;
	}
}

SDL_Surface* double_image_size(SDL_Surface *source)
{
    if(!source)
    {
        Utilities::fatalError("double_image_size has NULL == source");
    }

	Uint32 Rmask = source->format->Rmask;
	Uint32 Gmask = source->format->Gmask;
	Uint32 Bmask = source->format->Bmask;
	Uint32 Amask = source->format->Amask;      /* masks for desired format */
	int bpp = source->format->BitsPerPixel;                /* bits per pixel for desired format */

	SDL_Surface *dest = SDL_CreateRGBSurface(0, 2*source->w, 2*source->h,
													bpp, Rmask, Gmask,Bmask, Amask);

	if(not dest)
	{
		Utilities::fatalError("failed to create surface in double_image_size()");
	}
	else
	{
		SDL_LockSurface(source);
		SDL_LockSurface(dest);

		for (int y = 0; y < source->h; ++y)
		{
			for (int x = 0; x < source->w; ++x)
			{
				putpixel(dest, x*2, y*2, getpixel(source, x, y));
				putpixel(dest, x*2, y*2+1, getpixel(source, x, y));
				putpixel(dest, x*2+1, y*2, getpixel(source, x, y));
				putpixel(dest, x*2+1, y*2+1, getpixel(source, x, y));
			}
		}
		SDL_UnlockSurface(dest);
		SDL_UnlockSurface(source);
	}

	return dest;
}



