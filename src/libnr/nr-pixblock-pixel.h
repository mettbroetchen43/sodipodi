#ifndef __NR_PIXBLOCK_PIXEL_H__
#define __NR_PIXBLOCK_PIXEL_H__

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

void nr_compose_pixblock_pixblock_pixel (NRPixBlock *dpb, unsigned char *d, const NRPixBlock *spb, const unsigned char *s);

#endif
