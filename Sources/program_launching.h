//
//  program_launching.h
//  Forlorn_Fox_Mac
//
//  Created by Rob Probin on 24/10/2017.
/*
 * ------------------------------------------------------------------------------
 * Copyright (c) 2017 Rob Probin and Tony Park
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

#ifndef program_launching_h
#define program_launching_h

#include "lauxlib.h"

// launch a program as a fork.
//int launch_program(lua_State*L);

// This only returns if there is an error.
// Currently prints any error to stderr and returns without returning it.
// argv[0] is not used, and the call create a new array with argv[0]=exe_path
// argc should include space for the program. So 1 parameter would be argc=2.
//void switch_to_program_c(const char* exe_path, int argc, const char* argv[]);
int switch_to_program(lua_State*L);

#endif /* program_launching_hpp */
