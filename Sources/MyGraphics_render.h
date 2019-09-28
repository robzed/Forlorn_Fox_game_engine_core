/*
 *  MyGraphics_render.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 17/07/2011.
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

#ifndef MYGRAPHICS_RENDER_H
#define MYGRAPHICS_RENDER_H

#include "MyGraphics.h"
#include "LuaMain.h"
#include "LuaBridge.h"
#include "map"

struct GameTexInfo;

// constants for character sets
const int characters_per_set = 0x0100;
const int first_private_use = 0xE000;
const int last_private_use = 0xF8FF;
const int low_value_characters = 0x400;
const int number_of_low_value_sets = low_value_characters / characters_per_set;
const int number_of_private_use_sets = ((last_private_use+1) - first_private_use) / characters_per_set;

struct GameTexInfo {
    // ensure always constructed ok
    GameTexInfo():characters_per_line(0), glyph_size(0), number_lines(0), texture(0)
    {
    }
    // make sure always deleted ok
    ~GameTexInfo()
    {
        if(texture)
        {
            SDL_DestroyTexture(texture);
        }
    }
    void unlink_textures()
    {
        texture = 0;
    }

    unsigned char characters_per_line;  // how many cells per line in texture
    unsigned char glyph_size;           // 16x16 or 32x32 generally
    unsigned char number_lines;         // check for out of bounds
    SDL_Texture* texture;
};

class MyGraphics_render : public MyGraphics {
public:
	MyGraphics_render(SDL_Renderer *created_renderer);
	~MyGraphics_render();
	void load_textures_from_glyph_set(luabridge::LuaRef glyph_set);
	void print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle = 0.0);
	void print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int cells_wide, int cells_high);
	void print(pos_t line, pos_t column, const SDL_Colour& fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int cells_wide, int cells_high);
	void print(int character, double rotation_angle, double size_ratio, int cells_wide, int cells_high);
    int printEx(pos_t line, pos_t column, simple_colour_t fg_colour, int character, double scale_x, double scale_y, double angle, double rot_center_x, double rot_center_y, const int flip);
    int printExT(pos_t line, pos_t column, simple_colour_t fg_colour, int character,
                                     luabridge::LuaRef attrs, lua_State* L);
    
	void print(pos_t line, pos_t column, simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle = 0.0);
	void print(pos_t line, pos_t column, int character, double rotation_angle = 0.0, int cell_width = 1, int cell_height = 1);
	void print(simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle = 0.0);
	void print(simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle = 0.0);

	void print(int character, double rotation_angle = 0.0);
	
	void set_fg_fullcolour(const SDL_Colour& colour);
	void set_fg_colour(simple_colour_t colour);
	void set_bg_fullcolour(const SDL_Colour& colour);
	void set_bg_colour(simple_colour_t colour);
	void set_bg_opaque();
	void set_bg_transparent();
	void set_dim_alpha();
	void set_full_alpha();
	
	void go_to(pos_t line, pos_t column);
	pos_t get_column();
	pos_t get_line();
	void skip_1_forward();
    void wrap(bool on);
    bool get_wrap();
    void set_wrap_limits(double line_start, double line_end, double column_start, double column_end);
    
	void clear_screen(const SDL_Colour& colour);
    
    // these are mostly for debugging, so I haven't put the ones that draw multiple
    // multiple lines, rectangles or points. And they all have colours - so I haven't
    // done the ones without colours either!

	void DrawRect(const SDL_Colour& colour,
                        double x1, double y1, double x2, double y2);
    void DrawAbsoluteRect(const SDL_Colour& colour,
                        int x1, int y1, int x2, int y2);
    void DrawLine(const SDL_Colour& colour,
                        double x1, double y1, double x2, double y2);
    void DrawPoint(const SDL_Colour& colour,
                         double x, double y);
    void FillRect(const SDL_Colour&,
                        double x1, double y1, double x2, double y2);
	void FillRectColour(const SDL_Colour&, const SDL_Rect& rect);
	void FillRectSimple(const SDL_Rect& rect);

    void RenderCopy(SDL_Texture* texture, SDL_Rect* source, SDL_Rect* dest);
	//void set_game_screen_offset(bool limit_to_game_screen_size);

	//bool load_glyph_as_array(Uint32* glyph_array, int character, const char* file_name, int& bytes_per_pixel);
	//bool save_array_as_glyph(unsigned int* glyph_array, int character, luabridge::LuaRef glyph_set);

	void create_texture_set(luabridge::LuaRef glyph_set);
	void update_texture_set_pixel(int base_character_code, int glyph, int glyph_x, int glyph_y, Uint32 pixel);
	void update_texture_set_glyph(int base_character_code, int glyph, int source_glyph_size, SDL_Surface* source, int source_glyph, int source_characters_per_line);
    virtual void SetTextureAlphaMod(int base_character_code, Uint8 alpha);
    virtual void set_viewport(Viewport& vp);
    //virtual void set_glyph_size_in_pixels(int pixels);
private:
	// private functions
	void drawBlank(int x, int y, int width, int height, const SDL_Colour& colour);
//	void internal_printxy(int x, int y, simple_colour_t fg_colour, int character);
	void internal_printxy(int x, int y, const SDL_Colour& fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int width, int height);
    int internal_printxy_extended(int x, int y, const SDL_Colour& fg_colour, int character, double scale_x, double scale_y, double angle, const SDL_Point* center, const SDL_RendererFlip flip);
    
    SDL_Texture* common_transform(int &x, int &y, int character, const SDL_Colour& fg_colour,
                         SDL_Rect &srcRect, int width, int height);
	
    void set_texture(int character, GameTexInfo& gti);
    GameTexInfo* get_texture(int character);
    
    int line_to_y(pos_t line);
	int column_to_x(pos_t column);
	
    Viewport viewport;

	// data members
	SDL_Colour our_bg_colour;
	SDL_Colour our_fg_colour;
    
    // texture maps for character sets
    GameTexInfo low_textures[number_of_low_value_sets];
    GameTexInfo private_use_textures[number_of_private_use_sets];
    std::map<int, GameTexInfo> other_texture_store;   // key: character/256, value - composite info
    
	SDL_Renderer* renderer;
	pos_t current_line;
	pos_t current_column;
    bool wrap_text;
    double wrap_line_start;
    double wrap_line_end;
    double wrap_column_start;
    double wrap_column_end;
	bool bg_transparent;
	bool dim;
};

#endif

