#ifndef __SP_ART_UTILS_H__
#define __SP_ART_UTILS_H__

/*
 * Libart-related convenience methods
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-types.h>

#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>

ArtSVP *art_svp_translate (const ArtSVP *svp, double dx, double dy);

ArtVpath *sp_vpath_from_bpath_transform_closepath (const ArtBpath *bpath, NRMatrixF *transform, int close, int perturb, double flatness);

#endif

