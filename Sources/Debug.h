/*
 *  Debug.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 21/10/2011.
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

#ifndef DEBUG_H
#define DEBUG_H

#include "SDL.h"
class MyGraphics;
#include <deque>
#include <string>
#include <vector>
#include "GameToScreenMapping.h"

#define MiniTimeBuffer_uses_deque 1
class MiniTimeBuffer
{
public:
    MiniTimeBuffer(size_t size);
    size_t size();
    void add_to_end(double v);
    double get_from_end(int index);
private:
    size_t max_size;
#if MiniTimeBuffer_uses_deque
    std::deque<double> buff;
#else
    std::vector<double> buff;
    size_t current_size;
    size_t write_index;
#endif
};


class Debug
{
public:
	void print(MyGraphics& gr);
	
	// debug control functions
	void info_show();
	void info_hide();
	void info_toggle();

	// timing related calls
	void timing_loop_start();
	void timing_loop_end_predelay();
	void timing_prerender();

	// DrawListElement related calls
	void inc_dle_count() { dle_count++; }
	void dec_dle_count() { dle_count--; }

	// MazeData related calls
	void inc_md_count() { md_count++; }
	void dec_md_count() { md_count--; }

	void set_lua_info_string(const char* display_string);
	// -----------
	Debug();
	~Debug();
private:
	bool info_shown;

    // frame time stuff
	int game_loops;
	Uint64 loop_start_time;
	Uint64 loop_start_time_last;
	Uint64 loop_end_predelay;
	Uint64 prerender;
	//typedef std::deque<double> time_queue_t;
    typedef MiniTimeBuffer time_queue_t;
	time_queue_t frame_times;
	//time_queue_t processing_times;
    //time_queue_t long_times;
    void queue_times(time_queue_t& q, Uint64 start, Uint64 end);
    double calc_times(time_queue_t& q, double* min=0, double* max=0, size_t max_size=0);

	// DrawListElement / MazeData stuff
	unsigned int dle_count;
	unsigned int md_count;
	
	std::string lua_info_string_copy;
    
    // 
    // time display caches
    double average_loop_time;
    double min_average;
    double average_processing_time;
    double average_total_work_time;
    double min60;
    double max60;
    double last_60_loop_time;
    Uint64 SDL_GetPerformanceFrequency_stored;
    int draw_count;

    Viewport viewport;

};

extern Debug debug;

#endif
