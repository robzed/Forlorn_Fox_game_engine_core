/*
 *  LoadPath.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 02/03/2014.
 * ------------------------------------------------------------------------------
 * Copyright (c) 2014 Rob Probin and Tony Park
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

// this will return the a file in the preference directory IF IT EXISTS
// otherwise will return a path to the application directory. 
//
// Does not work for all instances - e.g. when searching lots of directories
// e.g. luaconf.h
//
// *** Do not save in this directory *** (Use SaveDataPath for all saves)
//
#ifndef LOAD_PATH_H
#define LOAD_PATH_H

#include <string>

class LoadPath
{
public:
	LoadPath(std::string file_name);
	virtual ~LoadPath();
	// c_str - The pointer returned may be invalidated by further calls to other member functions that modify the object.
	//
	// NOTE: c_str() will *never* return 0 (NULL)
	//
	// NOTE2: We resolve the actual path as late as possible, each time the 
	// LoadPath is returned as to a string. This might result in a different 
	// file path for each c_str/str call - but that means we definately have as 
	// close to the latest information as possible. Also minimises the chance 
	// that a file is deleted from the save path (preferences) before we do 
	// stuff to it.
	virtual const char* c_str();		// same lifetime as std::string::c_str()
	virtual std::string str();
	// no override - uses AppResourcePath and SaveDataPath folders
private:
	std::string resolve();
	// prohibit these
	LoadPath(const LoadPath&);
	LoadPath& operator=(const LoadPath& );
	LoadPath();
	// member data
	std::string filename_store;
	std::string last_full_filename;
};

#endif
