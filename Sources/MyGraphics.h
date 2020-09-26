/*
 *  MyGraphics.h
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

#ifndef MYGRAPHICS_H
#define MYGRAPHICS_H

#include "SDL.h"
#include <string>
#include "ColourManagement.h"
#include "GameConfig.h"
#include "BasicTypes.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "LuaBridge.h"
#include "GameToScreenMapping.h"

//static const int text_size = 32;

// forward declaration
struct GameTexInfo;

class MyGraphics {
public:
	virtual ~MyGraphics() = 0;	// still have to provide implementation for pure virtual
	virtual void print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle = 0.0) = 0;
	virtual void print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int width, int height) = 0;
	virtual void print(pos_t line, pos_t column, const SDL_Colour& fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int width, int height) = 0;
	virtual void print(pos_t line, pos_t column, simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle = 0.0) = 0;
	virtual void print(pos_t line, pos_t column, int character, double rotation_angle = 0.0, int cell_width = 1, int cell_height = 1) = 0;
	virtual void print(simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle = 0.0) = 0;
	virtual void print(simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle = 0.0) = 0;
	virtual void print(int character, double rotation_angle = 0.0) = 0;
	virtual void print(int character, double rotation_angle, double size_ratio, int cells_wide, int cells_high) = 0;

    virtual int printEx(pos_t line, pos_t column, simple_colour_t fg_colour, int character, double scale_x, double scale_y, double angle, double rot_center_x, double rot_center_y, const int flip) = 0;
    virtual int printExT(pos_t line, pos_t column, simple_colour_t fg_colour, int character,
                 luabridge::LuaRef attrs, lua_State* L) = 0;

	virtual void set_fg_fullcolour(const SDL_Colour& colour) = 0;
	virtual void set_fg_colour(simple_colour_t colour) = 0;
	virtual void set_bg_fullcolour(const SDL_Colour& colour) = 0;
	virtual void set_bg_colour(simple_colour_t colour) = 0 ;
	virtual void set_bg_opaque() = 0;
	virtual void set_bg_transparent() = 0;
	virtual void set_dim_alpha() = 0;
	virtual void set_full_alpha() = 0;

	virtual void go_to(pos_t line, pos_t column) = 0;
	virtual pos_t get_column() = 0;
	virtual pos_t get_line() = 0;
	virtual void skip_1_forward() = 0;
    virtual void wrap(bool on) = 0;
    virtual bool get_wrap() = 0;
    virtual void set_wrap_limits(double line_start, double line_end, double column_start, double column_end) = 0;

	virtual void clear_screen(const SDL_Colour& colour) = 0;

	virtual void DrawRect(const SDL_Colour& colour,
                        double x1, double y1, double x2, double y2) = 0;
    virtual void DrawAbsoluteRect(const SDL_Colour& colour,
                        int x1, int y1, int x2, int y2) = 0;
    virtual void DrawLine(const SDL_Colour& colour,
                        double x1, double y1, double x2, double y2) = 0;
    virtual void DrawPoint(const SDL_Colour& colour,
                         double x, double y) = 0;
    virtual void FillRect(const SDL_Colour&,
                        double x1, double y1, double x2, double y2) = 0;

    virtual void FillRectColour(const SDL_Colour&, const SDL_Rect& rect) = 0;
    virtual void FillRectSimple(const SDL_Rect& rect) = 0;


    virtual void RenderCopy(SDL_Texture* texture, SDL_Rect* source, SDL_Rect* dest) = 0;

    virtual void SetTextureAlphaMod(int base_character_code, Uint8 alpha) = 0;
    virtual void set_viewport(Viewport& vp) = 0;

    virtual GameTexInfo* get_GameTexInfo(int character) = 0;
    virtual void overwrite_GameTexInfo(int character, GameTexInfo* gti) = 0;

private:

};

// helper functions
void print_glyph(MyGraphics* gr, int glyph);
void print_glyph_ex(MyGraphics* gr, int glyph, double rotation, double size_ratio, int cell_width, int cell_height);

void print_cstring(MyGraphics* gr, const char* string);
//void print_string(MyGraphics& gr, const char* string);
//void print_string(MyGraphics& gr, pos_t line, pos_t column, const char* string);
void print_string(MyGraphics& gr, const std::string& string);
//void print_string(MyGraphics& gr, pos_t line, pos_t column, const std::string& string);

/*
enum justify  {		justify_none = 0x00,
					justify_left = 0x01,
					justify_right = 0x02,
					justify_top = 0x04,
					justify_bottom = 0x08,
					justify_centre = 0x10,
					justify_vertical_centre = 0x20	};

void print_justified_string(MyGraphics& gr, pos_t line, pos_t column, const char* string, justify j);
void print_justified_string(MyGraphics& gr, pos_t line, pos_t column, const std::string& string, justify j);

void print_centered_string(MyGraphics& gr, pos_t line, const char* string);
void print_centered_string(MyGraphics& gr, pos_t line, const std::string& string);
*/



#endif
