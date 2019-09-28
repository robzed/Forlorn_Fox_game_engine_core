//
//  LuaStateQueue.cpp
//  Forlorn_Fox_Mac
//
//  Created by Rob Probin on 08/10/2015.
/*
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

#include "LuaStateQueue.h"
#include "lauxlib.h"
#include "lualib.h"
#include "LuaBridge.h"
#include "Utilities.h"
#include <memory>   // for unique_ptr ... can't stand the various rule-of-three/rule-of-5 junk required otherwise


// Locking/signals based on: https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//
// Other references
// http://lazyfoo.net/tutorials/SDL/49_mutexes_and_conditions/index.php
// https://wiki.libsdl.org/SDL_LockMutex
// http://cboard.cprogramming.com/linux-programming/116184-best-solution-inter-thread-communication.html
// also see StdinThread
// http://fabiensanglard.net/doom3_bfg/threading.php
// http://stackoverflow.com/questions/5592583/different-mutex-for-push-and-pop
// https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem

// @todo: Make sure our mutex locking is exception safe by using RAII.

// On MUTEX vs. Atomic locks
// Most operations are short operations, hopefully. Could use atomic lock here
// but we also signal to wake a thread that's waiting ... which needs a proper Mutex.



// We use the Lua types here:
// LUA_TNIL, LUA_TNUMBER, LUA_TBOOLEAN, LUA_TSTRING, LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA, LUA_TTHREAD, and LUA_TLIGHTUSERDATA
// not allowed anywhere: thread, function, userdata, lightuserdata
// (and additionally table and nil not allowed as key in tables)
//
// For tables:
// Allowed as key: number, bool, string
// Allowed as value: nil, number, bool, string, table
//
// No int & float sub-type until Lua 5.3

// this is more like an advanced struct than a class
class GenericLuaTypeStore
{
public:
    GenericLuaTypeStore();
    ~GenericLuaTypeStore();
    //
    // data
    //
    // NOTICE: some of these we could union to save space. We don't worry about that, however.
    int type;
    double val;
    std::string s;
    
    int key_type;
    int key_val;
    std::string key_s;

    //std::unique_ptr<GenericLuaTypeStore> nested_into;   // points to a nested table
    //std::unique_ptr<GenericLuaTypeStore> list_next;     // points to the next element in the same table
    GenericLuaTypeStore* nested_into;   // points to a nested table
    GenericLuaTypeStore* list_next;     // points to the next element in the same table
private:
    //GenericLuaTypeStore& operator=();
    //GenericLuaTypeStore(GenericLuaTypeStore&);
    
};

GenericLuaTypeStore::GenericLuaTypeStore()
: type(-998), val(0), key_type(-999), key_val(0), nested_into(0), list_next(0)
{
}


GenericLuaTypeStore::~GenericLuaTypeStore()
{
    delete nested_into;
    nested_into = 0;
    delete list_next;
    list_next = 0;
}


namespace  {
    

    void copy_object_from_lua_state(lua_State* L, GenericLuaTypeStore& obj)
    {
        int index = -1;
        int ltype = lua_type(L, index);
        
        // initialise this object
        obj.type = ltype;
        
        // create the value for this type
        switch(ltype)
        {
            case LUA_TNIL:
                break;
            case LUA_TBOOLEAN:
                obj.val = lua_toboolean(L, index);
                break;
            case LUA_TNUMBER:
                obj.val = lua_tonumber(L, index);
                break;
            case LUA_TSTRING:
            {
                size_t len;
                const char* c = lua_tolstring(L, index, &len);
                obj.s = std::string(c, len);
                break;
            }
            case LUA_TTABLE:
            {
                int t = -2;
                GenericLuaTypeStore** link = &obj.nested_into;  // the top level object gets a nested pointer
                
                /* table is in the stack at index 't' */
                lua_pushnil(L);  /* first key */
                while (lua_next(L, t) != 0) {
                    int ktype = lua_type(L, -2);
                    // only allow specific key type
                    if(ktype == LUA_TBOOLEAN or ktype == LUA_TNUMBER or ktype == LUA_TSTRING)
                    {
                        GenericLuaTypeStore* new_obj = new GenericLuaTypeStore;
                        *link = new_obj;    // point to next object (either higher up, or previous in table)
                        link = &new_obj->list_next;
                        
                        /* uses 'key' (at index -2) and 'value' (at index -1) */
                        //printf("%s - %s\n",
                        //       lua_typename(L, ktype),
                        //       lua_typename(L, lua_type(L, -1)));
                        
                        // set up value
                        copy_object_from_lua_state(L, *new_obj);
                        
                        // set up key
                        new_obj->key_type = lua_type(L, -2);
                        if(ktype == LUA_TBOOLEAN)
                        {
                            new_obj->key_val = lua_toboolean(L, -2);
                        }
                        else if(ktype == LUA_TNUMBER)
                        {
                            new_obj->key_val = lua_tonumber(L, -2);
                        }
                        else if(ktype == LUA_TSTRING)
                        {
                            size_t len;
                            const char* c = lua_tolstring(L, -2, &len);
                            new_obj->key_s = std::string(c, len);
                        }
                        else
                        {
                            Utilities::fatalError("Coding error in copy_object_from_lua_state");
                        }

                    }
                    /* removes 'value'; keeps 'key' for next iteration */
                    lua_pop(L, 1);
                }
                break;
            }
            default:
                // don't know what to do with this type
                obj.type = LUA_TNIL;
                break;
        }
    }

    int copy_object_to_lua_state(lua_State* L, GenericLuaTypeStore& obj)
    {
        switch(obj.type)
        {
            case LUA_TNIL:
                //printf("nil\n");
                lua_pushnil(L);
                break;
            case LUA_TBOOLEAN:
                //printf("boolean\n");
                lua_pushboolean(L, obj.val);
                break;
            case LUA_TSTRING:
                //printf("string %s\n", obj.s.c_str());
                lua_pushlstring(L, obj.s.c_str(), obj.s.length());
                break;
            case LUA_TNUMBER:
                //printf("number\n");
                lua_pushnumber(L, obj.val);
                break;
            case LUA_TTABLE:
            {
                //printf("table\n");
                GenericLuaTypeStore* current = obj.nested_into;
                //GenericLuaTypeStore* current = obj.nested_into.get();       // @todo: get() is horrible, replace with nicer mechanism
                // note: empty tables are valid!
                lua_newtable(L);
                while(current)
                {
                    switch(current->key_type)
                    {
                        case LUA_TBOOLEAN:
                            //printf("k-bool %d\n", current->key_val);
                            lua_pushboolean(L, current->key_val);
                            break;
                        case LUA_TSTRING:
                            //printf("k-string %s\n", current->key_s.c_str());
                            lua_pushlstring(L, current->key_s.c_str(), current->key_s.length());
                            break;
                        case LUA_TNUMBER:
                            //printf("k-number %d\n", current->key_val);
                            lua_pushnumber(L, current->key_val);
                            break;
                        default:
                            Utilities::fatalError("Unknown key type in copy_object_to_lua_state()");
                            break;
                    }
                    
                    copy_object_to_lua_state(L, *current);
                    // t[k] = v, where t is the value at the given index, v is the value at the top of the stack, and k is the value just below the top.
                    // function pops key and value
                    lua_rawset(L, -3);  // same as lua_settable()a
                    current = current->list_next;
                    //current = current->list_next.get();        // @todo: get() is horrible, replace with nicer mechanism
                }
                break;
            }
            default:
                Utilities::fatalError("Unknown type in copy_object_to_lua_state()");
                break;
        }
        
        return 1;
    }

}

static void LSQ_abort(const char* s)
{
    Utilities::fatalError("LuaStateQueue failed to %s (SDL Error %s)",  s, SDL_GetError());
}


LuaStateQueue::LuaStateQueue(std::string identifier_name)
:id(identifier_name)
{
#if THREADSAFE_LUASTATEQUEUE
    q_protect = SDL_CreateMutex(); if(q_protect == 0) {  LSQ_abort("create mutex"); }
    data_available_cond = SDL_CreateCond(); if(data_available_cond == 0) { LSQ_abort("create condition"); }
#endif
}

LuaStateQueue::~LuaStateQueue()
{
#if THREADSAFE_LUASTATEQUEUE
    SDL_DestroyMutex(q_protect);
    SDL_DestroyCond(data_available_cond);
#endif
}


// add something to the queue
void LuaStateQueue::send(lua_State* Lua_object)
{
    //if(replyQ_source==0)
    //{
    //    Utilities::fatalError("replyQ_source cannot be NIL");
    //}
    
    GenericLuaTypeStore* temp = new GenericLuaTypeStore;
    copy_object_from_lua_state(Lua_object, *temp);
    
#if THREADSAFE_LUASTATEQUEUE
    if(SDL_LockMutex(q_protect) != 0) { LSQ_abort("lock mutex in send"); }
#endif
    q.push(temp);           // simple pointer copy
#if THREADSAFE_LUASTATEQUEUE
    if(SDL_UnlockMutex(q_protect) != 0) { LSQ_abort("unlock mutex in send"); }
    if(SDL_CondSignal(data_available_cond) != 0) { LSQ_abort("signal"); }
#endif
    // we could check if it's empty before signalling ... if there is only
    // one reader to avoid unnessary signalling ... but "condition variable notify calls are pretty light-weight when there are no threads waiting."
    // could have used SDL_CondBroadcast() if multiple readers were to be woken
}

// get something from the queue
int LuaStateQueue::read(lua_State *L)
{
    lua_pushnumber(L, SDL_MUTEX_MAXWAIT);
    return read_timeout(L);
}

int LuaStateQueue::read_timeout(lua_State *L)
{
    Uint32 ms = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    int return_values = 0;
#if THREADSAFE_LUASTATEQUEUE
    // wait for something in queue?
    if(SDL_LockMutex(q_protect) != 0) { LSQ_abort("lock mutex in read"); }
    while(q.empty())
    {
        int result = SDL_CondWaitTimeout(data_available_cond, q_protect, ms);
        if (result == SDL_MUTEX_TIMEDOUT)
        {
            lua_pushnil(L);
            return 1;
        }
        else if(result != 0)
        {
            LSQ_abort("wait on signal");
        }
    }
    // return item from queue
#else
    if(!q.empty())
    {
#endif
        GenericLuaTypeStore* temp =  q.front();
        q.pop();
#if THREADSAFE_LUASTATEQUEUE
        if(SDL_UnlockMutex(q_protect) != 0) { LSQ_abort("unlock mutex in read"); }
#endif
        
        if(temp->list_next)
        {
            Utilities::fatalError("Why is temp.list_next on top level set?");
        }
        //temp.list_next = 0;     // make sure that next is not set for top level item. Shouldn't be ... but just in case
        
        return_values =  copy_object_to_lua_state(L, *temp);

        // manual memory management ... replace with unique_ptr()
        delete temp;
        
#if (THREADSAFE_LUASTATEQUEUE == 0)
    }
    else
    {
        // what should we do in this case?
        // maybe we should suspend until the queue is empty?
        //  or return nil immediately?
        //  or wait for an amount of time specified by an option input parameter?
        Utilities::fatalError("queue is empty - case not handled yet");
    }
#endif
    return return_values;
}


// is anything in queue?
bool LuaStateQueue::is_empty()
{
#if THREADSAFE_LUASTATEQUEUE
    if(SDL_LockMutex(q_protect) != 0) { LSQ_abort("lock mutex in is_empty"); }
#endif
    bool result = q.empty();
#if THREADSAFE_LUASTATEQUEUE
    if(SDL_UnlockMutex(q_protect) != 0) { LSQ_abort("unlock mutex in is_empty"); }
#endif

    return result;
}

std::string LuaStateQueue::get_identifier()
{
    return id;
}


