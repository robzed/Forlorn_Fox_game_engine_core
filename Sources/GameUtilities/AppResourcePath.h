/*
 *  AppResourcePath.h
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
 */

#ifndef APP_RESOURCE_PATH_H
#define APP_RESOURCE_PATH_H

#include <string>
//class SaveDataPath;

// Uses SDL_GetBasePath() to find where the application 
// resources are. Class manages ownership of string and game specifics.
//
// *** Do not save in this directory ***
//
// NOTE: Do *NOT* use this path for saving. On some platforms that will
// be prohibited. Use SaveDataPath instead.
//
// Also transforms all '/' style slashes to '\' on Windows.
//

class AppResourcePath
{
public:
	AppResourcePath(std::string file_name);
	virtual ~AppResourcePath();
	// c_str - The pointer returned may be invalidated by further calls to other member functions that modify the object.
	// NOTE: c_str() will *never* return 0 (NULL)
	virtual const char* c_str();		// same lifetime as std::string::c_str()
	virtual std::string str();
	static void override(std::string base_path);
private:
	AppResourcePath(const AppResourcePath&);
	AppResourcePath& operator=(const AppResourcePath& );
	AppResourcePath();
	std::string path_to_file;
	static std::string root_app_path;
};

#endif
