#define __NR_PATH_C__

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

#include "nr-path.h"

NRBPath *
nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform)
{
	int i;

	if (!s->path) {
		d->path = NULL;
		return;
	}

	i = 0;
	while (s->path[i].code != ART_END) i += 1;

	d->path = nr_new (ArtBpath, i + 1);

	i = 0;
	while (s->path[i].code != ART_END) {
		d->path[i].code = s->path[i].code;
		if (s->path[i].code == ART_CURVETO) {
			d->path[i].x1 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].x2 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x2, s->path[i].y2);
			d->path[i].y2 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x2, s->path[i].y2);
		}
		d->path[i].x3 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x3, s->path[i].y3);
		d->path[i].y3 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x3, s->path[i].y3);
		i += 1;
	}
	d->path[i].code = ART_END;

	return d;
}

static void
nr_line_wind_distance (double x0, double y0, double x1, double y1, NRPointF *pt, int *wind, float *best)
{
	double Ax, Ay, Bx, By, Dx, Dy, Px, Py, s;
	double dist2;

	/* Find distance */
	Ax = x0;
	Ay = y0;
	Bx = x1;
	By = y1;
	Dx = x1 - x0;
	Dy = y1 - y0;
	Px = pt->x;
	Py = pt->y;

	if (best) {
		s = ((Px - Ax) * Dx + (Py - Ay) * Dy) / (Dx * Dx + Dy * Dy);
		if (s <= 0.0) {
			dist2 = (Px - Ax) * (Px - Ax) + (Py - Ay) * (Py - Ay);
		} else if (s >= 1.0) {
			dist2 = (Px - Bx) * (Px - Bx) + (Py - By) * (Py - By);
		} else {
			double Qx, Qy;
			Qx = Ax + s * Dx;
			Qy = Ay + s * Dy;
			dist2 = (Px - Qx) * (Px - Qx) + (Py - Qy) * (Py - Qy);
		}

		if (dist2 < (*best * *best)) *best = sqrt (dist2);
	}

	if (wind) {
		/* Find wind */
		if ((Ax >= Px) && (Bx >= Px)) return;
		if ((Ay >= Py) && (By >= Py)) return;
		if ((Ay < Py) && (By < Py)) return;
		if (Ay == By) return;
		/* Ctach upper y bound */
		if (Ay == Py) {
			if (Ax < Px) *wind -= 1;
			return;
		} else if (By == Py) {
			if (Bx < Px) *wind += 1;
			return;
		} else {
			double Qx;
			/* Have to calculate intersection */
			Qx = Ax + Dx * (Py - Ay) / Dy;
			if (Qx < Px) {
				*wind += (Dy > 0.0) ? 1 : -1;
			}
		}
	}
}

static void
nr_curve_bbox_wind_distance (double x000, double y000,
			     double x001, double y001,
			     double x011, double y011,
			     double x111, double y111,
			     NRPointF *pt,
			     NRRectF *bbox, int *wind, float *best,
			     float tolerance)
{
	double x0, y0, x1, y1, len2;
	double Px, Py;
	int needdist, needwind, needbbox, needline;

	Px = pt->x;
	Py = pt->y;

	needdist = 0;
	needwind = 0;
	needbbox = 0;
	needline = 0;

	x0 = MIN (x000, x001);
	x0 = MIN (x0, x011);
	x0 = MIN (x0, x111);
	y0 = MIN (y000, y001);
	y0 = MIN (y0, y011);
	y0 = MIN (y0, y111);
	x1 = MAX (x000, x001);
	x1 = MAX (x1, x011);
	x1 = MAX (x1, x111);
	y1 = MAX (y000, y001);
	y1 = MAX (y1, y011);
	y1 = MAX (y1, y111);

	if (best) {
		/* Quicly adjust to endpoints */
		len2 = (x000 - Px) * (x000 - Px) + (y000 - Py) * (y000 - Py);
		if (len2 < (*best * *best)) *best = sqrt (len2);
		len2 = (x111 - Px) * (x111 - Px) + (y111 - Py) * (y111 - Py);
		if (len2 < (*best * *best)) *best = sqrt (len2);

		if (((x0 - Px) < *best) && ((y0 - Py) < *best) && ((Px - x1) < *best) && ((Py - y1) < *best)) {
			/* Point is inside sloppy bbox */
			/* Now we have to decide, whether subdivide */
			/* fixme: (Lauris) */
			if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
				needdist = 1;
			} else {
				needline = 1;
			}
		}
	}
	if (!needdist && wind) {
		if ((y1 >= Py) && (y0 < Py) && (x0 < Px)) {
			/* Possible intersection at the left */
			/* Now we have to decide, whether subdivide */
			/* fixme: (Lauris) */
			if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
				needwind = 1;
			} else {
				needline = 1;
			}
		}
	}

	if (bbox) {
		bbox->x0 = MIN (bbox->x0, x111);
		bbox->y0 = MIN (bbox->y0, y111);
		bbox->x1 = MAX (bbox->x1, x111);
		bbox->y1 = MAX (bbox->y1, y111);
		if (!needdist && !needwind) {
			if (((bbox->x0 - x001) > tolerance) ||
			    ((x001 - bbox->x1) > tolerance) ||
			    ((bbox->y0 - y001) > tolerance) ||
			    ((y001 - bbox->y1) > tolerance) ||
			    ((bbox->x0 - x011) > tolerance) ||
			    ((x011 - bbox->x1) > tolerance) ||
			    ((bbox->y0 - y011) > tolerance) ||
			    ((y011 - bbox->y1) > tolerance)) {
				needbbox = 1;
			}
		}
	}

	if (needdist || needwind || needbbox) {
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

		nr_curve_bbox_wind_distance (x000, y000, x00t, y00t, x0tt, y0tt, xttt, yttt, pt, bbox, wind, best, tolerance);
		nr_curve_bbox_wind_distance (xttt, yttt, x1tt, y1tt, x11t, y11t, x111, y111, pt, bbox, wind, best, tolerance);
	} else if (1 || needline) {
		nr_line_wind_distance (x000, y000, x111, y111, pt, wind, best);
	}
}

void
nr_path_matrix_f_point_f_bbox_wind_distance (NRBPath *bpath, NRMatrixF *m, NRPointF *pt,
					     NRRectF *bbox, int *wind, float *dist,
					     float tolerance)
{
	double x0, y0, x3, y3;
	const ArtBpath *p;

	if (!bpath->path) {
		if (wind) *wind = 0;
		if (dist) *dist = NR_HUGE_F;
		return;
	}

	if (!m) m = &NR_MATRIX_F_IDENTITY;

	x0 = y0 = 0.0;
	x3 = y3 = 0.0;

	for (p = bpath->path; p->code != ART_END; p+= 1) {
		switch (p->code) {
		case ART_MOVETO_OPEN:
		case ART_MOVETO:
			x0 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y0 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			if (bbox) {
				bbox->x0 = MIN (bbox->x0, x0);
				bbox->y0 = MIN (bbox->y0, y0);
				bbox->x1 = MAX (bbox->x1, x0);
				bbox->y1 = MAX (bbox->y1, y0);
			}
			break;
		case ART_LINETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			if (bbox) {
				bbox->x0 = MIN (bbox->x0, x3);
				bbox->y0 = MIN (bbox->y0, y3);
				bbox->x1 = MAX (bbox->x1, x3);
				bbox->y1 = MAX (bbox->y1, y3);
			}
			if (dist || wind) {
				nr_line_wind_distance (x0, y0, x3, y3, pt, wind, dist);
			}
			x0 = x3;
			y0 = y3;
			break;
		case ART_CURVETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			nr_curve_bbox_wind_distance (x0, y0,
						     m->c[0] * p->x1 + m->c[2] * p->y1 + m->c[4],
						     m->c[1] * p->x1 + m->c[3] * p->y1 + m->c[5],
						     m->c[0] * p->x2 + m->c[2] * p->y2 + m->c[4],
						     m->c[1] * p->x2 + m->c[3] * p->y2 + m->c[5],
						     x3, y3,
						     pt,
						     bbox, wind, dist, tolerance);
			x0 = x3;
			y0 = y3;
			break;
		default:
			break;
		}
	}
}

/* Fast bbox calculation */

static void
nr_curve_bbox (double x000, double y000,
	       double x001, double y001,
	       double x011, double y011,
	       double x111, double y111,
	       NRRectF *bbox,
	       float tolerance)
{
	bbox->x0 = MIN (bbox->x0, x111);
	bbox->y0 = MIN (bbox->y0, y111);
	bbox->x1 = MAX (bbox->x1, x111);
	bbox->y1 = MAX (bbox->y1, y111);

	if (((bbox->x0 - tolerance) > x001) ||
	    ((bbox->x0 - tolerance) > x011) ||
	    ((bbox->x1 + tolerance) < x001) ||
	    ((bbox->x1 + tolerance) < x011) ||
	    ((bbox->y0 - tolerance) > y001) ||
	    ((bbox->y0 - tolerance) > y011) ||
	    ((bbox->y1 + tolerance) < y001) ||
	    ((bbox->y1 + tolerance) < y011)) {
		double x00t, x0tt, xttt, x1tt, x11t, x01t;
		double y00t, y0tt, yttt, y1tt, y11t, y01t;

		/*
		 * t = 0.5;
		 * s = 1 - t;

		 * x00t = s * x000 + t * x001;
		 * x01t = s * x001 + t * x011;
		 * x11t = s * x011 + t * x111;
		 * x0tt = s * x00t + t * x01t;
		 * x1tt = s * x01t + t * x11t;
		 * xttt = s * x0tt + t * x1tt;
		 *
		 * y00t = s * y000 + t * y001;
		 * y01t = s * y001 + t * y011;
		 * y11t = s * y011 + t * y111;
		 * y0tt = s * y00t + t * y01t;
		 * y1tt = s * y01t + t * y11t;
		 * yttt = s * y0tt + t * y1tt;
		 */

		x00t = 0.5 * (x000 + x001);
		x01t = 0.5 * (x001 + x011);
		x11t = 0.5 * (x011 + x111);
		x0tt = 0.5 * (x00t + x01t);
		x1tt = 0.5 * (x01t + x11t);
		xttt = 0.5 * (x0tt + x1tt);

		y00t = 0.5 * (y000 + y001);
		y01t = 0.5 * (y001 + y011);
		y11t = 0.5 * (y011 + y111);
		y0tt = 0.5 * (y00t + y01t);
		y1tt = 0.5 * (y01t + y11t);
		yttt = 0.5 * (y0tt + y1tt);

		nr_curve_bbox (x000, y000, x00t, y00t, x0tt, y0tt, xttt, yttt, bbox, tolerance);
		nr_curve_bbox (xttt, yttt, x1tt, y1tt, x11t, y11t, x111, y111, bbox, tolerance);
	}
}

void
nr_path_matrix_f_bbox_f_union (NRBPath *bpath, NRMatrixF *m,
			       NRRectF *bbox,
			       float tolerance)
{
	double x0, y0, x3, y3;
	const ArtBpath *p;

	if (!bpath->path) return;

	if (!m) m = &NR_MATRIX_F_IDENTITY;

	x0 = y0 = 0.0;
	x3 = y3 = 0.0;

	for (p = bpath->path; p->code != ART_END; p+= 1) {
		switch (p->code) {
		case ART_MOVETO_OPEN:
		case ART_MOVETO:
			x0 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y0 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			bbox->x0 = MIN (bbox->x0, x0);
			bbox->y0 = MIN (bbox->y0, y0);
			bbox->x1 = MAX (bbox->x1, x0);
			bbox->y1 = MAX (bbox->y1, y0);
			break;
		case ART_LINETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			bbox->x0 = MIN (bbox->x0, x3);
			bbox->y0 = MIN (bbox->y0, y3);
			bbox->x1 = MAX (bbox->x1, x3);
			bbox->y1 = MAX (bbox->y1, y3);
			x0 = x3;
			y0 = y3;
			break;
		case ART_CURVETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			nr_curve_bbox (x0, y0,
				       m->c[0] * p->x1 + m->c[2] * p->y1 + m->c[4],
				       m->c[1] * p->x1 + m->c[3] * p->y1 + m->c[5],
				       m->c[0] * p->x2 + m->c[2] * p->y2 + m->c[4],
				       m->c[1] * p->x2 + m->c[3] * p->y2 + m->c[5],
				       x3, y3,
				       bbox,
				       tolerance);
			x0 = x3;
			y0 = y3;
			break;
		default:
			break;
		}
	}
}

