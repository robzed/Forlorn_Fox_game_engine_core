/*
 *  Utilties.h
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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include "SDL.h"

namespace Utilities {
//public:

	int randomInt(int min, int max);
	void initialise_random_seed();
	int make_unit(int integer);	// isn't this sign or signum? http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
	// error functions, with optional error code
	void fatalErrorSDL(const std::string& string, int error_code = 42);
	void fatalErrorSDL(const char *string, int error_code = 42);
	void fatalError(const std::string& string);
	void fatalErrorLua(const char *string);
	void fatalError(const char *string, ...);
	
	void debugMessage(const std::string& string);
	void debugMessage(const char *string, ...);

	// utf8 helpers
	int utf8_to_int_helper(int* value, const unsigned char* s1);

	// file helpers
	bool file_exists(const char* filename);
    bool file_exists(const std::string& filename);
    double get_time_since_last_call();
    void print_time_since_last_call(const char* section_name);
    void enable_print_time_since_last_call();
};

// these functions are so useful they are globally defined...
std::string to_utf8(int character);
void to_utf8_string(std::string& s, int character);	// add character to existing string

int from_utf8(int* num_bytes_in_utf8_block, const unsigned char* s0);

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
Uint32 getpixel(const SDL_Surface *surface, int x, int y);

#endif
