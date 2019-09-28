/*
 *  AppResourcePath.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 30/11/2013.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2013 Rob Probin and Tony Park
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
 *
 */

#include "AppResourcePath.h"
#include "GameConfig.h"
#include "SDL.h"
#include "Utilities.h"
#include <iostream>
#include <algorithm>

// added this temporarily for the Pandora version, before we update the SDL
#if !SDL_VERSION_ATLEAST(2,0,1)
#define SDL_GetBasePath(x) 0
#warning "Compiling against a pre-SDL v2.0.1 version"
#endif

std::string AppResourcePath::root_app_path;

AppResourcePath::AppResourcePath(std::string file_name)
{
#ifdef __ANDROID__

	path_to_file = SDL_AndroidGetInternalStoragePath()+("/arp/"+file_name);

#else

	bool windows_flag = false;

	if(root_app_path.length() == 0)
	{
		char *base_path = SDL_GetBasePath();
		if (base_path) {
			root_app_path = base_path;
			SDL_free(base_path);
			// Windows path replacement hack
			windows_flag = not root_app_path.empty() and *root_app_path.rbegin() == '\\';
		} else {
			Utilities::debugMessage("AppResourcePath SDL_GetBasePath failed: %s", SDL_GetError());
			root_app_path = "./";
		}
	}

	path_to_file = root_app_path + DATA_DIR + file_name;
	if(windows_flag)
	{
		std::replace(path_to_file.begin(), path_to_file.end(), '/', '\\');
	}

#endif
}

AppResourcePath::~AppResourcePath()
{
	// nothing at the moment
}

const char* AppResourcePath::c_str()
{
	return path_to_file.c_str();
}

std::string AppResourcePath::str()
{
	return path_to_file;
}

void AppResourcePath::override(std::string base_path)
{
	root_app_path = base_path;
}
