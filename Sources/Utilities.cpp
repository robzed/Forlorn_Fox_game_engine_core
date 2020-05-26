/*
 *  Utilties.cpp
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

#include "Utilities.h"
#include <time.h>
#include <stdlib.h>
#include "SDL.h"
#include <stdio.h>
#include <iostream>
#include "GameConfig.h"
#include <chrono>

#ifdef __ANDROID__
	#include <android/log.h>
#endif

extern SDL_threadID main_threadID;

int Utilities::randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void Utilities::initialise_random_seed()
{
	srand(static_cast<unsigned int>(time(NULL)));
}

int Utilities::make_unit(int integer)
{
	if(integer > 0) return 1;
	if(integer < 0) return -1;
	return 0;
}

void Utilities::fatalErrorSDL(const std::string& string, int error_code)
{
	fatalErrorSDL(string.c_str(), error_code);
}

void Utilities::fatalErrorSDL(const char *string, int error_code)
{
    std::string message = string;
    message = message + " '" + SDL_GetError() + "'";

    if(main_threadID == SDL_ThreadID())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error - QUIT", message.c_str(), NULL);
    }
    else
    {
        // cant' show a simple message box in this case... no GUI in alternate threads
        // @todo: we can't return from a fatalError (could we do a Lua exception?)
    }
    
#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_ERROR, "ForlornFox", "%s '%s'", string, SDL_GetError());
#else
	std::cout << message << std::endl;
#endif
    exit(error_code);
}

void Utilities::fatalError(const std::string& string)
{
	fatalError(string.c_str());
}

void Utilities::fatalErrorLua(const char *string)
{
	fatalError(string);
}

void Utilities::fatalError(const char *string, ...)
{
   const int str_size = 2048;    // something like apath error does this
	char str[str_size];

	va_list args;
	va_start(args, string);
   vsnprintf(str, str_size-1, string, args);
   str[str_size-1] = 0;   // whatever happens make sure it's zero terminated, probably unnecessary
	va_end(args);

    if(main_threadID == SDL_ThreadID())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error - QUIT", str, NULL);
    }
    else
    {
        // cant' show a simple message box in this case... no GUI in alternate threads
        // @todo: we can't return from a fatalError (could we do a Lua exception?)
    }
    
#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_ERROR, "ForlornFox", "%s", str);
#else
	std::cout << str << std::endl;
#endif

	exit(43);
}

void Utilities::debugMessage(const std::string& string)
{
	debugMessage(string.c_str());
}

void Utilities::debugMessage(const char *string, ...)
{
#ifndef Windows
#define SELF_ALLOCATE
#endif // Windows

#ifndef SELF_ALLOCATE

	// this version uses a big fix size char array
	const int str_size = 20000;
	char *str = new char[str_size];

	va_list args;
	va_start(args, string);
	// use vsnprintf or vasprintf here... rather than the unsafe vsprintf
	//vsprintf(str, string, args);
	vsnprintf(str, str_size-1, string, args);
	str[str_size-1] = 0;	// whatever happens make sure it's zero terminated.
	va_end(args);

#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, "ForlornFox", "%s", str);
#else
	std::cout << str << std::endl;
#endif
	delete[] str;


#else // not SELF_ALLOCATE

	char *str = 0;

	va_list args;
	va_start(args, string);
	// use vsnprintf or vasprintf here... rather than the unsafe vsprintf
	int bytes_printed = vasprintf(&str, string, args);
	va_end(args);

	if(bytes_printed < 0) { str = 0; }
	if(str) {
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_INFO, "ForlornFox", "%s", str);
#else
		std::cout << str << std::endl;
#endif
		free(str);	// vasprintf uses malloc, apparently
	}

#endif // not self_ALLOCATE
}

bool Utilities::file_exists(const char* filename)
{
    FILE* f = fopen(filename, "r");
    if(f)
    {
        fclose(f);
        return true;
    }
    return false;
}
bool Utilities::file_exists(const std::string& filename) { return file_exists(filename.c_str()); }




// NOTES ABOUT UTF-8
// http://en.wikipedia.org/wiki/UTF-8
// http://www.cl.cam.ac.uk/~mgk25/ucs/utf-8-history.txt (code is obsolete!)
// http://lua-users.org/wiki/LuaUnicode
// http://www.codeproject.com/Articles/14637/UTF-8-With-C-in-a-Portable-Way

// utf8_to_int_helper
// DESCRIPTION: Helper for string functions where they detect the byte
// is not ASCII (i.e. is >= 0x80).
//
// PARAMETERS:
// value - [IN] pointer to value of leading byte (first byte).
// value - [OUT] pointer to calculated value, if return is not zero.
// s1 - [IN] pointer to first continuation byte.
// RETURN:
// return - error (0 or negative) or number of continuation bytes (1, 2 or 3 only).
// errors = 0 is error in continuation byte, negative = number of continuation bytes to skip.
//
// NOTICE: Error can be zero termination byte as one of continuation bytes!
int Utilities::utf8_to_int_helper(int* value, const unsigned char* s1)
{
	int c0 = *value;
	// we don't need to check the less than 0x80 cases here
	// Check it doesn't look like a continuation byte
	if(c0 < 0xc0)   // less than 1100 0000, i.e. 0x0000 0000 to 0x1011 1111
	{
		// error - looks like a continuation byte (or escaped ASCII byte)
		return 0;
	}

	int c1 = *(s1+0);
	if(c1 < 0x80 || c1 >= 0xC0) { return -1; } // bit7 must be set and bit6 must be clear
	c1 &= 0x3F;
	if(c0 < 0xe0) // less than 1110 0000, i.e. 0x1100 0000 to 0x1101 1111
	{
		// 110X XXXX  two bytes
		*value = ((0x1f & c0) << 6) + c1;
		return 1;
	}

	int c2 = *(s1+1);
	if(c2 < 0x80 || c2 >= 0xC0) { return -2; } // bit7 must be set and bit6 must be clear
	c2 &= 0x3F;
	if(c0 < 0xf0) // less than 1111 0000, i.e. 0x1110 0000 to 0x1110 1111
	{
		// 1110 XXXX  three bytes
		*value = ((0x0f & c0) << 12) + (c1 << 6) + c2;
		return 2;
	}
	int c3 = *(s1+2);
	if(c3 < 0x80 || c3 >= 0xC0) { return -3; } // bit7 must be set and bit6 must be clear
	c3 &= 0x3F;
	if(c0 < 0xf8) // less than 1111 1000, i.e. 0x1111 0000 to 0x1111 1011
	{
		// 1111 0XXX  four bytes
		*value = ((0x07 & c0) << 18) + (c1 << 12) + (c2 << 6) + c3;
		return 3;
	}

	// 5 and 6 bytes sequences, plus 0xFE and 0xFF are illegal in UTF-8.
	return 0;
}


// from_utf8
// DESCRIPTION: Converts a UTF-8 stream to a int byte.
//
// PARAMETERS:
// num_bytes_in_utf8_block [IN] <not used>
// num_bytes_in_utf8_block [OUT] Number of bytes (1, 2, 3 or 4 only).
// s0 [IN] pointer to leading byte
//
// RETURN: Unicode codepoint value (can be 0xFFFD replacement character or zero on error)
// NOTICE: Error can be zero termination byte if zero is one of continuation bytes, to allow
// proper string termination.
int from_utf8(int* num_bytes_in_utf8_block, const unsigned char* s0)
{
	int len = 0;	// assume just c0 leading byte
	int c0 = *s0;	// leading byte

	if(c0 > 0x80)	// not normal ASCII character?
	{
		len = Utilities::utf8_to_int_helper(&c0, s0+1);
		if(len <= 0)	// error condition
		{
			len = -len;
			c0 = 0xFFFD;
			// we need to check for zeros in continuation bytes...
			for(int i = 1; i <= len; i++)
			{
				if(s0[i] == 0)
				{
					if(num_bytes_in_utf8_block) // only set if not zero
					{
						*num_bytes_in_utf8_block = len+1;
					}
					// zero terminator should be flagged, in case this is a c string call.
					return 0; // return a c string terminator
				}
			}
		}
	}

	if(num_bytes_in_utf8_block) // only set if not zero
	{
		*num_bytes_in_utf8_block = len+1;
	}
	return c0;		// skip this byte to continue
}



// This uses a std::string, so isn't always the fastest function for some uses.
// But it should be adequate in most cases.
// It will never produce more than 4 characters.
std::string to_utf8(int character)
{
	std::string s;
	to_utf8_string(s, character);
	return s;
}

//
// alternate function that adds a character to an existing string
//
void to_utf8_string(std::string& s, int character)
{
	if(character < 0)
	{
		s += to_utf8(0xFFFD);	// Unicode replacement character http://en.wikipedia.org/wiki/Replacement_character#Replacement_character
		return;
	}
	if(character < 0x80)
	{
		// 1 byte 0xxxxxxx
		s += (char)character;
		return;
	}
	if(character <= 0x7FF)
	{
		// 2 bytes 110xxxxx 10xxxxxx
		s += (char)(0xC0 + (character >> 6));	// leading byte
		s += (char)(0x80 + (character & 0x3F));	// continuation byte
		return;
	}
	if(character <= 0xFFFF)
	{
		// 3 bytes 1110xxxx 10xxxxxx 10xxxxxx
		s += (char)(0xE0 + (character >> 12));		// leading byte
		s += (char)(0x80 + (0x3f & (character >> 6)));	// first continuation byte
		s += (char)(0x80 + (character & 0x3F));	// last continuation byte
		return;
	}
	if(character <= 0x10FFFF)	// this is the Unicode limit, not the UTF-8 4 byte limit (0x1FFFFF)
	{
		// 4 bytes 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		s += (char)(0xF0 + (character >> 18));		// leading byte
		s += (char)(0x80 + (0x3f & (character >> 12)));		// first continuation byte
		s += (char)(0x80 + (0x3f & (character >> 6)));		// second continuation byte
		s += (char)(0x80 + (character & 0x3F));	// last continuation byte
		return;
	}

	// out of bounds, not supported
	s += to_utf8(0xFFFD);
}


//
// These two routines are from http://www.libsdl.org/cgi/docwiki.cgi/Pixel_Access
//
Uint32 getpixel(const SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
		case 1:
			return *p;
			break;

		case 2:
			return *(Uint16 *)p;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
			break;

		case 4:
			return *(Uint32 *)p;
			break;

		default:
			return 0;       /* shouldn't happen, but avoids warnings */
    }

	return 0;			// also shouldn't happen ... again avoids warnings when inlining.
}
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *)p = pixel;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = pixel;
			break;
    }
}

// This is rubbish for several reasons:
//  1. Sometimes std::chrono::high_resolution_clock might be millisecond resolution
//  2. Printing takes a while anyway
//
// See: http://en.cppreference.com/w/cpp/chrono/high_resolution_clock
// and: https://stackoverflow.com/questions/1487695/c-cross-platform-high-resolution-timer/5524138#5524138
//
static bool timing_printing_enabled = false;

double Utilities::get_time_since_last_call()
{
    using namespace std::chrono;
    static high_resolution_clock::time_point last_time;
    static bool first_time_set = false;
    if(not first_time_set)
    {
        first_time_set = true;
        last_time = high_resolution_clock::now();
    }
    
    auto t = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t - last_time);
    last_time = t;
    
    
    return time_span.count();
}

void Utilities::print_time_since_last_call(const char* section_name)
{
    if(timing_printing_enabled)
    {
        debugMessage("%s%.4f ms\n", section_name, 1000 * get_time_since_last_call());
    }
}

void Utilities::enable_print_time_since_last_call()
{
    timing_printing_enabled = true;
}
