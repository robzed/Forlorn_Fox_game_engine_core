/*
 *  SaveDataPathDeveloper.cpp
 *  Forlorn_Fox_Mac
 *
 *  Created by Rob Probin on 21/01/2015.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2014 Tony Park, Rob Probin.
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
 */

#include "SaveDataPathDeveloper.h"

bool SaveDataPathDeveloper::developer_mode_flag = false;

SaveDataPathDeveloper::SaveDataPathDeveloper(std::string f)
: file(f)
, a(0)
, s(0)
{}

void SaveDataPathDeveloper::set_developer_mode(bool flag)
{
    developer_mode_flag = flag;
}

const char* SaveDataPathDeveloper::c_str()
{
    if(developer_mode_flag)
    {
        if(a == 0) { a = new AppResourcePath(file);}
        return a->c_str();
    }
    else
    {
        if(s == 0) { s = new SaveDataPath(file);}
        return s->c_str();
    }
}
std::string SaveDataPathDeveloper::str()
{
    if(developer_mode_flag)
    {
        if(a == 0) { a = new AppResourcePath(file);}
        return a->str();
    }
    else
    {
        if(s == 0) { s = new SaveDataPath(file);}
        return s->str();
    }
}

