#ifndef __NR_BLIT_H__
#define __NR_BLIT_H__

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

void nr_blit_pixblock_pixblock (NRPixBlock *d, NRPixBlock *s);
void nr_blit_pixblock_mask_rgba32 (NRPixBlock *d, NRPixBlock *m, unsigned long rgba32);

#endif
