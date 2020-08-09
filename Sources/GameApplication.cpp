/*
 *  GameApplication.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 17/07/2011.
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

#include "GameApplication.h"
#include "MyGraphics_render.h"
#include "GameToScreenMapping.h"
#include "Utilities.h"
#include "LuaCppInterface.h"

#include <iostream>
#include <stdio.h>
#include <sstream>
#include <cstdlib>
#include <ctime>
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include <algorithm>	// for std::find
#include <sstream>

// set up the renderer
int double_buffering_on = 1;

const char* GameApplication::copyright = "Copyright © 2014-2020 Rob Probin, Tony Park, Kenny Osborne. All rights reserved";

//const char* GameApplication::return_copyright()
//{
//    return copyright;
//}

void GameApplication::render(SDL_Renderer *renderer, MyGraphics& graphics)
{
	// Fill screen with a colour
	// We ALWAYS do this on every frame
	//
	// Assume it's a full 3D accelerated context that wants a new set of polys
	// every frame. Cause it is accelerated. And it does :-)
	//
	// Theoretically we could do incremental rendering on non-double-buffer
	// targets, but we don't bother because things like the iPhone chip
	// does block by block not whole frame - hence it's slower to do incremental.
	//
	SDL_Colour c;
	get_rgb_from_simple_colour(&c, fill_background_colour);
	graphics.clear_screen(c);

    luabridge::push(lua_user_interface, &graphics);
    run_gulp_function_if_exists(&lua_user_interface, "draw", 1);
    
    debug.print(graphics);

    // ((done from Lua))
	// draw UI elements etc
	//absolute_draw_list.render(graphics, 0);

	debug.timing_prerender();
    /* update screen */
    SDL_RenderPresent(renderer);
}

void GameApplication::setup_ff_lua_state(LuaMain* l) //, int argc, char* argv[])
{
    if(not l) { Utilities::fatalError("LuaMain null in setup_ff_lua_state()"); }
    LuaMain& L = *l;

    // Copy any command line arguments to Lua via global 'arg' as per command
    // line. However, unlike the command line interpreter, we don't call a
    // specific script with a vardac argument.
    lua_newtable(L);
    int i = 0;
    for(; i < game_argc; i++)
    {
        lua_pushstring(L, game_argv[i]);
        lua_rawseti(L, -2, i);
    }

    lua_setglobal(L, "arg");

    // Lua library_init done very early so it can override paths in conf.lus
    L.library_init();
    set_up_basic_ff_libraries(l);
    load_conf_file(l);
}


int GameApplication::main(int argc, char* argv[])
{
    game_argc = argc;
    game_argv = argv;

    absolute_draw_list.set_size(32);

    setup_ff_lua_state(&lua_user_interface);

    load_main_file(&lua_user_interface, "scripts/boot.lua");
    luabridge::push(lua_user_interface, this);

    run_gulp_function_if_exists(&lua_user_interface, "boot", 1, 1);	// called once on game load

    int result_isnum = false;
    lua_Integer result = lua_tointegerx(lua_user_interface, -1, &result_isnum);
    if(result_isnum) { return (int)result; }
    
    bool gui_enabled = true;
    const char * str_result = lua_tostring(lua_user_interface, -1);
    if(str_result and std::string(str_result) == "nogui")
    {
       gui_enabled = false;
    }
    lua_pop(lua_user_interface, 1);

   
    // only check if we don't specify no gui
   
    if(gui_enabled)
    {

       if(not renderer)
       {
           Utilities::fatalError("Game boot didn't set renderer");
       }
       


      set_up_ui_ff_libraries(&lua_user_interface, *this);

      int error = SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &double_buffering_on);
      if(error)
      {
         Utilities::debugMessage("******>>>>> SDL_GL_GetAttribute failed: %s", SDL_GetError());
      }
      error = SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      if(error)
      {
         Utilities::fatalError("SDL_SetRenderDrawBlendMode failed: %s", SDL_GetError());
      }

      graphics = new MyGraphics_render(renderer);
   }

	frame_rate_limit.set(100);

    // as soon as we can, load the main file
    //-----------------------------------------------------------------------------
	load_main_file(&lua_user_interface, "scripts/main.lua");

   if(gui_enabled)
   {
      // before loading, create the splash screen and render it
      luabridge::push(lua_user_interface, this);
      run_gulp_function_if_exists(&lua_user_interface, "splash", 1);
      render(renderer, *graphics);
   }

	// push app as arg to gulp.load in lua
	luabridge::push(lua_user_interface, this);
	run_gulp_function_if_exists(&lua_user_interface, "load", 1);	// called once on game load


    /* Enter render loop, waiting for user to quit */
    done = false;
    SDL_Event event;
    while (!done)
    {
		debug.timing_loop_start();
        while (SDL_PollEvent(&event))
		{
            if (event.type == SDL_QUIT)
			{
                run_gulp_function_if_exists(&lua_user_interface, "quit_event");
                done = true;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                lua_pushnumber(lua_user_interface, event.window.event);
                lua_pushnumber(lua_user_interface, event.window.data1);
                lua_pushnumber(lua_user_interface, event.window.data2);
                run_gulp_function_if_exists(&lua_user_interface, "windowevent", 3);

            	// pause on minimise
            	//if(event.window.event == SDL_WINDOWEVENT_MINIMIZED)
            	//{
            		//paused = true;
            	//}

            }
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
                int x, y;
				SDL_MouseButtonEvent mb = event.button;
				x = mb.x; y = mb.y;

				screen_to_game(x, y);
                mouse_button_down_event(x, y, mb.button);
            }
			else if(event.type == SDL_MOUSEBUTTONUP)
			{
                int x, y;
				SDL_MouseButtonEvent mb = event.button;
				x = mb.x; y = mb.y;

				screen_to_game(x, y);
				mouse_button_up_event(x, y, mb.button);
			}
			else if(event.type == SDL_MOUSEMOTION)
			{
                int x, y;
				SDL_MouseMotionEvent mb = event.motion;
				x = mb.x; y = mb.y;
				Uint8 state = mb.state;

				//SDL_GetRelativeMouseState(&dx, &dy);        /* find how much the mouse moved */
				screen_to_game(x, y);
				if (state & SDL_BUTTON_LMASK)
				{     /* is the mouse (touch) down? */
					mouse_moved_and_is_down_event(x, y, SDL_BUTTON_LEFT);
				}
                else if (state & SDL_BUTTON_RMASK)
				{     /* is the mouse (touch) down? */
					mouse_moved_and_is_down_event(x, y, SDL_BUTTON_RIGHT);
				}
                else

                {
                    mouse_moved_button_up(x, y);
                }
			}
			else if(event.type == SDL_KEYDOWN)
			{
				SDL_KeyboardEvent key = event.key;
				SDL_Keysym keysym = key.keysym;
				bool handled = false;

				//
				// These keys are ALWAYS decoded...
				//
				//
				// Apple-Q (immediately quits)
				if((keysym.mod & KMOD_GUI) && keysym.sym == SDLK_q)
				{
                    run_gulp_function_if_exists(&lua_user_interface, "quit_event");
					done = true;
					handled = true;
				}

                if(not handled)
				{
					// push key to Lua callback
					lua_pushnumber(lua_user_interface, keysym.sym);
					lua_pushnumber(lua_user_interface, keysym.scancode);
					lua_pushnumber(lua_user_interface, keysym.mod);
                    lua_pushboolean(lua_user_interface, (key.repeat!=0));
					int error = run_gulp_function_if_exists(&lua_user_interface, "keypressed", 4);
					if(error == LUA_OK)
					{
					}
				}

			}
			else if(event.type == SDL_KEYUP)
			{
				SDL_KeyboardEvent key = event.key;

				SDL_Keysym keysym = key.keysym;
				SDL_Keycode sym = keysym.sym;
				// push key to Lua callback
				lua_pushnumber(lua_user_interface, sym);
				lua_pushnumber(lua_user_interface, keysym.scancode);
				lua_pushnumber(lua_user_interface, keysym.mod);
                int error = run_gulp_function_if_exists(&lua_user_interface, "keyreleased", 3);
                if(error == LUA_OK)
                {
                }
			}
            else if(event.type == SDL_TEXTINPUT)
            {
                lua_pushstring(lua_user_interface, event.text.text);
                /* int error = */ run_gulp_function_if_exists(&lua_user_interface, "textinput", 1);
            }
            else if(event.type == SDL_TEXTEDITING)
            {
                /*
                 Update the composition text.
                 Update the cursor position.
                 Update the selection length (if any).
                 */
                //composition = event.edit.text;
                //cursor = event.edit.start;
                //selection_len = event.edit.length;
                lua_pushstring(lua_user_interface, event.edit.text);   // composition
                lua_pushnumber(lua_user_interface, event.edit.start);     // cursor
                lua_pushnumber(lua_user_interface, event.edit.length);    // selection_len
                /* int error = */ run_gulp_function_if_exists(&lua_user_interface, "textediting", 3);
            }
            else if(event.type == SDL_MULTIGESTURE)
            {
                /*
				 Touchscreen gestures
                 */
            	// touchscreen gestures affect clicking as well and so need to be handled in the engine
            	touch_gesture(event.mgesture.dTheta, event.mgesture.dDist, event.mgesture.x, event.mgesture.y, event.mgesture.numFingers);
            }
            else
            {
                lua_pushnumber(lua_user_interface, event.type);
                run_gulp_function_if_exists(&lua_user_interface, "unknown_event", 1);
            }
        }

		lua_user_interface.process_console();
		//
		// update the timestep
		//
		static uint32_t prev_ticks = SDL_GetTicks();
		uint32_t new_ticks = SDL_GetTicks();
		// a note about roll-over
		// if prev_ticks = 0xffffffff and new_ticks = 1, then hopefully
		// new - prev = 2 ticks
		uint32_t tick_diff = new_ticks - prev_ticks;
		double tick_step = tick_diff / 1000.0;
		prev_ticks = new_ticks;
		// Protect against going backwards (just in case).
		// Notice, it is goes backward permanently (e.g. at wrap), the
		// the prev_ticks = new_ticks will sort that. It does mean that we lose
		// a bit of time, but that's probably ok as long as it doesn't happen too
		// often.
		if (tick_step < 0)
		{
			tick_step = 0;
		}
		// tick_step is the number of seconds (fractional, usually) that
		// time has advanced.

		// handle touch gestures
		touch_gesture_update(tick_step);

		// and update lua
		lua_pushnumber(lua_user_interface, tick_step);
		run_gulp_function_if_exists(&lua_user_interface, "update", 1);
      
      if(gui_enabled)
      {
         render(renderer, *graphics);
      }

		debug.timing_loop_end_predelay();
		frame_rate_limit.limit(we_think_vsync_is_enabled);
    }

	run_gulp_function_if_exists(&lua_user_interface, "quit");
	delete graphics;
	return 0;
}

void GameApplication::SetRenderer(SDL_Renderer *renderer_in, bool vsync_guess)
{
    renderer = renderer_in;
    we_think_vsync_is_enabled = vsync_guess;
}

GameApplication::GameApplication()
: renderer(NULL) // renderer_in)
, fill_background_colour(BLACK)
, graphics(0)
, game_argc(0)
, game_argv(0)
, multitouch_active(false)
, touch_timeout(0.0)
, touch_rotation(0.0)
, touch_pinch_distance(0)
, touch_x(0)
, touch_y(0)
, touch_fingers(0)
, touch_rotation_chunk((2*M_PI) / 32)     // 12.25 degrees
, touch_pinch_chunk(0.02)
{
	// don't add stuff here ... put it at the start of main() ... lots of things aren't set up yet!

	//SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer_in, info);
    //we_think_vsync_is_enabled = (info.flags | SDL_RENDERER_PRESENTVSYNC) ? true : false;
    we_think_vsync_is_enabled = 0; //vsync_guess;
}

void GameApplication::mouse_button_down_event(int x, int y, int button)
{
    // if we think we are getting multitouch gesture input, don't interpret as clicks too
	if (multitouch_active) return;

	bool right_button = (button==SDL_BUTTON_RIGHT);
	double column = (x);
    double line = (y);
    bool handled = false;

    if(not handled) handled = run_mouse_target(column, line, true, false);
    if(not handled) handled = check_clickable_targets(x, y, true, false);

    if(not handled)
    {
        // push mouse press to Lua callback
        lua_pushnumber(lua_user_interface, column);
        lua_pushnumber(lua_user_interface, line);
        lua_pushboolean(lua_user_interface, handled);
        lua_pushboolean(lua_user_interface, right_button);
        int error = run_gulp_function_if_exists(&lua_user_interface, "mousepressed", 4, 1);
        if(error == LUA_OK)
        {
            handled = (bool)lua_toboolean(lua_user_interface, -1);
            lua_pop(lua_user_interface, 1);
        }
    }
}

void GameApplication::mouse_button_up_event(int x, int y, int button)
{
    // if we think we are getting multitouch gesture input, don't interpret as clicks too
	if (multitouch_active) return;

	bool right_button = (button==SDL_BUTTON_RIGHT);
	double column = (x);
    double line = (y);
    bool handled = false;

    if(not handled) handled = run_mouse_target(column, line, false, false);
    if(not handled) handled = check_clickable_targets(x, y, false, false);

    if(not handled)
    {
        // push mouse press to Lua callback
        lua_pushnumber(lua_user_interface, column);
        lua_pushnumber(lua_user_interface, line);
        lua_pushboolean(lua_user_interface, handled);
        lua_pushboolean(lua_user_interface, right_button);
        int error = run_gulp_function_if_exists(&lua_user_interface, "mousereleased", 4, 1);
        if(error == LUA_OK)
        {
            handled = (bool)lua_toboolean(lua_user_interface, -1);
            lua_pop(lua_user_interface, 1);
        }
    }
}

void GameApplication::mouse_moved_and_is_down_event(int x, int y, int button)
{
    // if we think we are getting multitouch gesture input, don't interpret as clicks too
	if (multitouch_active) return;

	// could do some single touch gesture work here (e.g. swipes)

	bool right_button = (button==SDL_BUTTON_RIGHT);
	double column = (x);
    double line = (y);
    bool handled = false;

    if(not handled) handled = run_mouse_target(column, line, true, true);
    if(not handled) handled = check_clickable_targets(x, y, true, true);

    if(not handled)
    {
        // push mouse press to Lua callback
        lua_pushnumber(lua_user_interface, column);
        lua_pushnumber(lua_user_interface, line);
        lua_pushboolean(lua_user_interface, handled);
        lua_pushboolean(lua_user_interface, right_button);
        int error = run_gulp_function_if_exists(&lua_user_interface, "mouse_down_moved", 4, 1);
        if(error == LUA_OK)
        {
            handled = (bool)lua_toboolean(lua_user_interface, -1);
            lua_pop(lua_user_interface, 1);
        }
    }

}

void GameApplication::mouse_moved_button_up(int x, int y)
{
    // if we think we are getting multitouch gesture input, don't interpret as clicks too
	if (multitouch_active) return;

	double column = (x);
    double line = (y);
    bool handled = false;

    if(not handled) handled = run_mouse_target(column, line, false, true);
    if(not handled) handled = check_clickable_targets(x, y, false, true);

    // don't care about handled, because we don't call into Lua at the moment with
    // button up drag events
}


void GameApplication::remove_mouse_target(MouseTargetBaseType* target)
{
    if(target)
    {
        // we don't check if it exists.
        mouse_target_list.remove(target);  // might remove no elements...
    }
    else
    {
        Utilities::debugMessage("remove_mouse_target with NULL target?");
    }
}
void GameApplication::add_mouse_target(MouseTargetBaseType* target)
{
    if(target)
    {
        // We push new ones on the front, because these ones added later (in time)
        // get hit first
        // which means that they are (in virtual terms) closer to the front
        // of the mouse click. Older ones are 'hidden' from this perspective
        // if they
        mouse_target_list.push_front(target);
    }
    else
    {
        Utilities::debugMessage("add_mouse_target with NULL target?");
    }
}

bool GameApplication::run_mouse_target(double x, double y, bool button_down, bool drag)
{
    for(std::list<MouseTargetBaseType*>::iterator i = mouse_target_list.begin() ;
        i != mouse_target_list.end(); ++i)
    {
        MouseTargetBaseType* p = *i;
        if(p->inside(x, y))
        {
            p->run(x, y, button_down, drag);
            return true;
        }
    }
    return false;
}

void GameApplication::remove_clickable_target(Clickable* target)
{
    if(target)
    {
        // we don't check if it exists.
        clickable_list.remove(target);  // might remove no elements...
    }
    else
    {
        Utilities::debugMessage("remove_clickable_target with NULL target?");
    }
}
void GameApplication::add_clickable_target(Clickable* target, bool high_priority)
{
    if(target)
    {
        if(high_priority)
        	clickable_list.push_front(target);
        else
        	clickable_list.push_back(target);
    }
    else
    {
        Utilities::debugMessage("add_clickable_target with NULL target?");
    }
}
bool GameApplication::check_clickable_targets(int x, int y, bool button_down, bool drag)
{
    for(auto i = clickable_list.begin(); i != clickable_list.end(); ++i)
    {
    	bool consumed = (*i)->check_for_click(x, y, button_down, drag);
    	if(consumed) return true;
    }
    return false;
}


// touch gestures


void GameApplication::touch_gesture_update(double dt)
{
    if (multitouch_active)
    {
        touch_timeout = touch_timeout + dt;
        if (touch_timeout > 0.25)
        {
            touch_rotation = 0;
            touch_pinch_distance = 0;
            multitouch_active = false;
            //Utilities::debugMessage("reset touch");
        }
    }
}

void GameApplication::touch_gesture(double rotation, double pinch_distance, double x, double y, int num_fingers)
{
    //Utilities::debugMessage("Touch! ",rotation,pinch_distance,x,y,num_fingers);

    // Rotation appears to be in radians
    // Distance appears to be as a proportion of screen size,
	// maximum I can get on my phone seems about 0.4

    // got a touch message, set timeout to 0
    touch_timeout = 0;
    multitouch_active = true;

    // update touch location
    touch_x = x;
    touch_y = y;

    // handle rotation gesture
    touch_rotation = touch_rotation + rotation;
    // Utilties::debugMessage("rot ", touch_rotation);
    while (abs(touch_rotation) >= touch_rotation_chunk)
    {
        // moved by 12.25 degrees, do something!
        lua_pushstring(lua_user_interface, touch_rotation >= 0 ? "clockwise" : "anticlockwise");
        lua_pushnumber(lua_user_interface, x);
        lua_pushnumber(lua_user_interface, y);
        run_gulp_function_if_exists(&lua_user_interface, "gesture_rotate", 3, 0);
        touch_rotation = touch_rotation + ((touch_rotation >= 0) ? -touch_rotation_chunk : touch_rotation_chunk);
    }

    // handle pinch
    touch_pinch_distance = touch_pinch_distance + pinch_distance;
    //Utilities::debugMessage("dist ", touch_pinch_distance);
    while (abs(touch_pinch_distance) >= touch_pinch_chunk)
    {
        // moved by some amount of doodahs, do something!
        lua_pushstring(lua_user_interface, touch_pinch_distance >= 0 ? "out" : "in");
        lua_pushnumber(lua_user_interface, x);
        lua_pushnumber(lua_user_interface, y);
        run_gulp_function_if_exists(&lua_user_interface, "gesture_pinch", 3, 0);
        touch_pinch_distance = touch_pinch_distance + ((touch_pinch_distance >= 0) ? -touch_pinch_chunk : touch_pinch_chunk);
    }

    // handle number of fingers
    if (num_fingers != touch_fingers)
    {
        // number of fingers changed, do something!
        touch_fingers = num_fingers;
        lua_pushnumber(lua_user_interface, touch_fingers);
        lua_pushnumber(lua_user_interface, x);
        lua_pushnumber(lua_user_interface, y);
        run_gulp_function_if_exists(&lua_user_interface, "gesture_fingers", 3, 0);
	}
}



GameApplication::~GameApplication()
{
    if(_engine_verbose) { Utilities::debugMessage("Destroy GameApplication"); }
}


void GameApplication::quit_main_loop()
{
    if(_engine_verbose) { Utilities::debugMessage("quit_main_loop"); }
	done = true;
}

void GameApplication::verbose_engine(bool enabled)
{
        _engine_verbose = enabled;
}



SDL_Renderer* GameApplication::get_SDL_renderer()
{
    return renderer;
}

MyGraphics_render* GameApplication::get_standard_graphics_context()
{
    return graphics;
}

void GameApplication::set_background_colour(simple_colour_t c)
{
    fill_background_colour = c;
}
