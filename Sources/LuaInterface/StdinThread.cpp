/*
 *  StdinThread.cpp
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

#include "StdinThread.h"
#include <iostream>
using std::cout;
using std::endl;

StdinThread::StdinThread()
: running(false)
, buffer_ready(false)
{
}

bool StdinThread::start()
{
	if(not running)
	{
		mutex = SDL_CreateMutex();
		if (not mutex)
		{
			cout << "\nCouldn't create mutex in StdinThread " << SDL_GetError()
			<< endl;
		}
		else
		{
			running = true;
			thread = SDL_CreateThread(stdin_thread, "StdinThread", reinterpret_cast<void*>(this));
			if(not thread)
			{
				cout << "\nSDL_CreateThread failed in StdinThread::start(): "
				<< SDL_GetError() << endl;
			}
		}
	}
	return running;
}

bool StdinThread::is_line_maybe_ready()
{
	// not locking here, so only advisory, use get_line() for actual
	return running and buffer_ready;
}

std::string StdinThread::get_line()
{
	if(running and SDL_TryLockMutex(mutex) == 0 and buffer_ready and not lines.empty())
	{
		/* Do stuff while mutex is locked */
		std::string line = lines.front();
		lines.pop();
		if(lines.empty())
		{
			buffer_ready = false;
		}
		SDL_UnlockMutex(mutex);
		return line;
	}
	else
	{
		return "";
	}
}

int StdinThread::stdin_thread()
{
	if(running)
	{
		std::string input;
		while(true)
		{
			input = "";
			if(not std::getline(std::cin, input))	// test std::cin, loop if a problem...
			{
				if (std::cin.bad()) {
					cout << "IO error in stdin\n";
				} else if (!std::cin.eof()) {
					cout << "Format error in stdin\n";
				} else {
					cout << "EOF error in stdin\n";
				}
				cout << "Aborting stdin reading\n";
				break;	// quit thread
			}
			input += "\n";		// real input, terminate with CR

			if (SDL_LockMutex(mutex) == 0)
			{
				/* Do stuff while mutex is locked */
				lines.push(input);
				buffer_ready = true;
				SDL_UnlockMutex(mutex);
			} else {
				cout << "Couldn't lock mutex in stdin_thread " << SDL_GetError() << endl;
				cout << "Dropping line!" << endl;
			}
		}
	}
	cout << "stdin_thread() finished" << endl;
	return 0;
}

int StdinThread::stdin_thread(void* this_ptr)
{
	return reinterpret_cast<StdinThread*>(this_ptr)->stdin_thread();
}
