/*
 *  timing.cpp
 *  Forlorn Fox - previously Z_dungeon
 *
 *  Created by Rob Probin on Sun Jan 12 2003.
 *  Copyright (c) 2003/2011 Rob Probin. All rights reserved.
 *   
 *  Modified 1st October 2011 to fix wraparound issue with timing.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2003-2013 Rob Probin
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

#include "timing.h"
#include "SDL.h"
#include "Utilities.h"
#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif

// +--------------------------------+-------------------------+-----------------------
// | TITLE: NewGetTicks             | AUTHOR(s): Rob Probin   | DATE STARTED: 1 Oct 11
// +
// | DESCRIPTION: Very like SDL_GetTicks apart from it checks for time going backwards
// | and avoids it. Returns number of milliseconds. But work correctly past wrap 
// | ... mostly :-) We should be ok as long as this routine is called at least 
// | every 12 days.
// |
// | For wrap it will return a number LOWER than the previous call.
// | 
// |
// | Suggested in SDL digest, Vol 1 #621 
// +----------------------------------------------------------------ROUTINE HEADER----

uint32_t NewGetTicks()
{
	static uint32_t previous_ticks=0;
	uint32_t new_ticks; 
	
	new_ticks = SDL_GetTicks();

	// check for time running backwards over wrap...
	if (new_ticks > previous_ticks)
	{
		if((new_ticks-previous_ticks) > 0x8000000)
		{
			// unlikely .. jump forward 24 days?
			// more likely time running backwards over wrap
			return previous_ticks;
		}
	}
	else if (new_ticks < previous_ticks)
	{
		// oops - time went backwards... (time does click backwards on some machines, otherwise
		// after 49 days it wraps.)
		
		// look for a likely wrap
		if(previous_ticks > 0xc0000000 && new_ticks < 0x40000000)
		{
			// this looks like wrap! (because the time shouldn't go backwards 
			// 24 days!)
		}
		else	// more likely time running backwards
		{
			return previous_ticks; // pretend it didn't happen
		}
	}
	// store the previous ticks for the next call
	previous_ticks = new_ticks;
	return new_ticks;
}

#if 0
// +--------------------------------+-------------------------+-----------------------
// | TITLE: LS_GetTicks             | AUTHOR(s): Rob Probin   | DATE STARTED: 30 Dec 02
// +
// | DESCRIPTION: Very like SDL_GetTicks apart from it checks for time going backwards
// | and avoids it. Returns number of milliseconds.
// |
// | Suggested in SDL digest, Vol 1 #621 
// +----------------------------------------------------------------ROUTINE HEADER----

uint32_t LS_GetTicks(void)
{
	static uint32_t previous_ticks=0;
	uint32_t new_ticks; 
	
	new_ticks = SDL_GetTicks();
	if (new_ticks < previous_ticks)
	{
		// oops - time went backwards... (time does click backwards on some machines, otherwise
		// after 49 days it wraps. We can't really handle wraps so that would break us.)
		return previous_ticks; // pretend it didn't happen
	}
	previous_ticks = new_ticks;
	return new_ticks;
}


// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: 
// |
// +----------------------------------------------------------------ROUTINE HEADER----
const int NUMBER_OF_AVERAGE_FRAMES = 5;
const int NUMBER_OF_STORED_FRAMES = 20;

// used for calculating the average frame time for the last 5 frames - this is used as the world timing
static uint32_t last_frame_ticks;
static uint32_t previous_frame_ticks[NUMBER_OF_AVERAGE_FRAMES];
static double average_frame_time;					// the average frame time of the last 5 frames

// overall game monitoring data
static uint32_t beginning_tick_count;					// used to calculate the FPS for the entire game
static uint32_t number_of_frames;						// number of frames for the entire game

static uint32_t long_term_num_of_frames;				// used for the slow FPS
static uint32_t long_term_next_frame_ticks;
static double long_term_average_frame_rate;
const int LONG_TERM_FRAME_RATE_TICKS_TOTAL = 500;			// 0.5 second - relative long period

static double total_frame_time_floating_point;				// keep a check of how long we ran things - for comparison with total ticks.


void init_frame_time(void)
{
	int count;
	
	last_frame_ticks = LS_GetTicks();
	
	for(count=0; count < NUMBER_OF_AVERAGE_FRAMES; count++)
	{
		// fill these in reverse order... last is newest therefore should have closest time...
		previous_frame_ticks[NUMBER_OF_AVERAGE_FRAMES - count] = last_frame_ticks-((count+1)*30);	// assume 33 fps to start with
	}
	
	// frame measurement setup
	beginning_tick_count = SDL_GetTicks();
	number_of_frames=0;
	
	// long term frame rate setup
	long_term_next_frame_ticks = LS_GetTicks() + LONG_TERM_FRAME_RATE_TICKS_TOTAL;
	long_term_num_of_frames = 0;
	long_term_average_frame_rate = 0;		// default
	
	// data for tracking accuracy of gametime against realtime.
	total_frame_time_floating_point = 0;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: Time between ticks for this frame
// |
// +----------------------------------------------------------------ROUTINE HEADER----

#define FRAME_TIME_RECORD_FOR_DEBUG 0

#if FRAME_TIME_RECORD_FOR_DEBUG
uint32_t frames_tick[200];
int frame_index = 0;
#endif

void measure_frame_time(void)
{
	uint32_t new_frame_ticks;
	int count;
	
	new_frame_ticks = LS_GetTicks();
	
#if FRAME_TIME_RECORD_FOR_DEBUG
	frames_tick[frame_index++]=new_frame_ticks-last_frame_ticks;
	if(frame_index >= 200) frame_index = 0;
#endif
	
	// move previous frame ticks down by 1 slot
	for(count=0; count < (NUMBER_OF_AVERAGE_FRAMES-1); count++)
	{
		previous_frame_ticks[count] = previous_frame_ticks[count+1];
	}
	
	// record current frame tick count (ms time)
	previous_frame_ticks[NUMBER_OF_AVERAGE_FRAMES-1] = last_frame_ticks;
	
	// store current ticks for next time
	last_frame_ticks = new_frame_ticks;
	
	// 
	// calculate average frame time
	// Do this once per frame - since we are going to read this quite a few times - no need to do the calculation more than once...
	// This used to add up the previous 5 frame times and divide by 5. This way does the same and is quicker.
	//
	average_frame_time = (static_cast<double>(last_frame_ticks - previous_frame_ticks[0])) / NUMBER_OF_AVERAGE_FRAMES;
	average_frame_time /= 1000.0;
	// Accuracy... each time difference appears in the average 5 times - therefore appart from floating point inaccuracies (pretty minimal
	// with doubles) we should have a nice accurate tracking of real time with game time. If we were using just ints for average_frame_time
	// then we would have a round error since we would round down each time. There is another advantage of using doubles - we get a 
	// better resolution with higher frame rates and lower accuracy of the tick return - which is usually every 10ms on some machines.
	//
	total_frame_time_floating_point += average_frame_time;		// keep a check of how long we ran things - for comparison with total ticks.
	
	// update total frames - so frame measurement can be updated
	number_of_frames++;
	
	// long term calculations
	long_term_num_of_frames++;
	if(new_frame_ticks > long_term_next_frame_ticks)
	{
		long_term_average_frame_rate  =  (1000.0 * long_term_num_of_frames) / (new_frame_ticks + LONG_TERM_FRAME_RATE_TICKS_TOTAL - long_term_next_frame_ticks);
		long_term_num_of_frames = 0;
		long_term_next_frame_ticks = new_frame_ticks + LONG_TERM_FRAME_RATE_TICKS_TOTAL;
		
	}
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: Time between ticks for this frame
// +----------------------------------------------------------------ROUTINE HEADER----
uint32_t get_frame_time(void)
{
	return last_frame_ticks - previous_frame_ticks[NUMBER_OF_AVERAGE_FRAMES-1];
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: Time average between ticks for this frame.
// |
// | This routine needs to be as accurate as possible without losing time to either
// | inaccuracy (originally it was a uint32_t) or due to the averaging process losing 
// | milliseconds - this is because we are trying to keep two remote machines in check.
// +----------------------------------------------------------------ROUTINE HEADER----

double get_average_frame_time(void)
{
	return average_frame_time;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 5 Feb 05
// +
// | DESCRIPTION: Current Frames per second (averaged over 5 frames)
// +----------------------------------------------------------------ROUTINE HEADER----
double get_current_FPS(void)
{
	return 1/average_frame_time;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 5 Feb 05
// +
// | DESCRIPTION: Current Frames per second (averaged over 250ms)
// +----------------------------------------------------------------ROUTINE HEADER----
double get_long_average_FPS(void)
{
	return long_term_average_frame_rate;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

uint32_t total_game_ticks(void)
{
	return SDL_GetTicks() - beginning_tick_count;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE: get_total_gametime_run  | AUTHOR(s): Rob Probin   | DATE STARTED: 5 Feb 05
// +
// | DESCRIPTION: This returns the number of seconds that has existed in the game-world.
// +----------------------------------------------------------------ROUTINE HEADER----

double get_total_gametime_run(void)
{
	return total_frame_time_floating_point;
}


// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

double overall_average_FPS(void)
{
	double FPS;
	
	FPS = (double)number_of_frames/(double)total_game_ticks();
	FPS *= 1000;
	
	return FPS;
}



// +--------------------------------+-------------------------+-----------------------
// | TITLE:                         | AUTHOR(s): Rob Probin   | DATE STARTED: 4 Jan 03
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

uint32_t total_game_frames(void)
{
	return number_of_frames;
}



// +-------------------------------------+-------------------------+-----------------------
// | TITLE: MaximumRateTimer constructor | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

MaximumRateTimer::MaximumRateTimer(uint32_t minimum_time_in_ms)
{
	time_period = minimum_time_in_ms/1000.0;
	accumulator = 0;
}


// +--------------------------------+-------------------------+-----------------------
// | TITLE: frame_update_and_flag   | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

bool MaximumRateTimer::frame_update_and_flag()
{
	bool flag = false;
	
	accumulator += get_average_frame_time();
	
	if(accumulator>time_period)	// if certain time has passed, it's time to run the animations
	{
		accumulator -= time_period;	// make a time period it less
		if(accumulator>time_period)	// if still more than time period we can't keep up so reset back to zero
		{
			accumulator=0;	// i.e. average frame time > time_period  (frame rate < 1/time_period), therefore stuff will run slow
		}
		
		flag = true;
	}
	
	return flag;
}



// +--------------------------------+-------------------------+-----------------------
// | TITLE: change_update_rate      | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: 
// +----------------------------------------------------------------ROUTINE HEADER----

void MaximumRateTimer::change_update_rate(uint32_t new_minimum_time_in_ms)
{
	time_period = new_minimum_time_in_ms;
}



// +------------------------------------+-------------------------+-----------------------
// | TITLE: SingleShotTimer constructor | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: create a new single shot timer - intially disabled
// +----------------------------------------------------------------ROUTINE HEADER----

SingleShotTimer::SingleShotTimer()
{
	enabled = false;			// timer starts off disabled
	expired_flag = false;		// not expired to start with
	held = false;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE: expired                 | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: returns true when the timer has come to the end
// +----------------------------------------------------------------ROUTINE HEADER----

bool SingleShotTimer::expired()
{
	bool local_expired_flag = false;
	
	if(enabled) // always false if not enabled.
	{
		//if(NewGetTicks() < start_time)
		//{
		//	// wrapped (because checks NewGetTicks does)
		//	
		//}
		else if(NewGetTicks() > destination_time)
		{
			expired_flag = true;
		}
		
		local_expired_flag = expired_flag;
	}
	
	return local_expired_flag;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE: start_timer            | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: start (or restart) the timer with a specific target duration
// +----------------------------------------------------------------ROUTINE HEADER----

void SingleShotTimer::start_timer(uint32_t minimum_time_in_ms)
{
	enabled = true;
	expired_flag = false;		// not expired to start with
	held = false;
	
	start_time = NewGetTicks();
	destination_time = start_time + minimum_time_in_ms;
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE: hold_timer              | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: temporarily disabled the timer from incrementing
// +----------------------------------------------------------------ROUTINE HEADER----

// *** NOT TESTED ***
void SingleShotTimer::hold_timer()
{
	if(!held && enabled)
	{
		enabled = false;
		held = true;
		
		destination_time = destination_time - LS_GetTicks();		// convert the destination time into time left
	}
}

// +--------------------------------+-------------------------+-----------------------
// | TITLE: release_timer           | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: release a previously held timer
// +----------------------------------------------------------------ROUTINE HEADER----

// *** NOT TESTED ***
void SingleShotTimer::release_timer()
{
	if(held && !enabled)
	{
		destination_time = LS_GetTicks() + destination_time;	// convert the time left back into a destination time
	}
}


// +--------------------------------+-------------------------+-----------------------
// | TITLE: disable_timer           | AUTHOR(s): Rob Probin   | DATE STARTED: 29 Apr 04
// +
// | DESCRIPTION: stops the timer returing expired ever
// +----------------------------------------------------------------ROUTINE HEADER----

// *** NOT TESTED ***
void SingleShotTimer::disable_timer()
{
	enabled = false;
	held = false;
}



#endif

// +---------------------------------------------------------------------------
// | TITLE: OneShotTimer constructor
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 1 Oct 11
// +
// | DESCRIPTION: create a new single shot timer - intially disabled
// +---------------------------------------------------------------------------
OneShotTimer::OneShotTimer()
: expired_flag(false)
, sdl_timer(0)
{
}

// +---------------------------------------------------------------------------
// | TITLE: OneShotTimer destructor
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Jan 12
// +
// | DESCRIPTION: remove the SDL timer ... otherwise nasty things will happen :-(
// +---------------------------------------------------------------------------
OneShotTimer::~OneShotTimer()
{
	if(sdl_timer)
	{
		SDL_bool result = SDL_RemoveTimer(sdl_timer);
		if(result == SDL_FALSE)
		{
			// We ignore if it's already been removed by expiring
			if(!expired_flag)
			{
				Utilities::fatalErrorSDL("OneShotTimer Delete Error =");
			}
		}
	}
}


// +---------------------------------------------------------------------------
// | TITLE: expired
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 1 Oct 11
// +
// | DESCRIPTION: returns true when the timer has come to the end
// +---------------------------------------------------------------------------
bool OneShotTimer::expired()
{
	return expired_flag;
}

// +---------------------------------------------------------------------------
// | TITLE: start_time
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 1 Oct 11
// +
// | DESCRIPTION: start (or restart) the timer with a specific target duration
// +---------------------------------------------------------------------------
void OneShotTimer::start_time(uint32_t minimum_time_in_ms)
{
	if(sdl_timer)
	{
		SDL_bool result = SDL_RemoveTimer(sdl_timer);
		if(result == SDL_FALSE)
		{
			// We ignore if it's already been removed by expiring
			if(!expired_flag)
			{
				Utilities::fatalErrorSDL("OneShotTimer Delete Error =");
			}
			
		}
	}

	requested_interval = minimum_time_in_ms;
	sdl_timer = SDL_AddTimer(minimum_time_in_ms, callback, this);
	if(not sdl_timer)
	{
		Utilities::fatalErrorSDL("OneShotTimer SDL_AddTimer failed");
	}
	expired_flag = false;
}


// +---------------------------------------------------------------------------
// | TITLE: callback
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 1 Oct 11
// +
// | DESCRIPTION: start (or restart) the timer with a specific target duration
// +---------------------------------------------------------------------------
Uint32 OneShotTimer::callback(Uint32 interval, void* param)
{
	OneShotTimer* timer = static_cast<OneShotTimer*>(param);
	
	if(interval == timer->requested_interval)
	{
		timer->expired_flag = true;
	}
	
	return 0;		// 0=cancel, interval=repeat same rate, another_value = next interval
}

//
// ############################################################################
//


// +---------------------------------------------------------------------------
// | TITLE: RepeatingTimer constructor
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 15 Oct 11
// +
// | DESCRIPTION: create a new repeating timer - intially disabled
// +---------------------------------------------------------------------------
RepeatingTimer::RepeatingTimer()
: expired_flag(false)
, sdl_timer(0)
{
}

// +---------------------------------------------------------------------------
// | TITLE: RepeatingTimer destructor
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Jan 12
// +
// | DESCRIPTION: remove the SDL timer ... otherwise nasty things will happen :-(
// +---------------------------------------------------------------------------
RepeatingTimer::~RepeatingTimer()
{
	if(sdl_timer)
	{
		SDL_bool result = SDL_RemoveTimer(sdl_timer);
		if(result == SDL_FALSE)
		{
			Utilities::fatalErrorSDL("RepeatingTimer Delete Error =");
		}
	}
}

// +---------------------------------------------------------------------------
// | TITLE: expired
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 15 Oct 11
// +
// | DESCRIPTION: returns true **ONCE** when the timer has come to the end
// +---------------------------------------------------------------------------
bool RepeatingTimer::expired()
{
	bool flag = expired_flag;
	expired_flag = false;
	return flag;
}

// +---------------------------------------------------------------------------
// | TITLE: start_time
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 15 Oct 11
// +
// | DESCRIPTION: start (or restart) the timer with a specific target duration
// +---------------------------------------------------------------------------
void RepeatingTimer::start_time(uint32_t minimum_time_in_ms)
{
	if(sdl_timer)
	{
		SDL_bool result = SDL_RemoveTimer(sdl_timer);
		if(result == SDL_FALSE)
		{
			Utilities::fatalErrorSDL("RepeatingTimer remove Error =");
		}
	}
	
	requested_interval = minimum_time_in_ms;
	sdl_timer = SDL_AddTimer(minimum_time_in_ms, callback, this);
	if(not sdl_timer)
	{
		Utilities::fatalErrorSDL("RepeatingTimer SDL_AddTimer failed");
	}
	expired_flag = false;
}


// +---------------------------------------------------------------------------
// | TITLE: callback
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 15 Oct 11
// +
// | DESCRIPTION: start (or restart) the timer with a specific target duration
// +---------------------------------------------------------------------------
Uint32 RepeatingTimer::callback(Uint32 interval, void* param)
{
	RepeatingTimer* timer = static_cast<RepeatingTimer*>(param);
	
	if(interval == timer->requested_interval)
	{
		timer->expired_flag = true;
	}
	
	return interval;		// 0=cancel, interval=repeat same rate, another_value = next interval
}


