/*
 *  StdinThread.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 08/12/2013.
 *  Copyright 2013 Rob Probin. All rights reserved.
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

#ifndef STD_IN_THREAD_H
#define STD_IN_THREAD_H

#include "SDL.h"
#include <string>
#include <queue>

class StdinThread {
public:
	StdinThread();
	bool start();
	std::string get_line();	// return "" if no line available
	bool is_line_maybe_ready(); // not locking here, so only advisory, use get_line() for actual
private:
	int stdin_thread();
	static int stdin_thread(void *ptr);

	// data
	bool running;
	SDL_mutex *mutex;
	bool buffer_ready;
	std::queue<std::string> lines;
	SDL_Thread *thread;
};

#endif

