#ifndef __NR_PIXBLOCK_PATTERN_H__
#define __NR_PIXBLOCK_PATTERN_H__

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

void nr_pixblock_render_gray_noise (NRPixBlock *pb, NRPixBlock *mask);

#endif
