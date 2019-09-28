/*
 *  FrameRateLimiter.cpp
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

#include "FrameRateLimiter.h"


// +---------------------------------------------------------------------------
// | TITLE: 
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION: 
// +---------------------------------------------------------------------------
void FrameRateLimiter::limit(bool vsync)
{
	end = SDL_GetTicks();
	Uint32 frame_time = end - start;
	Uint32 delay_required = 1;
	if(target_frame_time > frame_time)
	{
		// has to be greater by at least 1 ... so minimum will be 1 to allow system to schedule
		delay_required = target_frame_time - frame_time;
	}
	
    if(vsync)
    {
        // should we make this optional based on very long times?
        //if(delay_required > 16)
        {
            // I wonder if the vsync yields back to system. If so, this is probably not
            // required... if it doesn't than platforms like MacOSX penalise tasks that
            // don't yield
            SDL_Delay(vsync_throttle);
        }
        // Other methods, like at very high frame rates (maybe vsync is broke?), maybe
        // we should limit the time in vsync a lot?
    }
    else
    {
        SDL_Delay(delay_required);
    }
	start = SDL_GetTicks();
}

// +---------------------------------------------------------------------------
// | TITLE: 
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION: 
// +---------------------------------------------------------------------------
void FrameRateLimiter::set(int frames_per_second)
{
	fps_target = frames_per_second;
	target_frame_time = 1000 / fps_target;
}


// +---------------------------------------------------------------------------
// | TITLE: 
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION: 
// +---------------------------------------------------------------------------
FrameRateLimiter::FrameRateLimiter()
: start(SDL_GetTicks())
, end(SDL_GetTicks())
, vsync_throttle(1)
{
	set(60);		// default fps
}

// +---------------------------------------------------------------------------
// | TITLE:
// | AUTHOR(s): Rob Probin
// | DATE STARTED: 21 Oct 11
// +
// | DESCRIPTION:
// +---------------------------------------------------------------------------
void FrameRateLimiter::test(int frames_per_second)
{
    set(frames_per_second);
    vsync_throttle = target_frame_time;
}
