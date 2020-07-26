/*
 *  SaveDataPath.h
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

#ifndef SAVE_DATA_PATH_H
#define SAVE_DATA_PATH_H

#include <string>

// Uses SDL_GetBasePath() to find where to save stuff is (like preferences, 
// save games, etc.)
// Class manages ownership of string and game specifics.
// This directory is unique per user and per application.
//
// Also does path seperator transformation so we don't need to create folders
// in the preferences directory. 
//
// Also transforms all '/' style slashes to '\' on Windows.
//
class SaveDataPath
{
public:
	SaveDataPath(std::string file_name);
	virtual ~SaveDataPath();
	// c_str - The pointer returned may be invalidated by further calls to other member functions that modify the object.
	// NOTE: c_str() will return 0 (NULL) if there can be no saving!
	// NOTE: str() will return "" if there can be no saving!
	virtual const char* c_str();		// same lifetime as std::string::c_str()
	virtual std::string str();
	static void override(std::string base_path);
   static void set_subfolder(std::string folder_name);
   static void set_org_name(std::string folder_name);
   static void set_app_name(std::string app_name);
   static const char* get_base_pref_path();
private:
	std::string path_to_file;
	static std::string root_data_path;
	static bool failed;
   static std::string stored_subfolder_name;
   static std::string org_name;
   static std::string app_name;
};


#endif
