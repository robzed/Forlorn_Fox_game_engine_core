/*
 *  SoundManager.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 20/07/2011.
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

#include "MySoundManager.h"
#include "Utilities.h"
#include "GameConfig.h"
#include "LoadPath.h"

#ifdef _MSC_VER
#include <ciso646>   // Visual Studio is not C++ standards complaint...
#endif
#include <string.h>

#include <iostream>

#include <algorithm>

using Utilities::fatalErrorSDL;


MySoundManager::MySoundManager()
{
    audio_rate = 22050;
    audio_format = AUDIO_S16;
    audio_channels = 2;
    audio_buffers = 1024;
    music_volume = MIX_MAX_VOLUME;

    /* Initialize the SDL library */
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        SDL_Log("Couldn't initialize SDL: %s\n",SDL_GetError());
    }
    else
    {
		/* Open the audio device */
		if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) < 0)
		{
			SDL_Log("Couldn't open audio: %s\n", SDL_GetError());
		}
		else
		{
			Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
			SDL_Log("Opened audio at %d Hz %d bit%s %s %d bytes audio buffer\n", audio_rate,
				(audio_format&0xFF),
				(SDL_AUDIO_ISFLOAT(audio_format) ? " (float)" : ""),
				(audio_channels > 2) ? "surround" : (audio_channels > 1) ? "stereo" : "mono",
				audio_buffers);
			audio_open = 1;
		}
    }

    // replicate the old mixer for now
    // in future we might want to allocate channels to particular entities on the lua side
    Mix_AllocateChannels(mNumberOfChannels);
}

MySoundManager::~MySoundManager()
{
    if(Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }

    if (mMusic) {
        Mix_FreeMusic(mMusic);
        mMusic = NULL;
    }

    for_each(mSounds.begin(), mSounds.end(), [](Mix_Chunk* chunk){Mix_FreeChunk(chunk);});

    if (audio_open) {
        Mix_CloseAudio();
        audio_open = 0;
    }

}

void MySoundManager::play_music(std::string filename, unsigned int volume_percentage, unsigned int fade_ms, unsigned int position)
{
    if(Mix_PlayingMusic())
    {
        Mix_FadeOutMusic(1000);
    }

    if(mMusic) Mix_FreeMusic(mMusic);

    mMusic = Mix_LoadMUS(filename.c_str());

    if(mMusic)
    {
        Mix_VolumeMusic((volume_percentage * MIX_MAX_VOLUME)/100);
        Mix_FadeInMusicPos(mMusic,0,fade_ms,position);
    }
}

void MySoundManager::set_master_volume(unsigned int level)
{
	// This function is rather lacking.

	// To implement a real master volume control we would have to record our master volume
	// percentage, and volume percentage of each sound, and modify all incoming and ongoing
	// sounds by that percentage.

	// But this will do for 'Escape', since it only has the background music track

	if(level > 8 ) level = 8;

	if(level > 0)
	{
		level = level - 1;
		level = 1 << level;
	}

	Mix_VolumeMusic(level);
}

//void MySoundManager::play_sound(sound_t sound, unsigned int sound_level){}

// To add sound data dynamically. Returns the sound number. Sound data should be between -1 and 1.
//sound_t MySoundManager::add_mono_sound(lua_State* L){}
//sound_t MySoundManager::add_stereo_sound(lua_State* L){}

double MySoundManager::get_sample_frequency()
{
	return audio_rate;
}

sound_ref MySoundManager::load_sound(std::string filename)
{
    Mix_Chunk* chunk = Mix_LoadWAV(filename.c_str());
    mSounds.push_back(chunk);
    return mSounds.size() - 1;
}

sound_ref MySoundManager::add_mono_sound(lua_State* L)
{
	// tests for parameter error, if so, raise error.
	luaL_checktype(L, -1, LUA_TTABLE);

	Mix_Chunk* new_sound = new Mix_Chunk;
	new_sound->volume = MIX_MAX_VOLUME;
	new_sound->allocated = true;

	Uint32 input_data_size = luaL_len(L, -1);

	Uint32 length = create_fixed_Sint_buffer(input_data_size, new_sound);
	if(length == input_data_size)
	{
		Sint8* buf = (Sint8*)new_sound->abuf;
		for(Uint32 i = 1; i <= length; i++)
		{
			lua_pushinteger(L, i);
			lua_gettable(L, -2);		// replaces key with value
			double v = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 0;
			lua_pop(L, 1);		// drop number/other value

			// clip
			if(v > 1) { v = 1; }
			if(v < -1) { v = -1; }
			*buf = v * 127;
			buf ++;
		}
		convert_from_Sint8_mono(new_sound);
	}

	mSounds.push_back(new_sound);
	return (mSounds.size() - 1);
}


void MySoundManager::play_sound(sound_ref sound, unsigned int volume_percentage)
{
	// find a free channel
	int channel = -1;
	for(int ch = 0; ch < mNumberOfChannels; ch++)
	{
		if(!Mix_Playing(ch))
		{
			channel = ch;
			break;
		}
	}

	// if there are no free channels, allocate another channel
	if(channel == -1)
	{
		mNumberOfChannels++;
	    Mix_AllocateChannels(mNumberOfChannels);
		channel = mNumberOfChannels - 1;

		//std::cout << "New sound channel allocated for total of " << mNumberOfChannels << std::endl;
	}

	//std::cout << "Playing sound " << sound << " on channel " << channel << std::endl;


	// set the volume for the channel
    Mix_Volume(channel, (volume_percentage * MIX_MAX_VOLUME)/100);

	// now play the sound
    Mix_PlayChannel(channel, mSounds[sound], 0);
}




/*
 Takes an audio stream from 8 bit signed  and converts it to the mixer's output format,
 and stores the resulting buffer and length in the sound structure
 */
void MySoundManager::convert_from_Sint8(Mix_Chunk *s, Uint8 channels)
{
    SDL_AudioCVT cvt;           /* used to convert .wav to output format when formats differ */
    int result;
/*
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 4096;
    int music_volume = MIX_MAX_VOLUME;
    int effects_volume = MIX_MAX_VOLUME;
    */

    /* build the audio converter */
    result = SDL_BuildAudioCVT(&cvt, AUDIO_S8, channels, audio_rate,
    		audio_format, audio_channels, audio_rate);

    if (result < 0) {
        fatalErrorSDL("could not build audio CVT");
    } else if (result != 0) {
        /*
		 this happens when the .wav format differs from the output format.
		 we convert the .wav buffer here
         */
        cvt.buf = (Uint8 *) SDL_malloc(channels * s->alen * cvt.len_mult);       /* allocate conversion buffer */
        cvt.len = s->alen;    /* set conversion buffer length */
        SDL_memcpy(cvt.buf, s->abuf, s->alen);      /* copy sound to conversion buffer */
        if (SDL_ConvertAudio(&cvt) == -1) {     /* convert the sound */
            fatalErrorSDL("could not convert waveform");
        }
        SDL_free(s->abuf);    /* free the original (unconverted) buffer */
        s->abuf = cvt.buf;    /* point sound buffer to converted buffer */
        s->alen = cvt.len_cvt;        /* set sound buffer's new length */
    }
	
}

void MySoundManager::convert_from_Sint8_mono(Mix_Chunk *s)
{
	const Uint8 channels_mono = 1;
	convert_from_Sint8(s, channels_mono);
}


// create_fixed_Sint_buffer
// Create a signed int 8-bit sound buffer
//
// Returns: Length of buffer
Uint32 MySoundManager::create_fixed_Sint_buffer(Uint32 length, Mix_Chunk* sound)
{
	// populate the sound structure
	sound->abuf = (Uint8 *) SDL_malloc(length);       /* allocate buffer */
	if( ! sound->abuf )
	{
		// broken buffer
		length = 0;
	}
	
	sound->alen = length;    /* set buffer length */
	return length;	
}

