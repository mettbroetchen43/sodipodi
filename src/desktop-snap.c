#define SP_DESKTOP_SNAP_C

#include <math.h>
#include "sp-guide.h"
#include "sp-namedview.h"
#include "desktop-snap.h"

// minimal distance to norm before point is considered for snap
#define PNDist 1

#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))

#define SNAP_ON(d) (((d)->gridsnap > 0.0) || ((d)->guidesnap > 0.0))

gdouble
sp_desktop_free_snap (SPDesktop * desktop, ArtPoint * req)
/* snap a point in horizontal and vertical direction */
{
	gdouble dh, dv;

	dh = sp_desktop_horizontal_snap (desktop, req);
	dv = sp_desktop_vertical_snap (desktop, req);

	if ((dh < 1e18) && (dv < 1e18)) return hypot (dh, dv);
	if (dh < 1e18) return dh;
	if (dv < 1e18) return dv;
	return 1e18;
}

/* snap a point in horizontal direction */

gdouble
sp_desktop_horizontal_snap (SPDesktop * desktop, ArtPoint * req)
{
	SPNamedView * nv;
	ArtPoint actual;
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
		for (l = nv->vguides; l != NULL; l = l->next) {
			if (fabs (SP_GUIDE (l->data)->position - req->x) < best) {
				best = fabs (SP_GUIDE (l->data)->position - req->x);
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

gdouble
sp_desktop_vertical_snap (SPDesktop * desktop, ArtPoint * req)
{
	SPNamedView * nv;
	ArtPoint actual;
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
			if (fabs (SP_GUIDE (l->data)->position - req->y) < best) {
				best = fabs (SP_GUIDE (l->data)->position - req->y);
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

/* look for snappoint along a line given by req and the vector (dx,dy) */

gdouble
sp_desktop_vector_snap (SPDesktop * desktop, ArtPoint * req, gdouble dx, gdouble dy)
{
	SPNamedView * nv;
	ArtPoint actual;
	gdouble len, best = 1e18, dist, delta;
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

gdouble
sp_desktop_circular_snap (SPDesktop * desktop, ArtPoint * req, gdouble cx, gdouble cy)
{
	SPNamedView * nv;
	ArtPoint actual;
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
 * All functions take a list of ArtPoint and parameter indicating the proposed transformation.
 * They return the upated transformation parameter. 
 */

gdouble
sp_desktop_horizontal_snap_list (SPDesktop * desktop, GSList * l, gdouble dx)
{
  ArtPoint q;
  GSList * points;
  gdouble xdist, xpre, dist = 1e18, d;

  if (!SNAP_ON (desktop)) return dx;

  xdist = dx;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
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

gdouble
sp_desktop_vertical_snap_list (SPDesktop * desktop, GSList * l, gdouble dy)
{
  ArtPoint q;
  GSList * points;
  gdouble ydist, ypre, dist = 1e18, d;

  if (!SNAP_ON (desktop)) return dy;

  ydist = dy;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
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

gdouble
sp_desktop_horizontal_snap_list_scale (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx)
{
  ArtPoint q, check;
  GSList * points;
  gdouble xscale, xdist = 1e18, d;

  if (!SNAP_ON (desktop)) return sx;

  xscale = sx;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
    check.x = sx * (q.x - norm->x) + norm->x;
    if (fabs (q.x - norm->x) > PNDist) {
      d = sp_desktop_horizontal_snap (desktop, &check);
      if ((d < 1e18) && (d < fabs (xdist))) {
	xdist = d;
	xscale = (check.x - norm->x) / (q.x - norm->x);
      }
    }
  }

  return xscale;
}

gdouble
sp_desktop_vertical_snap_list_scale (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sy)
{
  ArtPoint q, check;
  GSList * points;
  gdouble yscale, ydist = 1e18, d;

  if (!SNAP_ON (desktop)) return sy;

  yscale = sy;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
    check.y = sy * (q.y - norm->y) + norm->y;
    if (fabs (q.y - norm->y) > PNDist) {
      d = sp_desktop_vertical_snap (desktop, &check);
      if ((d < 1e18) && (d < fabs (ydist))) {
	ydist = d;
	yscale = (check.y - norm->y)/(q.y - norm->y);
      }
    }
  }

  return yscale;
}

gdouble
sp_desktop_vector_snap_list (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx, gdouble sy)
{
  ArtPoint q, check;
  GSList * points;
  gdouble dist = 1e18, d, ratio ;

  if (!SNAP_ON (desktop)) return sx;

  ratio = fabs (sx);

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
    check.x = (q.x - norm->x) * sx + norm->x;
    check.y = (q.y - norm->y) * sy + norm->y;
    if ((fabs (q.y - norm->y) > PNDist) || (fabs (q.y - norm->y) > PNDist)) {
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

gdouble
sp_desktop_horizontal_snap_list_skew (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx)
{
  ArtPoint q, check;
  GSList * points;
  gdouble xskew, xdist = 1e18, d;

  if (!SNAP_ON (desktop)) return sx;

  xskew = sx;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
    check.x = sx * (q.y - norm->y) + q.x;
    if (fabs (q.y - norm->y) > PNDist) {
      d = sp_desktop_horizontal_snap (desktop, &check);
      if ((d < 1e18) && (d < fabs (xdist))) {
	xdist = d;
	xskew = (check.x - q.x) / (q.y - norm->y);
      }
    }
  }

  return xskew;
}

gdouble
sp_desktop_vertical_snap_list_skew (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sy)
{
  ArtPoint q, check;
  GSList * points;
  gdouble yskew, ydist = 1e18, d;

  if (!SNAP_ON (desktop)) return sy;

  yskew = sy;

  for (points = l; points != NULL; points = points->next) {
    q = *((ArtPoint *)(points->data));
    check.y = sy * (q.x - norm->x) + q.y;
    if (fabs (q.x - norm->x) > PNDist) {
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
gdouble *
sp_desktop_circular_snap_list (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble * rotate)
{
        ArtPoint q1, q2, q, check;
	GSList * spoints;
	gdouble d, best=1e18, h1, h2, r1[6], r2[6], p2n[6], n2p[6];

	if (!SNAP_ON (desktop)) return rotate;

	for (spoints = l; spoints != NULL; spoints = spoints->next) {
	  q = *((ArtPoint *)(spoints->data));
	  art_affine_point (&check, &q, rotate);
	  d = sp_desktop_circular_snap (desktop, &check, norm->x, norm->y);
	  if (d < best) {
	    q1 = q;
	    q2 = check;
	    best = d;
	  }
	}

	// compute the new transformation (rotation) from the snapped point
	if (best < 1e18) {
	  h1 = hypot (q1.x - norm->x, q1.y - norm->y);
	  q1.x = (q1.x - norm->x) / h1;
	  q1.y = (q1.y - norm->y) / h1;
	  h2 = hypot (q2.x - norm->x, q2.y - norm->y);
	  q2.x = (q2.x - norm->x) / h2;
	  q2.y = (q2.y - norm->y) / h2;
	  r1[0] = q1.x;  r1[1] = -q1.y;  r1[2] =  q1.y;  r1[3] = q1.x;  r1[4] = 0;  r1[5] = 0;
	  r2[0] = q2.x;  r2[1] =  q2.y;  r2[2] = -q2.y;  r2[3] = q2.x;  r2[4] = 0;  r2[5] = 0;
	  art_affine_translate (n2p, norm->x, norm->y);
	  art_affine_invert (p2n, n2p);
	  art_affine_multiply (rotate, p2n, r1);
	  art_affine_multiply (rotate, rotate, r2);
	  art_affine_multiply (rotate, rotate, n2p);
	}

	return rotate;
}
