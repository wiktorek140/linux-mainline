/*
 * Copyright (C) 2012 - 2017, Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/div64.h>

#include "panel-samsung-s6e3fa7-ams604nl01-gamma.h"

#define LUMINANCE_MAX	    74
#define GRAY_SCALE_MAX	    256
#define RGB_COMPENSATION    9
#define FP_SHIFT	    22
#define VREG0_REF_6P5 ((int) (6.5 * (2 << 21)))

static const signed char red_offsets[LUMINANCE_MAX][RGB_COMPENSATION] = {
/*	{  R255,  R203,  R151,  R87,  R51,  R35,  R23,  R11,   R7  },	*/
	{    -5,    -2,    -5,  -14,  -22,  -19,  -22,  -23,  -13  },
	{    -3,    -2,    -3,  -11,  -21,  -19,  -24,  -25,  -13  },
	{    -2,    -2,    -3,  -11,  -20,  -19,  -22,  -24,  -13  },
	{    -1,    -2,    -3,  -10,  -20,  -20,  -21,  -24,  -13  },
	{     0,    -1,    -3,   -9,  -20,  -20,  -22,  -24,  -13  },
	{     0,    -1,    -3,   -7,  -20,  -18,  -22,  -25,  -13  },
	{     0,    -1,    -3,   -7,  -18,  -15,  -20,  -20,  -10  },
	{     0,    -1,    -3,   -6,  -16,  -15,  -17,  -20,  -10  },
	{     0,    -1,    -3,   -6,  -14,  -11,  -16,  -16,   -9  },
	{     0,     0,    -3,   -4,  -14,   -9,  -16,  -15,   -9  },
	{     0,     0,    -3,   -4,  -13,  -10,  -16,  -11,   -8  },
	{     0,     0,    -3,   -4,  -13,  -10,  -14,  -11,   -8  },
	{     0,     0,    -3,   -4,  -13,  -10,  -14,   -9,   -8  },
	{     1,     0,    -3,   -4,  -13,  -10,  -14,   -9,   -8  },
	{     1,     0,    -2,   -4,  -13,  -10,  -12,   -7,   -7  },
	{     1,     0,    -2,   -4,  -12,  -10,  -13,   -9,   -7  },
	{     1,     0,    -2,   -3,  -11,  -10,  -13,  -10,   -7  },
	{     0,     0,    -2,   -3,  -11,   -9,  -13,  -12,   -7  },
	{     1,     0,    -2,   -3,   -7,  -11,  -11,  -15,   -7  },
	{     1,     0,    -2,   -3,   -7,  -11,  -12,  -11,   -7  },
	{     1,     0,    -2,   -2,   -7,  -10,  -11,  -12,   -7  },
	{     1,     0,    -1,   -2,   -7,   -7,  -10,  -13,   -7  },
	{     1,     0,    -1,   -2,   -6,   -6,   -8,  -15,   -7  },
	{     1,     0,    -1,   -2,   -6,   -6,   -9,  -15,   -4  },
	{     1,     0,    -1,   -2,   -4,   -7,  -10,  -15,   -4  },
	{     1,     0,    -1,   -2,   -4,   -5,  -10,  -14,   -5  },
	{     1,     0,    -1,   -2,   -4,   -5,  -10,  -16,   -5  },
	{     1,     0,    -1,   -2,   -4,   -5,  -11,  -14,   -4  },
	{     1,     0,    -1,   -2,   -5,   -5,  -11,  -14,   -4  },
	{     1,     0,    -1,   -2,   -5,   -4,  -11,  -15,   -5  },
	{     1,     0,    -1,   -2,   -2,   -3,   -9,  -15,   -5  },
	{     1,     0,    -1,   -2,   -2,   -3,   -7,  -13,   -3  },
	{     1,     0,    -1,   -2,   -2,   -3,   -7,  -11,   -3  },
	{     1,     0,    -1,   -2,   -2,   -3,   -7,  -11,   -5  },
	{     1,     0,    -1,   -1,   -3,   -4,   -6,  -15,   -3  },
	{     1,     0,    -1,   -2,   -2,   -2,   -7,  -15,   -5  },
	{     0,     0,    -1,   -1,   -2,   -3,   -9,  -15,   -4  },
	{     1,     0,    -1,   -1,   -2,   -4,   -5,  -16,   -4  },
	{     1,     0,    -2,   -1,   -1,   -3,   -8,  -18,   -2  },
	{     1,     0,    -1,   -1,   -2,   -5,   -6,  -18,   -1  },
	{     1,     0,    -2,    1,   -2,   -4,   -4,  -14,   -6  },
	{     0,     1,     0,   -1,   -3,   -1,   -4,  -13,   -8  },
	{     1,     0,    -1,    0,   -1,   -5,   -3,  -13,   -8  },
	{     0,     0,     0,    0,   -2,   -1,   -6,  -16,   -5  },
	{     1,     0,    -1,    0,   -1,   -2,   -4,  -16,   -5  },
	{     0,     1,     0,   -1,   -1,   -1,   -2,  -16,   -6  },
	{     0,     0,    -1,   -1,   -2,   -1,   -3,  -13,   -6  },
	{     1,     1,    -1,   -2,   -2,    0,   -3,  -14,   -6  },
	{     0,     0,     0,    0,   -3,    0,   -5,  -15,   -8  },
	{     0,     1,    -1,    0,   -2,   -1,   -2,  -15,  -11  },
	{    -1,     0,     0,   -1,    0,    1,   -6,  -12,  -11  },
	{    -1,     0,    -1,    1,   -1,   -1,   -4,  -11,  -11  },
	{    -1,     1,     0,    0,    0,    0,   -1,   -8,  -11  },
	{     0,     1,     1,   -1,   -2,   -1,    0,   -8,  -10  },
	{     0,     1,     0,   -1,   -1,    0,    0,   -7,  -13  },
	{     0,     1,     0,   -1,   -1,   -1,   -1,   -9,   -5  },
	{     0,     0,     0,    0,    0,    0,   -2,   -8,   -3  },
	{     0,     0,     0,    0,    0,    0,    1,   -7,   -4  },
	{    -1,     0,     1,    1,    0,    0,   -2,   -5,   -4  },
	{     0,     0,     0,    0,   -1,   -1,    0,   -5,   -5  },
	{     0,     0,     0,    0,    1,   -1,    0,   -7,   -7  },
	{     0,     0,     0,    0,    0,    1,   -2,   -2,   -8  },
	{     1,     0,     0,   -1,    0,    1,    1,   -2,   -9  },
	{     0,     0,     0,    1,    0,    0,   -3,    0,   -5  },
	{     0,     0,     1,    0,    0,    0,   -3,    0,    0  },
	{     1,    -1,     0,    0,    1,    2,   -4,    0,   -2  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   -1  },
	{     0,     0,     1,   -1,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
};

static const signed char green_offsets[LUMINANCE_MAX][RGB_COMPENSATION] = {
/*	{  G255,  G203,  G151,  G87,  G51,  G35,  G23,  G11,  G7  },	*/
	{     0,     1,     1,    1,   -1,   -1,   -8,   -3,  -6  },
	{     0,     1,     0,    0,   -4,   -3,   -8,   -5,  -6  },
	{     0,     1,    -1,   -1,   -2,   -3,   -9,   -7,  -6  },
	{     0,     0,    -1,    0,   -5,   -6,   -2,   -7,  -6  },
	{     0,     0,    -1,    0,   -5,   -6,   -5,   -7,  -6  },
	{     0,     0,    -1,   -1,   -5,   -6,   -7,   -8,  -6  },
	{     0,     0,    -1,   -1,   -5,   -2,   -5,   -8,  -6  },
	{     0,     0,    -1,   -1,   -4,   -2,   -4,   -8,  -6  },
	{     0,     0,    -1,   -1,   -3,    0,   -3,   -6,  -5  },
	{     0,     1,    -1,   -1,   -2,    2,   -4,   -5,  -5  },
	{     0,     1,    -1,   -1,   -1,    1,   -4,    0,  -4  },
	{     0,     1,    -1,   -1,   -1,    0,   -3,    0,  -4  },
	{     0,     1,    -1,   -1,   -2,    0,   -3,    0,  -4  },
	{     0,     1,    -1,   -1,   -2,    0,   -3,    0,  -4  },
	{     0,     1,     0,   -1,   -2,    0,    0,    2,  -3  },
	{     0,     1,     0,   -1,   -2,   -1,   -1,    2,  -5  },
	{     0,     1,     0,   -1,   -1,   -1,   -2,    1,  -5  },
	{     0,     1,     0,   -1,   -2,    1,   -4,    0,  -5  },
	{     0,     1,    -1,    0,    1,   -3,    0,   -3,  -5  },
	{     0,     1,    -1,    0,    0,   -3,   -1,    2,  -5  },
	{     0,     1,    -1,    0,    0,   -1,    0,    1,  -5  },
	{     0,     1,     0,    0,    0,    1,    1,    0,  -5  },
	{     0,     1,     0,    0,    0,    3,    2,    1,  -5  },
	{     0,     1,     0,   -1,    0,    2,    1,    1,   1  },
	{     0,     1,     0,   -1,    2,    0,    0,   -1,   1  },
	{     0,     1,     0,   -1,    2,    2,    1,    0,   0  },
	{     0,     1,     0,   -1,    1,    2,    0,   -2,   0  },
	{     0,     1,     0,   -1,    0,    2,   -1,    2,   1  },
	{     0,     1,     0,   -1,   -1,    1,   -1,    2,   1  },
	{     0,     1,     0,   -1,   -1,    1,   -2,    0,   0  },
	{     0,     1,     0,   -2,    2,    2,   -2,    0,  -1  },
	{     0,     1,     0,   -2,    1,    1,    1,    2,   1  },
	{     0,     1,     0,   -2,    1,    1,    1,    4,   1  },
	{     0,     1,     0,   -2,    1,    0,    1,    3,   0  },
	{     0,     1,     0,   -2,    1,    0,    0,    1,   0  },
	{     0,     1,     0,   -2,    0,    1,   -1,    0,  -1  },
	{     0,     0,     0,   -1,    0,   -1,   -3,    0,  -2  },
	{     0,     1,     0,   -1,    0,   -2,    2,   -1,  -1  },
	{     0,     0,    -1,   -1,    1,    0,   -2,   -3,   4  },
	{     0,     1,     0,   -1,    0,   -3,    0,   -2,   4  },
	{     0,     1,     0,    0,    1,   -2,    1,    1,  -2  },
	{     0,     1,     0,   -1,    0,    1,    1,    1,  -2  },
	{     0,     0,     0,    0,    1,   -1,    1,    2,  -3  },
	{     0,     0,     0,    0,    1,    1,   -2,   -2,   3  },
	{     0,     0,     0,    0,    0,    1,   -1,   -1,   2  },
	{     0,     1,     0,   -1,    0,    2,    0,   -2,   2  },
	{     0,     0,     0,   -1,   -2,    0,    1,    1,   2  },
	{     0,     0,     0,   -1,   -1,    2,    2,    0,   1  },
	{     0,     0,     0,    0,   -1,    1,   -2,   -2,  -1  },
	{     0,     0,     0,    0,   -1,    0,    1,   -4,  -1  },
	{     0,     0,     0,    0,    0,    2,   -3,    0,  -1  },
	{     0,    -1,     0,    1,   -1,    1,   -1,    0,   0  },
	{     0,     0,     1,    0,    1,    1,    2,    2,  -1  },
	{     0,     0,     1,    0,    0,    0,    3,    3,  -1  },
	{     0,     0,     0,    0,    1,    1,    2,    1,  -2  },
	{     0,     0,     0,    0,    1,   -1,    1,    0,   2  },
	{     0,     0,     0,    0,    1,    0,    1,    2,   2  },
	{     0,     0,     0,    0,    0,    1,    3,    2,   1  },
	{     0,     0,     0,    1,    0,    1,   -1,    3,   2  },
	{     0,     0,     0,    0,    0,    1,    2,    2,   0  },
	{     0,     0,     0,    0,    0,    2,    1,    1,  -2  },
	{     0,     0,     0,    0,    0,    1,   -1,    2,  -3  },
	{     0,     0,     0,    0,    0,    1,    2,    1,  -4  },
	{     0,     0,     0,    1,    0,    0,   -2,    3,   2  },
	{     0,     0,     1,    0,    0,    1,   -2,    2,   2  },
	{     0,     0,     0,    0,    1,    1,   -3,    2,   2  },
	{     0,     0,     0,    0,    0,    0,    0,    1,   2  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   2  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   2  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,   0  },
};

static const signed char blue_offsets[LUMINANCE_MAX][RGB_COMPENSATION] = {
/*	{  B255,  B203,  B151,  B87,  B51,  B35,  B23,  B11,   B7  },	*/
	{    -2,    -2,    -2,   -7,  -10,   -6,  -10,   -4,   -5  },
	{    -1,    -1,    -1,   -7,  -12,   -7,  -11,   -8,   -8  },
	{    -1,    -1,    -1,   -7,  -12,   -7,  -10,  -11,   -8  },
	{    -1,    -1,    -1,   -7,  -15,   -9,   -7,  -11,   -8  },
	{     0,    -1,    -1,   -6,  -16,   -9,   -9,  -11,   -8  },
	{     0,    -1,    -1,   -5,  -16,  -10,  -10,  -12,   -8  },
	{     0,    -1,    -1,   -5,  -16,   -7,  -10,  -12,   -8  },
	{     0,    -1,    -1,   -5,  -14,   -7,  -10,  -12,   -8  },
	{     0,    -1,    -1,   -5,  -13,   -6,   -9,  -12,   -8  },
	{    -1,     0,    -1,   -3,  -14,   -4,  -10,  -11,   -8  },
	{    -1,     0,    -1,   -3,  -13,   -6,  -10,   -8,   -6  },
	{    -1,     0,    -1,   -3,  -13,   -7,  -10,   -8,   -6  },
	{    -1,     0,    -1,   -3,  -13,   -7,  -10,   -8,   -6  },
	{    -1,     0,    -1,   -3,  -13,   -7,  -11,   -8,   -6  },
	{    -1,     0,    -1,   -3,  -13,   -7,  -10,   -8,   -5  },
	{    -1,     0,    -1,   -3,  -12,   -7,  -11,   -9,   -7  },
	{    -1,     0,    -1,   -2,  -12,   -7,  -11,   -9,   -7  },
	{    -1,     0,    -1,   -2,  -11,   -7,  -11,  -11,   -7  },
	{    -1,     0,    -1,   -2,   -9,   -9,   -9,  -14,   -7  },
	{    -1,     0,    -1,   -2,   -9,   -9,  -10,  -10,   -7  },
	{    -1,     0,    -1,   -1,   -9,   -8,   -9,  -12,   -7  },
	{    -1,     0,     0,   -2,   -9,   -5,   -8,  -12,   -7  },
	{    -1,     0,     0,   -2,   -8,   -4,   -7,  -12,   -7  },
	{    -1,     0,     0,   -2,   -8,   -4,   -8,  -12,   -4  },
	{    -1,     0,     0,   -2,   -7,   -5,   -8,  -12,   -4  },
	{    -1,     0,     0,   -2,   -7,   -2,   -8,  -13,   -4  },
	{    -1,     0,     0,   -2,   -7,   -2,   -8,  -15,   -4  },
	{    -1,     0,     0,   -2,   -7,   -2,   -9,  -13,   -3  },
	{    -1,     0,     0,   -2,   -8,   -2,   -8,  -13,   -4  },
	{    -1,     0,     0,   -2,   -8,   -2,   -8,  -14,   -5  },
	{    -1,     0,     0,   -2,   -5,   -2,   -7,  -14,   -5  },
	{    -1,     0,     0,   -2,   -5,   -2,   -5,  -14,   -2  },
	{    -1,     0,     0,   -2,   -5,   -2,   -5,  -12,   -2  },
	{    -1,     0,     0,   -2,   -5,   -2,   -5,  -12,   -4  },
	{    -1,     0,     0,   -2,   -5,   -2,   -5,  -14,   -4  },
	{    -1,     0,     0,   -2,   -5,   -1,   -5,  -14,   -4  },
	{    -1,    -1,     0,   -1,   -5,   -2,   -7,  -14,   -5  },
	{    -1,     1,     0,   -1,   -5,   -2,   -4,  -15,   -5  },
	{    -1,     0,     0,   -1,   -4,   -1,   -7,  -15,   -3  },
	{    -1,     0,     1,   -1,   -5,   -3,   -5,  -15,   -3  },
	{    -1,     0,     0,    1,   -5,   -2,   -3,  -12,   -7  },
	{    -1,     0,     1,   -1,   -4,    0,   -3,  -12,   -8  },
	{    -1,     0,     0,    1,   -4,   -2,   -3,  -11,   -9  },
	{     0,    -1,     1,    0,   -4,   -1,   -5,  -14,   -4  },
	{     0,     0,     0,    0,   -4,    0,   -5,  -12,   -5  },
	{    -1,     0,     0,   -1,   -2,    0,   -2,  -13,   -6  },
	{     0,    -1,     0,    1,   -5,   -1,   -2,  -11,   -6  },
	{     0,     0,     0,    0,   -3,    0,   -1,  -12,   -7  },
	{     0,    -1,     0,    1,   -4,    0,   -4,  -12,   -9  },
	{     0,     0,     0,    1,   -2,   -1,   -2,  -14,  -11  },
	{    -1,    -1,    -1,    0,   -1,    1,   -5,  -11,  -11  },
	{    -1,    -1,    -1,    1,   -2,   -1,   -4,  -10,  -10  },
	{    -1,     0,     1,    1,   -1,    0,   -2,   -6,  -11  },
	{     0,     0,     1,    1,   -2,   -1,    0,   -7,  -10  },
	{     0,     0,     0,    1,   -1,    0,   -1,   -7,  -11  },
	{     0,     0,     0,    1,   -1,   -1,   -2,   -8,   -4  },
	{     0,     0,     0,    1,   -1,    0,   -1,   -7,   -4  },
	{     0,    -1,     0,    1,   -1,    1,    1,   -4,   -5  },
	{    -1,     0,     0,    1,    0,    0,   -2,   -4,   -5  },
	{     0,     0,     0,    1,   -2,    0,    0,   -4,   -3  },
	{    -1,     0,     0,    1,    0,    0,    0,   -6,   -6  },
	{    -1,     0,     0,    1,    0,    1,   -2,   -1,   -7  },
	{     0,     0,     0,    0,    0,    1,    1,   -1,   -8  },
	{    -1,     0,     0,    1,   -1,    0,   -2,    2,    0  },
	{     0,     0,     1,    0,    1,    0,   -3,    1,    1  },
	{     0,     0,     0,    0,    1,    2,   -4,    1,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    1  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    1  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
	{     0,     0,     0,    0,    0,    0,    0,    0,    0  },
};

static const unsigned char gamma_offsets[LUMINANCE_MAX][11] = {
	{ 0x00, 0x01, 0x22, 0x22, 0x26, 0x26, 0x2b, 0x37, 0x53, 0x6d, 0x86 },
	{ 0x00, 0x01, 0x1f, 0x21, 0x23, 0x24, 0x29, 0x34, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x1e, 0x1f, 0x21, 0x22, 0x27, 0x34, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x1d, 0x1d, 0x1f, 0x22, 0x27, 0x33, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x1c, 0x1c, 0x1e, 0x21, 0x25, 0x32, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x1b, 0x1b, 0x1d, 0x20, 0x25, 0x32, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x1a, 0x1a, 0x1c, 0x1f, 0x25, 0x32, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x19, 0x19, 0x1b, 0x1e, 0x24, 0x32, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x18, 0x18, 0x1a, 0x1d, 0x24, 0x32, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x17, 0x17, 0x19, 0x1c, 0x23, 0x31, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x16, 0x16, 0x19, 0x1c, 0x22, 0x31, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x16, 0x16, 0x19, 0x1c, 0x22, 0x31, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x16, 0x16, 0x19, 0x1c, 0x22, 0x31, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x16, 0x16, 0x19, 0x1c, 0x22, 0x31, 0x52, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x15, 0x15, 0x18, 0x1c, 0x22, 0x31, 0x51, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x15, 0x15, 0x18, 0x1c, 0x22, 0x31, 0x51, 0x6c, 0x86 },
	{ 0x00, 0x01, 0x14, 0x14, 0x17, 0x1b, 0x21, 0x31, 0x51, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x14, 0x14, 0x17, 0x1a, 0x21, 0x30, 0x51, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x14, 0x14, 0x16, 0x1a, 0x20, 0x30, 0x51, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x13, 0x13, 0x16, 0x1a, 0x20, 0x30, 0x51, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x12, 0x12, 0x15, 0x19, 0x20, 0x30, 0x51, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x11, 0x11, 0x14, 0x18, 0x1f, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x10, 0x10, 0x13, 0x17, 0x1f, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0f, 0x10, 0x13, 0x17, 0x1f, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0f, 0x10, 0x13, 0x17, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0e, 0x0f, 0x12, 0x16, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0e, 0x0f, 0x12, 0x16, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0d, 0x0e, 0x12, 0x16, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0d, 0x0e, 0x12, 0x16, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0d, 0x0e, 0x12, 0x16, 0x1e, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0c, 0x0d, 0x11, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0b, 0x0c, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x15, 0x1d, 0x2f, 0x50, 0x6b, 0x86 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x10, 0x16, 0x1e, 0x30, 0x53, 0x6f, 0x8a },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x11, 0x16, 0x1f, 0x32, 0x55, 0x72, 0x8e },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x11, 0x17, 0x1f, 0x33, 0x57, 0x75, 0x92 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x11, 0x17, 0x20, 0x34, 0x5a, 0x79, 0x96 },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x11, 0x17, 0x21, 0x36, 0x5c, 0x7c, 0x9a },
	{ 0x00, 0x01, 0x0a, 0x0b, 0x11, 0x18, 0x21, 0x37, 0x5f, 0x80, 0x9e },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x12, 0x18, 0x22, 0x39, 0x62, 0x83, 0xa3 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x12, 0x19, 0x23, 0x3b, 0x65, 0x88, 0xa8 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x12, 0x1a, 0x24, 0x3c, 0x68, 0x8a, 0xad },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x13, 0x1b, 0x26, 0x3f, 0x6c, 0x8e, 0xb2 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x13, 0x1b, 0x27, 0x41, 0x6e, 0x91, 0xb6 },
	{ 0x00, 0x01, 0x0b, 0x0d, 0x14, 0x1c, 0x28, 0x43, 0x72, 0x97, 0xbc },
	{ 0x00, 0x01, 0x0b, 0x0d, 0x14, 0x1c, 0x29, 0x45, 0x75, 0x9a, 0xc1 },
	{ 0x00, 0x01, 0x0b, 0x0d, 0x15, 0x1d, 0x2a, 0x47, 0x79, 0x9f, 0xc7 },
	{ 0x00, 0x01, 0x0b, 0x0d, 0x15, 0x1e, 0x2b, 0x48, 0x7d, 0xa4, 0xcd },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x15, 0x1e, 0x2b, 0x49, 0x7e, 0xa7, 0xd1 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x15, 0x1f, 0x2c, 0x4b, 0x81, 0xab, 0xd6 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x15, 0x1f, 0x2c, 0x4b, 0x81, 0xab, 0xd6 },
	{ 0x00, 0x01, 0x0a, 0x0c, 0x15, 0x1f, 0x2c, 0x4b, 0x81, 0xab, 0xd6 },
	{ 0x00, 0x01, 0x09, 0x0b, 0x15, 0x1f, 0x2c, 0x4b, 0x81, 0xab, 0xd6 },
	{ 0x00, 0x01, 0x08, 0x0b, 0x14, 0x1f, 0x2c, 0x4b, 0x81, 0xaa, 0xd6 },
	{ 0x00, 0x01, 0x08, 0x0b, 0x15, 0x1f, 0x2d, 0x4c, 0x83, 0xae, 0xda },
	{ 0x00, 0x01, 0x09, 0x0b, 0x15, 0x20, 0x2e, 0x4e, 0x86, 0xb3, 0xdf },
	{ 0x00, 0x01, 0x09, 0x0b, 0x16, 0x21, 0x2f, 0x50, 0x8a, 0xb6, 0xe4 },
	{ 0x00, 0x01, 0x09, 0x0b, 0x16, 0x21, 0x30, 0x51, 0x8c, 0xba, 0xe8 },
	{ 0x00, 0x01, 0x09, 0x0b, 0x16, 0x22, 0x31, 0x53, 0x8f, 0xbe, 0xed },
	{ 0x00, 0x01, 0x08, 0x0b, 0x17, 0x22, 0x31, 0x54, 0x92, 0xc2, 0xf2 },
	{ 0x00, 0x01, 0x08, 0x0b, 0x16, 0x23, 0x32, 0x55, 0x92, 0xc5, 0xf6 },
	{ 0x00, 0x01, 0x08, 0x0a, 0x17, 0x22, 0x31, 0x56, 0x95, 0xc6, 0xf8 },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x33, 0x57, 0x96, 0xc8, 0xfa },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x33, 0x57, 0x95, 0xc8, 0xfa },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x34, 0x57, 0x95, 0xc8, 0xfa },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x34, 0x57, 0x95, 0xc8, 0xfa },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x34, 0x57, 0x95, 0xc8, 0xfa },
	{ 0x00, 0x01, 0x07, 0x0b, 0x18, 0x24, 0x34, 0x58, 0x97, 0xca, 0xfc },
	{ 0x00, 0x01, 0x07, 0x0c, 0x18, 0x24, 0x34, 0x58, 0x98, 0xcb, 0xfd },
	{ 0x00, 0x01, 0x07, 0x0b, 0x17, 0x23, 0x33, 0x57, 0x97, 0xcb, 0xff },
};

/* TP */
enum { VT, V1, V7, V11, V23, V35, V51, V87, V151, V203, V255, V_MAX };

/* RGB  */
enum {
	V_R       = 0 * V_MAX,
	V_G       = 1 * V_MAX,
	V_B       = 2 * V_MAX,
	V_RGB_MAX = 3 * V_MAX,
};

static const int inflection_voltages[V_MAX] = {0, 1, 7, 11, 23, 35, 51, 87, 151, 203, 255};

// center (max) gamma value (Hex)
static const int center_gamma[V_RGB_MAX] = {
/*	 VT    V1    V7   V11   V23   V35   V51   V87  V151  V203  V255	    */
	0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x100, // Red
	0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x100, // Green
	0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x100, // Doesn't matter anyway
};

/* fraction for gamma code */
static const int fraction[V_MAX][2] = {
	/* {offset, denominator} */
	{0,  860},	/* VT */
	{0,  256},	/* V1 */
	{64, 320},	/* V7 */
	{64, 320},	/* V11 */
	{64, 320},	/* V23 */
	{64, 320},	/* V35 */
	{64, 320},	/* V51 */
	{64, 320},	/* V87 */
	{64, 320},	/* V151 */
	{64, 320},	/* V203 */
	{129, 860},	/* V255 */
};

struct s6e3fa7_gamma_ctx {
	/* mtp_offsets offsets */
	int mtp_offsets[V_RGB_MAX];

	/* TP's gamma voltage */
	int rgb_out[V_RGB_MAX];

	/* grayscale (0~255) */
	int grayscale[GRAY_SCALE_MAX * 3];

	char generated_gamma[LUMINANCE_MAX][33];
};

/**********************************************************************************************\
 * 1. VT       : VREG1 - (VREG1 -     0   ) * (OutputGamma + numerator_TP)/(denominator_TP)   *
 * 2. V255     : VREG1 - (VREG1 -     0   ) * (OutputGamma + numerator_TP)/(denominator_TP)   *
 * 3. VT1, VT7 : VREG1 - (VREG1 - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)   *
 * 4. other VT :    VT - (   VT - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)   *
\**********************************************************************************************/

static unsigned long long calc_gamma_voltage(int VT, int V_nextTP, int OutputGamma,
								const int fraction[])
{
	unsigned long long val1, val2, val3, val4;

	val1 = VT - V_nextTP;

	val2 = (unsigned long long)(fraction[0] + OutputGamma) << FP_SHIFT;
	do_div(val2, fraction[1]);
	val3 = (val1 * val2) >> FP_SHIFT;

	val4 = VT - val3;

	return val4;
}

/*
 *	each TP's gamma voltage calculation (2.3.3)
 */
static void TP_gamma_voltage_calc(struct s6e3fa7_gamma_ctx *ctx, int component)
{
	int i;
	int OutputGamma;
	int MTP_OFFSET;
	int vt_coef;

	/* VT first */
	MTP_OFFSET = ctx->mtp_offsets[VT + component];

	switch(MTP_OFFSET) {
		case 0 ... 9: vt_coef = 12 * MTP_OFFSET ; break;
		case 10 ... 14: vt_coef = 10 * MTP_OFFSET + 38; break;
		case 15: vt_coef = 10 * MTP_OFFSET + 36; break;
	}

	ctx->rgb_out[VT + component] =
		calc_gamma_voltage(VREG0_REF_6P5, 0, vt_coef, fraction[VT]);

	/* V255 */
	MTP_OFFSET = ctx->mtp_offsets[V255 + component];
	OutputGamma = MTP_OFFSET + center_gamma[V255 + component];
	ctx->rgb_out[V255 + component] =
		calc_gamma_voltage(VREG0_REF_6P5, 0, OutputGamma, fraction[V255]);

	/* V203 ~ V11 */
	for (i = V203; i >= V11; i--) {
		MTP_OFFSET = ctx->mtp_offsets[i + component];
		OutputGamma = MTP_OFFSET + center_gamma[i + component];
		ctx->rgb_out[i + component] =
			calc_gamma_voltage(ctx->rgb_out[VT + component], ctx->rgb_out[i + component + 1],
					OutputGamma, fraction[i]);
	}

	/* V7, V1*/
	for (i = V7; i >= V1; i--) {
		MTP_OFFSET = ctx->mtp_offsets[i + component];
		OutputGamma = MTP_OFFSET + center_gamma[i + component];
		ctx->rgb_out[i + component] =
			calc_gamma_voltage(VREG0_REF_6P5, ctx->rgb_out[i + component + 1],
					OutputGamma, fraction[i]);
	}

}



// 1. V255 : ((VREG1 - V255) * denominator_TP / vreg) - numerator_TP
static unsigned long long v255_TP_gamma_code_calc(int vreg, int grayscale, const int fraction[])
{
	unsigned long long val1, val2, val3;

	val1 = vreg - grayscale;
	val2 = val1 * fraction[1];
	do_div(val2, vreg);
	val3 = val2 - fraction[0];

	return val3;
}

// 2. other : (VT    - V_TP)* denominator_TP /(VT    - V_nextTP) - numerator_TP
// 3. V7,V1 : (VREG1 - V_TP)* denominator_TP /(VREG1 - V_nextTP) - numerator_TP
static unsigned long long other_TP_gamma_code_calc(int VT, int grayscale, int nextGRAY, const int fraction[])
{
	signed long long val1, val2, val3, val4;
	int gray_sign = 1, nextgray_sign = 1;

	if (unlikely((VT - grayscale) < 0))
		gray_sign = -1;

	if (unlikely((VT - nextGRAY) < 0))
		nextgray_sign = -1;

	val1 = VT - grayscale;
	val2 = val1 * fraction[1];
	val3 = VT - nextGRAY;
	val2 *= gray_sign;
	val3 *= nextgray_sign;
	do_div(val2, val3);
	val2 *= (gray_sign * nextgray_sign);
	val4 = val2 - fraction[0];
	return (unsigned long long) val4;
}

/* gray scale = V_down + (V_up - V_down) * num / den */
static int gray_scale_calc(int v_up, int v_down, int num, int den)
{
	unsigned long long val1, val2;

	val1 = v_up - v_down;

	val2 = (unsigned long long)(val1 * num) << FP_SHIFT;

	do_div(val2, den);

	val2 >>= FP_SHIFT;

	val2 += v_down;

	return (int)val2;
}

static int generate_gray_scale(struct s6e3fa7_gamma_ctx *ctx, int component)
{
	int V_idx = 0;
	int cnt = 0, cal_cnt = 0;
	int den = 0;

	int *grayscale = &ctx->grayscale[GRAY_SCALE_MAX * component / V_MAX];
	/*
		grayscale OUTPUT VOLTAGE of TP's (V1,V1,V7,V11,V23,V35,V51,V87,V151,V203,V255)
		(V1 is VREG1)
	*/

	grayscale[0] = VREG0_REF_6P5;

	for (cnt = 1; cnt < V_MAX; cnt++) {
		grayscale[inflection_voltages[cnt]] = ctx->rgb_out[cnt + component];
	}

	/*
		ALL grayscale OUTPUT VOLTAGE (0~255)
	*/
	V_idx = V1; // start V of gray scale
	for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {
		if (cnt == inflection_voltages[V_idx]) {
			cal_cnt = 1;
			V_idx++;
		} else {
			den = inflection_voltages[V_idx] - inflection_voltages[V_idx-1];
			grayscale[cnt] = gray_scale_calc(
				grayscale[inflection_voltages[V_idx-1]],
				grayscale[inflection_voltages[V_idx]],
				den - cal_cnt, den);
			cal_cnt++;
		}
	}

	return 0;
}

static int char_to_int(char data1)
{
	int cal_data;

	if (data1 & 0x80) {
		cal_data = data1 & 0x7F;
		cal_data *= -1;
	} else
		cal_data = data1;

	return cal_data;
}

static void mtp_sorting(struct s6e3fa7_gamma_ctx *ctx, char *pfrom)
{
	int i, j, cnt;

	/* V255 */
	ctx->mtp_offsets[V255 + V_R] = pfrom[2] * (pfrom[0] & 0x04 ? -1 : 1);
	ctx->mtp_offsets[V255 + V_G] = pfrom[3] * (pfrom[0] & 0x02 ? -1 : 1);
	ctx->mtp_offsets[V255 + V_B] = pfrom[4] * (pfrom[0] & 0x01 ? -1 : 1);

	/* V203 ~ V1 */
	cnt = 5;
	for (i = V203; i > VT; i--) {
		for (j = 0; j < V_RGB_MAX; j += V_MAX)
			ctx->mtp_offsets[i + j] = char_to_int(pfrom[cnt++]);
	}

	/* VT */
	ctx->mtp_offsets[VT + V_R] = char_to_int((pfrom[0] & 0xF0) >> 4);
	ctx->mtp_offsets[VT + V_G] = char_to_int((pfrom[1] & 0xF0) >> 4);
	ctx->mtp_offsets[VT + V_B] = char_to_int(pfrom[1] & 0x0F);
}

int s6e3fa7_get_gamma(struct s6e3fa7_gamma_ctx *ctx, int table_index, char* output, int output_len)
{
	table_index = clamp(table_index, 0, LUMINANCE_MAX - 1);

	if (output_len != 33)
		return -EINVAL;

	memcpy(output, ctx->generated_gamma[table_index], output_len);
	return 0;
}
EXPORT_SYMBOL(s6e3fa7_get_gamma);

int s6e3fa7_generate_gamma(struct s6e3fa7_gamma_ctx *ctx, int table_index, char* output, int output_len)
{
	const unsigned char *M_GRAY;
	int gamma[V_RGB_MAX];
	int component;
	int i;

	table_index = clamp(table_index, 0, LUMINANCE_MAX - 1);

	if (output_len != 33)
		return -EINVAL;

	M_GRAY = &gamma_offsets[table_index][0];

	if (table_index == LUMINANCE_MAX - 1)
		memcpy(&gamma, &center_gamma, sizeof(gamma));
	else for (component = 0; component < V_RGB_MAX; component += V_MAX) {
		int TP, nextTP;
		int i;
		int *grayscale = &ctx->grayscale[GRAY_SCALE_MAX * component / V_MAX];
		const signed char *comp_offsets;

		/* Generate gamma code */
		// V255
		TP = M_GRAY[V255];
		gamma[V255 + component] = v255_TP_gamma_code_calc(VREG0_REF_6P5,
				grayscale[TP], fraction[V255]);

		// V203 ~ V11
		for (i = V203; i >= V11; i--) {
			TP = M_GRAY[i];
			nextTP = M_GRAY[i+1];
			gamma[i + component] = other_TP_gamma_code_calc(ctx->rgb_out[VT + component],
					grayscale[TP], grayscale[nextTP], fraction[i]);
		}

		// V7, V1
		for (i = V7; i >= V1; i--) {
			TP = M_GRAY[i];
			nextTP = M_GRAY[i+1];
			gamma[i + component] = other_TP_gamma_code_calc(VREG0_REF_6P5, grayscale[TP],
					grayscale[nextTP], fraction[i]);
		}

		// VT
		gamma[VT + component] = 0;

		/* Color Shift (RGB compensation) */
		switch (component) {
			case V_R: comp_offsets = red_offsets[table_index]; break;
			case V_G: comp_offsets = green_offsets[table_index]; break;
			case V_B: comp_offsets = blue_offsets[table_index]; break;
			default: break;
		}

		for (i = 0; i < RGB_COMPENSATION; i++) {
			gamma[V255 - i + component] += comp_offsets[i];
		}

		/* Subtract mtp offset from generated gamma table */

		gamma[V255 + component] = clamp(gamma[V255 + component] - ctx->mtp_offsets[V255 + component], 0, 0xffff);

		for (i = V203; i >= V1; i--) {
			gamma[i + component] = clamp(gamma[i + component] - ctx->mtp_offsets[i + component], 0, 0xff);
		}
	}

	/* VT RGB */
	output[0] = (gamma[VT + V_R] & 0x0f) << 4;
	output[1] = (gamma[VT + V_G] & 0x0f) << 4;
	output[1] |= (gamma[VT + V_B] & 0x0f);

	/* V255 RGB */
	output[0] |= (gamma[V255 + V_R] >> 6 & 4) | (gamma[V255 + V_G] >> 7 & 2) | (gamma[V255 + V_B] >> 8 & 1);
	output[2] = gamma[V255 + V_R] & 0xff;
	output[3] = gamma[V255 + V_G] & 0xff;
	output[5] = gamma[V255 + V_B] & 0xff;

	/* Nothing */
	output[4] = 0;

	/* V203 ~ v1 */
	for (i = V203; i >= V1; i--) {
		output[6 + (V203 - i) * 3] = gamma[i + V_R];
		output[7 + (V203 - i) * 3] = gamma[i + V_G];
		output[8 + (V203 - i) * 3] = gamma[i + V_B];
	}

	return 0;
}
EXPORT_SYMBOL(s6e3fa7_generate_gamma);

struct s6e3fa7_gamma_ctx *s6e3fa7_gamma_init(char* mtp_data, int mtp_len)
{
	int component;
	struct s6e3fa7_gamma_ctx *ctx;
	int i;

	if (mtp_len != 32)
		return ERR_PTR(-EINVAL);

	ctx = kzalloc(sizeof(ctx[0]), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ctx))
		return ctx;

	/********************************************************************************************/
	/* Each TP's gamma voltage calculation							    */
	/* 1. VT, V255 : VREG1 - VREG1 * (OutputGamma + numerator_TP)/(denominator_TP)		    */
	/* 2. VT1, VT7 : VREG1 - (VREG1 - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP) */
	/* 3. other VT : VT - (VT - V_nextTP) * (OutputGamma + numerator_TP)/(denominator_TP)	    */
	/********************************************************************************************/

	mtp_sorting(ctx, mtp_data);

	for (component = 0; component < V_RGB_MAX; component += V_MAX) {
		TP_gamma_voltage_calc(ctx, component);

		/* Gray Output Voltage */
		if (generate_gray_scale(ctx, component)) {
			pr_err(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
			return ERR_PTR(-EINVAL);
		}
	}

	for (i = 0; i < LUMINANCE_MAX; i ++)
		s6e3fa7_generate_gamma(ctx, i, ctx->generated_gamma[i], 33);

	return ctx;
}
EXPORT_SYMBOL(s6e3fa7_gamma_init);

void s6e3fa7_gamma_destroy(struct s6e3fa7_gamma_ctx *ctx) {
		kfree(ctx);
}
EXPORT_SYMBOL(s6e3fa7_gamma_destroy);

MODULE_DESCRIPTION("gamma generator for S6E3FA7_AMS604NL01 panels");
MODULE_LICENSE("GPL v2");
