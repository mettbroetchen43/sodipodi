#ifndef __SP_BEZIER_UTILS_H__
#define __SP_BEZIER_UTILS_H__

/*
 * An Algorithm for Automatically Fitting Digitized Curves
 * by Philip J. Schneider
 * from "Graphics Gems", Academic Press, 1990
 *
 * Authors:
 *   Philip J. Schneider
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1990 Philip J. Schneider
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <glib.h>
#include <libart_lgpl/art_point.h>

/* Bezier approximation utils */

gint sp_bezier_fit_cubic (ArtPoint *bezier, const ArtPoint *data, gint len, gdouble error);

gint sp_bezier_fit_cubic_r (ArtPoint *bezier, const ArtPoint *data, gint len, gdouble error, gint max_depth);

gint sp_bezier_fit_cubic_full (ArtPoint *bezier, const ArtPoint *data, gint len,
			       ArtPoint *tHat1, ArtPoint *tHat2, gdouble error, gint max_depth);


/* Data array */

void	sp_darray_left_tangent		(const ArtPoint *d,
					 gint		first,
					 ArtPoint      *tHat1);

void	sp_darray_right_tangent		(const ArtPoint *d,
					 gint            last,
					 ArtPoint       *tHat2);

void	sp_darray_center_tangent	(const ArtPoint *d,
					 gint            center,
					 ArtPoint       *tHatCenter);



#endif /* __SP_BEZIER_UTILS_H__ */
