/*
 *  ColourManagement.h
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

#ifndef COLOUR_MANAGEMENT_H
#define COLOUR_MANAGEMENT_H

#include "SDL.h"
#include <string>

//
// NOTE: Although this currently uses just Sinclair ZX Spectrum colours
//       this file is written to support ANY colour, i.e. we might want to add
//       brown, orange, sea-green, grey, and more brightness levels. So 
//       consider that before any change or new code.

enum simple_colour_t 
{ 
	// ZX Spectrum-style colours
	BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, WHITE, 
	BRIGHT_BLACK, BRIGHT_BLUE, BRIGHT_RED, BRIGHT_MAGENTA, BRIGHT_GREEN, 
	BRIGHT_CYAN, BRIGHT_YELLOW, BRIGHT_WHITE, 
	
	// Non Spectrum colours
	ORANGE, BRIGHT_ORANGE, X11_PURPLE, X11_SADDLE_BROWN, X11_PINK, X11_DIM_GRAY,
	
	/// other Non Spectrum colours we might consider
	//PURPLE, BROWN, ORANGE, GREY, PINK, 
	//BRIGHT_PURPLE, BRIGHT_BROWN, BRIGHT_ORANGE, BRIGHT_GREY, BRIGHT_PINK,
	
	// End of list
	NUMBER_OF_COLOURS
};

const int number_of_colours = NUMBER_OF_COLOURS;


extern SDL_Colour zx_spectrum_black;
extern SDL_Colour zx_spectrum_blue;
extern SDL_Colour zx_spectrum_red;
extern SDL_Colour zx_spectrum_magenta;

extern SDL_Colour zx_spectrum_green;
extern SDL_Colour zx_spectrum_cyan;
extern SDL_Colour zx_spectrum_yellow;
extern SDL_Colour zx_spectrum_white;

extern SDL_Colour zx_spectrum_bright_blue;
extern SDL_Colour zx_spectrum_bright_red;
extern SDL_Colour zx_spectrum_bright_magenta;

extern SDL_Colour zx_spectrum_bright_green;
extern SDL_Colour zx_spectrum_bright_cyan;
extern SDL_Colour zx_spectrum_bright_yellow;
extern SDL_Colour zx_spectrum_bright_white;

// specific 'magic' colours
extern SDL_Colour hot_pink_means_transparent;

extern SDL_Colour transparent;

// not spectrum colours
extern SDL_Colour orange;
extern SDL_Colour bright_orange;
extern SDL_Colour x11_purple;
extern SDL_Colour x11_saddle_brown;
extern SDL_Colour x11_pink;
extern SDL_Colour x11_dim_gray;

void get_rgb_from_simple_colour(SDL_Color* c, simple_colour_t colour_in);
SDL_Color get_rgb_from_simple_colour(simple_colour_t colour_in);
SDL_Color _get_rgb_from_simple_colour(simple_colour_t colour_in);
SDL_Color get_rgb_from_simple_colour_transparent(simple_colour_t colour_in, Uint8 transparency);


simple_colour_t convert_to_colour(int index);
simple_colour_t toggle_bright(simple_colour_t colour);
simple_colour_t decode_colour(const std::string colour);
const char* get_colour_name(simple_colour_t);

#endif
