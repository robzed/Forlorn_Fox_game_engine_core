/*
 *  image_loader.cpp
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 18/02/2014.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2014 Rob Probin
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

#include "image_loader.h"
#include "lodepng.h"
#include "SDL.h"
#include <vector>
#include <iostream>
#include <cstdio>
#include "Utilities.h"

#define SDL2_image_load 0
#if SDL2_image_load
#include "SDL_image.h"
#endif

SDL_Surface* load_image(const char* filename)
{

	SDL_Surface* img = 0;

	size_t namesize = strlen(filename);
	size_t end_index = namesize <= 4 ? 0 : namesize - 4;
	const char* extension = filename + end_index;

	if(strcmp(extension, ".PNG")==0 or strcmp(extension, ".png")==0)
	{
#if SDL2_image_load
		static int SDL2_image_init = 0;
		if(SDL2_image_init == 0) {
			if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) { Utilities::fatalError("Couldn't init SDL2_image", 42077); }
			SDL2_image_init = 1;
		}
		img = IMG_Load(filename);
#else
		img = load_PNG(filename);
#endif
	}
	else if(strcmp(extension, ".BMP")==0 or strcmp(extension, ".bmp")==0)
	{
		img = SDL_LoadBMP(filename);
		if(img==NULL)
		{
			Utilities::debugMessage("SDL_LoadBMP failed: %s\n", SDL_GetError());
		}
	}
	else
	{
		Utilities::debugMessage("Unknown image filename extension in load_image\n");
	}

	return img;
}

SDL_Surface* load_PNG(const char* filename)
{
    // stop lodepng::load_file from crashing when the file does not exist...
    if(not Utilities::file_exists(filename))
    {
        Utilities::fatalError("load_PNG() file does not exist: %s", filename);
    }

	std::vector<unsigned char> buffer, image;
	lodepng::load_file(buffer, filename); //load the image file with given filename
	unsigned w, h;
	unsigned error = lodepng::decode(image, w, h, buffer); //decode the png

	if (error)
	{
		Utilities::fatalError("load_PNG() decode error: %s",lodepng_error_text(error));
	}

	Uint32 rmask, gmask, bmask, amask;
	/* SDL interprets each pixel as a 32-bit number, so our masks must depend
	 on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

	//avoid too large window size by downscaling large image
	unsigned jump = 1;
	//if(w / 1024 >= jump) jump = w / 1024 + 1;
	//if(h / 1024 >= jump) jump = h / 1024 + 1;

	SDL_Surface *dest = SDL_CreateRGBSurface(0, w, h, 32,
											  rmask, gmask, bmask, amask);
	if(dest == NULL)
	{
		Utilities::fatalError("CreateRGBSurface failed: %s",SDL_GetError());
    }

	if(SDL_MUSTLOCK(dest)) { SDL_LockSurface(dest); }

	//plot the pixels of the PNG file
	for(unsigned y = 0; y + jump - 1 < h; y += jump)
		for(unsigned x = 0; x + jump - 1 < w; x += jump)
		{
			//get RGBA components
			Uint32 r = image[4 * y * w + 4 * x + 0]; //red
			Uint32 g = image[4 * y * w + 4 * x + 1]; //green
			Uint32 b = image[4 * y * w + 4 * x + 2]; //blue
			Uint32 a = image[4 * y * w + 4 * x + 3]; //alpha

			//make translucency visible by placing checkerboard pattern behind image
			//int checkerColor = 191 + 64 * (((x / 16) % 2) == ((y / 16) % 2));
			//r = (a * r + (255 - a) * checkerColor) / 255;
			//g = (a * g + (255 - a) * checkerColor) / 255;
			//b = (a * b + (255 - a) * checkerColor) / 255;

			//give the color value to the pixel of the screenbuffer
			Uint32* bufp = (Uint32 *)dest->pixels + (y * dest->pitch / 4) / jump + (x / jump);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			*bufp = 0x01000000* r + 65536 * g + 256 * b + a;
#else
			*bufp = 0x01000000* a + 65536 * b + 256 * g + r;
#endif
		}

	if(SDL_MUSTLOCK(dest)) { SDL_UnlockSurface(dest); }
	return dest;
}


//#define save_png_with_SDL
#ifdef save_png_with_SDL
#include "SDL_image.h"
#endif

int save_PNG(SDL_Surface* surface, const char* filename)
{
#ifdef save_png_with_SDL
	return IMG_SavePNG(surface, filename);
#else
	// double check parameters
	if(!surface) return -2000;
	if(!filename) return -2001;

	Uint32 rmask, gmask, bmask, amask;
	/* SDL interprets each pixel as a 32-bit number, so our masks must depend
	 on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

	SDL_PixelFormat* f = surface->format;
	if(!f or f->Rmask != rmask or f->Gmask != gmask or f->Bmask != bmask or
	   f->Amask != amask)
	{
		Utilities::debugMessage("save_PNG: WRONG PIXEL FORMAT FOR SURFACE!\n");
		return -2002;
	}


	std::vector<unsigned char> image;
	unsigned w = surface->w;
	unsigned h = surface->h;
	image.resize(w*h*4);		// make sure image vector is big enough

	if(SDL_MUSTLOCK(surface)) { SDL_LockSurface(surface); }
	//decode the pixels for the PNG file
	for(unsigned y = 0; y < h; y ++)
		for(unsigned x = 0; x < w; x ++)
		{
			//get RGBA components from the pixel of the screenbuffer
			Uint32* bufp = (Uint32 *)surface->pixels + (y * surface->pitch / 4) + (x);
			Uint32 data = *bufp;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			Uint32 r = data >> 24;
			Uint32 g = (data >> 16) & 0xff;
			Uint32 b = (data >> 8) & 0xff;
			Uint32 a = data & 0xff;
#else
			Uint32 a = data >> 24;
			Uint32 b = (data >> 16) & 0xff;
			Uint32 g = (data >> 8) & 0xff;
			Uint32 r = data & 0xff;
#endif
			// write the r g b a components into the image
			image[4 * y * w + 4 * x + 0] = r; //red
			image[4 * y * w + 4 * x + 1] = g; //green
			image[4 * y * w + 4 * x + 2] = b; //blue
			image[4 * y * w + 4 * x + 3] = a; //alpha
		}
	if(SDL_MUSTLOCK(surface)) { SDL_UnlockSurface(surface); }

	std::vector<unsigned char> png;
	unsigned error = lodepng::encode(png, image, w, h);

	if(error)
	{
		Utilities::debugMessage("PNG encoding error %u : %s", error, lodepng_error_text(error));
		return -2003;
	}

	lodepng::save_file(png, filename);
	return 0;
#endif
}

#if 0
#include "SDL_image.h"
void SDL2_image_test()
{
	system("pwd");
	using namespace std;
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) exit(1);

	//SDL_Window *win = SDL_CreateWindow("whisper8.com", 50, 50, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	//if (!win) exit(1);

	SDL_Surface* bitmap = IMG_Load("test.bmp");
	if(!bitmap) exit(1);
	SDL_Surface* png = IMG_Load("test.png");
	if(!png) exit(1);

	if(SDL_MUSTLOCK(bitmap)) { SDL_LockSurface(bitmap); }
	if(SDL_MUSTLOCK(png)) { SDL_LockSurface(png); }

	Uint32 bp00 = getpixel(bitmap, 0, 0);
	Uint32 pp00 = getpixel(png, 0, 0);
	Uint32 bp31 = getpixel(bitmap, 31, 0);
	Uint32 pp31 = getpixel(png, 31, 0);

	cout << hex;
	cout << "bitmap(0, 0) = " << bp00 << endl;
	cout << "png(0, 0) = " << pp00 << endl;
	cout << "bitmap(31, 0) = " << bp31 << endl;
	cout << "png(31, 0) = " << pp31 << endl;

	if(SDL_MUSTLOCK(png)) { SDL_UnlockSurface(png); }
	if(SDL_MUSTLOCK(bitmap)) { SDL_UnlockSurface(bitmap); }

	IMG_Quit();
}
#endif
