#ifndef __NR_PIXBLOCK_LINE_H__
#define __NR_PIXBLOCK_LINE_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-pixblock.h>
#include <libnr/nr-path.h>

void nr_pixblock_draw_line_rgba32 (NRPixBlock *pb, int x0, int y0, int x1, int y1,
				   unsigned int first, unsigned int rgba);

void nr_pixblock_draw_path_rgba32 (NRPixBlock *pb, NRPath *path, NRMatrixF *transform, unsigned int rgba);

#endif
