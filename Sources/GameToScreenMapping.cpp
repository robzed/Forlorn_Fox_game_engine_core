/*
 *  GameToScreenMapping.cpp
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

#include "GameToScreenMapping.h"
#include "GameConfig.h"	// required for GAME_WIDTH, GLYPH_SIZE_SCREEN, SCREEN_WIDTH
#include "SDL.h"
#ifdef Windows
#include "image_loader.h"
#endif
#include "Utilities.h"
#include <iostream>
#include <stdio.h>
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include "LuaCppInterface.h"
#include "lua.h"
#include "GameApplication.h"
#include <cmath>


// the full window size
static int window_width_in_pixels;
static int window_height_in_pixels;
static int window_diagonal;
static int window_width_in_screen_coords;
static int window_height_in_screen_coords;

// where to draw initial window on a desktop
static int window_x_start;
static int window_y_start;

static SDL_Window *game_window = NULL;

#ifdef Desktop
static bool fullscreen = false;
#else
static bool fullscreen = true;
#endif

static void convert_screen_coords_to_pixels(double &x, double &y)
{
    x /= window_width_in_screen_coords;
    y /= window_height_in_screen_coords;
    x *= window_width_in_pixels;
    y *= window_height_in_pixels;
}

// Use SDL to detect the size of the screen available to us,
// and calculate the game size and offset, sprite size etc
// Call at startup and when we detect the screen being resized or rotated
void UpdateScreenData(SDL_Window* window)
{
    SDL_GetWindowSize(window, &window_width_in_screen_coords, &window_height_in_screen_coords);
    SDL_GL_GetDrawableSize(window, &window_width_in_pixels, &window_height_in_pixels);

	window_diagonal = sqrt(window_width_in_pixels*window_width_in_pixels + window_height_in_pixels*window_height_in_pixels);

	//Utilities::debugMessage("SetUpScreen(): window_width=%d window_height=%d", window_width_in_pixels, window_height_in_pixels);
}

int GetWindowWidth()
{
	return window_width_in_pixels;
}

int GetWindowHeight()
{
	return window_height_in_pixels;
}


void UpdateScreenData_game_window()
{
    UpdateScreenData(game_window);
}


void GetInitialScreenSettings()
{
	// get the screen size from SDL...
#if 0
    Utilities::debugMessage("Available resolutions:\n");
	int screen = 0;
	int modes = SDL_GetNumDisplayModes(screen);
	for (int i = 0; i < modes; i++) {
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(screen, i, &mode);
		Utilities::debugMessage("%dx%d\n", mode.w, mode.h);
	}
    Utilities::debugMessage("-----------\n");
#endif


    SDL_DisplayMode desktop_display_mode;


    // query SDL here and form a window about 80% of the screen size or something
    // NOTE: This is in screen coords, not pixels at this point...

    int error = SDL_GetDesktopDisplayMode(0, &desktop_display_mode);
    if(error)
    {
        Utilities::fatalErrorSDL(std::string("GetInitialScreenSettings() SDL_GetDesktopDisplayMode()") + std::string(SDL_GetError()));
    }

    if(fullscreen)
    {
        window_width_in_screen_coords = desktop_display_mode.w;
        window_height_in_screen_coords = desktop_display_mode.h;
        window_x_start = 0;
        window_y_start = 0;
    }
    else
    {
        window_width_in_screen_coords = desktop_display_mode.w * 0.875;
        window_height_in_screen_coords = desktop_display_mode.h * 0.875;
        window_x_start = SDL_WINDOWPOS_CENTERED;
        window_y_start = SDL_WINDOWPOS_CENTERED;
    }

    // we don't have the pixels yet...
	//window_diagonal = sqrt(window_width_in_pixels*window_width_in_pixels + window_height_in_pixels*window_height_in_pixels);
}

float GetWindowDiagonalInches()
{
	float ddpi = 0.0;
	float hdpi = 0.0;
	float vdpi = 0.0;

    // @todo: iPhone simulator returns error - is this true on a phone?
    int err = SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	if(0 == err)
	{
        //printf("DIAG %f %f %f\n", ddpi, hdpi, vdpi);
		if(ddpi != 0.0)
		{
			return window_diagonal / ddpi;
		}

		if(hdpi != 0.0)
		{
			return window_diagonal / hdpi;
		}

		if(vdpi != 0.0)
		{
			return window_diagonal / vdpi;
		}
	}
    //printf("SDL_GetDisplayDPI Error: %s\n", SDL_GetError());

	return 0.0;
}

bool isFullScreen()
{
    return fullscreen;
}

bool ToggleFullscreen()
{
#ifdef Desktop
	fullscreen = not fullscreen;

	if(fullscreen)
	{
		SDL_DisplayMode desktop_display_mode;

		SDL_GetDesktopDisplayMode(0, &desktop_display_mode);
		SDL_SetWindowDisplayMode(game_window, &desktop_display_mode);
		SDL_SetWindowSize(game_window, desktop_display_mode.w, desktop_display_mode.h);

		if (SDL_SetWindowFullscreen(game_window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
		{
			Utilities::debugMessage("Unable to switch window mode: %s", SDL_GetError());
			SDL_ClearError();
		}

		UpdateScreenData(game_window);
	}
	else
	{
		int w, h;

		if (SDL_SetWindowFullscreen(game_window, 0) != 0)
		{
			Utilities::debugMessage("Unable to switch window mode: %s", SDL_GetError());
			SDL_ClearError();

			fullscreen = not fullscreen;
			return false;
		}

        SDL_DisplayMode desktop_display_mode;
        SDL_GetDesktopDisplayMode(0, &desktop_display_mode);

        // tailored to work well on 1920x1080 screen, we could do better here by taking into account
        // glyph size and level size
        w = desktop_display_mode.w * 0.55;
        h = w * 0.75;						// 4:3 aspect ratio

        if(h > desktop_display_mode.h * 0.9)
        {
            double ratio = (desktop_display_mode.h * 0.9) / h;
            w *= ratio;
            h *= ratio;
        }

		SDL_SetWindowSize(game_window, w, h);
		SDL_SetWindowPosition(game_window, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
		UpdateScreenData(game_window);
	}
#endif
	return true;
}

void SetWindowTitle(const std::string title)
{
	SDL_SetWindowTitle(game_window, title.c_str());
}

bool CreateGameWindow()
{
	/* Get the screen size, game size and offsets */
	GetInitialScreenSettings();

#ifdef Desktop
    Uint32 flags = 	 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	if(fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#elif defined iPhone
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
//    if(fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if(fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
#else
	Uint32 flags = 	 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
#endif
    //printf("fullscreen=%i\n", (int)fullscreen);


	// Changed title for Mac version from NULL. Hopefully iPhone won't mind...
	game_window = SDL_CreateWindow("ForlornFox Engine",
					 window_x_start, window_y_start,
					 window_width_in_screen_coords, window_height_in_screen_coords,
					 flags
					 );

	if(!game_window)
	{
		Utilities::debugMessage("%s\n", SDL_GetError());
		return false;
	}

	// If windows, set the window icon
	// Win7 doesn't use the embedded application icon as the default
	// icon at runtime. It seems you need to set it.
#ifdef Windows
	SDL_Surface* icon = load_image("icon_32x32.png");
	if(icon != NULL)
	{
		SDL_SetWindowIcon(game_window, icon);
	}
#endif

	// Set up the window to work in fullscreen mode too
	SDL_DisplayMode desktop_display_mode;
	if(SDL_GetDesktopDisplayMode(0, &desktop_display_mode) == 0)    // don't set on failure
    {
        SDL_SetWindowDisplayMode(game_window, &desktop_display_mode);
    }

	UpdateScreenData(game_window);
	return true;
}




SDL_Window* GetGameWindow()
{
	return game_window;
}


// We simulate the landscape mode in order to avoid figuring out or fixing the
// SDL1.3.0 5557 build. I think it will be easier :-)
//


// iOS used to requires the app to handle screen orientation itself

// The YouTube application is landscape left, and according to this
// landscape left is more popular. Ideally we'd want to support landscape left
// and landscape right. But it would be nicer to do this via SDL...
// http://www.greengar.com/2010/04/usc-landscape/



void screen_to_game(int &x, int &y)
{
    double xd = x, yd = y;
    convert_screen_coords_to_pixels(xd, yd);
    x = xd; y= yd;
}




