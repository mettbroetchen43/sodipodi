#define __SP_ART_UTILS_C__

/*
 * Libart-related convenience methods
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Raph Levien
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 1998 Raph Levien
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-values.h>
#include <libnr/nr-matrix.h>

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

/* --------------- Cut'n'paste from libart -------------------- */

#define VPATH_BLOCK_SIZE 64

/* Basic constructors and operations for bezier paths */

/* p must be allocated 2^level points. */

/* level must be >= 1 */
static ArtPoint *
art_bezier_to_vec (double x0, double y0,
		   double x1, double y1,
		   double x2, double y2,
		   double x3, double y3,
		   ArtPoint *p,
		   int level)
{
  double x_m, y_m;

#ifdef VERBOSE
  printf ("bezier_to_vec: %g,%g %g,%g %g,%g %g,%g %d\n",
	  x0, y0, x1, y1, x2, y2, x3, y3, level);
#endif
  if (level == 1) {
    x_m = (x0 + 3 * (x1 + x2) + x3) * 0.125;
    y_m = (y0 + 3 * (y1 + y2) + y3) * 0.125;
    p->x = x_m;
    p->y = y_m;
    p++;
    p->x = x3;
    p->y = y3;
    p++;
#ifdef VERBOSE
    printf ("-> (%g, %g) -> (%g, %g)\n", x_m, y_m, x3, y3);
#endif
  } else {
    double xa1, ya1;
    double xa2, ya2;
    double xb1, yb1;
    double xb2, yb2;

    xa1 = (x0 + x1) * 0.5;
    ya1 = (y0 + y1) * 0.5;
    xa2 = (x0 + 2 * x1 + x2) * 0.25;
    ya2 = (y0 + 2 * y1 + y2) * 0.25;
    xb1 = (x1 + 2 * x2 + x3) * 0.25;
    yb1 = (y1 + 2 * y2 + y3) * 0.25;
    xb2 = (x2 + x3) * 0.5;
    yb2 = (y2 + y3) * 0.5;
    x_m = (xa2 + xb1) * 0.5;
    y_m = (ya2 + yb1) * 0.5;
#ifdef VERBOSE
    printf ("%g,%g %g,%g %g,%g %g,%g\n", xa1, ya1, xa2, ya2,
	    xb1, yb1, xb2, yb2);
#endif
    p = art_bezier_to_vec (x0, y0, xa1, ya1, xa2, ya2, x_m, y_m, p, level - 1);
    p = art_bezier_to_vec (x_m, y_m, xb1, yb1, xb2, yb2, x3, y3, p, level - 1);
  }
  return p;
}

#define RENDER_LEVEL 4
#define RENDER_SIZE (1 << (RENDER_LEVEL))

/**
 * art_vpath_render_bez: Render a bezier segment into the vpath. 
 * @p_vpath: Where the pointer to the #ArtVpath structure is stored.
 * @pn_points: Pointer to the number of points in *@p_vpath.
 * @pn_points_max: Pointer to the number of points allocated.
 * @x0: X coordinate of starting bezier point.
 * @y0: Y coordinate of starting bezier point.
 * @x1: X coordinate of first bezier control point.
 * @y1: Y coordinate of first bezier control point.
 * @x2: X coordinate of second bezier control point.
 * @y2: Y coordinate of second bezier control point.
 * @x3: X coordinate of ending bezier point.
 * @y3: Y coordinate of ending bezier point.
 * @flatness: Flatness control.
 *
 * Renders a bezier segment into the vector path, reallocating and
 * updating *@p_vpath and *@pn_vpath_max as necessary. *@pn_vpath is
 * incremented by the number of vector points added.
 *
 * This step includes (@x0, @y0) but not (@x3, @y3).
 *
 * The @flatness argument guides the amount of subdivision. The Adobe
 * PostScript reference manual defines flatness as the maximum
 * deviation between the any point on the vpath approximation and the
 * corresponding point on the "true" curve, and we follow this
 * definition here. A value of 0.25 should ensure high quality for aa
 * rendering.
**/
static void
art_vpath_render_bez (ArtVpath **p_vpath, int *pn, int *pn_max,
		      double x0, double y0,
		      double x1, double y1,
		      double x2, double y2,
		      double x3, double y3,
		      double flatness)
{
  double x3_0, y3_0;
  double z3_0_dot;
  double z1_dot, z2_dot;
  double z1_perp, z2_perp;
  double max_perp_sq;

  double x_m, y_m;
  double xa1, ya1;
  double xa2, ya2;
  double xb1, yb1;
  double xb2, yb2;

  /* It's possible to optimize this routine a fair amount.

     First, once the _dot conditions are met, they will also be met in
     all further subdivisions. So we might recurse to a different
     routine that only checks the _perp conditions.

     Second, the distance _should_ decrease according to fairly
     predictable rules (a factor of 4 with each subdivision). So it might
     be possible to note that the distance is within a factor of 4 of
     acceptable, and subdivide once. But proving this might be hard.

     Third, at the last subdivision, x_m and y_m can be computed more
     expeditiously (as in the routine above).

     Finally, if we were able to subdivide by, say 2 or 3, this would
     allow considerably finer-grain control, i.e. fewer points for the
     same flatness tolerance. This would speed things up downstream.

     In any case, this routine is unlikely to be the bottleneck. It's
     just that I have this undying quest for more speed...

  */

  x3_0 = x3 - x0;
  y3_0 = y3 - y0;

  /* z3_0_dot is dist z0-z3 squared */
  z3_0_dot = x3_0 * x3_0 + y3_0 * y3_0;

  /* todo: this test is far from satisfactory. */
  if (z3_0_dot < 0.001)
    goto nosubdivide;

  /* we can avoid subdivision if:

     z1 has distance no more than flatness from the z0-z3 line

     z1 is no more z0'ward than flatness past z0-z3

     z1 is more z0'ward than z3'ward on the line traversing z0-z3

     and correspondingly for z2 */

  /* perp is distance from line, multiplied by dist z0-z3 */
  max_perp_sq = flatness * flatness * z3_0_dot;

  z1_perp = (y1 - y0) * x3_0 - (x1 - x0) * y3_0;
  if (z1_perp * z1_perp > max_perp_sq)
    goto subdivide;

  z2_perp = (y3 - y2) * x3_0 - (x3 - x2) * y3_0;
  if (z2_perp * z2_perp > max_perp_sq)
    goto subdivide;

  z1_dot = (x1 - x0) * x3_0 + (y1 - y0) * y3_0;
  if (z1_dot < 0 && z1_dot * z1_dot > max_perp_sq)
    goto subdivide;

  z2_dot = (x3 - x2) * x3_0 + (y3 - y2) * y3_0;
  if (z2_dot < 0 && z2_dot * z2_dot > max_perp_sq)
    goto subdivide;

  if (z1_dot + z1_dot > z3_0_dot)
    goto subdivide;

  if (z2_dot + z2_dot > z3_0_dot)
    goto subdivide;

 nosubdivide:
  /* don't subdivide */
  art_vpath_add_point (p_vpath, pn, pn_max,
		       ART_LINETO, x3, y3);
  return;

 subdivide:

  xa1 = (x0 + x1) * 0.5;
  ya1 = (y0 + y1) * 0.5;
  xa2 = (x0 + 2 * x1 + x2) * 0.25;
  ya2 = (y0 + 2 * y1 + y2) * 0.25;
  xb1 = (x1 + 2 * x2 + x3) * 0.25;
  yb1 = (y1 + 2 * y2 + y3) * 0.25;
  xb2 = (x2 + x3) * 0.5;
  yb2 = (y2 + y3) * 0.5;
  x_m = (xa2 + xb1) * 0.5;
  y_m = (ya2 + yb1) * 0.5;
#ifdef VERBOSE
  printf ("%g,%g %g,%g %g,%g %g,%g\n", xa1, ya1, xa2, ya2,
	  xb1, yb1, xb2, yb2);
#endif
  art_vpath_render_bez (p_vpath, pn, pn_max,
			x0, y0, xa1, ya1, xa2, ya2, x_m, y_m, flatness);
  art_vpath_render_bez (p_vpath, pn, pn_max,
			x_m, y_m, xb1, yb1, xb2, yb2, x3, y3, flatness);
}

ArtVpath *
sp_vpath_from_bpath_transform_closepath (const ArtBpath *bpath, NRMatrixF *transform, int close, int perturb, double flatness)
{
	const ArtBpath *bp;
	ArtVpath *vpath;
	int vpath_len, vpath_size;
	double x, y, sx, sy;

	vpath_size = VPATH_BLOCK_SIZE;
	vpath = art_new (ArtVpath, vpath_size);
	vpath_len = 0;

	x = y = 0.0;
	sx = sy = 0.0;

	for (bp = bpath; bp->code != ART_END; bp++) {
		if (vpath_len >= vpath_size) art_expand (vpath, ArtVpath, vpath_size);
		switch (bp->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			if (close && ((x != sx) || (y != sy))) {
				/* Add closepath */
				if (vpath_len >= vpath_size) art_expand (vpath, ArtVpath, vpath_size);
				vpath[vpath_len].code = ART_LINETO;
				vpath[vpath_len].x = sx;
				vpath[vpath_len].y = sy;
				vpath_len += 1;
			}
			vpath[vpath_len].code = ART_MOVETO;
			if (transform) {
				sx = x = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				sy = y = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				sx = x = bp->x3;
				sy = y = bp->y3;
			}
			vpath[vpath_len].x = x;
			vpath[vpath_len].y = y;
			vpath_len += 1;
			break;
		case ART_LINETO:
			vpath[vpath_len].code = ART_LINETO;
			if (transform) {
				x = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				y = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				x = bp->x3;
				y = bp->y3;
			}
			vpath[vpath_len].x = x;
			vpath[vpath_len].y = y;
			vpath_len += 1;
			break;
		case ART_CURVETO:
			if (transform) {
				art_vpath_render_bez (&vpath, &vpath_len, &vpath_size,
						      x, y,
						      NR_MATRIX_DF_TRANSFORM_X (transform, bp->x1, bp->y1),
						      NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x1, bp->y1),
						      NR_MATRIX_DF_TRANSFORM_X (transform, bp->x2, bp->y2),
						      NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x2, bp->y2),
						      NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3),
						      NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3),
						      flatness);
				x = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				y = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				art_vpath_render_bez (&vpath, &vpath_len, &vpath_size,
						      x, y,
						      bp->x1, bp->y1,
						      bp->x2, bp->y2,
						      bp->x3, bp->y3,
						      flatness);
				x = bp->x3;
				y = bp->y3;
			}
			break;
		default:
			break;
		}
	}

	if ((x != sx) || (y != sy)) {
		/* Add closepath */
		if (vpath_len >= vpath_size) art_expand (vpath, ArtVpath, vpath_size);
		vpath[vpath_len].code = ART_LINETO;
		vpath[vpath_len].x = sx;
		vpath[vpath_len].y = sy;
		vpath_len += 1;
	}

	if (vpath_len >= vpath_size) art_expand (vpath, ArtVpath, vpath_size);
	vpath[vpath_len].code = ART_END;

	if (perturb) {
		ArtVpath *vp;
		int closed;
		closed = 0;
		for (vp = vpath; vp->code != ART_END; vp++) {
			switch (vp->code) {
			case ART_MOVETO:
				closed = 1;
				break;
			case ART_MOVETO_OPEN:
				closed = 0;
				break;
			case ART_LINETO:
				if (!closed || vp[1].code == ART_LINETO) {
					vp->x += 0.5 * flatness * (((double) rand () / RAND_MAX) - 0.5);
					vp->y += 0.5 * flatness * (((double) rand () / RAND_MAX) - 0.5);
				}
				break;
			default:
				break;
			}
		}
	}

	return vpath;
}
