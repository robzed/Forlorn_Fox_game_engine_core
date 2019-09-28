/*
 *  ColourManagement.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 11/02/2012.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2012-2013 Rob Probin and Tony Park
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

#include "ColourManagement.h"
#include <algorithm>
#include <map>

//
// On the new game, these are the colours used by text (fonts)...
//
SDL_Colour zx_spectrum_black = { 0, 0, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_blue = { 0, 0, 0xCD, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_red = { 0xCD, 0, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_magenta = { 0xCD, 0, 0xCD, SDL_ALPHA_OPAQUE };

SDL_Colour zx_spectrum_green = { 0, 0xCD, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_cyan = { 0, 0xCD, 0xCD, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_yellow = { 0xCD, 0xCD, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_white = { 0xCD, 0xCD, 0xCD, SDL_ALPHA_OPAQUE };

SDL_Colour zx_spectrum_bright_blue = { 0, 0, 255, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_bright_red = { 255, 0, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_bright_magenta = { 255, 0, 255, SDL_ALPHA_OPAQUE };

SDL_Colour zx_spectrum_bright_green = { 0, 255, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_bright_cyan = { 0, 255, 255, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_bright_yellow = { 255, 255, 0, SDL_ALPHA_OPAQUE };
SDL_Colour zx_spectrum_bright_white = { 255, 255, 255, SDL_ALPHA_OPAQUE };

SDL_Colour hot_pink_means_transparent = { 238, 0, 252, SDL_ALPHA_OPAQUE };

SDL_Colour transparent = { 0, 0, 0, SDL_ALPHA_TRANSPARENT };

// not spectrum colours
SDL_Colour orange = { 255, 128, 0, SDL_ALPHA_OPAQUE };
SDL_Colour bright_orange = { 0xCD, 0x66, 0, SDL_ALPHA_OPAQUE };
SDL_Colour x11_purple = { 0xA0, 0x20, 0xF0, SDL_ALPHA_OPAQUE };
SDL_Colour x11_saddle_brown = { 0x8B, 0x45, 0x13, SDL_ALPHA_OPAQUE };
SDL_Colour x11_pink = { 0xFF, 0xC0, 0xCB, SDL_ALPHA_OPAQUE };
SDL_Colour x11_dim_gray = { 0x69, 0x69, 0x69, SDL_ALPHA_OPAQUE};


//--------------------------------------------------------------------------
//
//
static simple_colour_t colours_int_index_to_enum_value[NUMBER_OF_COLOURS] = 
{ 
	BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, WHITE, 
	BRIGHT_BLACK, BRIGHT_BLUE, BRIGHT_RED, BRIGHT_MAGENTA, BRIGHT_GREEN, 
	BRIGHT_CYAN, BRIGHT_YELLOW, BRIGHT_WHITE, 
	
	// not spectrum colours
	ORANGE, BRIGHT_ORANGE, X11_PURPLE, X11_SADDLE_BROWN, X11_PINK, X11_DIM_GRAY,
};


//--------------------------------------------------------------------------
//
//
simple_colour_t convert_to_colour(int index)
{
	if(index < 0) return BLACK;
	if(index >= NUMBER_OF_COLOURS) return BRIGHT_WHITE;
	return colours_int_index_to_enum_value[index];
}

//--------------------------------------------------------------------------
//
//
simple_colour_t toggle_bright(simple_colour_t colour)
{
	switch(colour)
	{
		case BLACK:
			colour = BRIGHT_BLACK;
			break;
		case BLUE:
			colour = BRIGHT_BLUE;
			break;
		case RED:
			colour = BRIGHT_RED;
			break;
		case MAGENTA:
			colour = BRIGHT_MAGENTA;
			break;
		case GREEN:
			colour = BRIGHT_GREEN;
			break;
		case CYAN:
			colour = BRIGHT_CYAN;
			break;
		case YELLOW:
			colour = BRIGHT_YELLOW;
			break;
		case WHITE:
			colour = BRIGHT_WHITE;
			break;
		case BRIGHT_BLACK:
			colour = BLACK;
			break;
		case BRIGHT_BLUE:
			colour = BLUE;
			break;
		case BRIGHT_RED:
			colour = RED;
			break;
		case BRIGHT_MAGENTA:
			colour = MAGENTA;
			break;
		case BRIGHT_GREEN:
			colour = GREEN;
			break;
		case BRIGHT_CYAN:
			colour = CYAN;
			break;
		case BRIGHT_YELLOW:
			colour = YELLOW;
			break;
		case BRIGHT_WHITE:
			colour = WHITE;
			break;
			
		case ORANGE:
			colour = BRIGHT_ORANGE;
			break;
			
		case BRIGHT_ORANGE:
			colour = ORANGE;

		default:
			// don't know how to handle this colour
			break;
	}
	
	return colour;
}


SDL_Colour simple_colours[number_of_colours] = 
{
	zx_spectrum_black,
	zx_spectrum_blue,
	zx_spectrum_red,
	zx_spectrum_magenta,
	
	zx_spectrum_green,
	zx_spectrum_cyan,
	zx_spectrum_yellow,
	zx_spectrum_white,
	
	zx_spectrum_black,
	zx_spectrum_bright_blue,
	zx_spectrum_bright_red,
	zx_spectrum_bright_magenta,
	
	zx_spectrum_bright_green,
	zx_spectrum_bright_cyan,
	zx_spectrum_bright_yellow,
	zx_spectrum_bright_white,
	
	orange,
	bright_orange,
	x11_purple,
	x11_saddle_brown,
	x11_pink,
	x11_dim_gray,
};

const char* simple_colour_names[number_of_colours] = 
{
	"BLACK",
	"BLUE",
	"RED",
	"MAGENTA",
	"GREEN",
	"CYAN",
	"YELLOW",
	"WHITE",
	
	"BRIGHT_BLACK",
	"BRIGHT_BLUE",
	"BRIGHT_RED",
	"BRIGHT_MAGENTA",
	"BRIGHT_GREEN",
	"BRIGHT_CYAN",
	"BRIGHT_YELLOW",
	"BRIGHT_WHITE",
	"ORANGE", 
	"BRIGHT_ORANGE", 
	"X11_PURPLE", 
	"X11_SADDLE_BROWN", 
	"X11_PINK", 
	"X11_DIM_GRAY",
};

void get_rgb_from_simple_colour(SDL_Color* c, simple_colour_t colour_in)
{
	c->r = simple_colours[colour_in].r;
	c->g = simple_colours[colour_in].g;
	c->b = simple_colours[colour_in].b;
	c->a = simple_colours[colour_in].a;
}

// hack to let luabridge resolve overloaded function
SDL_Color _get_rgb_from_simple_colour(simple_colour_t colour_in)
{
	return get_rgb_from_simple_colour(colour_in);
}

SDL_Color get_rgb_from_simple_colour(simple_colour_t colour_in)
{
	SDL_Colour c;
	c.r = simple_colours[colour_in].r;
	c.g = simple_colours[colour_in].g;
	c.b = simple_colours[colour_in].b;
	c.a = simple_colours[colour_in].a;
	return c;
}
SDL_Color get_rgb_from_simple_colour_transparent(simple_colour_t colour_in, Uint8 transparency)
{
	SDL_Colour c;
	c.r = simple_colours[colour_in].r;
	c.g = simple_colours[colour_in].g;
	c.b = simple_colours[colour_in].b;
	c.a = transparency;
	return c;
}


typedef std::map<std::string, simple_colour_t> colour_map_t;

// if this was C++11, we'd use an initialiser list feature...
static colour_map_t create_colour_string_map()
{
	colour_map_t m;
	m["BLACK"] = BLACK;
	m["BLUE"] = BLUE;
	m["RED"] = RED;
	m["MAGENTA"] = MAGENTA;
	m["GREEN"] = GREEN;
	m["CYAN"] = CYAN;
	m["YELLOW"] = YELLOW;
	m["WHITE"] = WHITE;
	m["BRIGHT BLACK"] = BRIGHT_BLACK;
	m["BRIGHT BLUE"] = BRIGHT_BLUE;
	m["BRIGHT RED"] = BRIGHT_RED;
	m["BRIGHT MAGENTA"] = BRIGHT_MAGENTA;
	m["BRIGHT GREEN"] = BRIGHT_GREEN;
	m["BRIGHT CYAN"] = BRIGHT_CYAN;
	m["BRIGHT YELLOW"] = BRIGHT_YELLOW;
	m["BRIGHT WHITE"] = BRIGHT_WHITE;
	
	m["ORANGE"] = ORANGE;
	m["BRIGHT_ORANGE"] = BRIGHT_ORANGE;
	m["X11_PURPLE"] = X11_PURPLE;
	m["X11_SADDLE_BROWN"] = X11_SADDLE_BROWN;
	m["X11_PINK"] = X11_PINK;
	m["X11_DIM_GRAY"] = X11_DIM_GRAY;

	return m;
}

static colour_map_t decode_colour_string_map = create_colour_string_map();


simple_colour_t decode_colour(const std::string colour)
{
	// make upper case
	std::string str = colour;
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	
	colour_map_t::iterator i = decode_colour_string_map.find(str);
	if (i == decode_colour_string_map.end()) {
		return MAGENTA;
	}
	/* Found, i->first is f, i->second is ++-- */
	return i->second;
}

const char* get_colour_name(simple_colour_t c)
{
	int ci = (int)c;
	if(ci < 0 or ci >= number_of_colours)
	{
		ci = 0;
	}
	return simple_colour_names[ci];
}

