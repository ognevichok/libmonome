/*
 * Copyright (c) 2007-2010, William Light <will@visinin.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>

#include <monome.h>
#include "internal.h"

/* faster than using the global libmonome functions
   also, the global functions are 1-indexed, these are 0-indexed */
#define ROWS(monome) (monome->rows - 1)
#define COLS(monome) (monome->cols - 1)

static uint top_quad_map[]    = {2, 0, 3, 1};
static uint bottom_quad_map[] = {1, 3, 0, 2};

/**
 * public
 */

/* you may notice the gratituous use of modulo when translating input
   coordinates...this is because it's possible to translate into negatives
   when pretending a bigger monome (say, a 256) is a smaller monome (say,
   a 128).  because we're using unsigned integers, this will cause a
   wrap-around into some very big numbers, which makes several of the example
   programs segfault (simple.c, in particular).

   while this bug is arguably contrived, I'd rather pay the minute
   computational cost here and avoid causing trouble in application code. */

static void left_cb(monome_t *monome, uint *x, uint *y) {
	return;
}

static void left_frame_cb(monome_t *monome, uint *quadrant, uint8_t *frame_data) {
	return;
}

static void bottom_output_cb(monome_t *monome, uint *x, uint *y) {
	uint t = *x;

	*x = COLS(monome) - *y;
	*y = t;
}

static void bottom_input_cb(monome_t *monome, uint *x, uint *y) {
	uint t = *x;

	*x = *y;
	*y = (COLS(monome) - t) % (COLS(monome) + 1);
}

static void bottom_frame_cb(monome_t *monome, uint *quadrant, uint8_t *frame_data) {
	uint64_t t, x = *((uint64_t *) frame_data);

	/* this is an algorithm for rotation of a bit matrix by 90 degrees.
	   in the case of bottom_frame_cb, the rotation is clockwise, in the case
	   of top_frame_cb it is counter-clockwise.

	   the matrix is made up of an array of 8 bytes, which, laid out
	   contiguously in memory, can be treated as a 64-bit integer, which I've
	   opted to do here.  this allows rotation to be accomplished solely with
	   bitwise operations.

	   inspired by "hacker's delight" by henry s. warren
	   see section 7-3 "transposing a bit matrix" */

#define swap(f, c)\
	t = (x ^ (x << f)) & c; x ^= t ^ (t >> f);

	swap(8, 0xFF00FF00FF00FF00);
	swap(7, 0x5500550055005500);

	swap(16, 0xFFFF0000FFFF0000);
	swap(14, 0x3333000033330000);

	swap(32, 0xFFFFFFFF00000000);
	swap(28, 0x0F0F0F0F00000000);
#undef swap

	*((uint64_t *) frame_data) = x;
	*quadrant = bottom_quad_map[*quadrant & 0x3];
}

static void right_output_cb(monome_t *monome, uint *x, uint *y) {
	*x = ROWS(monome) - *x;
	*y = COLS(monome) - *y;
}

static void right_input_cb(monome_t *monome, uint *x, uint *y) {
	*x = (ROWS(monome) - *x) % (ROWS(monome) + 1);
	*y = (COLS(monome) - *y) % (COLS(monome) + 1);
}

static void right_frame_cb(monome_t *monome, uint *quadrant, uint8_t *frame_data) {
	uint64_t x = *((uint64_t *) frame_data);

	/* straightforward 64bit integer reversal...
	   might not work on embedded platforms? */

	x = x >> 32 | x << 32;
	x = (x & 0xFFFF0000FFFF0000) >> 16 | (x & 0x0000FFFF0000FFFF) << 16;
	x = (x & 0xFF00FF00FF00FF00) >> 8  | (x & 0x00FF00FF00FF00FF) << 8;
	x = (x & 0xF0F0F0F0F0F0F0F0) >> 4  | (x & 0x0F0F0F0F0F0F0F0F) << 4;
	x = (x & 0xCCCCCCCCCCCCCCCC) >> 2  | (x & 0x3333333333333333) << 2;
	x = (x & 0xAAAAAAAAAAAAAAAA) >> 1  | (x & 0x5555555555555555) << 1;

	*((uint64_t *) frame_data) = x;
	*quadrant = (3 - *quadrant) & 0x3;
}

static void top_output_cb(monome_t *monome, uint *x, uint *y) {
	uint t = *x;

	*x = *y;
	*y = ROWS(monome) - t;
}

static void top_input_cb(monome_t *monome, uint *x, uint *y) {
	uint t = *x;

	*x = (ROWS(monome) - *y) % (ROWS(monome) + 1);
	*y = t;
}

static void top_frame_cb(monome_t *monome, uint *quadrant, uint8_t *frame_data) {
	uint64_t t, x = *((uint64_t *) frame_data);

	/* see bottom_frame_cb for a brief explanation */

#define swap(f, c)\
	t = (x ^ (x << f)) & c; x ^= t ^ (t >> f);

	swap(8, 0xFF00FF00FF00FF00);
	swap(9, 0xAA00AA00AA00AA00);

	swap(16, 0xFFFF0000FFFF0000);
	swap(18, 0xCCCC0000CCCC0000);

	swap(32, 0xFFFFFFFF00000000);
	swap(36, 0xF0F0F0F000000000);
#undef swap

	*((uint64_t *) frame_data) = x;
	*quadrant = top_quad_map[*quadrant & 0x3];
}

monome_rotspec_t rotation[4] = {
	[MONOME_CABLE_LEFT] = {
		.output_cb = left_cb,
		.input_cb = left_cb,
		.frame_cb = left_frame_cb,

		.flags    = 0,
	},
	
	[MONOME_CABLE_BOTTOM] = {
		.output_cb = bottom_output_cb,
		.input_cb = bottom_input_cb,
		.frame_cb = bottom_frame_cb,

		.flags    = ROW_COL_SWAP | COL_REVBITS
	},

	[MONOME_CABLE_RIGHT] = {
		.output_cb = right_output_cb,
		.input_cb = right_input_cb,
		.frame_cb = right_frame_cb,

		.flags    = ROW_REVBITS | COL_REVBITS
	},

	[MONOME_CABLE_TOP] = {
		.output_cb = top_output_cb,
		.input_cb = top_input_cb,
		.frame_cb = top_frame_cb,

		.flags    = ROW_COL_SWAP | ROW_REVBITS
	},
};
