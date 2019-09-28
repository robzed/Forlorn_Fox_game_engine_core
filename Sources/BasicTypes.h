/*
 *  BasicTypes.h
 *  Forlorn Fox
 *
 *  Created by Rob Probin on 21/02/2012.
 *
 * ------------------------------------------------------------------------------
 * Copyright (c) 2012-2013 Rob Probin and Tony Park
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

#ifndef _MSC_VER
#include <stdint.h>
#elif _MSC_VER >= 1300
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif

// pos_t
// What type are the position commands?
// They used to be int, and we had seperate pixel x,y commands - however:
//  (a) that's not resolution independant (causing real problems)
//  (b) there were twice as many methods
// Should this be float or double? On most modern chips doesn't matter, but
// float is less storage, and 32-bits, with a significand has a precision of
// 24 bits (about 7 decimal digits) is fine. We hardly use the exponent as well,
// so that's not a limitation.
// Therefore we are fine with single-precision floating points.
//
// This is also used for input sometimes.
typedef float pos_t;		// was int.

