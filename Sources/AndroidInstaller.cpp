/*
 * AndroidInstaller.cpp
 *
 *  Created on: 4 Nov 2018
 *      Author: Tony Park
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2018 Rob Probin and Tony Park
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

#ifdef __ANDROID__


#include "SDL.h"
#include "SDL_rwops.h"

#include "AppResourcePath.h"
#include "Utilities.h"
#include "GameConfig.h"

#include "sys/stat.h"
#include "dirent.h"

#include "android/asset_manager.h"
#include "android/asset_manager_jni.h"

// forward declarations
void CopyFile(std::string file);
void CopyDirectory(std::string directory);

static AAssetManager* asset_man;

void CheckAndroidInstallation()
{

	Utilities::debugMessage("Start file copy...");

	// AppResourcePath should point to the destination of the copy (where files will be loaded from in future)

    // make new class AndroidAssetsPath() which points to source

	std::string version_file_name = "version";

	AppResourcePath v_arp(version_file_name);

    //if(!Utilities::file_exists(v_arp.c_str()))
	{

		// get the asset manager with help from SDL
		JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
		jobject activity = (jobject)SDL_AndroidGetActivity();

		// https://stackoverflow.com/questions/22436259/android-ndk-why-is-aassetmanager-open-returning-null/22436260#22436260
		jclass activity_class = env->GetObjectClass(activity);
	    jmethodID activity_class_getAssets = env->GetMethodID(activity_class, "getAssets", "()Landroid/content/res/AssetManager;");
	    jobject asset_manager = env->CallObjectMethod(activity, activity_class_getAssets); // activity.getAssets();
	    auto global_asset_manager = env->NewGlobalRef(asset_manager);

	    asset_man = AAssetManager_fromJava(env, global_asset_manager);

		if(asset_man == NULL)
		{
			Utilities::fatalError("Failed to get asset manager");
		}

		std::string out_path = std::string(SDL_AndroidGetInternalStoragePath())+"/arp";
		mkdir(out_path.c_str(), 0755);

		// copy all the necessary directories to internal storage
    	CopyDirectory("scripts");
    	CopyDirectory("scripts/avatars");
    	CopyDirectory("scripts/controllers");
    	CopyDirectory("scripts/game_world_server");
    	CopyDirectory("scripts/glyph_editor");
    	CopyDirectory("scripts/graphics");
    	CopyDirectory("scripts/gui");
    	CopyDirectory("scripts/library");
    	CopyDirectory("scripts/main_menu");
    	CopyDirectory("scripts/main_menu/backgrounds");
    	CopyDirectory("scripts/map_controllers");
    	CopyDirectory("scripts/map_editor");
    	CopyDirectory("scripts/mobs");
    	CopyDirectory("scripts/networking");
    	CopyDirectory("scripts/objects");
    	CopyDirectory("scripts/player");
    	CopyDirectory("scripts/procedural_generation");
    	CopyDirectory("scripts/thirdparty");
    	CopyDirectory("scripts/thirdparty/copas");
    	CopyDirectory("scripts/thirdparty/json");
    	CopyDirectory("scripts/thirdparty/json/lunajson");
    	CopyDirectory("scripts/thirdparty/socket");
    	CopyDirectory("scripts/tutorial");
    	CopyDirectory("scripts/utilities");
    	CopyDirectory("maps");
    	CopyDirectory("graphics");
    	CopyDirectory("sounds");
    	CopyDirectory("text");

		FILE* f = fopen(v_arp.c_str(), "w");

		if(f == NULL)
		{
			Utilities::fatalError("Unable to write version file " + v_arp.str());
		}
		else
		{
			unsigned char b = '1';
			fwrite(&b, 1, 1, f);
		}
	}

	Utilities::debugMessage("... end file copy");
}

void CopyDirectory(std::string dir_name)
{
	std::string out_path = std::string(SDL_AndroidGetInternalStoragePath())+"/arp/"+dir_name;
	mkdir(out_path.c_str(), 0755);

	std::string asset_path = DATA_DIR + dir_name;

	AAssetDir* dir = AAssetManager_openDir(asset_man, asset_path.c_str());

	if(dir == NULL)
	{
		Utilities::fatalError("Failed to read directory " + asset_path);
	}
	else
	{
		const char* dir_entry;
		while((dir_entry = AAssetDir_getNextFileName(dir)))
		{
			// seems that getNextFileName never gets a directory name
			// no way to get a dir structure from assets??? FFS, android
			// if I have to encode the directory structure anyway, I don't need asset manager
			// well I guess having asset manager means I don;t need to add individual files

			CopyFile(dir_name+"/"+dir_entry);
		}

		AAssetDir_close(dir);
	}
}

void CopyFile(std::string file_name)
{
	std::string asset_path = DATA_DIR + file_name;

	SDL_RWops* sdl_f = SDL_RWFromFile(asset_path.c_str(), "r");

	if(sdl_f == NULL)
	{
		Utilities::fatalError("Failed to copy from" + asset_path);
	}
	else
	{
		AppResourcePath arp(file_name);
		FILE* f = fopen(arp.c_str(), "w");

		if(f == NULL)
		{
			Utilities::fatalError("Failed to copy to" + arp.str());
		}
		else
		{
			Sint64 size = sdl_f->size(sdl_f);

			if (size == -1)
			{
				// can't get a size for the file, copy byte at a time
				Utilities::debugMessage("Copying byte by byte: " + arp.str());

				unsigned char b;
				while (SDL_RWread(sdl_f, &b, 1, 1))
				{
					fwrite(&b, 1, 1, f);
				}
			}
			else
			{
				// we know how long the file is, so copy it in chunks
				//Utilities::debugMessage("Copying in chunks: " + arp.str());

				unsigned int copysize = 4096;
				unsigned char bytes[copysize];

				while(size)
				{
					if (size < copysize)
					{
						copysize = size;
					}

					SDL_RWread(sdl_f, bytes, copysize, 1);
					fwrite(bytes, copysize, 1, f);

					size = size - copysize;
				}
			}

			fclose(f);
		}

		SDL_RWclose(sdl_f);
	}
}

#endif



