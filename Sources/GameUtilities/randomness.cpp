//
//  randomness.cpp
//  Forlorn_Fox_Mac
//
//  Created by Rob Probin on 09/05/2015.
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

#include "randomness.h"



// From:
// http://www.lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
// http://stackoverflow.com/questions/1046714/what-is-a-good-random-number-generator-for-a-game
/* On that WELL page I linked you to, the number is the period of the algorithm. That is, you can get 2^N - 1 numbers before it needs reseeding, where N is: 512, 1024, 19937, or 44497. Mersenne Twister has a period of N = 19937, or 2^19937 - 1. You'll see this is a very large number :) */
// http://www.iro.umontreal.ca/~panneton/WELLRNG.html
// http://www.amazon.com/dp/1584505273

/* initialize state to random bits */
static unsigned long state[16];
/* init should also reset this to 0 */
static unsigned int WELL_index = 0;
/* return 32 bit random number */
unsigned long WELLRNG512(void)
{
    unsigned long a, b, c, d;
    a = state[WELL_index];
    c = state[(WELL_index+13)&15];
    b = a^c^(a<<16)^(c<<15);
    c = state[(WELL_index+9)&15];
    c ^= (c>>11);
    a = state[WELL_index] = b^c;
    d = a^((a<<5)&0xDA442D24UL);
    WELL_index = (WELL_index + 15)&15;
    a = state[WELL_index];
    state[WELL_index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
    return state[WELL_index];
}
// Original code doesn't appear free for commercial purposes?
// But this is not the original code.
// Also this has a period of only 512, so probably needs reseeding?
// There are longer period versions...
// We could reseed with system random or zex_internal_rand
// Could we use real randomness to seed rather than time/date?
// Minecraft had a very large original seed...


// More ideas:
//http://burtleburtle.net/bob/rand/isaacafa.html
// http://burtleburtle.net/bob/rand/isaac.html#IBAAcode
// http://en.wikipedia.org/wiki/Linear_congruential_generator
// http://en.wikipedia.org/wiki/Pseudorandom_number_generator
// http://nethackwiki.com/wiki/Source:NetHack_3.4.3/src/rnd.c
// http://nethackwiki.com/wiki/Random_number_generator


// +-----------------------------------+-------------------------+-----------------------
// | TITLE:                            | AUTHOR(s): Rob Probin   | DATE STARTED: 10 Oct 2005
// +
// |
// +----------------------------------------------------------------ROUTINE HEADER----
// http://stackoverflow.com/questions/15939495/is-the-bash-function-random-supposed-to-have-an-uniform-distribution

#define INTERNAL_RAND_MAX 0x7fffffff

static unsigned long int next = 1;

int zex_internal_rand()
{
    /*
     * Compute x = (7^5 * x) mod (2^31 - 1)
     * wihout overflowing 31 bits:
     *      (2^31 - 1) = 127773 * (7^5) + 2836
     * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */
    
    long hi, lo, x;
    
    /* Can't be initialized with 0, so use another value. */
    if (next == 0)
        next = 123459876;
    hi = next / 127773;
    lo = next % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    return ((next = x) % ((unsigned long int)INTERNAL_RAND_MAX + 1));
    
}

void zex_internal_srand(unsigned int seed)
{
    next = seed;
}
