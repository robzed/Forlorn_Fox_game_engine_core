/*
 *  FrameRateLimiter.h
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

#ifndef FRAMTERATELIMITER_H
#define FRAMTERATELIMITER_H

#include "SDL.h"

class FrameRateLimiter
{
public:
	void limit(bool vsync);
	void set(int frames_per_second);
	FrameRateLimiter();
    void test(int frames_per_second);
private:
	int fps_target;
	Uint32 target_frame_time;
	Uint32 start;
	Uint32 end;
    Uint32 vsync_throttle;
};


#endif
