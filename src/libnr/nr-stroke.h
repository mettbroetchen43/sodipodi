#ifndef __NR_STROKE_H__
#define __NR_STROKE_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-path.h>
#include <libnr/nr-svp.h>

NRSVL *nr_bpath_stroke (const NRBPath *path, NRMatrixF *transform, float width, float flatness);

#endif
