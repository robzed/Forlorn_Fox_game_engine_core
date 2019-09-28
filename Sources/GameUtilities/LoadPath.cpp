/*
 *  LoadPath.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 02/03/2014.
 *
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

#include "LoadPath.h"
#include "AppResourcePath.h"
#include "SaveDataPath.h"
#include "Utilities.h"
#include <stdio.h>

//#define DEBUG_RESOURCE_PATHS

LoadPath::LoadPath(std::string file_name)
: filename_store(file_name)
{
}

LoadPath::~LoadPath()
{
}

const char* LoadPath::c_str()
{
	// last full filename avoids the situation where a temporary disappears from scope
	last_full_filename = resolve();
	return last_full_filename.c_str();
}

std::string LoadPath::str()
{
	// we resolve as late as possible
	last_full_filename = resolve();
	return last_full_filename;
}

std::string LoadPath::resolve()
{
	// look for the file in the preferences first...
	SaveDataPath f1(filename_store);
	const char* s = f1.c_str();
	if(s)
	{
		// We know we are not creating it, so no security race condition.
		// Theoretically, change of file deletion before the operation,
		// but that will result in no file read - which will just cause
		// a file-not-found problem.
		FILE* file;
		file = fopen(s, "r");

		if(file)
		{
			fclose(file);	// close the file
#ifdef DEBUG_RESOURCE_PATHS
			Utilities::debugMessage("LoadPath::resolve() found file at SaveDataPath %s",s);
#endif
			return f1.str();

		}
	}

	AppResourcePath f2(filename_store);

#ifdef DEBUG_RESOURCE_PATHS
	Utilities::debugMessage("LoadPath::resolve() used AppResourcePath %s",f2.c_str());
#endif
	return f2.str();
}

