#define __SP_DESKTOP_SNAP_C__

/*
 * Various snapping methods
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include "sp-guide.h"
#include "sp-namedview.h"
#include "desktop-snap.h"

/* minimal distance to norm before point is considered for snap */
#define MIN_DIST_NORM 1.0

#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))

#define SNAP_ON(d) (((d)->gridsnap > 0.0) || ((d)->guidesnap > 0.0))

/* Snap a point in horizontal and vertical direction */

double
sp_desktop_free_snap (SPDesktop *desktop, NRPointF *req)
{
	double dh, dv;

	dh = sp_desktop_horizontal_snap (desktop, req);
	dv = sp_desktop_vertical_snap (desktop, req);

	if ((dh < 1e18) && (dv < 1e18)) return hypot (dh, dv);
	if (dh < 1e18) return dh;
	if (dv < 1e18) return dv;
	return 1e18;
}

/* Snap a point in horizontal direction */

double
sp_desktop_horizontal_snap (SPDesktop *desktop, NRPointF *req)
{
	SPNamedView * nv;
	NRPointF actual;
	double best = 1e18, dist;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	nv = desktop->namedview;
	actual = *req;
	snapped = FALSE;

	if (nv->snaptoguides) {
		/* snap distance in desktop units */
		best = desktop->guidesnap;
		for (l = nv->vguides; l != NULL; l = l->next) {
			dist = fabs (SP_GUIDE (l->data)->position - req->x);
			if (dist < best) {
				best = dist;
				actual.x = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	}

	if (nv->snaptogrid) {
		gdouble p;
		if (best == 1e18) best = desktop->gridsnap;
		p = nv->gridoriginx + floor ((req->x - nv->gridoriginx) / nv->gridspacingx) * nv->gridspacingx;
		if (fabs (req->x - p) < best) {
			best = fabs (req->x - p);
			actual.x = p;
			snapped = TRUE;
		}
		if (fabs (nv->gridspacingx - (req->x - p)) < best) {
			best = fabs (nv->gridspacingx - (req->x - p));
			actual.x = nv->gridspacingx + p;
			snapped = TRUE;
		}
	}

	dist = (snapped) ? fabs (actual.x - req->x) : 1e18;

	*req = actual;

	return dist;
}

/* snap a point in vertical direction */

double
sp_desktop_vertical_snap (SPDesktop *desktop, NRPointF *req)
{
	SPNamedView * nv;
	NRPointF actual;
	gdouble best = 1e18, dist;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	nv = desktop->namedview;
	actual = *req;
	snapped = FALSE;

	if (nv->snaptoguides) {
		/* snap distance in desktop units */
		best = desktop->guidesnap;
		for (l = nv->hguides; l != NULL; l = l->next) {
			dist = fabs (SP_GUIDE (l->data)->position - req->y);
			if (dist < best) {
				best = dist;
				actual.y = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	}

	if (nv->snaptogrid) {
		gdouble p;
		if (best == 1e18) best = desktop->gridsnap;
		p = nv->gridoriginy + floor ((req->y - nv->gridoriginy) / nv->gridspacingy) * nv->gridspacingy;
		if (fabs (req->y - p) < best) {
			best = fabs (req->y - p);
			actual.y = p;
			snapped = TRUE;
		}
		if (fabs (nv->gridspacingy - (req->y - p)) < best) {
			best = fabs (nv->gridspacingy - (req->y - p));
			actual.y = nv->gridspacingy + p;
			snapped = TRUE;
		}
	}

	dist = (snapped) ? fabs (actual.x - req->x) : 1e18;

	*req = actual;

	return dist;
}

/* Look for snappoint along a line given by req and the vector (dx,dy) */

double
sp_desktop_vector_snap (SPDesktop * desktop, NRPointF *req, double dx, double dy)
{
	SPNamedView * nv;
	NRPointF actual;
	double len, best = 1e18, dist, delta;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	len = hypot (dx, dy);
	if (len < 1e-18) sp_desktop_free_snap (desktop, req);
	dx /= len;
	dy /= len;

	nv = desktop->namedview;
	snapped = FALSE;
	actual = *req;

	if (nv->snaptoguides) {
	  best = desktop->guidesnap;
	  if (fabs (dx) > 1e-18) {
		/* Test horizontal snapping */
		for (l = nv->vguides; l != NULL; l = l->next) {
			delta = SP_GUIDE (l->data)->position - req->x;
			dist = hypot (delta, delta * dy / dx);
			if (dist < best) {
				best = dist;
				actual.x = SP_GUIDE (l->data)->position;
				actual.y = req->y + delta * dy / dx;
				snapped = TRUE;
			}
		}
	  }

	  if (fabs (dy) > 1e-18) {
		/* Test vertical snapping */
		for (l = nv->hguides; l != NULL; l = l->next) {
			delta = SP_GUIDE (l->data)->position - req->y;
			dist = hypot (delta, delta * dx / dy);
			if (dist < best) {
				best = dist;
				actual.x = req->x + delta * dx / dy;
				actual.y = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	  }
	}

	if (nv->snaptogrid) {
		gdouble p1, p2;
		if (best == 1e18) best = desktop->gridsnap;

		if (fabs (dx) > 1e-15) {
		  p1 = nv->gridoriginx + floor ((req->x - nv->gridoriginx) / nv->gridspacingx) * nv->gridspacingx;
		  p2 = p1 + nv->gridspacingx;

		  delta = p1 - req->x;
		  dist = hypot (delta, delta * dy / dx);
		  if (dist < best) {
		       best = dist;
		       actual.x = p1;
		       actual.y = req->y + delta * dy / dx;
		       snapped = TRUE;
		  }
		  delta = p2 - req->x;
		  dist = hypot (delta, delta * dy / dx);
		  if (dist < best) {
		       best = dist;
		       actual.x = p2;
		       actual.y = req->x + delta * dy / dx;
		       snapped = TRUE;
		  }
		}

		if (fabs (dy) > 1e-15) { 
		  p1 = nv->gridoriginy + floor ((req->y - nv->gridoriginy) / nv->gridspacingy) * nv->gridspacingy;
		  p2 = p1 + nv->gridspacingy;

		  delta = p1 - req->y;
		  dist = hypot (delta, delta * dx / dy);
		  if (dist < best) {
		       best = dist;
		       actual.x = req->x + delta * dx / dy;
		       actual.y = p1;
		       snapped = TRUE;
		  }
		  delta = p2 - req->y;
		  dist = hypot (delta, delta * dx / dy);
		  if (dist < best) {
		       best = dist;
		       actual.x = req->x + delta * dx / dy;
		       actual.y = p2;
		       snapped = TRUE;
		  }
		}
	}

	* req = actual;

	if (snapped) return best;

	return 1e18;
}

/* look for snappoint on a circle given by center (cx,cy) and distance center-req) */

double
sp_desktop_circular_snap (SPDesktop * desktop, NRPointF * req, double cx, double cy)
{
	SPNamedView * nv;
	NRPointF actual;
	gdouble best = 1e18, dist, h, dx, dy;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	nv = desktop->namedview;
	actual = *req;
	snapped = FALSE;

	h = (req->x - cx)*(req->x - cx) + (req->y - cy)*(req->y - cy); // h is sqare of hypotenuse
	if (h < 1e-15) return 1e18;

	if (nv->snaptoguides) {
		/* snap distance in desktop units */
		best = desktop->guidesnap;
		best *= best; // best is sqare of best distance 
		// vertical guides
		for (l = nv->vguides; l != NULL; l = l->next) {
		  dx = fabs(SP_GUIDE (l->data)->position - cx);
		  if (dx * dx <= h) {
		    dy = sqrt (h - dx * dx);
		    // down
		    dist = (req->x - SP_GUIDE (l->data)->position) * (req->x - SP_GUIDE (l->data)->position) + 
		           (req->y - (cy - dy)) * (req->y - (cy - dy));
		    if (dist < best) {
		      best = dist;
		      actual.x = SP_GUIDE (l->data)->position;
		      actual.y = cy - dy;
		      snapped = TRUE;
		    }
		    // up
		    dist = (req->x - SP_GUIDE (l->data)->position) * (req->x - SP_GUIDE (l->data)->position) + 
		           (req->y - (cy + dy)) * (req->y - (cy + dy));
		    if (dist < best) {
		      best = dist;
		      actual.x = SP_GUIDE (l->data)->position;
		      actual.y = cy + dy;
		      snapped = TRUE;
		    }
		    
		  }
		} // vertical guides
		// horizontal guides
		for (l = nv->hguides; l != NULL; l = l->next) {
		  dy = fabs(SP_GUIDE (l->data)->position - cy);
		  if (dy * dy <= h) {
		    dx = sqrt (h - dy * dy);
		    // down
		    dist = (req->y - SP_GUIDE (l->data)->position) * (req->y - SP_GUIDE (l->data)->position) + 
		           (req->x - (cx - dx)) * (req->x - (cx - dx));
		    if (dist < best) {
		      best = dist;
		      actual.y = SP_GUIDE (l->data)->position;
		      actual.x = cx - dx;
		      snapped = TRUE;
		    }
		    // up
		    dist = (req->y - SP_GUIDE (l->data)->position) * (req->y - SP_GUIDE (l->data)->position) + 
		           (req->x - (cx + dx)) * (req->x - (cx + dx));
		    if (dist < best) {
		      best = dist;
		      actual.y = SP_GUIDE (l->data)->position;
		      actual.x = cx + dx;
		      snapped = TRUE;
		    }
		    
		  }
		} // horizontal guides
	} // snaptoguides

	if (nv->snaptogrid) {
	        gdouble p1, p2;
		/* snap distance in desktop units */
		if (best == 1e18) {
		  best = desktop->gridsnap;
		  best *= best; // best is sqare of best distance 
		}
		// horizontal gridlines
       		p1 = nv->gridoriginx + floor ((req->x - nv->gridoriginx) / nv->gridspacingx) * nv->gridspacingx;
		p2 = p1 + nv->gridspacingx;
		// lower gridline
		dx = fabs(p1 - cx);
		if (dx * dx <= h) {
		  dy = sqrt (h - dx * dx);
		  // down
		  dist = (req->x - p1) * (req->x - p1) + (req->y - (cy - dy)) * (req->y - (cy - dy));
		  if (dist < best) {
		    best = dist;
		    actual.x = p1;
		    actual.y = cy - dy;
		    snapped = TRUE;
		  }
		  // up
		  dist = (req->x - p1) * (req->x - p1) + (req->y - (cy + dy)) * (req->y - (cy + dy));
		  if (dist < best) {
		    best = dist;
		    actual.x = p1;
		    actual.y = cy + dy;
		    snapped = TRUE;
		  }
		}
		// upper gridline
		dx = fabs(p2 - cx);
		if (dx * dx <= h) {
		  dy = sqrt (h - dx * dx);
		  // down
		  dist = (req->x - p2) * (req->x - p2) + (req->y - (cy - dy)) * (req->y - (cy - dy));
		  if (dist < best) {
		    best = dist;
		    actual.x = p2;
		    actual.y = cy - dy;
		    snapped = TRUE;
		  }
		  // up
		  dist = (req->x - p2) * (req->x - p2) + (req->y - (cy + dy)) * (req->y - (cy + dy));
		  if (dist < best) {
		    best = dist;
		    actual.x = p2;
		    actual.y = cy + dy;
		    snapped = TRUE;
		  }
		}
		
		// vertical gridline
		p1 = nv->gridoriginy + floor ((req->y - nv->gridoriginy) / nv->gridspacingy) * nv->gridspacingy;
		p2 = p1 + nv->gridspacingy;
		//lower gridline
		dy = fabs(p1 - cy);
		if (dy * dy <= h) {
		  dx = sqrt (h - dy * dy);
		  // down
		  dist = (req->y - p1) * (req->y - p1) + 
		         (req->x - (cx - dx)) * (req->x - (cx - dx));
		  if (dist < best) {
		    best = dist;
		    actual.y = p1;
		    actual.x = cx - dx;
		    snapped = TRUE;
		  }
		  // up
		  dist = (req->y - p1) * (req->y - p1) + 
		         (req->x - (cx + dx)) * (req->x - (cx + dx));
		  if (dist < best) {
		    best = dist;
		    actual.y = p1;
		    actual.x = cx + dx;
		    snapped = TRUE;
		  }
		}
		//lower gridline
		dy = fabs(p2 - cy);
		if (dy * dy <= h) {
		  dx = sqrt (h - dy * dy);
		  // down
		  dist = (req->y - p2) * (req->y - p2) + 
		         (req->x - (cx - dx)) * (req->x - (cx - dx));
		  if (dist < best) {
		    best = dist;
		    actual.y = p2;
		    actual.x = cx - dx;
		    snapped = TRUE;
		  }
		  // up
		  dist = (req->y - p2) * (req->y - p2) + 
		         (req->x - (cx + dx)) * (req->x - (cx + dx));
		  if (dist < best) {
		    best = dist;
		    actual.y = p2;
		    actual.x = cx + dx;
		    snapped = TRUE;
		  }
		}
	} // snaptogrid
	
	dist = (snapped) ? best : 1e18;

	*req = actual;

	return dist;
}


/* 
 * functions for lists of points
 *
 * All functions take a list of NRPointF and parameter indicating the proposed transformation.
 * They return the upated transformation parameter. 
 */

double
sp_desktop_horizontal_snap_list (SPDesktop *desktop, NRPointF *p, int length, double dx)
{
	NRPointF q;
	double xdist, xpre, dist, d;
	int i;

	if (!SNAP_ON (desktop)) return dx;

	dist = NR_HUGE_F;
	xdist = dx;

	for (i = 0; i < length; i++) {
		q = p[i];
		xpre = q.x;
		q.x += dx;
		d = sp_desktop_horizontal_snap (desktop, &q);
		if (d < dist) {
			xdist = q.x - xpre;
			dist = d;
		}
	}

	return xdist;
}

double
sp_desktop_vertical_snap_list (SPDesktop *desktop, NRPointF *p, int length, double dy)
{
	NRPointF q;
	double ydist, ypre, dist, d;
	int i;

	if (!SNAP_ON (desktop)) return dy;

	dist = NR_HUGE_F;
	ydist = dy;

	for (i = 0; i < length; i++) {
		q = p[i];
		ypre = q.y;
		q.y += dy;
		d = sp_desktop_vertical_snap (desktop, &q);
		if (d < dist) {
			ydist = q.y - ypre;
			dist = d;
		}
	}

	return ydist;
}

double
sp_desktop_horizontal_snap_list_scale (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, double sx)
{
	NRPointF q, check;
	double xscale, xdist, d;
	int i;

	if (!SNAP_ON (desktop)) return sx;

	xdist = NR_HUGE_F;
	xscale = sx;

	for (i = 0; i < length; i++) {
		q = p[i];
		check.x = sx * (q.x - norm->x) + norm->x;
		if (fabs (q.x - norm->x) > MIN_DIST_NORM) {
			d = sp_desktop_horizontal_snap (desktop, &check);
			if ((d < 1e18) && (d < fabs (xdist))) {
				xdist = d;
				xscale = (check.x - norm->x) / (q.x - norm->x);
			}
		}
	}

	return xscale;
}

double
sp_desktop_vertical_snap_list_scale (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, double sy)
{
	NRPointF q, check;
	double yscale, ydist, d;
	int i;

	if (!SNAP_ON (desktop)) return sy;

	ydist = NR_HUGE_F;
	yscale = sy;

	for (i = 0; i < length; i++) {
		q = p[i];
		check.y = sy * (q.y - norm->y) + norm->y;
		if (fabs (q.y - norm->y) > MIN_DIST_NORM) {
			d = sp_desktop_vertical_snap (desktop, &check);
			if ((d < 1e18) && (d < fabs (ydist))) {
				ydist = d;
				yscale = (check.y - norm->y)/(q.y - norm->y);
			}
		}
	}

	return yscale;
}

double
sp_desktop_vector_snap_list (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, double sx, double sy)
{
	NRPointF q, check;
	double dist, d, ratio;
	int i;

	if (!SNAP_ON (desktop)) return sx;

	dist = NR_HUGE_F;
	ratio = fabs (sx);

	for (i = 0; i < length; i++) {
		q = p[i];
		check.x = (q.x - norm->x) * sx + norm->x;
		check.y = (q.y - norm->y) * sy + norm->y;
		if ((fabs (q.y - norm->y) > MIN_DIST_NORM) || (fabs (q.y - norm->y) > MIN_DIST_NORM)) {
			d = sp_desktop_vector_snap (desktop, &check, check.x - norm->x, check.y - norm->y);
			if ((d < 1e18) && (d < dist)) {
				dist = d;
				ratio = (fabs(q.x - norm->x) > fabs(q.y - norm->y)) ? 
					(check.x - norm->x) / (q.x - norm->x) : 
					(check.y - norm->y) / (q.y - norm->y); 
			}
		}
	}
  
	return ratio;
}

double
sp_desktop_horizontal_snap_list_skew (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, double sx)
{
	NRPointF q, check;
	double xskew, xdist, d;
	int i;

	if (!SNAP_ON (desktop)) return sx;

	xdist = NR_HUGE_F;
	xskew = sx;

	for (i = 0; i < length; i++) {
		q = p[i];
		check.x = sx * (q.y - norm->y) + q.x;
		if (fabs (q.y - norm->y) > MIN_DIST_NORM) {
			d = sp_desktop_horizontal_snap (desktop, &check);
			if ((d < 1e18) && (d < fabs (xdist))) {
				xdist = d;
				xskew = (check.x - q.x) / (q.y - norm->y);
			}
		}
	}

	return xskew;
}

double
sp_desktop_vertical_snap_list_skew (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, double sy)
{
	NRPointF q, check;
	gdouble yskew, ydist, d;
	int i;

	if (!SNAP_ON (desktop)) return sy;

	ydist = NR_HUGE_F;
	yskew = sy;

	for (i = 0; i < length; i++) {
		q = p[i];
		check.y = sy * (q.x - norm->x) + q.y;
		if (fabs (q.x - norm->x) > MIN_DIST_NORM) {
			d = sp_desktop_vertical_snap (desktop, &check);
			if ((d < 1e18) && (d < fabs (ydist))) {
				ydist = d;
				yskew = (check.y - q.y)/(q.x - norm->x);
			}
		}
	}

	return yskew;
}

/* 
   this function takes the whole transformation matrix as parameter
   working with angles would be too complex
*/
NRMatrixF *
sp_desktop_circular_snap_list (SPDesktop *desktop, NRPointF *p, int length, NRPointF *norm, NRMatrixF *rotate)
{
	NRPointF q1, q2, q, check;
	gdouble d, best, h1, h2;
	int i;

	if (!SNAP_ON (desktop)) return rotate;

	best = NR_HUGE_F;

	for (i = 0; i < length; i++) {
		q = p[i];
		check.x = NR_MATRIX_DF_TRANSFORM_X (rotate, q.x, q.y);
		check.y = NR_MATRIX_DF_TRANSFORM_Y (rotate, q.x, q.y);
		d = sp_desktop_circular_snap (desktop, &check, norm->x, norm->y);
		if (d < best) {
			q1 = q;
			q2 = check;
			best = d;
		}
	}

	// compute the new transformation (rotation) from the snapped point
	if (best < 1e18) {
		NRMatrixF r1, r2, p2n, n2p;

		h1 = hypot (q1.x - norm->x, q1.y - norm->y);
		q1.x = (q1.x - norm->x) / h1;
		q1.y = (q1.y - norm->y) / h1;
		h2 = hypot (q2.x - norm->x, q2.y - norm->y);
		q2.x = (q2.x - norm->x) / h2;
		q2.y = (q2.y - norm->y) / h2;
		r1.c[0] = q1.x;  r1.c[1] = -q1.y;  r1.c[2] =  q1.y;  r1.c[3] = q1.x;  r1.c[4] = 0;  r1.c[5] = 0;
		r2.c[0] = q2.x;  r2.c[1] =  q2.y;  r2.c[2] = -q2.y;  r2.c[3] = q2.x;  r2.c[4] = 0;  r2.c[5] = 0;

		nr_matrix_f_set_translate (&n2p, norm->x, norm->y);
		nr_matrix_f_invert (&p2n, &n2p);
		nr_matrix_multiply_fff (rotate, &p2n, &r1);
		nr_matrix_multiply_fff (rotate, rotate, &r2);
		nr_matrix_multiply_fff (rotate, rotate, &n2p);
	}

	return rotate;
}
