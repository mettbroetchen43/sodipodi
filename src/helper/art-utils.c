#define __SP_ART_UTILS_C__

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

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_uta_svp.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_uta_vpath.h>
#include <libart_lgpl/art_vpath_svp.h>

#include "art-utils.h"

ArtSVP *
art_svp_translate (const ArtSVP * svp, double dx, double dy)
{
	ArtSVP * new;
	int i, j;

	new = (ArtSVP *) art_alloc (sizeof (ArtSVP) + (svp->n_segs - 1) * sizeof (ArtSVPSeg));

	new->n_segs = svp->n_segs;

	for (i = 0; i < new->n_segs; i++) {
		new->segs[i].n_points = svp->segs[i].n_points;
		new->segs[i].dir = svp->segs[i].dir;
		new->segs[i].bbox.x0 = svp->segs[i].bbox.x0 + dx;
		new->segs[i].bbox.y0 = svp->segs[i].bbox.y0 + dy;
		new->segs[i].bbox.x1 = svp->segs[i].bbox.x1 + dx;
		new->segs[i].bbox.y1 = svp->segs[i].bbox.y1 + dy;
		new->segs[i].points = art_new (ArtPoint, new->segs[i].n_points);

		for (j = 0; j < new->segs[i].n_points; j++) {
			new->segs[i].points[j].x = svp->segs[i].points[j].x + dx;
			new->segs[i].points[j].y = svp->segs[i].points[j].y + dy;
		}
	}

	return new;
}

ArtUta *
art_uta_from_svp_translated (const ArtSVP * svp, double x, double y)
{
	ArtSVP * tsvp;
	ArtUta * uta;

	tsvp = art_svp_translate (svp, x, y);

	uta = art_uta_from_svp (tsvp);

	art_svp_free (tsvp);

	return uta;
}

static void
sp_bpath_segment_bbox_d (double x000, double y000,
			 double x001, double y001,
			 double x011, double y011,
			 double x111, double y111,
			 ArtDRect *bbox,
			 double tolerance)
{
	/* We only check end here to avoid duplication */
	bbox->x0 = MIN (bbox->x0, x111);
	bbox->y0 = MIN (bbox->y0, y111);
	bbox->x1 = MAX (bbox->x1, x111);
	bbox->y1 = MAX (bbox->y1, y111);

	if (((bbox->x0 - x001) > tolerance) ||
	    ((x001 - bbox->x1) > tolerance) ||
	    ((bbox->y0 - y001) > tolerance) ||
	    ((y001 - bbox->y1) > tolerance) ||
	    ((bbox->x0 - x011) > tolerance) ||
	    ((x011 - bbox->x1) > tolerance) ||
	    ((bbox->y0 - y011) > tolerance) ||
	    ((y011 - bbox->y1) > tolerance)) {
		double x00t, x0tt, xttt, x1tt, x11t, x01t;
		double y00t, y0tt, yttt, y1tt, y11t, y01t;
		double s, t;

		t = 0.5;
		s = 1 - t;

		x00t = s * x000 + t * x001;
		x01t = s * x001 + t * x011;
		x11t = s * x011 + t * x111;
		x0tt = s * x00t + t * x01t;
		x1tt = s * x01t + t * x11t;
		xttt = s * x0tt + t * x1tt;

		y00t = s * y000 + t * y001;
		y01t = s * y001 + t * y011;
		y11t = s * y011 + t * y111;
		y0tt = s * y00t + t * y01t;
		y1tt = s * y01t + t * y11t;
		yttt = s * y0tt + t * y1tt;

		sp_bpath_segment_bbox_d (x000, y000, x00t, y00t, x0tt, y0tt, xttt, yttt, bbox, tolerance);
		sp_bpath_segment_bbox_d (xttt, yttt, x1tt, y1tt, x11t, y11t, x111, y111, bbox, tolerance);

	}
}

ArtDRect *
sp_bpath_matrix_d_bbox_d_union (const ArtBpath *bpath, const double *m, ArtDRect *bbox, double tolerance)
{
	double x0, y0, x3, y3;
	const ArtBpath *p;
	ArtDRect b;

	g_return_val_if_fail (bpath != NULL, NULL);
	g_return_val_if_fail (bbox != NULL, NULL);

	x0 = y0 = 0.0;
	x3 = y3 = 0.0;

	b.x0 = b.y0 = 1e18;
	b.x1 = b.y1 = -1e18;

	if (!m) m = NR_MATRIX_D_IDENTITY.c;

	for (p = bpath; p->code != ART_END; p+= 1) {
		switch (p->code) {
		case ART_MOVETO_OPEN:
		case ART_MOVETO:
			x0 = m[0] * p->x3 + m[2] * p->y3 + m[4];
			y0 = m[1] * p->x3 + m[3] * p->y3 + m[5];
			break;
		case ART_LINETO:
			x3 = m[0] * p->x3 + m[2] * p->y3 + m[4];
			y3 = m[1] * p->x3 + m[3] * p->y3 + m[5];
			b.x0 = MIN (b.x0, x0);
			b.x0 = MIN (b.x0, x3);
			b.y0 = MIN (b.y0, y0);
			b.y0 = MIN (b.y0, y3);
			b.x1 = MAX (b.x1, x0);
			b.x1 = MAX (b.x1, x3);
			b.y1 = MAX (b.y1, y0);
			b.y1 = MAX (b.y1, y3);
			x0 = x3;
			y0 = y3;
			break;
		case ART_CURVETO:
			x3 = m[0] * p->x3 + m[2] * p->y3 + m[4];
			y3 = m[1] * p->x3 + m[3] * p->y3 + m[5];
			/* We check start here to avoid duplication */
			b.x0 = MIN (b.x0, x0);
			b.y0 = MIN (b.y0, y0);
			b.x1 = MAX (b.x1, x0);
			b.y1 = MAX (b.y1, y0);
			sp_bpath_segment_bbox_d (x0, y0,
						 m[0] * p->x1 + m[2] * p->y1 + m[4],
						 m[1] * p->x1 + m[3] * p->y1 + m[5],
						 m[0] * p->x2 + m[2] * p->y2 + m[4],
						 m[1] * p->x2 + m[3] * p->y2 + m[5],
						 x3, y3,
						 &b,
						 tolerance);
			x0 = x3;
			y0 = y3;
			break;
		default:
			g_warning ("Corrupted bpath: code %d", p->code);
			break;
		}
	}

	if (!art_drect_empty (&b)) {
		art_drect_union (bbox, bbox, &b);
	}

	return bbox;
}

