/*
 *  GameApplication.h
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

#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "SDL.h"
#include "GameConfig.h"
#include "MyGraphics.h"
#include "MyGraphics_render.h"
#include "MySoundManager.h"
class SpecialStates;
#include "Debug.h"
#include "FrameRateLimiter.h"
class ControllerBaseType;
#include <vector>
#include <map>
#include "LuaMain.h"
#include "DrawList.h"
#include "PresentationMaze.h"
#include "MouseTarget.h"
#include <list>
#include <set>
#include "lua.h"
#include "Clickable.h"

class MyGraphics_record;



class GameApplication {
public:
    GameApplication();
	~GameApplication();
	void mouse_button_down_event(int x, int y, int button);
	void mouse_button_up_event(int x, int y, int button);
	void mouse_moved_and_is_down_event(int x, int y, int button);
    void mouse_moved_button_up(int x, int y);

	int main(int argc, char* argv[]);
	void render(SDL_Renderer *renderer, MyGraphics& graphics);

    void add_mouse_target(MouseTargetBaseType* target);
    void remove_mouse_target(MouseTargetBaseType* target);
    bool run_mouse_target(double x, double y, bool button_down, bool drag);

    void add_clickable_target(Clickable* target, bool high_priority = false);
    void remove_clickable_target(Clickable* target);
    bool check_clickable_targets(int x, int y, bool button_down, bool drag);



public:

	void add_controller(ControllerBaseType* controller, double time_step);
	void schedule_remove_controller(ControllerBaseType* controller);

	//friend void set_up_basic_ff_libraries(LuaMain*l, GameApplication& app);	// allow LuaMain to access our privates; Limited interface.
    friend void set_up_ui_ff_libraries(LuaMain*l, GameApplication& app);	// allow LuaMain to access our privates; Limited interface.

    void setup_ff_lua_state(LuaMain* L);

	void exit_glyph_editor();

	DrawList* get_absolute_draw_list() { return &absolute_draw_list; }
	DrawList* get_status_draw_list() { return &status_draw_list; }
	DrawList* get_multiplayer_status_draw_list() { return &multiplayer_status_draw_list; }
	void quit_main_loop();

    void print_status_lines(MyGraphics* gr, int maze_line_offset);

    SDL_Renderer* get_SDL_renderer();
    MyGraphics_render* get_standard_graphics_context();
    void set_background_colour(simple_colour_t c);
    void fps_test(int frames_per_second) { frame_rate_limit.test(frames_per_second); }

    void SetRenderer(SDL_Renderer *renderer_in, bool vsync_guess);

    lua_State* get_ui_lua_state(){return lua_user_interface.get_internal_state();};

    void verbose_engine(bool enabled);
    const char* return_copyright() { return copyright; }
private:

    bool we_think_vsync_is_enabled;
	// data
	SDL_Renderer *renderer;

	// some draw lists are clickable, so clickable_list must be initialised before, and destroyed after,
	// the draw lists
    std::list<Clickable*> clickable_list;

	// it would be a good idea to add a system here when lua can register additional DrawLists as it sees fit

	// absolute draw list is really window relative and is used for effects / animations that span UI and game areas, and menus
	DrawList absolute_draw_list;

	// status draw list is relative to the status area of the screen and is used for UI elements
	DrawList status_draw_list;
	DrawList multiplayer_status_draw_list;



	MySoundManager make_sound;

	// game variables
	FrameRateLimiter frame_rate_limit;

	// do we want to fill the background?
	simple_colour_t fill_background_colour;


	// other variable storage
	MyGraphics_render* graphics;

	bool done;

    // a forward_list would probably work here fine - but that's C++11.
    std::list<MouseTargetBaseType*> mouse_target_list;

    // game arguments
    int game_argc;
    char** game_argv;

    bool _engine_verbose = false;

    // relies on things like mouse_target_list .. to create last, and importantly
    // destroy FIRST
    LuaMain lua_user_interface;
    
    static const char* copyright;           // buy in C++ so it's harder to edit out

    bool multitouch_active;
    double touch_timeout;
	double touch_rotation;
	double touch_pinch_distance;
    double touch_x;
    double touch_y;
    int touch_fingers;

    const double touch_rotation_chunk;	// 12.25 degrees
    const double touch_pinch_chunk;

    void touch_gesture_update(double dt);
    void touch_gesture(double rotation, double pinch_distance, double x, double y, int num_fingers);

};

extern GameApplication* gulp_cpp;

#endif
