/*
 *  GameConfig.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 17/01/2013.
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

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "SDL.h"

// Need to detect Windows, OSX, iPhone, Android, Linux builds
// We know we need to do specific stuff for at least Windows,
// iPhone and Android


#ifdef __APPLE__
	// http://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
	// http://www.opensource.apple.com/source/CarbonHeaders/CarbonHeaders-18.1/TargetConditionals.h
	// http://stackoverflow.com/questions/3181321/which-conditional-compile-to-use-to-switch-between-mac-and-iphone-specific-code
	// http://sealiesoftware.com/blog/archive/2010/8/16/TargetConditionalsh.html
	#include "TargetConditionals.h"
	#if TARGET_OS_IPHONE
		#define iPhone
		#define Mobile
	#elif TARGET_IPHONE_SIMULATOR
		// iOS Simulator
		#define iPhone
		#define Mobile
	#elif TARGET_OS_MAC
		// Other kinds of Mac OS
		#define MacOSX
		#define Desktop
	#else
		// Unsupported platform
	#endif
#elif WIN32
	#define Windows
	#define Desktop
#elif __LINUX__
	#define Desktop
#elif __ANDROID__
	#define Mobile
#else
	// other platforms...
	#define Other_platform
#endif


#define DATA_DIR "data/"

static const int cell_size_lower_limit = 8;


// 0.07 - no known history to here :-)
// 0.08 - added volume to sound manager
// 0.09 - Fixed full screen problem
// 0.12 - DEBUG changes
// 0.16 - Tony's changes for the scripting
// 0.17 - Rob's changes to get the version
// 0.20 - Added LuaCommandLineInterpreter patch to Lua
// 0.23 - GlyphEditor / graphics refactoring
// 0.24 - Debug changes
// 0.25 - Mouse movement changes
// 0.26 - Ensure engine changes
// 0.27 - Moved absolute draw list render into Lua
// 0.28 - Added RenderSetClipRect
// 0.29 - Added variables for screen size
// 0.30 - Modified LuaState constructors to split UI from other things.
// 0.31 - Lua state queues
// 0.32 - Tweaked data included in basic bindings vs UI bindings
// 0.33 - Graphics made per-image file
// 0.34 - Fixes in queue read for empty tables
// 0.35 - Added ability to query mouse targets - for debug
// 0.36 - Updated LuaSocket to the trunk revison as of 15/02/16
// 0.37 - Tony's right click change
// 0.38 - Save glyph returns if changed
// 0.39 - Added method to get if running in full screen mode
// 0.40 - Changed bg maze to full colour
// 0.41 - Animated maze elements
// 0.42 - Added current_lines to Lua interface
// 0.43 - Fixed text wrap
// 0.44 - set_wrap_limits
// 0.45 - fixes for scrolling
// 0.46 - Added true threading support
// 0.47 - Added timeout function for queue read
// 0.48 - Added render info access
// 0.49 - Added SDL_Delay access from Lua
// 0.50 - Added buildsize to gulp table.
// 0.51 - Added MD5/SHA2 routines.
// 0.52 - Some undocumented change by Tony
// 0.53 - Patch in crc32 and inflate to Lua
// 0.54 - Program execution
// 0.55 - Changes for boot.lua
// 0.56 - Finished Mac launching
// 0.57 - Added Mixer version numbers
// 0.58 - Changes to support screen resizes
// 0.59 - ?
// 0.60 - Support High res stuff (Retina)
// 0.61 - removed cell_size
// 0.62 - added get_SDL_renderer
// 0.63 - move UpdateScreenData_game_window so can be called from boot.lua
// 0.64 - Added viewport origin
// 0.65 - Modifications to print_time_since_last_call()
// 0.66 - SDL_DestroyTexture added, and others
// 0.67 - return_copyright
// 0.68 - fg_colour moves towards a full SDL_Colour
// 0.69 - added multiplayer status draw list
// 0.70 - multitouch
// 0.71 - high_priority mouse target
// 0.72 - Added performance counter access that is Lua friendly
// 0.73 - Extended performance counter
// 0.74 - Extended sound manager access
// 0.75 - Fix potential fatalError crash (e.g. on invalid require path)
// 0.76 - Hex rendering
// 0.77 - Added sqlite3
// 0.78 - Changed save data path to allow configuration of organisation and app name
// 0.79 - Added support for nogui
// 0.80 - More fixes for nogui
// 0.81 - moved setup_ff_lua_state() to main not ui
// 0.82 - Added Joystick and Gamepad support
// 0.83 - Enhanced unknown event support
// 0.84 - More character flexibility
// 0.85 - PresentationMaze::update_glyph()
#define FORLORN_FOX_ENGINE_VERSION 0.85
const double forlorn_fox_engine_version = FORLORN_FOX_ENGINE_VERSION;

#endif

