/*
 *  image_loader.h
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


#include "SDL.h"

SDL_Surface* load_image(const char* filename); // loads BMP or PNG files

// PNG specific
SDL_Surface* load_PNG(const char* filename);
int save_PNG(SDL_Surface* surface, const char* filename);

