#ifndef __NR_VALUES_H__
#define __NR_VALUES_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-types.h>

#define NR_EPSILON_D 1e-18
#define NR_EPSILON_F 1e-18

#define NR_HUGE_D 1e18
#define NR_HUGE_F 1e18
#define NR_HUGE_L (0x7fffffff)
#define NR_HUGE_S (0x7fff)

#ifdef __NR_VALUES_C__
NRMatrixD NR_MATRIX_D_IDENTITY = {{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}};
NRMatrixF NR_MATRIX_F_IDENTITY = {{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}};
NRRectD NR_RECT_D_EMPTY = {NR_HUGE_D, NR_HUGE_D, -NR_HUGE_D, -NR_HUGE_D};
NRRectF NR_RECT_F_EMPTY = {NR_HUGE_F, NR_HUGE_F, -NR_HUGE_F, -NR_HUGE_F};
NRRectL NR_RECT_L_EMPTY = {NR_HUGE_L, NR_HUGE_L, -NR_HUGE_L, -NR_HUGE_L};
NRRectS NR_RECT_S_EMPTY = {NR_HUGE_S, NR_HUGE_S, -NR_HUGE_S, -NR_HUGE_S};
#else
extern NRMatrixD NR_MATRIX_D_IDENTITY;
extern NRMatrixF NR_MATRIX_F_IDENTITY;
extern NRRectD NR_RECT_D_EMPTY;
extern NRRectF NR_RECT_F_EMPTY;
extern NRRectL NR_RECT_L_EMPTY;
extern NRRectS NR_RECT_S_EMPTY;
#endif

#endif
