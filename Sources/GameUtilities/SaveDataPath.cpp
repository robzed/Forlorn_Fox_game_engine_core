/*
 *  SaveDataPath.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 30/11/2013.
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
 */

#include "SaveDataPath.h"
#include "SDL.h"
#include <iostream>
#include <algorithm>
#include "GameConfig.h"
#include "Utilities.h"

//#define DEBUG_RESOURCE_PATHS

bool SaveDataPath::failed = false;
std::string SaveDataPath::root_data_path;

std::string SaveDataPath::stored_folder_name = "";

// added this temporarily for the Pandora version, before we update the SDL
#if !SDL_VERSION_ATLEAST(2,0,1)
#define SDL_GetPrefPath(x,y) 0
#warning "Compiling against a pre-SDL v2.0.1 version"
#endif


const char* SaveDataPath::get_base_pref_path()
{
    if(not failed and root_data_path.length() == 0)
    {
        char *base_path = SDL_GetPrefPath("RedGameWorks", "Escape");
        if (base_path) {
            root_data_path = base_path;
            SDL_free(base_path);
            // Windows path replacement hack
        } else {
            Utilities::debugMessage("SaveDataPath SDL_GetPrefPath failed: %s", SDL_GetError());
            /* Do something to disable writing in-game */
            failed = true;
            return 0;
        }
    }
    return root_data_path.c_str();
}


SaveDataPath::SaveDataPath(std::string file_name)
{
	bool windows_flag = false;
    if( not get_base_pref_path()) { return; }
    windows_flag = not root_data_path.empty() and *root_data_path.rbegin() == '\\';

    if(stored_folder_name == "")
    {
        Utilities::fatalError("SaveDataPath used before set_top_folder set up");
        failed = true;
        return;
    }

	path_to_file = root_data_path + stored_folder_name + '/' + file_name;

	if(windows_flag)
	{
		std::replace(path_to_file.begin(), path_to_file.end(), '/', '\\');
	}	

#if (DEBUG_RESOURCE_PATHS)
	Utilities::debugMessage("SaveDataPath: %s", path_to_file.c_str());
#endif
}

SaveDataPath::~SaveDataPath()
{
	// nothing at the moment
}

const char* SaveDataPath::c_str()
{
	if(failed)
	{
		return 0;	// don't return anything if saving is disabled
	}
	
	return path_to_file.c_str();
}

std::string SaveDataPath::str()
{
	return path_to_file;
}

void SaveDataPath::override(std::string base_path)
{
	root_data_path = base_path;
}

void SaveDataPath::set_top_folder(std::string folder_name)
{
    stored_folder_name = folder_name;
}
