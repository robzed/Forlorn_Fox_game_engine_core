/*
 *  Debug.cpp
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

#include "Debug.h"
#include "MyGraphics.h"
#include <sstream>
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include "GameToScreenMapping.h"

MiniTimeBuffer::MiniTimeBuffer(size_t size)
: max_size(size)
#if ! MiniTimeBuffer_uses_deque
, current_size(0)
, write_index(0)
#endif
{
#if MiniTimeBuffer_uses_deque
#else
    buff.resize(size);
#endif
}

size_t MiniTimeBuffer::size()
{
#if MiniTimeBuffer_uses_deque
    return buff.size();
#else
    return current_size;
#endif
}

void MiniTimeBuffer::add_to_end(double v)
{
#if MiniTimeBuffer_uses_deque
    if(buff.size() >= max_size)
    {
        buff.pop_front();
    }
    buff.push_back(v);
#else
    buff[write_index] = v;
    write_index ++;
    if(write_index > max_size)
    {
        write_index = 0;
    }
    if(current_size != max_size)
    {
        current_size++;
    }
#endif
}
double MiniTimeBuffer::get_from_end(int i)
{
#if MiniTimeBuffer_uses_deque
    return buff.at((buff.size()-1)-i);
#else
    if(i < 0 or (size_t)i >= current_size)
    {
        return buff.at(-10000);
    }
    int index = (write_index-1)-i;
    if(index < 0)
    {
        index += max_size;
    }
    return buff.at(index);
#endif
}


// a global variable of this type
Debug debug;

//#define RECORD_TIMINGS
#ifdef RECORD_TIMINGS
int i = 0;
const int tsize = 1200;   // at 60 fps, that's about 20 seconds of data
double t1[tsize];
double t2[tsize];
double t3[tsize];

static void tsave(double a, double b, double c)
{
    double f = SDL_GetPerformanceFrequency();

    i++;
    if (i < tsize) {
        t1[i-1] = a/f;
        t2[i] = b/f;
        t3[i] = c/f;
    }
    else if(i == tsize) {
        FILE* f = fopen("/Users/rob/timing_data.txt", "w");
        fprintf(f, "total, prerender, predelay\n");
        for(int k=0; k<tsize; k++)
        {
            // something that will import into a speadsheet for graphing
            fprintf(f, "%f, %f, %f\n", t1[k], t2[k], t3[k]);
        }
        fclose(f);
        printf("Saved Timing Data\n");
    }
}

#else
#define tsave(a, b, c)
#endif

// +---------------------------------------------------------------------------
// | TITLE: print
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::print(MyGraphics& gr)
{
    if(game_loops <= 0) { return; }

    gr.set_viewport(viewport);

    SDL_GetPerformanceFrequency_stored = SDL_GetPerformanceFrequency();
	queue_times(frame_times, loop_start_time_last, loop_start_time);
	//queue_times(processing_times, loop_start_time_last, prerender);
    //queue_times(processing_times, loop_start_time_last, loop_end_predelay);
    //queue_times(long_times, loop_start_time_last, loop_start_time);
    
    if(not info_shown) return;

    draw_count ++;
    if(draw_count > 5)
    {
        average_loop_time = calc_times(frame_times, 0, &min_average, 5);
        average_processing_time = 0;
        //average_processing_time = calc_times(processing_times);
        average_total_work_time = 0;
        //average_total_work_time = calc_times(processing_times);
        min60=0; max60=0;
        //last_60_loop_time = calc_times(long_times, &min60, &max60);
        last_60_loop_time = calc_times(frame_times, &min60, &max60);
        draw_count = 0;
    }
    
	gr.set_fg_colour(WHITE);
    SDL_Colour blue = {0, 0, 255, 128};
	gr.set_bg_fullcolour(blue);
	gr.go_to(0,0);

    std::stringstream s;
    
    //
    // instantaneous frame times
    //
    s.precision(1);
    s << std::fixed;
    //s << average_loop_time << "ms ";
    //s << average_processing_time/average_loop_time << "% ";
    //s << average_processing_time << "ms " ;
    //s << average_total_work_time << "ms " ;
    //s << average_total_work_time/average_loop_time << "% " ;
    s << 1000/average_loop_time << "fps (min=" << 1000/min_average << ")";
    //s << SDL_GetPerformanceFrequency();

    print_string(gr, s.str());
    gr.go_to(gr.get_line()+1, 0);
    s.str("");

    //
    // frame averages
    //
    s.precision(1);
    s << std::fixed;
    s << 1000/min60 << ">";
    s << 1000/last_60_loop_time << ">";
    s << 1000/max60;

    print_string(gr, s.str());
    gr.go_to(gr.get_line()+1, 0);
    s.str("");
    
    //
    // DLEs
    //
    s.precision(1);
    s << std::fixed;
    s << "DLE:" << dle_count;
    print_string(gr, s.str());
    gr.go_to(gr.get_line()+1, 0);
    s.str("");

    //
    // MazeData
    //
    s.precision(1);
    s << std::fixed;
    s << "MD:" << md_count;
    print_string(gr, s.str());
    gr.go_to(gr.get_line()+1, 0);
    s.str("");
    
    // junk from Lua :-)
    print_cstring(&gr, lua_info_string_copy.c_str());
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------

void Debug::queue_times(time_queue_t& q, Uint64 start, Uint64 end)
{
	double time = static_cast<double>(end - start);
	time /= SDL_GetPerformanceFrequency_stored;
	time *= 1000.0;

    q.add_to_end(time);
}

double Debug::calc_times(time_queue_t& q, double* min, double* max, size_t max_size)
{
	double total = 0;
    double mymax = 0.0;
    double mymin = 9999.0;
    if(max_size == 0 or max_size > q.size())
    {
        max_size = q.size();
    }
    for(int i=0; i < static_cast<int>(max_size); i++)// Stop warning: should never be bigger than (2^31)-1
	{
        double v = q.get_from_end(i);
		total += v;
        if(v > mymax) { mymax = v; }
        if(v < mymin) { mymin = v; }
	}
    if(max) { *max = mymax;}
    if(min) { *min = mymin; }
	double average = total / max_size;
	return average;
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::timing_loop_start()
{
	loop_start_time_last = loop_start_time;
	loop_start_time = SDL_GetPerformanceCounter();
	game_loops++;
}
// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::timing_loop_end_predelay()
{
	loop_end_predelay = SDL_GetPerformanceCounter();
    tsave((loop_start_time - loop_start_time_last) / f,
          (prerender - loop_start_time) / f,
          (loop_end_predelay - loop_start_time) / f
          );
}
// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::timing_prerender()
{
	prerender = SDL_GetPerformanceCounter();
}


// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
Debug::Debug()
: info_shown(false)
, game_loops(-1)
, loop_start_time(0)
, loop_start_time_last(0)
, loop_end_predelay(0)
, prerender(0)
, frame_times(60*60)
, md_count(0)
, lua_info_string_copy("")
, draw_count(0)
{
}

Debug::~Debug()
{
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::info_show()
{
	info_shown = true;
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::info_hide()
{
	info_shown = false;
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::info_toggle()
{
	info_shown = not info_shown;
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 7 July 2014
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void Debug::set_lua_info_string(const char* display_string)
{
	lua_info_string_copy =  display_string;
}
