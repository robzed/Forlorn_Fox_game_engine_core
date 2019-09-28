//
//  LuaStateQueue.hpp
//  Forlorn_Fox_Mac
//
/*  Created by Rob Probin on 08/10/2015.
 * ------------------------------------------------------------------------------
 * Copyright (c) 2015 Rob Probin and Tony Park
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

#ifndef LuaStateQueue_h
#define LuaStateQueue_h

#include "SDL.h"
#include <string>
#include <queue>
#include "lua.h"

namespace luabridge
{
    class LuaRef;
};

class GenericLuaTypeStore;

#define THREADSAFE_LUASTATEQUEUE 1


// Single consumer, multiple producers
class LuaStateQueue {
public:
    LuaStateQueue(std::string identifier_name);
    ~LuaStateQueue();
    void send(lua_State* Lua_object);
    
    // Make it so any LuaState can read ... but only the owner should read.
    // We can enforce it using the dervied class below, but we don't so that
    // running everything from a single state is easier (for debugging).
    
    // thread-version will block if queue is empty...
    int read(lua_State *L);
    int read_timeout(lua_State *L);
    
    // this one will return nil if nothing is available
    //int try_read(lua_State *L);
    
    bool is_empty();
    std::string get_identifier();
    
private:
    std::queue<GenericLuaTypeStore*> q;
    std::string id;

#if THREADSAFE_LUASTATEQUEUE
    SDL_mutex* q_protect;
    SDL_cond* data_available_cond;
#endif

    
#if 0
//protected:
//    // functions for our Lua state
//    void get(std::string destination, LuaRef Lua_object);   // should this be non-blocking?
//    bool is_empty();
//};
//
//class LuaStateQueueOwned : public LuaStateQueue {
//public:
//    void read();
//    bool is_empty();
#endif

};


#endif /* LuaStateQueue_h */
