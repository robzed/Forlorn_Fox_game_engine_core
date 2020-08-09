/*
 *  LuaCppInteface.cpp
 *  This module provides the patching between Lua and the C++ Engine
 *
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 25/11/2013.
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

#include "GameConfig.h"
#include "LuaCppInterface.h"
#include "lauxlib.h"
#include "LuaMain.h"

#include "LuaBridge.h"
#include "AppResourcePath.h"
#include "LoadPath.h"
#include "SaveDataPath.h"
#include "SaveDataPathDeveloper.h"
#include "GameApplication.h"
#include "MySoundManager.h"
#include "Utilities.h"
#include "DrawList.h"
#include "Utilities.h"
#include "PresentationMaze.h"
#include "Debug.h"
#include "MapUtils.h"
#include "GameToScreenMapping.h"
#include "glyph_set_utilities.h"
#include "MouseTarget.h"
#include "LuaStateQueue.h"
#include "md5.h"
#include "sha224.hpp"
#include "sha256.hpp"
#include "sha384.hpp"
#include "sha512.hpp"
#include "lodepng.h"
#include "program_launching.h"
#include "image_loader.h"

#ifdef __ANDROID__
	#include "sys/stat.h"
#endif

// There are plenty of bridge/binding libraries. It would easily be possible to
// do this manually, but (after looking at it for a couple of days) seemed a
// bit boring so I used LuaBridge which seems ok.
//
// MANUALLY
//
// http://loadcode.blogspot.co.uk/2007/02/wrapping-c-classes-in-lua.html
//		Note: This previous article states that only userdata has finalizers, not
//			tables - but this isn't true for Lua5.2, that has finalizers on both
//			userdata and tables.
// Also http://lua-users.org/lists/lua-l/2005-10/msg00091.html
// Details on game engine managing object  http://stackoverflow.com/questions/19388427/lua-c-class-binding-garbage-collection-off
// Naive: http://stackoverflow.com/questions/11177746/lua-userdata-object-management
//
//
// For more 'automated' bridge/binding library options, see the following.
// We probably will end up writing a bunch of Proxies.
//
// Huge amount of methods for C++ in http://lua-users.org/wiki/BindingCodeToLua
//
// Also http://www.lua.org/notes/ltn005.html
// Quite like this one ... http://vinniefalco.com/LuaBridge/Manual.html
//
// This one might be marginally faster https://code.google.com/p/oolua/ but the
//    macros look UGLY...
//
// This requires boost http://www.rasterbar.com/products/luabind.html
// Also: http://lua-users.org/wiki/CppBindingWithLunar
//
// This looks quite simple and good https://code.google.com/p/lualite/ but
// maybe LuaBridge is a bit more fully features?
//
// LuaWrapper
// https://bitbucket.org/alexames/luawrapper/src/e87f24c39849b9d7e0352550d5bc8b79dc4b9fc8/luawrapper.hpp?at=default
// https://bitbucket.org/alexames/luawrapperexample/src
// From: http://stackoverflow.com/questions/7286486/how-to-sync-lua-and-c-garbage-collection
//
// Placement new and C++ allocation in user data
// http://lua-users.org/lists/lua-l/2012-05/msg00399.html
//
// Lua userdata object management
// http://stackoverflow.com/questions/11177746/lua-userdata-object-management
//
// LuaCpp bridge https://github.com/JakobOvrum/LuaCpp
// LuaC++ makes use of the following C++0x features, which means your favorite
// compiler might not yet be able to compile and use LuaC++:
//		variadic templates
//		rvalue references
//		the tuple module
//
// Additionally, as LuaC++ uses C++ exceptions for error handling, make sure your Lua library is compiled as C++.
//
// Std::function http://www.stroustrup.com/C++11FAQ.html#std-function
// C++11 http://en.wikipedia.org/wiki/C%2B%2B11


const std::string master_table_name = "gulp";


// thin wrapper for SDL Thread
class LuaThread {
public:
    // no parameters into Lua allowed?
    LuaThread(LuaMain* lua, const char* lua_function, const char* name)
    :lua_function_name(lua_function), l(lua), running(true)
    {
        thread = SDL_CreateThread(run_thread_function, name, (void*)this);
        // throw an except from a constructor on failure?
        if(not thread)
        {
            running = false;
            //lua_function = SDL_GetError();
            if(thread==0) Utilities::fatalErrorSDL("Failed to create thread");
        }
    }
    ~LuaThread()
    {
//        if(not running)
//        {
//            SDL_WaitThread(thread, NULL);
//        }
        // what should we do here?
        // stop? wait for thread?
//        if(thread)
        if(running)
        {
           std::string s = "Trying to destroy thread still running - ";
           s += GetThreadName();
           Utilities::fatalError(s);
        }

    }
    //bool failed()
    //{
    //    return thread==0;
    //}
    SDL_threadID GetThreadID()
    {
        if(thread==0) Utilities::fatalError("thread invalid in GetThreadID");
        return SDL_GetThreadID(thread);
    }
    const char* GetThreadName()
    {
        if(thread==0) Utilities::fatalError("thread invalid in GetthreadName");
        return SDL_GetThreadName(thread);
    }
    int WaitThread()
    {
        if(thread==0) Utilities::fatalError("thread invalid in WaitThread");
        int threadReturnValue;
        SDL_WaitThread(thread, &threadReturnValue);
        thread = 0;
        return threadReturnValue;
    }
    bool IsRunning()
    {
        return running;
    }
    
    // Link problem...
    //void DetachThread()
    //{
    //    SDL_DetachThread(thread);
    //}
private:
    SDL_Thread* thread;
    std::string lua_function_name;
    LuaMain* l;
    bool running;           // is the thread running code (or potentially running code)
    
    static int run_thread_function(void *ptr)
    {
        int return_value=-1;		// should never get this one... all code paths go through fatal error or set return_value
        LuaThread* obj = (LuaThread*) ptr;
        if(obj)
        {
            LuaMain& L = *obj->l;
            int error = run_gulp_function_if_exists(obj->l, obj->lua_function_name.c_str(), 0, 1);
            if(error == LUA_OK)
            {
                return_value = (int)lua_tonumber(L, -1);
                lua_pop(L, 1);
            }
            else
            {
                Utilities::fatalError("run_thread_function failed to call function %s", obj->lua_function_name.c_str());
            }
        }
        else
        {
            Utilities::fatalError("run_thread_function passed no thread pointer");
        }
        
        obj->running = false;
        return return_value;
    }
};


SDL_threadID GetCurrentThreadID()
{
    return SDL_ThreadID();
}
int SetCurrentThreadPriority(SDL_ThreadPriority priority)
{
    int error = SDL_SetThreadPriority(priority);
    //if(error != 0)
    //{
    //    Utilities::fatalErrorSDL("Couldn't set thread priority");
    //}
    return error;
}



// NOTE: My version of LuaBridge fixes one error
// There are also other pending pull requests on github project.

//
// Create a Stack specialisation for sound_t ... otherwise we get wierd table
// errors on trying to call functions that take this as a parameter...
//
/*
namespace luabridge
{
	template <>
	struct Stack <sound_t>
	{
	static void push (lua_State* L, sound_t sound)
	{
		lua_pushinteger (L, sound);
	}

	static sound_t get (lua_State* L, int index)
	{
		return sound_t (luaL_checkinteger (L, index));
	}
	};
}*/
namespace luabridge
{
	template <>
	struct Stack <simple_colour_t>
	{
		static void push (lua_State* L, simple_colour_t colour)
		{
			lua_pushinteger (L, colour);
		}

		static simple_colour_t get (lua_State* L, int index)
		{
			return simple_colour_t (luaL_checkinteger (L, index));
		}
	};
}

/* translate a relative string position: negative means back from end */
static size_t pos_relative(ptrdiff_t pos, size_t len) {
	if (pos >= 0) return (size_t)pos;
	else if (0u - (size_t)pos > len) return 0;
	else return len - ((size_t)-pos) + 1;
}

// helper function for patching into lua_cpp
int from_utf8(lua_State*L)
{
	size_t l;
	const char* string = luaL_checklstring(L, 1, &l);
	size_t posi = pos_relative(luaL_optinteger(L, 2, 1), l);

	//int offset = 0;
	//if(lua_isnumber(L, 2))
	//{
	//	offset = lua_tointeger(L, 2))-1;
	//}
	// what about negative offsets??????
	int bytes_back;
	int value = from_utf8(&bytes_back, (const unsigned char*)(string+posi-1)); //+offset));
	lua_pushinteger(L, value);
	lua_pushinteger(L, bytes_back);
	return 2;
}

int table_from_utf8_string(lua_State*L)
{
	size_t length;
	const char* string = luaL_checklstring(L, 1, &length);
	size_t posi = pos_relative(luaL_optinteger(L, 2, 1), length);

	//int bytes_back;
	lua_newtable(L);
	int counter = 1;
	int bytes_back;
	string += (posi-1);
	size_t offset = 0;
	do
	{
		int value = from_utf8(&bytes_back, (const unsigned char*)(string));
		string += bytes_back;
		offset += bytes_back;
		lua_pushnumber(L, value);
		lua_rawseti(L, -2, counter++);
	} while(offset < length);
	return 1;
}


//
// one problem is that this routine might extend over string selected...
//
// modified version of str_byte()
int utf8_byte(lua_State*L)
{
	size_t l;
	const char *s = luaL_checklstring(L, 1, &l);
	size_t posi = pos_relative(luaL_optinteger(L, 2, 1), l);
	size_t pose = pos_relative(luaL_optinteger(L, 3, posi), l);

	if (posi < 1) posi = 1;
	if (pose > l) pose = l;
	if (posi > pose) return 0;  /* empty interval; return no values */
	int n = (int)(pose -  posi + 1);
	if (posi + n <= pose)  /* (size_t -> int) overflow? */
		return luaL_error(L, "string slice too long");
	luaL_checkstack(L, n, "string slice too long");
	int i=0;
	int entries = 0;
	while(i<n)
	{
		int utf8_len;
		lua_pushinteger(L, from_utf8(&utf8_len, (const unsigned char*)&s[posi+i-1]));
		i += utf8_len;
		entries ++;
	}
	return entries;
}

// modified version of str_char()
// speed is limited by std::string concatenation
int utf8_char(lua_State*L)
{
	std::string s;
	int n = lua_gettop(L);  /* number of arguments */
	for (int i=1; i<=n; i++) {
		int c = luaL_checkint(L, i);
		luaL_argcheck(L, (c >= 0) and (c <= 0x10FFFF), i, "value out of range");
		to_utf8_string(s, c);
	}
	lua_pushlstring(L, s.c_str(), s.length());
	return 1;
}

static void SDL_VERSION_helper(SDL_version* ver)
{
    SDL_VERSION(ver);
}
static void SDL_MIXER_VERSION_helper(SDL_version* ver)
{
    SDL_MIXER_VERSION(ver);
}

static bool SDL_IsTextInputActive_helper()
{
    return 0 != SDL_IsTextInputActive();
}

static void LuaMain_push_number(LuaMain *L, lua_Number n)
{
    lua_pushnumber(L->get_internal_state(), n);
}

static void LuaMain_push_queue(LuaMain *L, LuaStateQueue* q)
{
    luabridge::push(L->get_internal_state(), q);
}

class RendererInfo {
//private:
    SDL_RendererInfo info;
    int error_num;
    std::string error_str;
public:
    //RendererInfo(SDL_Renderer* r) { error_num = SDL_GetRendererInfo(r, &info); if(error_num) error_str = SDL_GetError(); }
    //RendererInfo(int index) { error_num = SDL_GetRenderDriverInfo(index, &info); if(error_num) error_str = SDL_GetError(); }
    RendererInfo(SDL_Renderer* r, int index) {
        if (r) {
            error_num = SDL_GetRendererInfo(r, &info);
        }
        else {
            error_num = SDL_GetRenderDriverInfo(index, &info);
        }
        if(error_num) error_str = SDL_GetError();
    }
    static int number_renderers() { return SDL_GetNumRenderDrivers(); }
    int error(lua_State* L) {
        lua_pushinteger(L, error_num);
        if(error_num == 0) return 1;
        lua_pushstring(L, error_str.c_str());
        return 2;
    }
    const char* name() const { return info.name; }
    Uint32 flags() const { return info.flags; }
    Uint32 num_texture_formats() const { return info.num_texture_formats; }
    Uint32 get_texture_format(int index) { if (index >= 0 and index < 16) return info.texture_formats[index]; else return 0; }
    int max_texture_width() const { return info.max_texture_width; }
    int max_texture_height() const { return info.max_texture_height; }
    
    // flags conversions
    Uint32 software_flags_mask() const { return SDL_RENDERER_SOFTWARE; }
    Uint32 accleratated_flags_mask() const { return SDL_RENDERER_ACCELERATED; }
    Uint32 vsync_flags_mask() const { return SDL_RENDERER_PRESENTVSYNC; }
    Uint32 target_texture_flags_mask() const { return SDL_RENDERER_TARGETTEXTURE; }
    bool software_renderer() const       { return (flags() | SDL_RENDERER_SOFTWARE) ? true:false; }
    bool accelerated_renderer() const    { return (flags() | SDL_RENDERER_ACCELERATED) ? true:false; }
    bool vsync_renderer() const          { return (flags() | SDL_RENDERER_PRESENTVSYNC) ? true:false; }
    bool target_texture_renderer() const { return (flags() | SDL_RENDERER_TARGETTEXTURE) ? true:false; }
    
    // pixel format conversions
    const char* get_pixel_format_name(int index) { return SDL_GetPixelFormatName(get_texture_format(index)); }
};
// ------------------------------------------------------------------------------
// MD5 helper class

class MD5 {
public:
    MD5() { MD5_Init(&my_md5); }
    void update( const std::string s ) { MD5_Update(&my_md5, s.c_str(), s.length());}
    std::string final() {
        char result[16];
        MD5_Final((unsigned char *)result, &my_md5);
        std::string myString(result, 16);
        return myString;
    }
private:
    MD5_CTX my_md5;
};
// ------------------------------------------------------------------------------
// helper class

class SHA224 {
public:
    SHA224() { sha_init(state); }
    void update( const std::string s ) { sha_process(state, s.c_str(), static_cast<std::uint32_t>(s.length()));}
    std::string final() {
        const int size = 224/8;
        char result[size];
        sha_done(state, (unsigned char *)result);
        std::string myString(result, size);
        return myString;
    }
private:
    sha224_state state;
};
// ------------------------------------------------------------------------------
// helper class

class SHA256 {
public:
    SHA256() { sha_init(state); }
    void update( const std::string s ) { sha_process(state, s.c_str(), static_cast<std::uint32_t>(s.length()));}
    std::string final() {
        const int size = 256/8;
        char result[size];
        sha_done(state, (unsigned char *)result);
        std::string myString(result, size);
        return myString;
    }
private:
    sha256_state state;
};
// ------------------------------------------------------------------------------
// helper class

class SHA384 {
public:
    SHA384() { sha_init(state); }
    void update( const std::string s ) { sha_process(state, s.c_str(), static_cast<std::uint32_t>(s.length()));}
    std::string final() {
        const int size = 384/8;
        char result[size];
        sha_done(state, (unsigned char *)result);
        std::string myString(result, size);
        return myString;
    }
private:
    sha384_state state;
};
// ------------------------------------------------------------------------------
// helper class

class SHA512 {
public:
    SHA512() { sha_init(state); }
    void update( const std::string s ) { sha_process(state, s.c_str(), static_cast<std::uint32_t>(s.length()));}
    std::string final() {
        const int size = 512/8;
        char result[size];
        sha_done(state, (unsigned char *)result);
        std::string myString(result, size);
        return myString;
    }
private:
    sha512_state state;
};

class PerformanceFrequencyCounter {
public:
   PerformanceFrequencyCounter() { t = SDL_GetPerformanceCounter(); }
   void set_now() { t = SDL_GetPerformanceCounter(); }
   double result_of_subtract_arg(PerformanceFrequencyCounter* p) {
      if(p)
      {
         return static_cast<double>(t - p->t)/SDL_GetPerformanceFrequency();
      }
      return 0.0;
      }
private:
   Uint64 t;
};

//
// CRC32 function
//
int calculate_crc32(lua_State*L)
{
    size_t length;
    const unsigned char* buf = (const unsigned char*)luaL_checklstring(L, 1, &length);
    
    lua_pushunsigned(L, lodepng_crc32(buf, length));
    return 1;
}
//
// Inflate function
//
int inflate(lua_State*L)
{
    size_t insize;
    const unsigned char* in = (const unsigned char*)luaL_checklstring(L, 1, &insize);

    unsigned char* out = 0;
    size_t outsize = 0;
    LodePNGDecompressSettings settings;
    lodepng_decompress_settings_init(&settings);
    
    /*Inflate a buffer. Inflate is the decompression step of deflate. Out buffer must be freed after use.*/
    unsigned error = lodepng_inflate(&out, &outsize,
                             in, insize,
                             &settings);

    
    lua_pushlstring(L, (char*)out, outsize);
    free(out);

    if(error)
    {
        lua_pushunsigned(L, error);
        lua_pushstring(L, lodepng_error_text(error));
        return 3;
    }

    return 1;
}

static int SDL_GetModState_helper()
{
    return SDL_GetModState();
}

static void debugMessageC(const char* string)
{
    Utilities::debugMessage(string);
}


static int SDL_GetWindowSize_helper(lua_State*L)
{
    int w, h;
    SDL_Window *window = luabridge::Userdata::get<SDL_Window>(L, 1, true);
    if(window)
    {
        SDL_GetWindowSize(window, &w, &h);
        lua_pushinteger(L, w);
        lua_pushinteger(L, h);
    }
    else
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "get SDL_Window failed");
    }
    return 2;
}

static int SDL_GL_GetDrawableSize_helper(lua_State*L)
{
    int w, h;
    SDL_Window *window = luabridge::Userdata::get<SDL_Window>(L, 1, true);
    if(window)
    {
        SDL_GL_GetDrawableSize(window, &w, &h);
        lua_pushinteger(L, w);
        lua_pushinteger(L, h);
    }
    else
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "get SDL_Window failed");
    }
    return 2;
}

static int SDL_GetRendererOutputSize_helper(lua_State*L)
{
    int w, h;
    SDL_Renderer *renderer = luabridge::Userdata::get<SDL_Renderer>(L, 1, true);
    if(renderer)
    {
        int error = SDL_GetRendererOutputSize(renderer, &w, &h);
        if(error)
        {
            lua_pushboolean(L, false);
            lua_pushstring(L, SDL_GetError());
        }
        else
        {
            lua_pushinteger(L, w);
            lua_pushinteger(L, h);
        }
    }
    else
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "get SDL_Renderer failed");
    }
    
    return 2;
}

int SDL_SetTextureBlendMode_helper(SDL_Texture*  texture, int blendMode)
{
    return SDL_SetTextureBlendMode(texture, static_cast<SDL_BlendMode>(blendMode));
}

double SDL_GetPerformanceCounter_helper()
{
   return SDL_GetPerformanceCounter();
}
double SDL_GetPerformanceFrequency_helper()
{
   return SDL_GetPerformanceFrequency();
}
bool SDL_AUDIO_ISFLOAT_helper(SDL_AudioFormat val)
{
   return SDL_AUDIO_ISFLOAT(val);
}

// ------------------------------------------------------------------------------
//
// We bind all the Forlorn Fox standard C++ classes in here, rather than spread them out over the project.
//
// ------------------------------------------------------------------------------
static void set_up_basic_ff_cpp_bindings(lua_State *L, std::string base_table_name)
{
    using namespace luabridge;
    
    //
    // Define some classes
    //
    getGlobalNamespace (L)
    .beginNamespace (base_table_name.c_str())
    //
    // simple functions
    //
    .addFunction ("to_utf8", to_utf8)
    .addCFunction("from_utf8", from_utf8)
    .addCFunction("utf8_byte", utf8_byte)
    .addCFunction("utf8_char", utf8_char)
    .addCFunction("table_from_utf8_string", table_from_utf8_string)
    //.addFunction("fatalError", Utilities::fatalErrorLua)  // use error("message") from Lua instead
    .addFunction("SDL_GetTicks", SDL_GetTicks)

    .addFunction("SDL_GetRevision", SDL_GetRevision)
    .addFunction("SDL_GetRevisionNumber", SDL_GetRevisionNumber)
    .addFunction("SDL_GetVersion", SDL_GetVersion)
    .addFunction("SDL_VERSION", SDL_VERSION_helper)
    .addFunction("Mix_Linked_Version", Mix_Linked_Version)
    .addFunction("SDL_MIXER_VERSION", SDL_MIXER_VERSION_helper)
    .addFunction("SDL_GetPlatform", SDL_GetPlatform)
    .addFunction("SDL_GetCPUCount", SDL_GetCPUCount)
    .addFunction("SDL_Delay", SDL_Delay)
    .addFunction("SDL_GetPerformanceCounter", SDL_GetPerformanceCounter_helper)
    .addFunction("SDL_GetPerformanceFrequency", SDL_GetPerformanceFrequency_helper)
    .beginClass<PerformanceFrequencyCounter>("PerformanceFrequencyCounter")
      .addConstructor <void (*) (void)> ()
      .addFunction("set_now", &PerformanceFrequencyCounter::set_now)
      .addFunction("result_of_subtract_arg", &PerformanceFrequencyCounter::result_of_subtract_arg)
    .endClass()

    .addFunction("SetWindowTitle", SetWindowTitle)

    // more SDL stuff
    .addFunction("SDL_GetModState", SDL_GetModState_helper)
    .addFunction("SDL_InitSubSystem", SDL_InitSubSystem)
    .addFunction("SDL_GetError", SDL_GetError)
    .addFunction("SDL_RenderClear", SDL_RenderClear)
    .addFunction("SDL_RenderPresent", SDL_RenderPresent)
    .addFunction("SDL_AUDIO_ISFLOAT", SDL_AUDIO_ISFLOAT_helper)
   
	.addFunction("map_transform", MapUtils::map_transform)
    .addCFunction("calculate_crc32", calculate_crc32)
    .addCFunction("inflate", inflate)
    
    .beginClass<MD5>("MD5")
        .addConstructor <void (*) (void)> ()
        .addFunction("update", &MD5::update)
        .addFunction("final", &MD5::final)
    .endClass()
    
    .beginClass<SHA224>("SHA224")
    .addConstructor <void (*) (void)> ()
    .addFunction("update", &SHA224::update)
    .addFunction("final", &SHA224::final)
    .endClass()
    
    .beginClass<SHA256>("SHA256")
    .addConstructor <void (*) (void)> ()
    .addFunction("update", &SHA256::update)
    .addFunction("final", &SHA256::final)
    .endClass()
    
    .beginClass<SHA384>("SHA384")
    .addConstructor <void (*) (void)> ()
    .addFunction("update", &SHA384::update)
    .addFunction("final", &SHA384::final)
    .endClass()
    
    .beginClass<SHA512>("SHA512")
    .addConstructor <void (*) (void)> ()
    .addFunction("update", &SHA512::update)
    .addFunction("final", &SHA512::final)
    .endClass()
    
    //
    // Class definitions
    //
    //
    
    .beginClass<SDL_version>("SDL_version")
    .addConstructor <void (*) ()> ()
    .addData("major", &SDL_version::major)
    .addData("minor", &SDL_version::minor)
    .addData("patch", &SDL_version::patch)
    .endClass()
    
    .beginClass <AppResourcePath> ("AppResourcePath")
    .addConstructor <void (*) (std::string)> ()
    .addFunction ("c_str", &AppResourcePath::c_str)
    .addFunction ("str", &AppResourcePath::str)
    .addStaticFunction ("override", &AppResourcePath::override)
    .endClass ()
    //
    .beginClass <LoadPath> ("LoadPath")
    .addConstructor <void (*) (std::string)> ()
    .addFunction ("c_str", &LoadPath::c_str)
    .addFunction ("str", &LoadPath::str)
    .endClass ()
    //
    .beginClass <SaveDataPath> ("SaveDataPath")
    .addConstructor <void (*) (std::string)> ()
    .addFunction ("c_str", &SaveDataPath::c_str)
    .addFunction ("str", &SaveDataPath::str)
    .addStaticFunction ("override", &SaveDataPath::override)
    .addStaticFunction("set_subfolder", &SaveDataPath::set_subfolder)
    .addStaticFunction("set_org_name", &SaveDataPath::set_org_name)
    .addStaticFunction("set_app_name", &SaveDataPath::set_app_name)
    .addStaticFunction("get_base_pref_path", &SaveDataPath::get_base_pref_path)
    .endClass ()
    
    .beginClass <SaveDataPathDeveloper> ("SaveDataPathDeveloper")
    .addConstructor <void (*) (std::string)> ()
    .addFunction ("c_str", &SaveDataPathDeveloper::c_str)
    .addFunction ("str", &SaveDataPathDeveloper::str)
    .addFunction ("set_developer_mode", &SaveDataPathDeveloper::set_developer_mode)
    .endClass ()
    
    

    .beginClass <PresentationMaze> ("PresentationMaze")
    .addConstructor <void (*) (double, double)> ()
    //.addFunction("set_maze_colours", &PresentationMaze::set_maze_colours)
    .addFunction("set_default_maze_colours", &PresentationMaze::set_default_maze_colours)
    .addFunction("load_current_maze", &PresentationMaze::load_current_maze)
    .addFunction("set_offset", &PresentationMaze::set_offset)
    .addFunction("width", &PresentationMaze::width)
    .addFunction("height", &PresentationMaze::height)
    //.addFunction("get_maze_draw_list", &PresentationMaze::get_maze_draw_list)
    //.addFunction("get_mobs_draw_list", &PresentationMaze::get_mobs_draw_list)
    //.addFunction("set_view_layer", &PresentationMaze::set_view_layer)
    .addFunction("get_glyph", &PresentationMaze::get_glyph)
    .addFunction("set_glyph", &PresentationMaze::set_glyph)
    .addFunction("set_rotation", &PresentationMaze::set_rotation)
    .addFunction("line_of_sight", &PresentationMaze::line_of_sight)
    //.addFunction("print", &PresentationMaze::print)
    //.addFunction("print_selected", &PresentationMaze::print_selected)
    .addFunction("set_rect", &PresentationMaze::set_rect)
    .addFunction("zoom", &PresentationMaze::zoom)
	.addFunction("set_render_option", &PresentationMaze::set_render_option)
    .endClass()
    

    .beginClass <MapUtils::MapFinder>("MapFinder")
    .addConstructor <void (*) (PresentationMaze*)> ()
    .addFunction("add_target", &MapUtils::MapFinder::add_target)
    .addFunction("find_anywhere", &MapUtils::MapFinder::find_anywhere)
    .addData("line", &MapUtils::MapFinder::line, &MapUtils::MapFinder::line)
    .addData("column", &MapUtils::MapFinder::column, &MapUtils::MapFinder::column)
    //.addProperty("line", &MapUtils::MapFinder::line, , &MapUtils::MapFinder::line)
    //.addProperty("column", &MapUtils::MapFinder::column, &MapUtils::MapFinder::column)
    .endClass()
    
    .beginClass <LuaStateQueue>("LuaStateQueue")
    .addConstructor <void (*) (std::string)> ()
    .addFunction("send", &LuaStateQueue::send)
    .addCFunction("read", &LuaStateQueue::read)
    .addCFunction("read_timeout", &LuaStateQueue::read_timeout)
    .addFunction("is_empty", &LuaStateQueue::is_empty)
    .addFunction("get_identifier", &LuaStateQueue::get_identifier)
    .endClass()
    
    .beginClass <LuaMain> ("LuaMain")
        .addConstructor<void (*) () >()
    //.addFunction("open_console", &LuaMain::open_console)
    //.addFunction("get_CLI", &LuaMain::get_CLI)
    .endClass ()

    .addFunction("set_up_basic_ff_libraries", set_up_basic_ff_libraries)
    .addFunction("load_main_file", load_main_file)
    .addFunction("load_conf_file", load_conf_file)
    .addFunction("run_gulp_function_if_exists", run_gulp_function_if_exists)
    .addFunction("run_method_in_gulp_object_if_exists", run_method_in_gulp_object_if_exists)
    .addFunction("LuaMain_push_number", LuaMain_push_number)
    .addFunction("LuaMain_push_queue", LuaMain_push_queue)
    
    .beginClass <LuaThread>("LuaThread")
    .addConstructor <void (*) (LuaMain*, const char*, const char*)> ()
    .addFunction("GetThreadID", &LuaThread::GetThreadID)
    .addFunction("GetThreadName", &LuaThread::GetThreadName)
    .addFunction("WaitThread", &LuaThread::WaitThread)
    .addFunction("IsRunning", &LuaThread::IsRunning)
    //.addFunction("DetachThread", &LuaThread::DetachThread)
    .endClass()
    .addFunction("GetCurrentThreadID", GetCurrentThreadID)
    .addFunction("SetCurrentThreadPriority", SetCurrentThreadPriority)
    
    .addFunction("initialise_random_seed", Utilities::initialise_random_seed)
    .addFunction("debugMessage", debugMessageC)
    .addFunction("CreateGameWindow", CreateGameWindow)
    .addFunction("SDL_CreateRenderer", SDL_CreateRenderer)
    .addFunction("GetGameWindow", GetGameWindow)
    
    //very limited GameApplication registration ... re-opened later
    .beginClass <GameApplication> ("GameApplication")
    .addFunction("SetRenderer", &GameApplication::SetRenderer)
    .addFunction("get_SDL_renderer", &GameApplication::get_SDL_renderer)
    .addFunction("return_copyright", &GameApplication::return_copyright)
    .endClass()
    
    // to allow use of these in boot.lua   @todo: find a way of avoiding this in server or others
    .beginClass <SDL_Window>("SDL_Window")
    .endClass()
    .beginClass<SDL_Renderer>("SDL_Renderer")
    .endClass()

    .addCFunction("SDL_GetWindowSize", SDL_GetWindowSize_helper)
    .addCFunction("SDL_GL_GetDrawableSize", SDL_GL_GetDrawableSize_helper)
    .addCFunction("SDL_GetRendererOutputSize", SDL_GetRendererOutputSize_helper)
    .addFunction("UpdateScreenData_game_window", UpdateScreenData_game_window)

    .addFunction("get_time_since_last_call", Utilities::get_time_since_last_call)
    .addFunction("print_time_since_last_call", Utilities::print_time_since_last_call)
    .addFunction("enable_print_time_since_last_call", Utilities::enable_print_time_since_last_call)
    .addCFunction("switch_to_program", switch_to_program)
    .endNamespace()

    ;
}

static double get_SDL_PIXELFORMAT_ABGR8888()
{
    return SDL_PIXELFORMAT_ABGR8888;
}

int SDL_PixelFormatEnumToMasks_helper(lua_State*L)
{
    Uint32 format = luaL_checkunsigned(L, 1);
    int    bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
    SDL_bool err = SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
    
    if(err == SDL_FALSE)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, SDL_GetError());
        return 2;
    }

    lua_pushinteger(L, bpp);
    lua_pushunsigned(L, Rmask);
    lua_pushunsigned(L, Gmask);
    lua_pushunsigned(L, Bmask);
    lua_pushunsigned(L, Amask);
    return 5;
}
SDL_Surface *SDL_CreateRGBSurface_helper(
double flags, int width, int height, int depth,
double Rmask, double Gmask, double Bmask, double Amask)
{
    return SDL_CreateRGBSurface
       ((Uint32) flags, width, height, depth,
       (Uint32) Rmask, (Uint32) Gmask, (Uint32) Bmask, (Uint32) Amask);
}

int get_SDL_Surface_width(SDL_Surface* s)
{
    return s->w;
}
int get_SDL_Surface_height(SDL_Surface* s)
{
    return s->h;
}
//SDL_Rect new_SDL_Rect(int x, int y, int h, int w)
//{
//    SDL_Rect r = {x, y, h, w};
//    return r;
//}

// ------------------------------------------------------------------------------
//
// We bind all the Forlorn Fox Main classes in here, rather than spread them out over the project.
//
// ------------------------------------------------------------------------------
static void set_up_main_ff_cpp_bindings(lua_State *L, std::string base_table_name)
{
   using namespace luabridge;

   //
   // Define some classes
   //
   getGlobalNamespace (L)
   .beginNamespace (base_table_name.c_str())

   //
   // Class definitions
   //
   //
        .beginClass<LuaCommandLineInterpreter>("LuaCommandLineInterpreter")
            .addFunction ("process_line", &LuaCommandLineInterpreter::process_line)
            .addFunction ("print_version", &LuaCommandLineInterpreter::print_version)
            .addFunction ("print_prompt", &LuaCommandLineInterpreter::print_prompt)
        .endClass ()
    
        // extend this one
        .beginClass <LuaMain> ("LuaMain")
         .addFunction("open_console", &LuaMain::open_console)
            .addFunction("get_CLI", &LuaMain::get_CLI)
      .endClass ()


        // re-open GameApplication to extend it
      .beginClass <GameApplication> ("GameApplication")
         .addFunction("quit_main_loop", &GameApplication::quit_main_loop)
         .addFunction("verbose_engine", &GameApplication::verbose_engine)
         .addFunction("setup_ff_lua_state", &GameApplication::setup_ff_lua_state)
      .endClass()


   .endNamespace();
}




// ------------------------------------------------------------------------------
//
// We bind all the Forlorn Fox UI classes in here, rather than spread them out over the project.
//
// ------------------------------------------------------------------------------
static void set_up_ui_ff_cpp_bindings(lua_State *L, std::string base_table_name)
{
	//lua_pushcfunction(L, _openConsole);
	//lua_setfield(L, -2, "_openConsole");

	using namespace luabridge;

	//getGlobalNamespace (L)
	//.beginNamespace (base_table_name.c_str())

	// basic constants
	// (Evil casts, but still)
    //.addVariable ("_version_str", const_cast<const char**>(&forlorn_fox_version), false)	// read-only

	// simple functions
	//.addFunction("open_console", open_console)
	//
	// Specific

	// broken
	//.addFunction ("open_console", &lua_main::open_console)

	//.endNamespace ();


	//
	// Define some classes
	//
	getGlobalNamespace (L)
	.beginNamespace (base_table_name.c_str())
	//
	// simple functions
	//
		//.addFunction("map_transform", MapUtils::map_transform)
		//.addFunction("SetLevelSize", SetLevelSize)
		.addFunction("GetWindowWidth", GetWindowWidth)
		.addFunction("GetWindowHeight", GetWindowHeight)
		.addFunction("GetWindowDiagonalInches", GetWindowDiagonalInches)
        .addFunction("ToggleFullscreen", ToggleFullscreen)
        .addFunction("IsFullscreen", isFullScreen)
        .addFunction("SDL_SetTextureAlphaMod", SDL_SetTextureAlphaMod)
		.addFunction("SDL_FreeSurface", SDL_FreeSurface)
        .addFunction("SDL_DestroyTexture", SDL_DestroyTexture)
        .addFunction("SDL_RenderGetClipRect", SDL_RenderGetClipRect)
        .addFunction("SDL_RenderSetClipRect", SDL_RenderSetClipRect)
        //.addFunction("SDL_RenderIsClipEnabled", SDL_RenderIsClipEnabled)  // v2.0.4
        .addFunction("SDL_GetError", SDL_GetError)
        .addFunction("load_image", load_image)
        .addFunction("get_SDL_PIXELFORMAT_ABGR8888", get_SDL_PIXELFORMAT_ABGR8888)
        .addCFunction("SDL_PixelFormatEnumToMasks", SDL_PixelFormatEnumToMasks_helper)
        .addFunction("SDL_CreateRGBSurface", SDL_CreateRGBSurface_helper)
        .addFunction("SDL_BlitSurface", SDL_BlitSurface)
        .addFunction("SDL_CreateTextureFromSurface", SDL_CreateTextureFromSurface)
        .addFunction("SDL_SetTextureBlendMode", SDL_SetTextureBlendMode_helper)
        .addFunction("get_SDL_Surface_width", get_SDL_Surface_width)
        .addFunction("get_SDL_Surface_height", get_SDL_Surface_height)

    // text editting SDL functions
        .addFunction("SDL_StartTextInput", SDL_StartTextInput)
        .addFunction("SDL_StopTextInput", SDL_StopTextInput)
        //.addFunction("SetTextInputRect", SetTextInputRect)
        .addFunction("SDL_SetTextInputRect", SDL_SetTextInputRect)
        .addFunction("SDL_IsTextInputActive", SDL_IsTextInputActive_helper)

	// glyph editor support functions
		.addFunction("save_glyph", save_glyph)
		.addFunction("load_glyph", load_glyph)
		.addFunction("get_colour_format", get_colour_format)
		.addFunction("load_glyph_file", load_glyph_file)
		.addFunction("save_glyph_file", save_glyph_file)

		.addFunction("get_rgb_from_simple_colour", _get_rgb_from_simple_colour)

    

    
	//
	// Class definitions
	//
	//
        .beginClass<SDL_Rect>("SDL_Rect")
            // Could allow access to data, but no point
            // because this *generally* represents an untranslated
            // screen address. Do for debug anyway.
            .addData("x", &SDL_Rect::x)
            .addData("y", &SDL_Rect::y)
            .addData("w", &SDL_Rect::w)
            .addData("h", &SDL_Rect::h)
            //.addStaticFunction("new", new_SDL_Rect)
            .addConstructor<void (*) (void)>()
        .endClass()
    

      .beginClass<Mix_Music>("Mix_Music")
      .endClass()
      .beginClass<Mix_Chunk>("Mix_Chunk")
         .addData("allocated",&Mix_Chunk::allocated)
         .addData("abuf",&Mix_Chunk::abuf)
         .addData("alen",&Mix_Chunk::alen)
         .addData("volume",&Mix_Chunk::volume)
      .endClass()

   
		.beginClass <MySoundManager> ("MySoundManager")
         .addFunction("get_sample_frequency", &MySoundManager::get_sample_frequency)
         .addFunction("load_sound", &MySoundManager::load_sound)
         .addFunction("add_mono_sound", &MySoundManager::add_mono_sound)
         //.addFunction("add_stereo_sound", &MySoundManager::add_stereo_sound)
         .addFunction("play_sound", &MySoundManager::play_sound)
			.addFunction("play_music", &MySoundManager::play_music)
			.addFunction("set_master_volume", &MySoundManager::set_master_volume)

         // expose everything
         .addData("audio_open",&MySoundManager::audio_open)
         .addData("audio_open",&MySoundManager::audio_open)

         .addData("audio_rate",&MySoundManager::audio_rate)
         .addData("audio_format",&MySoundManager::audio_format)
         .addData("audio_channels",&MySoundManager::audio_channels)
         .addData("audio_buffers",&MySoundManager::audio_buffers)
         .addData("music_volume",&MySoundManager::music_volume)
         .addData("effects_volume",&MySoundManager::effects_volume)

         .addData("mNumberOfChannels",&MySoundManager::mNumberOfChannels)
         .addData("mSounds",&MySoundManager::mSounds)

         .addFunction("convert_from_Sint8", &MySoundManager::convert_from_Sint8)
         .addFunction("convert_from_Sint8_mono", &MySoundManager::convert_from_Sint8_mono)
         .addFunction("create_fixed_Sint_buffer", &MySoundManager::create_fixed_Sint_buffer)
		.endClass()

		.beginClass <MyGraphics> ("MyGraphics")
			.addFunction("go_to", &MyGraphics::go_to)
			.addFunction("set_fg_colour", &MyGraphics::set_fg_colour)
			.addFunction("set_bg_colour", &MyGraphics::set_bg_colour)
			.addFunction("set_fg_fullcolour", &MyGraphics::set_fg_fullcolour)
			.addFunction("set_bg_fullcolour", &MyGraphics::set_bg_fullcolour)
			.addFunction("set_bg_transparent", &MyGraphics::set_bg_transparent)
			.addFunction("set_bg_opaque", &MyGraphics::set_bg_opaque)
			.addFunction("skip_1_forward", &MyGraphics::skip_1_forward)
            .addFunction("wrap", &MyGraphics::wrap)
            .addFunction("get_wrap", &MyGraphics::get_wrap)
            .addFunction("set_wrap_limits", &MyGraphics::set_wrap_limits)
			.addFunction("get_column", &MyGraphics::get_column)
			.addFunction("get_line", &MyGraphics::get_line)
            //.addFunction("FillRectColour", &MyGraphics::FillRectColour)
            .addFunction("SetTextureAlphaMod", &MyGraphics::SetTextureAlphaMod)
            //.addFunction("printEx", &MyGraphics::printEx)     // only 8 parameters are supported...
            .addFunction("printExT", &MyGraphics::printExT)
			.addFunction("set_viewport", &MyGraphics::set_viewport)

			.addFunction("DrawRect", &MyGraphics::DrawRect)
			.addFunction("DrawAbsoluteRect", &MyGraphics::DrawAbsoluteRect)
			.addFunction("DrawPoint", &MyGraphics::DrawPoint)
			.addFunction("DrawLine", &MyGraphics::DrawLine)
            .addFunction("FillRect", &MyGraphics::FillRect)
            .addFunction("FillRectSimple", &MyGraphics::FillRectSimple)
            //.addFunction("RenderSetClipRect", &MyGraphics::RenderSetClipRect)
            //.addFunction("RenderGetClipRect", &MyGraphics::RenderGetClipRect)
            .addFunction("RenderCopy", &MyGraphics::RenderCopy)
		.endClass()

		.addFunction("print_string", print_cstring)
		.addFunction("print_glyph", print_glyph)
		.addFunction("print_glyph_ex", print_glyph_ex)

		.deriveClass <MyGraphics_render, MyGraphics> ("MyGraphics_render")
			.addFunction("load_textures_from_glyph_set", &MyGraphics_render::load_textures_from_glyph_set)
			.addFunction("create_texture_set", &MyGraphics_render::create_texture_set)
			.addFunction("update_texture_set_glyph", &MyGraphics_render::update_texture_set_glyph)
			.addFunction("update_texture_set_pixel", &MyGraphics_render::update_texture_set_pixel)
		.endClass()


        // re-open GameApplication to extend it
		.beginClass <GameApplication> ("GameApplication")
			.addFunction("get_absolute_draw_list", &GameApplication::get_absolute_draw_list)
			.addFunction("get_status_draw_list", &GameApplication::get_status_draw_list)
			.addFunction("get_multiplayer_status_draw_list", &GameApplication::get_multiplayer_status_draw_list)
            .addFunction("get_SDL_renderer", &GameApplication::get_SDL_renderer)
            .addFunction("get_standard_graphics_context", &GameApplication::get_standard_graphics_context)
            .addFunction("set_background_colour", &GameApplication::set_background_colour)
            .addFunction("remove_mouse_target", &GameApplication::remove_mouse_target)
            .addFunction("add_mouse_target", &GameApplication::add_mouse_target)
            .addFunction("run_mouse_target", &GameApplication::run_mouse_target)
            .addFunction("fps_test", &GameApplication::fps_test)
		    //.addFunction("render", &GameApplication::render)
		.endClass()

		.beginClass <SDL_Color> ("SDL_Color")
	      	.addConstructor <void (*) (void)> ()
			.addData("r", &SDL_Color::r)
			.addData("g", &SDL_Color::g)
			.addData("b", &SDL_Color::b)
			.addData("a", &SDL_Color::a)
		.endClass()



		.beginClass <Viewport> ("Viewport")
			.addConstructor <void (*) (void)> ()
			.addFunction("SetRect", &Viewport::SetRect)
			.addFunction("SetCellSize", &Viewport::SetCellSize)
            .addFunction("SetOrigin", &Viewport::SetOrigin)
            .addFunction("SetPixelCoords", &Viewport::SetPixelCoords)
            .addFunction("SetCellCoords", &Viewport::SetCellCoords)
		.endClass()


		.beginClass <PresentationMaze> ("PresentationMaze")
			.addConstructor <void (*) (double, double)> ()
			.addFunction("set_maze_colours", &PresentationMaze::set_maze_colours)
			.addFunction("set_default_maze_colours", &PresentationMaze::set_default_maze_colours)
			.addFunction("load_current_maze", &PresentationMaze::load_current_maze)
			.addFunction("set_offset", &PresentationMaze::set_offset)
			.addFunction("width", &PresentationMaze::width)
			.addFunction("height", &PresentationMaze::height)
			.addFunction("get_maze_draw_list", &PresentationMaze::get_maze_draw_list)
			.addFunction("get_mobs_draw_list", &PresentationMaze::get_mobs_draw_list)
			.addFunction("set_view_layer", &PresentationMaze::set_view_layer)
			.addFunction("get_glyph", &PresentationMaze::get_glyph)
			.addFunction("set_glyph", &PresentationMaze::set_glyph)
			.addFunction("set_rotation", &PresentationMaze::set_rotation)
			.addFunction("line_of_sight", &PresentationMaze::line_of_sight)
            .addFunction("print", &PresentationMaze::print)
            .addFunction("print_selected", &PresentationMaze::print_selected)
			.addFunction("set_click_callback", &PresentationMaze::set_click_callback)
			.addFunction("GetAvailableLevelWidth", &PresentationMaze::GetAvailableLevelWidth)
			.addFunction("GetAvailableLevelHeight", &PresentationMaze::GetAvailableLevelHeight)
			.addFunction("GetCellSize", &PresentationMaze::GetCellSize)
			.addFunction("set_render_option", &PresentationMaze::set_render_option)
		.endClass()

		.beginClass <DrawList> ("DrawList")
			.addFunction("render_simple", &DrawList::render_simple)
		    .addFunction("set_rect", &DrawList::set_rect)
		    .addFunction("set_size", &DrawList::set_size)
		    .addFunction("set_draw_location_mode", &DrawList::set_draw_location_mode_from_lua)
			.addData("PIXEL_BASED", &DrawList::PIXEL_BASED, false)
			.addData("CELL_BASED", &DrawList::CELL_BASED, false)
			.addFunction("get_line_in_pixels", &DrawList::get_line_in_pixels)
			.addFunction("get_column_in_pixels", &DrawList::get_column_in_pixels)
		.endClass()

		.beginClass <DrawListElement> ("DrawListElement")
			//.addConstructor <void (*) (DrawList*, int, pos_t, pos_t, int, simple_colour_t, SDL_Color, bool)> ()
			// luabridge supports only one constructor....
			.addConstructor <void (*) (DrawList*, int, pos_t, pos_t, int, simple_colour_t)> ()
      		.addFunction("show", &DrawListElement::show)
      		.addFunction("hide", &DrawListElement::hide)
      		.addFunction("is_visible", &DrawListElement::is_visible)
      		.addFunction("set_glyph", &DrawListElement::set_glyph)
      		.addFunction("set_compound_glyph", &DrawListElement::set_compound_glyph)
      		.addFunction("new_compound_glyph", &DrawListElement::new_compound_glyph)
      		.addFunction("set_compound_glyph_layer", &DrawListElement::set_compound_glyph_layer)
      		.addFunction("set_compound_glyph_offsets", &DrawListElement::set_compound_glyph_offsets)
      		.addFunction("show_compound_glyph", &DrawListElement::show_compound_glyph)
      		.addFunction("hide_compound_glyph", &DrawListElement::hide_compound_glyph)
      		.addFunction("get_glyph", &DrawListElement::get_glyph)
			.addFunction("set_line", &DrawListElement::set_line)
			.addFunction("set_column", &DrawListElement::set_column)
			.addFunction("get_line", &DrawListElement::get_line)
			.addFunction("get_column", &DrawListElement::get_column)
			.addFunction("get_line_in_pixels", &DrawListElement::get_line_in_pixels)
			.addFunction("get_column_in_pixels", &DrawListElement::get_column_in_pixels)
			.addFunction("set_layer", &DrawListElement::set_layer)
			.addFunction("get_layer", &DrawListElement::get_layer)
			.addFunction("set_fg_colour", &DrawListElement::set_fg_colour)
			.addFunction("set_fg_fullcolour", &DrawListElement::set_fg_fullcolour)
			.addFunction("set_bg_colour", &DrawListElement::set_bg_colour)
			.addFunction("set_bg_transparent", &DrawListElement::set_bg_transparent)
			.addFunction("set_bg_opaque", &DrawListElement::set_bg_opaque)
			.addFunction("set_bg_transparency", &DrawListElement::set_bg_transparency)
			.addFunction("set_dim", &DrawListElement::set_dim)
			.addFunction("set_bright", &DrawListElement::set_bright)
			.addFunction("set_width", &DrawListElement::set_width)
			.addFunction("set_height", &DrawListElement::set_height)
			.addFunction("set_size_ratio", &DrawListElement::set_size_ratio)
			.addFunction("get_width", &DrawListElement::get_width)
			.addFunction("get_height", &DrawListElement::get_height)
			.addFunction("get_size_ratio", &DrawListElement::get_size_ratio)
			.addFunction("set_click_callback", &DrawListElement::set_click_callback)
			.addFunction("set_draw_list", &DrawListElement::set_draw_list)
			.addFunction("get_draw_list", &DrawListElement::get_draw_list)
			.addFunction("set_angle", &DrawListElement::set_angle)
			.addFunction("get_angle", &DrawListElement::get_angle)
		.endClass()

		.beginClass <MazeDrawList> ("MazeDrawList")
			.addFunction("check_integrity", &MazeDrawList::check_integrity)
		.endClass()

		.beginClass <MazeDrawListElement> ("MazeDrawListElement")
			.addConstructor <void (*) (MazeDrawList*, int, int, double)> ()
      		.addFunction("update_glyph", &MazeDrawListElement::update_glyph)
			.addFunction("hide", &MazeDrawListElement::hide)
			.addFunction("show", &MazeDrawListElement::show)
		.endClass()

        .beginClass <MazeDrawListAnimatedElement> ("MazeDrawListAnimatedElement")
            .addConstructor <void (*) (MazeDrawList*, int)> ()
            .addFunction("update_glyph_list", &MazeDrawListAnimatedElement::update_glyph_list)
            .addFunction("hide", &MazeDrawListAnimatedElement::hide)
            .addFunction("setInterval", &MazeDrawListAnimatedElement::setInterval)
        .endClass()
    
		.beginClass <Debug>("Debug")
			.addFunction("set_lua_info_string", &Debug::set_lua_info_string)
            .addFunction("info_toggle", &Debug::info_toggle)
		.endClass()
    
        /*
		.beginClass <MapUtils::MapFinder>("MapFinder")
			.addConstructor <void (*) (PresentationMaze*)> ()
			.addFunction("add_target", &MapUtils::MapFinder::add_target)
			.addFunction("find_anywhere", &MapUtils::MapFinder::find_anywhere)
			.addData("line", &MapUtils::MapFinder::line, &MapUtils::MapFinder::line)
			.addData("column", &MapUtils::MapFinder::column, &MapUtils::MapFinder::column)
			//.addProperty("line", &MapUtils::MapFinder::line, , &MapUtils::MapFinder::line)
			//.addProperty("column", &MapUtils::MapFinder::column, &MapUtils::MapFinder::column)
		.endClass()
         */
    
        .beginClass<SDL_Renderer>("SDL_Renderer")
        .endClass()
        
        .beginClass<SDL_Texture>("SDL_Texture")
        .endClass()

        .beginClass<SDL_Surface>("SDL_Surface")
        .endClass()
    
        .beginClass <MouseTargetBaseType>("MouseTargetBaseType")
            .addFunction("care_about_drag_button_down", &MouseTargetBaseType::care_about_drag_button_down)
            .addFunction("care_about_drag_button_up", &MouseTargetBaseType::care_about_drag_button_up)
            .addCFunction("get_shape", &MouseTargetBaseType::get_shape)
        .endClass()
        .deriveClass <MouseTargetRectangle, MouseTargetBaseType>("MouseTargetRectangle")
            .addConstructor<void (*) (GameApplication*,
                double, double, double, double,
                luabridge::LuaRef,
                luabridge::LuaRef,
                luabridge::LuaRef)> ()
            .addFunction("define", &MouseTargetRectangle::define)
        .endClass()
        .deriveClass <MouseTargetTriangle, MouseTargetBaseType>("MouseTargetTriangle")
            .addConstructor<void (*) (GameApplication*,
                luabridge::LuaRef,
                luabridge::LuaRef,
                luabridge::LuaRef)> ()
            .addFunction("define", &MouseTargetTriangle::define)
        .endClass()
        .deriveClass <MouseTargetCircle, MouseTargetBaseType>("MouseTargetCircle")
            .addConstructor<void (*) (GameApplication*,
                double, double, double,
                luabridge::LuaRef,
                luabridge::LuaRef,
                luabridge::LuaRef)> ()
            .addFunction("define", &MouseTargetCircle::define)
        .endClass()

         .beginClass <RendererInfo>("RendererInfo")
             //.addConstructor<void (*) (SDL_Renderer*)> ()
             //.addConstructor<void (*) (int)> ()
             .addConstructor<void (*) (SDL_Renderer*, int)> ()
             .addStaticProperty("number_renderers", &RendererInfo::number_renderers)
             .addCFunction("error", &RendererInfo::error)
    
            .addProperty("name", &RendererInfo::name)
            .addProperty("flags", &RendererInfo::flags)
            .addProperty("num_texture_formats", &RendererInfo::num_texture_formats)
            .addFunction("get_texture_format", &RendererInfo::get_texture_format)
            .addProperty("max_texture_width", &RendererInfo::max_texture_width)
            .addProperty("max_texture_height", &RendererInfo::max_texture_height)
            
            .addProperty("software_flags_mask", &RendererInfo::software_flags_mask)
            .addProperty("accleratated_flags_mask", &RendererInfo::accleratated_flags_mask)
            .addProperty("vsync_flags_mask", &RendererInfo::vsync_flags_mask)
            .addProperty("target_texture_flags_mask", &RendererInfo::target_texture_flags_mask)
            
            .addProperty("software_renderer", &RendererInfo::software_renderer)
            .addProperty("accelerated_renderer", &RendererInfo::accelerated_renderer)
            .addProperty("vsync_renderer", &RendererInfo::vsync_renderer)
            .addProperty("target_texture_renderer", &RendererInfo::target_texture_renderer)
            
            .addFunction("get_pixel_format_name", &RendererInfo::get_pixel_format_name)
        .endClass()

	.endNamespace();
}


/*
 //
 // MySoundManager Proxy functions
 //
 static int play_sound_proxy(lua_State *L)
 {
 // get game_app
 // get sound manager
 // get parameter
 sm->play_sound(sound);
 return 0;
 }
 */

// helper function to set a field of a table with a value
/*
#define write_field(L, name) _write_field(L, #name, name)
static void _write_field(lua_State *L, const char* name, int value)
{
	lua_pushinteger(L, value);
	lua_setfield(L, -2, name);
}
*/
//static void _write_field(lua_State *L, const char* name, double value)
//{
//	lua_pushnumber(L, value);
//	lua_setfield(L, -2, name);
//}
//static void _write_field(lua_State *L, const char* name, sound_t value)
//{
//	lua_pushinteger(L, value);
//	lua_setfield(L, -2, name);
//}
/*
void decode_and_drop_glyph_table(lua_State *L, unsigned int* glyph, int glyph_size)
{
	// table starts on lua stack

	int valid;

	for(int counter = 1; counter <= glyph_size*glyph_size; counter++)
	{
		lua_rawgeti(L, -1, counter);
		*glyph++ = lua_tounsignedx(L, -1, &valid);
		lua_remove(L, -1);
	}
}
*/
/*
void create_and_return_glyph_table(lua_State *L, unsigned int* glyph, int glyph_size)
{
	lua_newtable(L);

	for(int counter = 1; counter <= glyph_size*glyph_size; counter++)
	{
		lua_pushnumber(L, *glyph++);
		lua_rawseti(L, -2, counter);
	}

	// glyph table is left on the stack


}
*/
static void create_and_return_sound_table(lua_State *L, MySoundManager& manager)
{
	lua_newtable(L);

#if 0	// we don't do it this way...
	// store reference to sound manager in sound
	// http://lua-users.org/lists/lua-l/2005-10/msg00091.html
	MySoundManager* sound_manager_ptr = app.
	lua_pushlightuserdata(L, sound_manager_ptr);
	lua_setfield(L, -2, "__self");
#endif

	luabridge::push(L, &manager);
	lua_setfield(L, -2, "manager");

#if 0
	_add_method(MySoundManager&, play_sound);
#endif

	// sound table is on Lua stack
}

static void create_colour_table(lua_State *L, SDL_Colour& c, bool simple_colour, simple_colour_t colour)
{
	lua_newtable(L);
	lua_pushinteger(L, c.r);
	lua_setfield(L, -2, "r");
	lua_pushinteger(L, c.g);
	lua_setfield(L, -2, "g");
	lua_pushinteger(L, c.b);
	lua_setfield(L, -2, "b");
	lua_pushinteger(L, c.a);
	lua_setfield(L, -2, "a");

	// simple_colour is nil if we don't have one
	if(simple_colour)
	{
		luabridge::push(L, colour);
		lua_setfield(L, -2, "simple_colour");
		luabridge::push(L, colour);
		lua_setfield(L, -2, "simple_color");
	}
}

static void create_and_return_colour_table(lua_State *L)
{
	lua_newtable(L);

	for(int i = 0; i < number_of_colours; i ++)
	{
		simple_colour_t sc = (simple_colour_t)i;
		SDL_Color c = get_rgb_from_simple_colour(sc);
		create_colour_table(L, c, true, sc);
		lua_setfield(L, -2, get_colour_name(sc));
	}

	create_colour_table(L, hot_pink_means_transparent, false, BLACK);
	lua_setfield(L, -2, "hot_pink_means_transparent");
}

#define write_number(L, name) lua_pushnumber(L, name); lua_setfield(L, -2, #name);
#define write_integer(L, name) lua_pushinteger(L, name); lua_setfield(L, -2, #name);

static void create_SDL_Keymod_table(lua_State *L)
{
    lua_newtable(L);

    write_integer(L, KMOD_NONE);
    write_integer(L, KMOD_LSHIFT);
    write_integer(L, KMOD_RSHIFT);
    write_integer(L, KMOD_LCTRL);
    write_integer(L, KMOD_RCTRL);
    write_integer(L, KMOD_LALT);
    write_integer(L, KMOD_RALT);
    write_integer(L, KMOD_LGUI);
    write_integer(L, KMOD_RGUI);
    write_integer(L, KMOD_NUM);
    write_integer(L, KMOD_CAPS);
    write_integer(L, KMOD_MODE);
    write_integer(L, KMOD_RESERVED);
    //SDL_Keymod;
}

static void create_SDL_SubSystems(lua_State *L)
{
    lua_newtable(L);
    
    write_integer(L, SDL_INIT_TIMER);
    write_integer(L, SDL_INIT_AUDIO);
    write_integer(L, SDL_INIT_VIDEO);
    write_integer(L, SDL_INIT_JOYSTICK);
    write_integer(L, SDL_INIT_HAPTIC);
    write_integer(L, SDL_INIT_GAMECONTROLLER);
    write_integer(L, SDL_INIT_EVENTS);
    write_integer(L, SDL_INIT_NOPARACHUTE);
    write_integer(L, SDL_INIT_EVERYTHING);
}

static void create_SDL_RendererFlags(lua_State *L)
{
    lua_newtable(L);
    
    write_integer(L, SDL_RENDERER_SOFTWARE);
    write_integer(L, SDL_RENDERER_ACCELERATED);
    write_integer(L, SDL_RENDERER_PRESENTVSYNC);
    write_integer(L, SDL_RENDERER_TARGETTEXTURE);
}


// ------------------------------------------------------------------------------
//
// THESE LIBRARIES ARE SET UP FOR ALL INSTANCES
//
// Do not include any UI specific stuff (e.g. screen size, sound, colour, debug display, et.c)
// ------------------------------------------------------------------------------
void set_up_basic_ff_libraries(LuaMain* l)
{
    if(not l) { Utilities::fatalError("LuaMain null in set_up_basic_ff_libraries()"); }
    LuaMain& L = *l;

    set_up_basic_ff_cpp_bindings(L, master_table_name+"_cpp");
    
    // get the table that's just been created and add stuff manually..
    L.return_namespace_table(master_table_name);
    
    
#ifdef __GNUC__
    lua_pushnumber(L, __GNUC__);
    lua_setfield(L, -2, "__GNUC__");
    lua_pushnumber(L, __GNUC_MINOR__);
    lua_setfield(L, -2, "__GNUC_MINOR__");
    lua_pushnumber(L, __GNUC_PATCHLEVEL__);
    lua_setfield(L, -2, "__GNUC_PATCHLEVEL__");
    lua_pushnumber(L, __GNUC__+(__GNUC_MINOR__/10.0)+(__GNUC_PATCHLEVEL__/100.0));
    lua_setfield(L, -2, "GCC_VERSION");
#endif
    
    
    lua_pushnumber(L, forlorn_fox_engine_version);
    lua_setfield(L, -2, "forlorn_fox_engine_version");

    // This is set via command line nowadays
    //#ifdef DEBUG
    //	lua_pushboolean(L, 1);
    //	lua_setfield(L, -2, "debugging");
    //#endif
    
    // platform defines
#if defined(OpenPandora)
    lua_pushstring(L, "OpenPandora");
#elif defined(MacOSX)
    lua_pushstring(L, "MacOSX");
#elif defined(iPhone)
    lua_pushstring(L, "iOS");
#elif defined(Windows)
    lua_pushstring(L, "Windows");
#elif defined(__LINUX__)
    lua_pushstring(L, "Linux");
#else
    #ifdef __ANDROID__
    	lua_pushstring(L, "Android");
	#else
    	lua_pushstring(L, "unknown");
	#endif
#endif
    lua_setfield(L, -2, "platform");
    
#if defined(Desktop)
    lua_pushstring(L, "Desktop");
#elif defined(Mobile)
    lua_pushstring(L, "Mobile");
#else
    lua_pushstring(L, "unknown");
#endif
    lua_setfield(L, -2, "platform_type");
    
// Check windows
#if _WIN32 || _WIN64
#if _WIN64 && !defined(_WIN32)
#define COMPILE_BITS 64
#elif defined(_WIN32)
#define COMPILE_BITS 32
#else
#error "Windows unknown size"
#endif

#elif __GNUC__
    
    // Check GCC
#if __x86_64__ || __ppc64__
#define COMPILE_BITS 64
#else
#define COMPILE_BITS 32
#endif
#else
#error "Unknown platform size"
#endif

    lua_pushnumber(L, COMPILE_BITS);
    lua_setfield(L, -2, "buildsize");

	// add the 'colour' table to 'gulp'
	create_and_return_colour_table(L);
	lua_pushvalue(L, -1);	// copy colour
	lua_setfield(L, -3, "colour");		// UK spelling
	lua_setfield(L, -2, "color");		// USA spelling
    

    // LuaMain interface function return when function not called
    lua_pushnumber(L, LUA_FUNCTION_NOT_CALLED);
    lua_setfield(L, -2, "LUA_FUNCTION_NOT_CALLED");
    
    create_SDL_Keymod_table(L);
    lua_setfield(L, -2, "SDL_Keymod");
    create_SDL_SubSystems(L);
    lua_setfield(L, -2, "SDL_SubSystems");
    create_SDL_RendererFlags(L);
    lua_setfield(L, -2, "SDL_RendererFlags");
    
    lua_newtable(L);
    lua_pushnumber(L, SDL_BLENDMODE_NONE);
    lua_setfield(L, -2, "SDL_BLENDMODE_NONE");
    lua_pushnumber(L, SDL_BLENDMODE_BLEND);
    lua_setfield(L, -2, "SDL_BLENDMODE_BLEND");
    lua_pushnumber(L, SDL_BLENDMODE_ADD);
    lua_setfield(L, -2, "SDL_BLENDMODE_ADD");
    lua_pushnumber(L, SDL_BLENDMODE_MOD);
    lua_setfield(L, -2, "SDL_BLENDMODE_MOD");
    lua_setfield(L, -2, "SDL_BlendMode");

    lua_pop(L, 1);	// drop the table
}

// ------------------------------------------------------------------------------
//
// THESE LIBRARIES ARE SET UP ONLY FOR THE MAIN INSTANCE
//
// ------------------------------------------------------------------------------
void set_up_main_thread_libraries(LuaMain* l)
{
    if(not l) { Utilities::fatalError("LuaMain null in set_up_main_thread_libraries()"); }
    LuaMain& L = *l;
   
   set_up_main_ff_cpp_bindings(L, master_table_name+"_cpp");
   
   // get the table that's just been created and add stuff manually..
   L.return_namespace_table(master_table_name);
   
   //lua_pushcfunction(L, open_console);
   //lua_setfield(L, -2, "open_console");
   luabridge::push(L, &L);
   lua_setfield(L, -2, "lua_main");
   
   lua_pop(L, 1);   // drop the table
}

// ------------------------------------------------------------------------------
//
// THESE LIBRARIES ARE SET UP ONLY FOR THE UI INSTANCE
//
// ------------------------------------------------------------------------------
void set_up_ui_ff_libraries(LuaMain* l, GameApplication& app)
{
    if(not l) { Utilities::fatalError("LuaMain null in set_up_ui_ff_libraries()"); }
    LuaMain& L = *l;

	set_up_ui_ff_cpp_bindings(L, master_table_name+"_cpp");


	// get the table that's just been created and add stuff manually..
	L.return_namespace_table(master_table_name);



	// we put all the varibles into a gulp 'namespace' (table)
	// This is like Love 2D.
    
	// add the 'sound' table to 'gulp'
	create_and_return_sound_table(L, app.make_sound);
	lua_setfield(L, -2, "sound");

/*
	// add the 'colour' table to 'gulp'
	create_and_return_colour_table(L);
	lua_pushvalue(L, -1);	// copy colour
	lua_setfield(L, -3, "colour");		// UK spelling
	lua_setfield(L, -2, "color");		// USA spelling
*/

	//luabridge::push(L, print_cstring);
	//lua_setfield(L, -2, "print_string");

    // This is the debug display ... UI specific.
	luabridge::push(L, &debug);
	lua_setfield(L, -2, "debug");

    luabridge::push(L, &app);
    lua_setfield(L, -2, "app");

	lua_pop(L, 1);	// drop the table
}


void load_main_file(LuaMain* l, const char* file_name)
{
    if(not l) { Utilities::fatalError("LuaMain null in load_main_file()"); }
    LuaMain& L = *l;

	// make sure gulp table is set up
	//get_gulp_table_onto_stack();
	//lua_pop(L, 1);	// drop the table

	// load the main file
	int lua_error = L.run_lua_file(LoadPath(file_name).c_str());
	if(lua_error != LUA_OK)
	{
        std::string s = "Failed to load ";
        s += LoadPath(file_name).c_str();
        Utilities::fatalError(s);
	}
}

/* broken??
void load_file_vargs_ret(LuaMain* l, const char* file_name, int nargs, int nres)
{
    if(not l) { Utilities::fatalError("LuaMain null in load_file_expect1_return()"); }
    LuaMain& L = *l;
    
    // make sure gulp table is set up
    //get_gulp_table_onto_stack();
    //lua_pop(L, 1);	// drop the table
    
    // load the main file
    int lua_error = L.run_lua_file(LoadPath(file_name).c_str(), true, false, nargs, nres);
    if(lua_error != LUA_OK)
    {
        std::string s = "Failed to load ";
        s += LoadPath(file_name).c_str();
        Utilities::fatalError(s);
    }
}
*/

void load_conf_file(LuaMain* l)
{
    if(not l) { Utilities::fatalError("LuaMain null in load_conf_file()"); }
    LuaMain& L = *l;

	// load any config file
    //
    // This is via AppResourcePath because the LoadPath relies on SaveDataPath which
    // is set up by the conf.lua file.
    AppResourcePath arp("scripts/conf.lua");

	int lua_error = L.run_lua_file(arp.c_str(), true, true);
    
    // we've just made config.lua mandatory for loading
    if(lua_error != LUA_OK)
    {
        Utilities::fatalError("Failed to load conf.lua from " + arp.str());
    }
}

int run_gulp_function_if_exists(LuaMain* l, const char* function_name, int narg, int nres)
{
    if(not l) { Utilities::fatalError("LuaMain null in run_gulp_function_if_exists()"); }
    LuaMain& L = *l;

	int error = LUA_FUNCTION_NOT_CALLED;
	L.return_namespace_table(master_table_name);
	lua_getfield(L, -1, function_name);
	lua_remove(L, -2);			// drop table
	if(lua_isfunction(L, -1))
	{
		if(narg != 0)
		{
			int base = lua_gettop(L) - narg;  /* function index */
            if(base < 0)
            {
                Utilities::fatalError("gulp.%s function call given %d parameters but wanted %d", function_name, base+narg-1, narg);
            }
			lua_insert(L, base);  /* put it under args */
		}
		error = L.docall(narg, nres, false, function_name); // don't need to print errors, because they are included in lua error.
	}
	else
	{
		// remove arguments and not-a-function and table
		lua_pop(L, narg+1);
	}

    if(error != LUA_OK and error != LUA_FUNCTION_NOT_CALLED)
    {
        /*
         If we want to remove stack traces from the dialog box then:
         (1) [Horrible] remove the stack unwinding code on lua function calls from C++ (or store it elsewhere than a modified message) or
         (2) Just trim off the text starting with "stack traceback:"
         
         We'd want to print the stack trace to STDOUT anyway...
         (NOTE: We also might want to tell error() in Lua to avoid adding the text on the beinnging with "error(msg, 0)")
        */
        l->fatal_if_lua_errror(error, std::string(" in ") + function_name + std::string("()"));
        //Utilities::fatalError("gulp.%s failed when running (%d)", function_name, error);
    }
	return error;
}

//#include <iostream>

int run_method_in_gulp_object_if_exists(LuaMain* l, const char* gulp_subtable, const char* function_name, int narg, int nres)
{
    if(not l) { Utilities::fatalError("LuaMain null in run_method_in_gulp_object_if_exists()"); }
    LuaMain& L = *l;

	//int start = lua_gettop(L);
	int error = LUA_FUNCTION_NOT_CALLED;
	L.return_namespace_table(master_table_name);
	lua_getfield(L, -1, gulp_subtable);
	lua_remove(L, -2);			// drop gulp table
	if(lua_istable(L, -1))
	{
		lua_getfield(L, -1, function_name);
		if(lua_isfunction(L, -1))
		{
			// move function under arguments and table
			int base = lua_gettop(L) - (narg+1);  /* function index */
			lua_insert(L, base);  /* put it under args */

			if(narg != 0)
			{
				// move object table under arguments (make first argument)
				int base = lua_gettop(L) - narg;  /* function index */
				lua_insert(L, base);  /* put it under args */
			}
			error = L.docall(narg+1, nres, false, function_name);
		}
		else
		{
			// remove arguments and not-a-function and object-table
			lua_pop(L, narg+2);
		}
	}
	else
	{
		// remove arguments and not-a-table
		lua_pop(L, narg+1);
	}

	//if(lua_gettop(L) != start - narg + nres)
	//{
	//	std::cout << lua_gettop(L) << " " << start << " " << narg << " " << nres << std::endl;
	//}
    if(error != LUA_OK and error != LUA_FUNCTION_NOT_CALLED)
    {
        /*
         If we want to remove stack traces from the dialog box then:
         (1) [Horrible] remove the stack unwinding code on lua function calls from C++ (or store it elsewhere than a modified message) or
         (2) Just trim off the text starting with "stack traceback:"
         
         We'd want to print the stack trace to STDOUT anyway...
         (NOTE: We also might want to tell error() in Lua to avoid adding the text on the beinnging with "error(msg, 0)")
         */
        l->fatal_if_lua_errror(error, std::string(" in ") + function_name + std::string("()"));
        //Utilities::fatalError("gulp.%s failed when running (%d)", function_name, error);
    }

	return error;
}

