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

#include <math.h>
#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_bpath.h>

#include <libnr/nr-values.h>

#if 0
#define SP_MATRIX_D_IDENTITY NR_MATRIX_D_IDENTITY.c
#define SP_MATRIX_D_IS_IDENTITY(m) art_affine_equal (m, SP_MATRIX_D_IDENTITY);
#endif

ArtSVP *art_svp_translate (const ArtSVP *svp, double dx, double dy);
ArtUta *art_uta_from_svp_translated (const ArtSVP *svp, double cx, double cy);

ArtDRect *sp_bpath_matrix_d_bbox_d_union (const ArtBpath *bpath, const double *m, ArtDRect *bbox, double tolerance);

#define sp_distance_d_matrix_d_transform(d,m) (d * sqrt (fabs ((m)[0] * (m)[3] - (m)[1] * (m)[2])))

#endif

