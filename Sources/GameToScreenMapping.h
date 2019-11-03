/*
 *  GameToScreenMapping.h
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

#ifndef GAME_TO_SCREEN_MAPPING_H
#define GAME_TO_SCREEN_MAPPING_H
#include "SDL.h"
#include "BasicTypes.h"
#include "lua.h"

void screen_to_game(int &x, int &y);

void GetInitialScreenSettings();
void UpdateScreenData_game_window();

bool ToggleFullscreen();

bool isFullScreen();

// returns the last height in Pixels
int GetWindowWidth();
int GetWindowHeight();

float GetWindowDiagonalInches();

void SetWindowTitle(const std::string title);

bool CreateGameWindow();
SDL_Window* GetGameWindow();

struct Viewport
{
	enum draw_mode_t
	{
		pixel_based,
		cell_based,
	};

	SDL_Rect rect;
	int cell_size;
	draw_mode_t draw_mode;
    int origin_x;
    int origin_y;

	// default all values to something that won't crash anything
	Viewport()
    : cell_size(32)
    , draw_mode(cell_based)
    , origin_x(0)
    , origin_y(0)
	{
        rect.x = rect.y = 0;
        rect.w = 1024;
        rect.h = 768;
	}

	void SetRect(int x, int y, int w, int h) { rect.x = x; rect.y = y; rect.w = w; rect.h = h; }
	void SetCellSize(int s) { cell_size = s; }
    void SetOrigin(int x, int y) { origin_x = x; origin_y = y; }
    void SetPixelCoords()   { draw_mode = pixel_based; }
    void SetCellCoords()    { draw_mode = cell_based; }
};


#endif

