/*
 *  SoundManager.h
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

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <vector>
#include "SDL.h"
#include "SDL_mixer.h"

#include "lua.h"
#include "lauxlib.h"

typedef double sound_ref;

class MySoundManager {
public:

	
	double get_sample_frequency();

   // To add sound data dynamically. Returns the sound number. Sound data should be between -1 and 1.
	sound_ref load_sound(std::string filename);
	sound_ref add_mono_sound(lua_State* L);
   //sound_t add_stereo_sound(lua_State* L);

   // play a sound effect
   void play_sound(sound_ref sound, unsigned int volume_percentage);

	~MySoundManager();
	MySoundManager();

	// play music
	void play_music(std::string filename, unsigned int volume_percentage, unsigned int fade_ms, unsigned int position);

	// master volume control (level 0-7)
	void set_master_volume(unsigned int level);

private:

	int audio_open = 0;

	Mix_Music* mMusic = NULL;

    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 4096;
    int music_volume = MIX_MAX_VOLUME;
    int effects_volume = MIX_MAX_VOLUME;

    int mNumberOfChannels = 0;
	std::vector<Mix_Chunk*> mSounds;

	void convert_from_Sint8(Mix_Chunk *s, Uint8 channels);
	void convert_from_Sint8_mono(Mix_Chunk *s);
	Uint32 create_fixed_Sint_buffer(Uint32 length, Mix_Chunk* sound);
};

#endif
