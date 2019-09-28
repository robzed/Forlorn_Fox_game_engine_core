/*
 *  timing.h
 *  Forlorn Fox - previously Z_dungeon
 *
 *  Created by Rob Probin on Sun Jan 12 2003.
 *  Copyright (c) 2003/2011 Rob Probin. All rights reserved.
 *
 *  Modified 1st October 2011 to fix wraparound issue with timing.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2003-2013 Rob Probin and Tony Park
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

#ifndef _TIMING_H_
#define _TIMING_H_

#include "BasicTypes.h"
#include "SDL.h"

#if 0		// not enabled for Forlorn Fox currently
// get timing data
void init_frame_time(void);
void measure_frame_time(void);
Uint32 get_frame_time(void);			// in ms. Normally you use get_average_frame_time() since this is unstable
double get_average_frame_time(void);	// in seconds
Uint32 LS_GetTicks(void);

// stuff to get total game framerates
Uint32 total_game_frames(void);
double overall_average_FPS(void);		// very accurate game framerate over entire game
Uint32 total_game_ticks(void);
double get_current_FPS(void);			// Current Frames per second (averaged over 5 frames)
double get_long_average_FPS(void);		// over 500ms
double get_total_gametime_run(void);	// This returns the number of seconds that has existed in the game-world.
#endif

#if 0
// 
// This timer abstracts away the details of maintaining a periodic time and instead provides a call that is used 
// every frame to update a timer. A flag is returned to say whether the timer has expired for this frame. It maintains 
// a rollover so on average (if the frame rate is high enough) it will get very close to the actual requested rate.
//
// If the frame rate is not high enough to maintain the required interval then the routine will attempt to get
// as close as it can. In some cases this might flag up every frame for a low frame rate.
//
// The routine uses average frame rate since we cannot predict the actual time this frame.
//
class MaximumRateTimer
{
public:
	MaximumRateTimer(uint32_t minimum_time_in_ms);  // construct a timer of a certain
	bool frame_update_and_flag();			// should be called once a frame, and the action code called if the returned result is true.
	void change_update_rate(Uint32 new_minimum_time_in_ms);   // used to change the time it occurs. May cause slight timing glitches when changed.
	
	// at some point in the future we should change this to: (a) call back automatically, (b) do a less processor intensive timing (e.g. a queue of to expire times).
	//MaximumRateTimer(Uint32 time, void (*called_function)());
	//MaximumRateTimer(Uint32 time, object to call through interface class);
	// also note: sdl has callback timers...
private:
	double time_period;
	double accumulator;
};

//
// This class provides a class that returns expired after a certain amount of time.
//
// Will break if run over 49 days...
//
// NOTES
// 
// 1. Accuracy should be counted as 10ms - since this is the minimum base tick on most machines.
// 2. Holding and releasing may cause a +/-10ms alteration in the end time (per call).
// 3. Might be able to fix 49 day roll-over break with subtract instead of compare?
//
// *** NOT FULLY TESTED - checkout source for exact routines not tested ***
//
class SingleShotTimer
{
public:
	SingleShotTimer();							// create a new single shot timer - intially disabled
	bool expired();								// returns TRUE when the timer has come to the end
	void start_timer(uint32_t minimum_time_in_ms);   // start (or restart) the timer with a specific target duration
	void hold_timer();  						// temporarily disabled the timer from incrementing
	void release_timer();						// release a previously held timer
	void disable_timer();						// stops it returing expired

private:
	uint32_t destination_time;
	uint32_t start_time;
	bool enabled;
	bool expired_flag;
	bool held;
};
#endif



class OneShotTimer
{
public:
	OneShotTimer();							// create a new single shot timer - intially disabled
	virtual ~OneShotTimer();
	virtual bool expired();							// returns true when the timer has come to the end
	virtual void start_time(uint32_t minimum_time_in_ms);   // start (or restart) the timer with a specific target duration
private:
	// this should really call a virtual member callback function to be inhertable
	static Uint32 callback(Uint32 interval, void* param);
	bool expired_flag;
	uint32_t requested_interval;
	SDL_TimerID sdl_timer;
};



class RepeatingTimer
{
public:
	RepeatingTimer();						// create a new single shot timer - intially disabled
	virtual ~RepeatingTimer();
	virtual bool expired();							// returns true **ONCE** when the timer has come to the end
	virtual void start_time(uint32_t minimum_time_in_ms);   // start (or restart) the timer with a specific target duration
private:
	// this should really call a virtual member callback function to be inhertable
	static Uint32 callback(Uint32 interval, void* param);
	bool expired_flag;
	uint32_t requested_interval;
	SDL_TimerID sdl_timer;
};


#endif // _TIMING_H_

