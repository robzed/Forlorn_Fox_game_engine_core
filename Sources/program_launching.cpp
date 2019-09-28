//
//  program_launching.cpp
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

#include "program_launching.h"
#include "GameConfig.h"
#include <vector>
#include <string>

#ifdef Desktop
#ifdef Windows

#include <process.h>
#include <errno.h>


#elif defined(MacOSX) || defined(__LINUX__)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


#endif
#endif

/*
void switch_to_program_c(const char* exe_path, int argc, const char* argv[])
{
#ifdef Desktop
#if defined(MacOSX)
    // ensure there is always space for the name
    if(argc < 1) { argc = 1; }
    
    // +1 is for NULL terminator on argv
    //our_argv = new const char*[argc+1];
    const char* our_argv[argc+1];           // this might not be valid C++ :-)
    
    // make sure first is executable
    our_argv[0] = exe_path;
    
    // copy other arguments
    for(int i=1; i<argc; i++)
    {
        out_argv[i] = argv[i];
    }
    // ensure the end has a NULL terminator, as per (C++03 §3.6.1/2)
    out_argv[argc] = NULL;
    
    // run it ... don't care about the return value
    execv(exe_path, argv);      // only returns if there is an error!
    perror("Execv error");
    
    // delete the arguments
    // delete[] our_argv;
#endif
#endif
}
*/


int switch_to_program(lua_State*L)
{
#ifdef Desktop
#if defined(Windows) || defined(MacOSX) || defined(__LINUX__)
    
    // For Mac, maybe we want:
    //      'open' from a shell
    //      Launch Services for C.  https://developer.apple.com/documentation/coreservices/launch_services
    //      NSWorkspace & NSTask in Cocoa.
    // e.g. [[NSWorkspace sharedWorkspace] launchApplication:@"pathToApplication"];
    //
    
    // ensure there is always space for the name
    int argc = lua_gettop(L);
    if(argc < 1)
    {
        lua_pushnil(L);
        lua_pushstring(L, "No executable supplied");
        return 2;
    }
    
    // +1 is for NULL terminator on argv
    const char* dest_argv[argc+1];           // this might not be valid C++ :-)

    const char* exec_path = luaL_checkstring(L ,1);

    dest_argv[0] = exec_path;
    for(int i = 1; i < argc; i++)   // don't run if argc = 0 or 1
    {
        dest_argv[i] = luaL_checkstring(L, i+1); // start with second Lua parameter
    }
    // if argc = 2 (exec plus one parameter param1), then c array looks like:
    // { &exe_path, &param1, NULL };
    dest_argv[argc] = NULL;
    
    // Execv const-safey for second parameter...
    // https://stackoverflow.com/questions/36925388/can-i-pass-a-const-char-array-to-execv
    // https://stackoverflow.com/questions/32229637/cannot-convert-const-char-to-const-char
    // https://stackoverflow.com/questions/190184/execv-and-const-ness
    // https://stackoverflow.com/questions/5797837/how-to-pass-a-vector-of-strings-to-execv
    // https://stackoverflow.com/questions/14960969/constructing-charconst-from-string
    //C does not have a wholly satisfactory way to express the degree of const-ness that POSIX requires execv() to provide for the arguments. POSIX guarantees that the function will not change either the pointers in argv or the strings to which they point.
    // We could cast - But I don't want to.
    //std::vector<std::string> myvec;
    //char* argv[argc+1];
    //for (int i = 0;  i < argc;  ++i)
    //{
    //    myvec.push_back(dest_argv[i]);
    //    argv[i] = myvec[i].c_str();
    //}
    //argv[argc] = NULL;  // end of arguments sentinel is NULL

#if defined(MacOSX) || defined(__LINUX__)
    execv(exec_path, (char**)dest_argv);      // only returns if there is an error!
#elif defined(Windows)
    _execv(exec_path, (char**)dest_argv);      // only returns if there is an error!
#endif
    perror("Execv error");
    
    lua_pushnil(L);
    lua_pushstring(L, "Failed to switch");
    return 2;

#else
    // unknown platform - not supported
    lua_pushnil(L);
    lua_pushstring(L, "Unknown Platform");
    return 2;
#endif
#else
    // no support on non-Desktop platforms
    lua_pushnil(L);
    lua_pushstring(L, "Unsupported on Mobile");
    return 2;
#endif

}
                      
            
#if 0
int launch_program(lua_State*L)
{
#ifdef Desktop
#ifdef Windows
    //CreateProcess()
    lua_pushnil(L);
    lua_pushstring(L, "Windows not implemented yet");
    return 2;
#elif defined(MacOSX)

    // For Mac, maybe we want:
    //      'open' from a shell
    //      Launch Services for C.  https://developer.apple.com/documentation/coreservices/launch_services
    //      NSWorkspace & NSTask in Cocoa.
    // e.g. [[NSWorkspace sharedWorkspace] launchApplication:@"pathToApplication"];
    //
    
    pid_t processId;
    if ((processId = fork()) == 0)  // if 0, then we are child
    {
        const char* exec_path = luaL_checkstring(L ,1);
        /*
        luaL_checktype(L, 2, LUA_TTABLE);
        int argc = luaL_len(L, 2);
        const char** argv = new const char* const[argc+2];
        argv[0] = exe_path;
        for(int i = 0; i < argc; i++)
        {
            lua_pushinteger(L, i+1); // lua indexes start at 1
            lua_gettable(L, -2);
            const char* const arg = luaL_checkstring(L, -1);
            // "Because Lua has garbage collection, there is no guarantee that the pointer returned by lua_tolstring will be valid after the corresponding value is removed from the stack."
            argv[i+1] = arg;
            lua_pop(L, 1);
        }
         argv[argc+1] = NULL;
         */
        int argc = lua_gettop(L);
        const char** argv = new const char* const[argc+1];  // +1 for NULL
        argv[0] = exe_path;
        for(int i = 1; i < argc; i++)   // don't run if argc = 0 or 1
        {
            argv[i] = luaL_checkstring(L, i+1); // start with second Lua parameter
        }
        // if argc = 2 (exec plus one parameter param1), then c array looks like:
        //const char * const argv[] = { exe_path, param1, NULL };
        argv[argc] = NULL;
        
        
        execv(exe_path, argv);      // only returns if there is an error!
        //if (execv(exe_path, argv) < 0)
        //{
            perror("Execv error");
            exit(-1);                // leave the child
            // @todo: need to signal the parent process... how do we do this?
            // execv will be -1, but errno will contain error
        //}
        
        // we should never get here (either we've exited or replaced the old process)
        delete argv;
    }
    else if (processId < 0) // -1 means failure to fork
    {
        errno_t err = errno;
        perror("Fork error");
        lua_pushnil(L);
        lua_pushstring(L, "Fork Error");
        lua_pushinteger(L, err);
        return 3;
    }
    else
    {
        lua_pushinteger(L, processId);
        return 1;
    }
    
    lua_pushnil(L);
    lua_pushstring(L, "Unreachable code path");
    return 2;

    
#elif defined(__LINUX__)
    lua_pushnil(L);
    lua_pushstring(L, "Linux not implemented yet");
    return 2;
#else
    // unknown platform - not supported
    lua_pushnil(L);
    lua_pushstring(L, "Unknown Platform");
    return 2;
#endif
#else
    // no support on non-Desktop platforms
    lua_pushnil(L);
    lua_pushstring(L, "Unsupported on Mobile");
    return 2;
#endif

    
}

#endif

