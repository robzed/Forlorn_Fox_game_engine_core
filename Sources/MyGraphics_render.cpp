/*
 *  MyGraphics_render.cpp
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

#include "MyGraphics_render.h"
#include "GameToScreenMapping.h"
#include "Utilities.h"
#include "image_loader.h"
#include <stdio.h>
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include "SaveDataPathDeveloper.h"
#include "LoadPath.h"
#include "glyph_set_utilities.h"
#include <iostream>

using luabridge::LuaRef;


void Delete_SDLTexture(SDL_Texture* t)
{
    //printf("SDL_DestroyTexture %p", t);
    if(t) { SDL_DestroyTexture(t); }
}


void MyGraphics_render::go_to(pos_t line, pos_t column)
{
	current_line = line;
	current_column = column;
}
pos_t MyGraphics_render::get_column()
{
	return current_column;
}
pos_t MyGraphics_render::get_line()
{
	return current_line;
}

void MyGraphics_render::set_bg_opaque()
{
	bg_transparent = false;
}

void MyGraphics_render::set_bg_transparent()
{
	bg_transparent = true;
}

MyGraphics_render::MyGraphics_render(SDL_Renderer *created_renderer)
: renderer(created_renderer), current_line(0), current_column(0),
  wrap_text(true),
  // poor defaults for wrap, but we might not be set up here...
  wrap_line_start(0),
  wrap_line_end(24),
  wrap_column_start(0),
  wrap_column_end(32),
  bg_transparent(false),
  dim(false)
{
	our_bg_colour.r = our_bg_colour.g = our_bg_colour.b = 255;
	our_bg_colour.a = SDL_ALPHA_OPAQUE;

	set_fg_colour(BLACK);
}

MyGraphics_render::~MyGraphics_render()
{
    // GameTexInfo deals with delete of textures
}


void MyGraphics_render::set_fg_fullcolour(const SDL_Colour& colour)
{
	our_fg_colour.r = colour.r;
	our_fg_colour.g = colour.g;
	our_fg_colour.b = colour.b;
	our_fg_colour.a = colour.a;
}

void MyGraphics_render::set_fg_colour(simple_colour_t colour)
{
	get_rgb_from_simple_colour(&our_fg_colour, colour);
}

void MyGraphics_render::set_bg_fullcolour(const SDL_Colour& colour)
{
	our_bg_colour.r = colour.r;
	our_bg_colour.g = colour.g;
	our_bg_colour.b = colour.b;
	our_bg_colour.a = colour.a;
}

void MyGraphics_render::set_bg_colour(simple_colour_t colour)
{
	get_rgb_from_simple_colour(&our_bg_colour, colour);
}

int MyGraphics_render::line_to_y(pos_t line)
{
    if(viewport.draw_mode == Viewport::cell_based)
    {
        int line_pixel_position = line * viewport.cell_size;
        return line_pixel_position + viewport.rect.y + viewport.origin_y;
    }
    
    return line + viewport.rect.y + viewport.origin_y;
}


int MyGraphics_render::column_to_x(pos_t column)
{
    if(viewport.draw_mode == Viewport::cell_based)
    {
        int column_pixel_position = column * viewport.cell_size;
        return column_pixel_position + viewport.rect.x + viewport.origin_x;
    }

    return column + viewport.rect.x + viewport.origin_x;
}

void MyGraphics_render::print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle)
{
	print(line, column, fg_colour, bg_colour, character, rotation_angle, 1.0, 1, 1);
}

void MyGraphics_render::print(int character, double rotation_angle, double size_ratio, int width, int height)
{
	print(current_line, current_column, our_fg_colour, our_bg_colour, character, rotation_angle, size_ratio, width, height);
}

void MyGraphics_render::print(pos_t line, pos_t column, simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int width, int height)
{
	print(line, column, get_rgb_from_simple_colour(fg_colour), bg_colour, character, rotation_angle, size_ratio, width, height);
}

void MyGraphics_render::print(pos_t line, pos_t column, const SDL_Colour& fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int width, int height)
{
	int x, y;

    x = column_to_x(column);
    y = line_to_y(line);

    internal_printxy(x,y,fg_colour, bg_colour, character, rotation_angle, size_ratio, width, height);

	current_line = line;
	current_column = column;
	skip_1_forward();
}




SDL_Texture* MyGraphics_render::common_transform(int &x, int &y, int character,
										const SDL_Colour& fg_colour,
                                        SDL_Rect &srcRect, int width, int height)
{
    int original_character = character;

    //int selected_texture_set = fg_colour;
    // U+E000..U+F8FF BMP (0)	Private Use Area
    // there are 6400 available here
    // which at 256 per texture max, means 24 textures for graphics (256 characters each)

    GameTexInfo* gti;
    if(character < low_value_characters and character >= 0)
    {
        gti = &low_textures[character/characters_per_set];
    }
    else if(character >= first_private_use and character <= last_private_use)
    {
        int index = (character - first_private_use) / characters_per_set;
        gti = &private_use_textures[index];
    }
    else
    {
        typedef std::map<int, GameTexInfo>::iterator it_type;
        it_type obj = other_texture_store.find(character/characters_per_set);
        if(obj == other_texture_store.end())
        {
            character = '?';
            gti = &low_textures[0];
        }
        else
        {
            gti = &obj->second;
        }
    }
    int characters_per_line = gti->characters_per_line;
    if(characters_per_line == 0)
    {
        character = '?';
        gti = &low_textures[0];
        characters_per_line = gti->characters_per_line;
        if(characters_per_line==0)
        {
            Utilities::fatalError("Character set not loaded for character %i or 0x%x", original_character, original_character);
        }
    }
    
    int subcharacter = character % characters_per_set;
    
    if((subcharacter / characters_per_line) >= gti->number_lines)
    {
        subcharacter = '?';
        gti = &low_textures[0];
        characters_per_line = gti->characters_per_line;
    }
    
    int index = subcharacter % characters_per_line;
    int row = subcharacter / characters_per_line;
    
    int cell_size_image = gti->glyph_size;
    
    SDL_Rect new_srcRect =
    { cell_size_image * index, cell_size_image * row,
        cell_size_image*width, cell_size_image*height };
    srcRect = new_srcRect;
    
    SDL_Texture* tex = gti->texture.get();

// @todo: Disable this
#define WARNING_ABOUT_SDL_SETTEXTURECOLORMOD_ERROR 1
    //SDL_Color c = get_rgb_from_simple_colour(fg_colour);
#if WARNING_ABOUT_SDL_SETTEXTURECOLORMOD_ERROR
    int error =
#endif
    SDL_SetTextureColorMod(tex, fg_colour.r, fg_colour.g, fg_colour.b);
#if WARNING_ABOUT_SDL_SETTEXTURECOLORMOD_ERROR
    if(error) { Utilities::fatalErrorSDL("SDL_SetTextureColorMod", error); }
#endif
    return tex;
}

void MyGraphics_render::overwrite_GameTexInfo(int character, GameTexInfo* gti)
{
    if(gti)
    {
        set_GameTexInfo(character, *gti);
    }
}

void MyGraphics_render::set_GameTexInfo(int character, GameTexInfo& gti)
{
    if(character < low_value_characters and character >= 0)
    {
        low_textures[character/characters_per_set] = gti;
    }
    else if(character >= first_private_use and character <= last_private_use)
    {
        int index = (character - first_private_use) / characters_per_set;
        private_use_textures[index] = gti;
    }
    else
    {
        other_texture_store[character / characters_per_set] = gti;
    }
}
GameTexInfo* MyGraphics_render::get_GameTexInfo(int character)
{
    if(character < low_value_characters and character >= 0)
    {
        return &low_textures[character/characters_per_set];
    }
    else if(character >= first_private_use and character <= last_private_use)
    {
        int index = (character - first_private_use) / characters_per_set;
        return &private_use_textures[index];
    }
    else
    {
        typedef std::map<int, GameTexInfo>::iterator it_type;
        it_type obj = other_texture_store.find(character/characters_per_set);
        if(obj == other_texture_store.end())
        {
            return 0;
        }
        else
        {
            return &obj->second;
        }

    }
}

void MyGraphics_render::set_dim_alpha()
{
	dim = true;
}

void MyGraphics_render::set_full_alpha()
{
	dim = false;
}

void MyGraphics_render::internal_printxy(int x, int y, const SDL_Colour& fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle, double size_ratio, int cells_wide, int cells_high)
{
    SDL_Rect srcRect;
    SDL_Texture* tex = common_transform(x, y, character, fg_colour, srcRect, cells_wide, cells_high);

    // if necessary, dim the texture
    if(dim) SDL_SetTextureAlphaMod(tex, 96);

    int w = (int)(size_ratio*viewport.cell_size*cells_wide);
    int h = (int)(size_ratio*viewport.cell_size*cells_high);

    if(!bg_transparent) { drawBlank(x, y, w, h, bg_colour); }
    if(tex)
    {
        SDL_Rect dstRect = { x, y, w, h };
        
#define ROTATION_ALWAYS
#ifndef ROTATION_ALWAYS
        // @todo: is this faster or slower than just calling the more complex function without a test?
        if(rotation_angle == 0.0)
        {
            SDL_RenderCopy(renderer, tex, &srcRect, &dstRect);
        }
        else
#endif
        {
            SDL_RenderCopyEx(renderer, tex, &srcRect, &dstRect, rotation_angle, NULL, SDL_FLIP_NONE);
        }
    }

    // return the texture to full brightness
    if(dim) SDL_SetTextureAlphaMod(tex, 255);

}


int MyGraphics_render::printExT(pos_t line, pos_t column,
                                 simple_colour_t fg_colour, int character,
                                 LuaRef attrs, lua_State* L)
{
    // defaults
    double scale_x = 1.0;
    double scale_y = 1.0;
    double angle = 0;
    double rot_center_x = 0.5;
    double rot_center_y = 0.5;
    int flip = 0;
    
    if(attrs.isTable())
    {
        LuaRef _scale_x = attrs["scale_x"];
        if(_scale_x.isNumber()) { scale_x = _scale_x; }
        
        LuaRef _scale_y = attrs["scale_y"];
        if(_scale_y.isNumber()) { scale_y = _scale_y; }
        
        LuaRef _angle = attrs["angle"];
        if(_angle.isNumber()) { angle = _angle; }
        
        LuaRef _rot_center_x = attrs["rot_center_x"];
        if(_rot_center_x.isNumber()) { rot_center_x = _rot_center_x; }
        
        LuaRef _rot_center_y = attrs["rot_center_y"];
        if(_rot_center_y.isNumber()) { rot_center_y = _rot_center_y; }

        LuaRef _flip = attrs["flip"];
        if(_flip.isNumber()) { flip = _flip; }
    }
    
    return printEx(line, column, fg_colour, character, scale_x, scale_y, angle,
            rot_center_x, rot_center_y, flip);
}

int MyGraphics_render::printEx(pos_t line, pos_t column, simple_colour_t fg_colour, int character, double scale_x, double scale_y, double angle, double rot_center_x, double rot_center_y, const int flip)
{
	int x = column_to_x(column);
    int y = line_to_y(line);
    SDL_Point center = { static_cast<int>(viewport.cell_size * rot_center_x), static_cast<int>(viewport.cell_size * rot_center_y) };
    return internal_printxy_extended(x,y, get_rgb_from_simple_colour(fg_colour), character,
                              scale_x, scale_y, angle, &center, (SDL_RendererFlip)flip);
}

int MyGraphics_render::internal_printxy_extended(int x, int y, const SDL_Colour& fg_colour, int character, double scale_x, double scale_y, double angle, const SDL_Point* center, /*double rot_center_x, double rot_center_y,*/ const SDL_RendererFlip flip)
{
    SDL_Rect srcRect;
    SDL_Texture* tex = common_transform(x, y, character, fg_colour, srcRect, 1, 1);
    if(tex)
    {
        //SDL_Point center = { dstRect.w/2, dstRect.h/2 };
        
        SDL_Rect dstRect = { x, y, static_cast<int>(viewport.cell_size * scale_x), static_cast<int>(viewport.cell_size * scale_y) };
        return SDL_RenderCopyEx(renderer, tex, &srcRect, &dstRect, angle, center, flip);
    }
    return 0;
}



void MyGraphics_render::skip_1_forward()
{
	current_column = current_column+1;
    if(wrap_text)
    {
        if(current_column >= wrap_column_end)
        {
            current_column = wrap_column_start;
            current_line++;
            if(current_line >= wrap_line_end)
            {
                current_line = wrap_line_start;
            }
        }
    }
}

void MyGraphics_render::wrap(bool on)
{
    wrap_text = on;
}
bool MyGraphics_render::get_wrap()
{
    return wrap_text;
}
void MyGraphics_render::set_wrap_limits(double line_start, double line_end, double column_start, double column_end)
{
    wrap_line_start = line_start;
    wrap_line_end = line_end;
    wrap_column_start = column_start;
    wrap_column_end = column_end;
}

/* paints over a glyph sized region with the background color
 in effect it erases the area
 */
void MyGraphics_render::drawBlank(int x, int y, int width, int height, const SDL_Colour& colour)
{
    SDL_Rect rect = { x, y, width, height };
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderFillRect(renderer, &rect);
}


void MyGraphics_render::print(pos_t line, pos_t column, int character, double rotation_angle, int cell_width, int cell_height)
{
	print(line, column, our_fg_colour, our_bg_colour, character, rotation_angle, 1.0, cell_width, cell_height);
}

void MyGraphics_render::print(pos_t line, pos_t column, simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle)
{
	SDL_Colour selected;
	get_rgb_from_simple_colour(&selected, bg_colour);

	print(line, column, fg_colour, selected, character, rotation_angle);
}

void MyGraphics_render::print(simple_colour_t fg_colour, const SDL_Colour& bg_colour, int character, double rotation_angle)
{
	// assume this function is only used for text...
	print(current_line, current_column, fg_colour, bg_colour, character, rotation_angle);
}

void MyGraphics_render::print(simple_colour_t fg_colour, simple_colour_t bg_colour, int character, double rotation_angle)
{
	// assume this function is only used for text...
	print(current_line, current_column, fg_colour, bg_colour, character, rotation_angle);
}

void MyGraphics_render::print(int character, double rotation_angle)
{
	print(current_line, current_column, character, rotation_angle);
}



bool check_line_for_colour(SDL_Surface* surface, SDL_Color sdl_colour, int line_to_check)
{
	if(SDL_MUSTLOCK(surface)) { SDL_LockSurface(surface); }

	Uint32 colour = ((sdl_colour.a << surface->format->Ashift) & surface->format->Amask) |
					((sdl_colour.b << surface->format->Bshift) & surface->format->Bmask) |
					((sdl_colour.g << surface->format->Gshift) & surface->format->Gmask) |
					((sdl_colour.r << surface->format->Rshift) & surface->format->Rmask);


	int width = surface->w;
	for(int col = 0; col < width; col++)
	{
		Uint32 pixel = getpixel(surface, col, line_to_check);
		if(pixel == colour)
		{
			if(SDL_MUSTLOCK(surface)) { SDL_UnlockSurface(surface); }
			return true;
		}
	}
	if(SDL_MUSTLOCK(surface)) { SDL_UnlockSurface(surface); }
	return false;
}


void MyGraphics_render::load_textures_from_glyph_set(luabridge::LuaRef glyph_set)
{
	if(not glyph_set["load_from_file"] or
       not glyph_set["character_base_code"].isNumber() or
        not glyph_set["glyph_set_lines"].isNumber() or
        not glyph_set["glyph_size"].isNumber() or
        not glyph_set["characters_per_line"].isNumber())
	{
		Utilities::fatalError("Invalid glyph set");
		return;
	}
    
	SDL_Surface* surface = load_glyph_file(glyph_set);
    
	if(surface == 0 /*nullptr*/)
	{
		Utilities::fatalError("Failed to load glyphs");
		return;
	}

    GameTexInfo gti;
    gti.number_lines = glyph_set["glyph_set_lines"];
    gti.glyph_size = glyph_set["glyph_size"];
    gti.characters_per_line = glyph_set["characters_per_line"];
    
#if 0
        /* set the transparent color for the bitmap font (hot pink) */
        int error = SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format,
                                                           hot_pink_means_transparent.r,
                                                           hot_pink_means_transparent.g,
                                                           hot_pink_means_transparent.b));
        if(error)
        {
            Utilities::debugMessage("SDL_SetColorKey<2> error: %s\n", SDL_GetError());
            Utilities::fatalErrorSDL(std::string(SDL_GetError()));
        }

        /* now we convert the surface to our desired pixel format */
        int format = SDL_PIXELFORMAT_ABGR8888;  /* desired texture format */
        Uint32 Rmask, Gmask, Bmask, Amask;      /* masks for desired format */
        int bpp;                /* bits per pixel for desired format */
        int success = SDL_PixelFormatEnumToMasks(format,&bpp,&Rmask,&Gmask,&Bmask,&Amask);
        if(!success)
        {
            Utilities::fatalError("SDL_PixelFormatEnumToMasks error: %s\n", SDL_GetError());
        }

        SDL_Surface *converted = SDL_CreateRGBSurface(0, surface->w, surface->h,
                                                      bpp, Rmask, Gmask,Bmask, Amask);
        if(!converted)
        {
            Utilities::fatalError("SDL_CreateRGBSurface error: %s\n", SDL_GetError());
            //continue;	// memory leak
        }

        // can that previous call return NULL? Yes, probably...
        error = SDL_BlitSurface(surface, NULL, converted, NULL);
        if(error)
        {
            Utilities::debugMessage("SDL_BlitSurface<2> error: %s\n", SDL_GetError());
            Utilities::fatalErrorSDL(std::string(SDL_GetError()));
        }
#endif
    
        /* create our texture */
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, glyph_set["render_quality"]);
        gti.texture = shared_SDL_Texture(SDL_CreateTextureFromSurface(renderer, surface), Delete_SDLTexture);
        //gti.texture = SDL_CreateTextureFromSurface(renderer, converted);

        if (gti.texture == 0)
        {
            Utilities::debugMessage("texture creation failed: %s\n", SDL_GetError());
            Utilities::fatalErrorSDL(std::string(SDL_GetError()));
        }
        else
        {
            /* set blend mode for our texture */
            SDL_SetTextureBlendMode(gti.texture.get(), SDL_BLENDMODE_BLEND);
        }

        SDL_FreeSurface(surface);
        //SDL_FreeSurface(converted);

    set_GameTexInfo(glyph_set["character_base_code"], gti);
}


// modified from keyboard.c in SDL demo projects for iphone
void MyGraphics_render::create_texture_set(luabridge::LuaRef glyph_set)
{
	if(
        not glyph_set["character_base_code"].isNumber() or
        not glyph_set["glyph_set_lines"].isNumber() or
        not glyph_set["glyph_size"].isNumber() or
        not glyph_set["characters_per_line"].isNumber())
	{
		Utilities::fatalError("Invalid parameters for create_texture_set()");
		return;
	}

    GameTexInfo gti;
    gti.number_lines = glyph_set["glyph_set_lines"];
    gti.glyph_size = glyph_set["glyph_size"];
    gti.characters_per_line = glyph_set["characters_per_line"];

	//for(int colour = 0; colour < NUMBER_OF_COLOURS; colour++)
	//{
		/* create our texture */

		gti.texture = shared_SDL_Texture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, gti.characters_per_line*gti.glyph_size, gti.number_lines*gti.glyph_size), Delete_SDLTexture);

		if (gti.texture == 0)
		{
			Utilities::debugMessage("texture creation failed: %s\n", SDL_GetError());
			Utilities::fatalErrorSDL(std::string(SDL_GetError()));
		}
		else
		{
			/* set blend mode for our texture */
			SDL_SetTextureBlendMode(gti.texture.get(), SDL_BLENDMODE_BLEND);
		}

    set_GameTexInfo(glyph_set["character_base_code"], gti);
}

void MyGraphics_render::update_texture_set_pixel(int base_character_code, int glyph, int glyph_x, int glyph_y, Uint32 pixel)
{
    GameTexInfo* gti = get_GameTexInfo(base_character_code);
	if(not gti)
	{
		Utilities::fatalError("Invalid texture set in update_texture_set_pixel()");
		return;
	}

	int glyph_size = gti->glyph_size;

	SDL_Rect location;
	location.x = (glyph * glyph_size) + glyph_x;
	location.y = glyph_y;
	location.w = 1;
	location.h = 1;

	//int format = SDL_PIXELFORMAT_ABGR8888;  /* desired texture format */
	// no SDL function to convert desired formats to shifts, so hardcoded here...
	const Uint32 transparent_pixel = (hot_pink_means_transparent.a << 24) +
									 (hot_pink_means_transparent.b << 16) +
									 (hot_pink_means_transparent.g << 8) +
									 (hot_pink_means_transparent.r);

	for(int colour = 0; colour < NUMBER_OF_COLOURS; colour++)
	{
		Uint32 newpixel = pixel;

        if (pixel == transparent_pixel)
		{
			newpixel = 0x00000000;		// a pixel with a zero alpha channel
		}

		// write the pixel directly into the texture
		SDL_UpdateTexture(gti->texture.get(), &location, &newpixel, 4);

	}

}

void MyGraphics_render::update_texture_set_glyph(int base_character_code, int glyph, int source_glyph_size, SDL_Surface* source, int source_glyph, int source_characters_per_line)
{
    GameTexInfo* gti = get_GameTexInfo(base_character_code);
    if(not gti)
	{
		Utilities::fatalError("Invalid texture set in update_texture_set_glyph()");
		return;
	}

    int characters_per_line = source_characters_per_line;
    int location_glyph_size = gti->glyph_size;

    if(location_glyph_size < source_glyph_size)
	{
		Utilities::fatalError("Buffer glyph size is too small in update_texture_set_glyph()");
		return;
	}

	SDL_Rect location;
	location.x = (glyph * location_glyph_size);
	location.y = 0;
	location.w = source_glyph_size;
	location.h = source_glyph_size;

	SDL_Rect source_rect;
	source_rect.x = (source_glyph % characters_per_line) * source_glyph_size;
	source_rect.y = (source_glyph / characters_per_line) * source_glyph_size;
	source_rect.w = source_glyph_size;
	source_rect.h = source_glyph_size;


		SDL_Surface* surface = source;

		if(!surface)
		{
			Utilities::fatalError("Surface creation failed: %s\n", SDL_GetError());
		}

		/* set the transparent color for the bitmap font (hot pink) */
		int error = SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format,
														   hot_pink_means_transparent.r,
														   hot_pink_means_transparent.g,
														   hot_pink_means_transparent.b));
		if(error)
		{
			Utilities::debugMessage("SDL_SetColorKey<2> error: %s\n", SDL_GetError());
			Utilities::fatalErrorSDL(std::string(SDL_GetError()));
		}


		/* now we convert the surface to our desired pixel format */
		int format = SDL_PIXELFORMAT_ABGR8888;  /* desired texture format */
		Uint32 Rmask, Gmask, Bmask, Amask;      /* masks for desired format */
		int bpp;                /* bits per pixel for desired format */
		int success = SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
		if(!success)
		{
			Utilities::fatalError("SDL_PixelFormatEnumToMasks error: %s\n", SDL_GetError());
		}

		SDL_Surface *converted = SDL_CreateRGBSurface(0, gti->glyph_size, gti->glyph_size, bpp, Rmask, Gmask,Bmask, Amask);
		if(!converted)
		{
			Utilities::fatalError("SDL_CreateRGBSurface error: %s\n", SDL_GetError());
		}


		error = SDL_BlitSurface(surface, &source_rect, converted, NULL);
		if(error)
		{
			Utilities::debugMessage("SDL_BlitSurface<2> error: %s\n", SDL_GetError());
			Utilities::fatalErrorSDL(std::string(SDL_GetError()));
		}




		//void* source_pointer = source->pixels + (source_rect.y * source->pitch) + (source_rect.x * source->format->BytesPerPixel);

		// write the pixel directly into the texture
		SDL_UpdateTexture(gti->texture.get(), &location, converted->pixels, converted->pitch);

		SDL_FreeSurface(converted);
		SDL_FreeSurface(surface);


}


void MyGraphics_render::clear_screen(const SDL_Colour& colour)
{
    /* draw the background, we'll just paint over it */

	int error1 = SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, SDL_ALPHA_OPAQUE);
	if(error1) { Utilities::fatalErrorSDL("MyGraphics_render::clear_screen SDL_SetRenderDrawColor Error ="); }

	// don't paint a rectangle, clear the renderer properly...

    //SDL_RenderFillRect(renderer, NULL);
	int error2 = SDL_RenderClear(renderer);
	if(error2) { Utilities::fatalErrorSDL("MyGraphics_render::clear_screen SDL_RenderClear Error ="); }
}


void MyGraphics_render::set_viewport(Viewport& vp)
{
    static bool error_before = false;
    int err = SDL_RenderSetClipRect(renderer, &vp.rect);
    if(err and not error_before) { // what do we do here?
        Utilities::debugMessage("SDL_RenderSetClipRect returned an error?");
        Utilities::debugMessage(SDL_GetError());
        error_before = true;
    }

	viewport = vp;
}


void MyGraphics_render::FillRectSimple(const SDL_Rect& rect)
{
	SDL_RenderFillRect(renderer, &rect);
}

void MyGraphics_render::FillRectColour(const SDL_Colour& colour,
                                       const SDL_Rect& rect)
{
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
    		colour.a);

    FillRectSimple(rect);
}

void MyGraphics_render::FillRect(const SDL_Colour& colour,
                                       double x1, double y1, double x2, double y2)
{
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
    		colour.a);

    if(viewport.draw_mode == Viewport::cell_based)
    {
        x1 *= viewport.cell_size;
        y1 *= viewport.cell_size;
        x2 *= viewport.cell_size;
        y2 *= viewport.cell_size;
    }
    const SDL_Rect rect = { static_cast<int>(x1) + viewport.rect.x + viewport.origin_x,
        static_cast<int>(y1) + viewport.rect.y + viewport.origin_y,
    		static_cast<int>(x2-x1), static_cast<int>(y2-y1) };
    FillRectSimple(rect);
}

void MyGraphics_render::DrawRect(const SDL_Colour& colour,
                                       double x1, double y1, double x2, double y2)
{
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
                           colour.a);

    if(viewport.draw_mode == Viewport::cell_based)
    {
        x1 *= viewport.cell_size;
        y1 *= viewport.cell_size;
        x2 *= viewport.cell_size;
        y2 *= viewport.cell_size;
    }
    const SDL_Rect rect = { static_cast<int>(x1)  + viewport.rect.x + viewport.origin_x,
        static_cast<int>(y1)  + viewport.rect.y + viewport.origin_y,
        static_cast<int>(x2-x1), static_cast<int>(y2-y1) };
    SDL_RenderDrawRect(renderer, &rect);
}

void MyGraphics_render::DrawAbsoluteRect(const SDL_Colour& colour,
                                       int x1, int y1, int x2, int y2)
{
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
                           colour.a);

    const SDL_Rect rect = { x1,y1,x2-x1,y2-y1 };
    SDL_RenderDrawRect(renderer, &rect);
}

void MyGraphics_render::DrawLine(const SDL_Colour& colour,
                                       double x1, double y1, double x2, double y2)
{
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
                           colour.a);
    
    if(viewport.draw_mode == Viewport::cell_based)
    {
        x1 *= viewport.cell_size;
        y1 *= viewport.cell_size;
        x2 *= viewport.cell_size;
        y2 *= viewport.cell_size;
    }
    int X1 = static_cast<int>(x1)  + viewport.rect.x + viewport.origin_x;
    int Y1 = static_cast<int>(y1)  + viewport.rect.y + viewport.origin_y;
    int X2 = static_cast<int>(x2);
    int Y2 = static_cast<int>(y2);
    const SDL_Rect srect = { X1, Y1, X2-X1, Y2-Y1 };
    SDL_RenderDrawLine(renderer, srect.x, srect.y, srect.x+srect.w, srect.y+srect.h);
}

void MyGraphics_render::DrawPoint(const SDL_Colour& colour,
                                        double x, double y)
{
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b,
                           colour.a);
    
    if(viewport.draw_mode == Viewport::cell_based)
    {
        x *= viewport.cell_size;
        y *= viewport.cell_size;
    }
    int x1 = x  + viewport.rect.x + viewport.origin_x;
    int y1 = y  + viewport.rect.y + viewport.origin_y;
    SDL_RenderDrawPoint(renderer, x1, y1);
}



void MyGraphics_render::RenderCopy(SDL_Texture* texture, SDL_Rect* source, SDL_Rect* dest)
{
	int error = 0;
#if WARNING_ABOUT_SDL_SETTEXTURECOLORMOD_ERROR
    error =
#endif
    SDL_SetTextureColorMod(texture, 255, 255, 255);
#if WARNING_ABOUT_SDL_SETTEXTURECOLORMOD_ERROR
    if(error) { Utilities::fatalErrorSDL("SDL_SetTextureColorMod", error); }
#endif
    
	// dest is a copy, so we are safe to modify it
	if(dest)
	{
		error = SDL_RenderCopy(renderer, texture, source, dest);
	}
	else
	{
		error = SDL_RenderCopy(renderer, texture, source, NULL);
	}

	if(error)
	{
		Utilities::debugMessage("Error in RenderCopy %s", SDL_GetError());
	}
}

void MyGraphics_render::SetTextureAlphaMod(int base_character_code, Uint8 alpha)
{
    GameTexInfo* gti = get_GameTexInfo(base_character_code);
    if(not gti)
    {
        Utilities::fatalError("Invalid texture set in SetTextureAlphaMod()");
        return;
    }

    SDL_SetTextureAlphaMod(gti->texture.get(), alpha);
}

