/*
 *  MyGraphics.cpp
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

#include "MyGraphics.h"
#include "GameToScreenMapping.h"
#include <cstring>
#include "Utilities.h"

using Utilities::utf8_to_int_helper;


// pure virtual means derived classes must override this method, this is orthogonal to having an implementation.
MyGraphics::~MyGraphics()
{
}

/*
void print_string(MyGraphics& gr, pos_t line, pos_t col, const char* string)
{
	// check for invalid string
	if(!string) { return; }
	
	int c;
	const unsigned char* ustr = (const unsigned char*)(string); // make unsigned
	while((c = *ustr))
	{
		ustr ++;

		if(c >= 0x80)
		{
			int len = utf8_to_int_helper(&c, ustr);
			if(len <= 0)	// we don't process errors at all
			{
				gr.print(line, col, text_size, '?');	// show error
				break;	// get out, don't parse or print more (we could recover here, but we don't).
			}
			ustr += len;
		}

		gr.print(line, col, text_size, c);
		col++;
	}
}
*/

void print_cstring(MyGraphics* gr, const char* string)
{
	// check for invalid string
	if(!string) { return; }

	int c;
	const unsigned char* ustr = (const unsigned char*)(string); // make unsigned
	while((c = *ustr))
	{
		ustr ++;
		
		if(c >= 0x80)
		{
			int len = utf8_to_int_helper(&c, ustr);
			if(len <= 0)	// we don't process errors at all
			{
				gr->print('?');	// show error
				break;	// get out, don't parse or print more (we could recover here, but we don't).
			}
			ustr += len;
		}

		gr->print(c);
	}
}

void print_glyph(MyGraphics* gr, int glyph)
{
	gr->print(glyph);
}

void print_glyph_ex(MyGraphics* gr, int glyph, double rotation, double size_ratio, int cell_width, int cell_height)
{
	gr->print(glyph, rotation, size_ratio, cell_width, cell_height);
}
/*
void print_string(MyGraphics& gr, const char* string)
{
	print_cstring(&gr, string); 
}
*/

/*
void print_justified_string(MyGraphics& gr, pos_t line, pos_t col, const char* string, justify j)
{
	// BROKEN!!
	int width = 32;

	if(j & justify_right)
	{
		// adjust column (argument is an offset)
        int len = static_cast<int>(std::strlen(string)); // Stop warning: should never be bigger than (2^31)-1
		col += (width - len);
	}

	if(j & justify_centre)
	{
		// adjust column (argument is an offset)
        int len = static_cast<int>(std::strlen(string)); // Stop warning: should never be bigger than (2^31)-1
		col += (width - len) / 2 ;
	}

	if(j & justify_bottom)
	{
		// adjust line (argument is an offset)
		int height = 1;
		line += height - height;
	}

	if(j & justify_vertical_centre)
	{
		// adjust line (argument is an offset)
		int height = 1;
		line += (height - height) / 2;
	}

	print_string(gr, line, col, string);
}

*/

/*void print_string(MyGraphics& gr, pos_t line, pos_t col, const std::string& string)
{
	print_string(gr, line, col, string.c_str());
}*/

void print_string(MyGraphics& gr, const std::string& string)
{
	print_cstring(&gr, string.c_str());
}

/*
void print_justified_string(MyGraphics& gr, pos_t line, pos_t col, const std::string& string, justify j)
{
	print_justified_string(gr, line, col, string.c_str(), j);
}


void print_centered_string(MyGraphics& gr, pos_t line, const char* string)
{
	print_justified_string(gr, line, 0, string, justify_centre);
}


void print_centered_string(MyGraphics& gr, pos_t line, const std::string& string)
{
	print_justified_string(gr, line, 0, string.c_str(), justify_centre);
}
*/

