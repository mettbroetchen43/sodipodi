#ifndef __NR_PIXBLOCK_LINE_H__
#define __NR_PIXBLOCK_LINE_H__

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

#include <libnr/nr-pixblock.h>

void nr_pixblock_draw_line_rgba32 (NRPixBlock *d, long x0, long y0, long x1, long y1, short first, unsigned long rgba);

#endif
