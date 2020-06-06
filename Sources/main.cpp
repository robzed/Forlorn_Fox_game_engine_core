/*
 *	Forlorn Fox
 *	Written by Rob Probin
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2011-2013 Rob Probin and Tony Park
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

#include "SDL.h"
#include "GameApplication.h"
#include "Utilities.h"
#include "AndroidInstaller.h"
//#include "GameToScreenMapping.h"
//#include <stdio.h>
//#include <iostream>
//#include <cstring>
//#include "SaveDataPath.h"
//#include "program_launching.h"
//#include "LuaMain.h"
//#include "LuaCppInterface.h"

#define MAKE_MACRO_STRING(arg) #arg
#define MACRO2(arg) MAKE_MACRO_STRING(arg)
#define FORLORN_FOX_ENGINE_VERSION_STRING MACRO2(FORLORN_FOX_ENGINE_VERSION)
const char* private_engine_version_string = "FORLORN_FOX_ENGINE_VERSION " FORLORN_FOX_ENGINE_VERSION_STRING " ";


// so we can spot when we are on the main thread (where all the UI is)
SDL_threadID main_threadID = 0;

GameApplication* gulp_cpp;

int main(int argc, char *argv[])
{
    Utilities::get_time_since_last_call();

    // initialize basic SDL - can't get paths to load files from app or prefs without it.
    if (SDL_Init(0) != 0) {
        Utilities::debugMessage("Could not initialize SDL\n");
        return 1;
    }

    main_threadID = SDL_ThreadID(); // The SDL file I/O and threading subsystems are initialized by default by SDL_Init(0);

#ifdef __ANDROID__
    CheckAndroidInstallation();
#endif

    gulp_cpp = new GameApplication();
    int return_code = gulp_cpp->main(argc, argv);

    // delete game before SDL_Quit
    delete gulp_cpp;
    /* shutdown SDL */
    SDL_Quit();

    if(return_code == -9999)
    {
        // this is really to avoid optimiser removing string definition.
        Utilities::debugMessage(private_engine_version_string);
    }

    return return_code;
}



