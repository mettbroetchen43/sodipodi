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

#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_bpath.h>

#ifdef __SP_ART_UTILS_C__
double SP_MATRIX_D_IDENTITY[] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
#else
extern double SP_MATRIX_D_IDENTITY[];
#endif

#define SP_MATRIX_D_IS_IDENTITY(m) art_affine_equal (m, SP_MATRIX_D_IDENTITY);

ArtSVP *art_svp_translate (const ArtSVP * svp, double dx, double dy);
ArtUta *art_uta_from_svp_translated (const ArtSVP * svp, double cx, double cy);

ArtDRect *sp_bpath_matrix_d_bbox_d_union (const ArtBpath *bpath, const gdouble *m, ArtDRect *bbox, gdouble tolerance);

#endif

