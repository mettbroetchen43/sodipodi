#define __NR_STROKE_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <math.h>

#include "nr-rect.h"
#include "nr-matrix.h"
#include "nr-svp-uncross.h"

#include "nr-stroke.h"

#define NR_QUANT_X 16.0F
#define NR_QUANT_Y 4.0F
#define NR_COORD_X_FROM_ART(v) (floor (NR_QUANT_X * (v) + 0.5F) / NR_QUANT_X)
#define NR_COORD_Y_FROM_ART(v) (floor (NR_QUANT_Y * (v) + 0.5F) / NR_QUANT_Y)
#define NR_COORD_TO_ART(v) (v)

typedef struct _NRSVLBuild NRSVLBuild;
typedef struct _NRSVLSide NRSVLSide;

struct _NRSVLSide {
	/* Vertex list */
	NRVertex *vx;
	/* Direction */
	int dir;
	/* Start coordinates */
	NRCoord sx, sy;
};

struct _NRSVLBuild {
	NRSVL *svl;
	NRFlat *flats;
	NRMatrixF transform;
	unsigned int closed : 1;
	int npoints;
	NRRectF bbox;
	NRCoord x[4];
	NRCoord y[4];
	NRSVLSide left, right;
};

static void
nr_svl_build_finish_left (NRSVLBuild *svlb)
{
	if (svlb->left.vx) {
		NRSVL *new;
		/* We have running segment */
		if (svlb->left.dir > 0) {
			/* We are upwards, prepended, so reverse */
			svlb->left.vx = nr_vertex_reverse_list (svlb->left.vx);
		}
		new = nr_svl_new_full (svlb->left.vx, &svlb->bbox, svlb->left.dir);
		svlb->svl = nr_svl_insert_sorted (svlb->svl, new);
	}
	svlb->left.vx = NULL;
}

static void
nr_svl_build_finish_right (NRSVLBuild *svlb)
{
	if (svlb->right.vx) {
		NRSVL *new;
		/* We have running segment */
		if (svlb->right.dir > 0) {
			/* We are upwards, prepended, so reverse */
			svlb->right.vx = nr_vertex_reverse_list (svlb->right.vx);
		}
		new = nr_svl_new_full (svlb->right.vx, &svlb->bbox, svlb->right.dir);
		svlb->svl = nr_svl_insert_sorted (svlb->svl, new);
	}
	svlb->right.vx = NULL;
}

static void
nr_svl_build_moveto_left (NRSVLBuild *svlb, float x, float y)
{
	nr_svl_build_finish_left (svlb);
	svlb->left.sx = NR_COORD_X_FROM_ART (x);
	svlb->left.sy = NR_COORD_Y_FROM_ART (y);
	svlb->left.dir = 0;
}

static void
nr_svl_build_moveto_right (NRSVLBuild *svlb, float x, float y)
{
	nr_svl_build_finish_right (svlb);
	svlb->right.sx = NR_COORD_X_FROM_ART (x);
	svlb->right.sy = NR_COORD_Y_FROM_ART (y);
	svlb->right.dir = 0;
}

static void
nr_svl_build_lineto_left (NRSVLBuild *svlb, float x, float y)
{
	x = (float) NR_COORD_X_FROM_ART (x);
	y = (float) NR_COORD_Y_FROM_ART (y);
	if (y != svlb->left.sy) {
		NRVertex *vertex;
		int newdir;
		/* We have valid line */
		newdir = (y > svlb->left.sy) ? 1 : -1;
		if (newdir != svlb->left.dir) {
			/* We have either start or turn */
			nr_svl_build_finish_left (svlb);
			svlb->left.dir = newdir;
		}
		if (!svlb->left.vx) {
			svlb->left.vx = nr_vertex_new_xy (svlb->left.sx, svlb->left.sy);
			nr_rect_f_union_xy (&svlb->bbox, svlb->left.sx, svlb->left.sy);
		}
		/* Add vertex to list */
		vertex = nr_vertex_new_xy (x, y);
		vertex->next = svlb->left.vx;
		svlb->left.vx = vertex;
		/* Stretch bbox */
		nr_rect_f_union_xy (&svlb->bbox, x, y);
		svlb->left.sx = x;
		svlb->left.sy = y;
	} else if (x != svlb->left.sx) {
		NRFlat *flat;
		/* Horizontal line ends running segment */
		nr_svl_build_finish_left (svlb);
		svlb->left.dir = 0;
		/* Add horizontal lines to flat list */
		flat = nr_flat_new_full (y, MIN (svlb->left.sx, x), MAX (svlb->left.sx, x));
		svlb->flats = nr_flat_insert_sorted (svlb->flats, flat);
		svlb->left.sx = x;
		/* sy = y ;-) */
	}
}

static void
nr_svl_build_lineto_rigth (NRSVLBuild *svlb, float x, float y)
{
	x = (float) NR_COORD_X_FROM_ART (x);
	y = (float) NR_COORD_Y_FROM_ART (y);
	if (y != svlb->right.sy) {
		NRVertex *vertex;
		int newdir;
		/* We have valid line */
		newdir = (y > svlb->right.sy) ? 1 : -1;
		if (newdir != svlb->right.dir) {
			/* We have either start or turn */
			nr_svl_build_finish_left (svlb);
			svlb->right.dir = newdir;
		}
		if (!svlb->right.vx) {
			svlb->right.vx = nr_vertex_new_xy (svlb->right.sx, svlb->right.sy);
			nr_rect_f_union_xy (&svlb->bbox, svlb->right.sx, svlb->right.sy);
		}
		/* Add vertex to list */
		vertex = nr_vertex_new_xy (x, y);
		vertex->next = svlb->right.vx;
		svlb->right.vx = vertex;
		/* Stretch bbox */
		nr_rect_f_union_xy (&svlb->bbox, x, y);
		svlb->right.sx = x;
		svlb->right.sy = y;
	} else if (x != svlb->right.sx) {
		NRFlat *flat;
		/* Horizontal line ends running segment */
		nr_svl_build_finish_left (svlb);
		svlb->right.dir = 0;
		/* Add horizontal lines to flat list */
		flat = nr_flat_new_full (y, MIN (svlb->right.sx, x), MAX (svlb->right.sx, x));
		svlb->flats = nr_flat_insert_sorted (svlb->flats, flat);
		svlb->right.sx = x;
		/* sy = y ;-) */
	}
}

static void
nr_svl_build_start_closed_subpath (NRSVLBuild *svlb, float x, float y)
{
	x = (float) NR_COORD_X_FROM_ART (x);
	y = (float) NR_COORD_Y_FROM_ART (y);
	svlb->closed = TRUE;
	svlb->x[0] = svlb->x[3] = x;
	svlb->y[0] = svlb->y[3] = y;
	svlb->npoints = 1;
}

static void
nr_svl_build_start_open_subpath (NRSVLBuild *svlb, float x, float y)
{
	x = (float) NR_COORD_X_FROM_ART (x);
	y = (float) NR_COORD_Y_FROM_ART (y);
	svlb->closed = FALSE;
	svlb->x[0] = svlb->x[3] = x;
	svlb->y[0] = svlb->y[3] = y;
	svlb->npoints = 1;
}

static void
nr_svl_build_lineto (NRSVLBuild *svlb, float x, float y)
{
	x = (float) NR_COORD_X_FROM_ART (x);
	y = (float) NR_COORD_Y_FROM_ART (y);
	if ((x != svlb->x[3]) || (y != svlb->y[3])) {
		if (svlb->npoints == 1) {
			/* Second point on line */
			svlb->x[1] = x;
			svlb->y[1] = y;
			/* Draw cap if open */
		} else if (svlb->npoints == 2) {
			if (svlb->closed) {
				/* Draw join */
			} else {
				/* Draw 2->3 plus join */
			}
		} else {
			/* Draw 2->3 plus join */
		}
		svlb->x[2] = svlb->x[3];
		svlb->y[2] = svlb->y[3];
		svlb->x[3] = x;
		svlb->y[3] = y;
		svlb->npoints += 1;
	}
}

static void
nr_svl_build_finish_subpath (NRSVLBuild *svlb)
{
	if (svlb->closed) {
		/* Draw 2->3 plus join 0->1 */
		/* Draw 0->1 */
	} else {
		/* Draw 2->3 plus cap */
	}
}

static void
nr_svl_build_finish_segment (NRSVLBuild *svlb)
{
}

static void
nr_svl_build_curveto (NRSVLBuild *svlb, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, float flatness)
{
	double t_x0, t_y0, t_x1, t_y1, t_x2, t_y2, t_x3, t_y3;
	double dx1_0, dy1_0, dx2_0, dy2_0, dx3_0, dy3_0, dx2_3, dy2_3, d3_0_2;
	double s1_q, t1_q, s2_q, t2_q, v2_q;
	double f2, f2_q;
	double x00t, y00t, x0tt, y0tt, xttt, yttt, x1tt, y1tt, x11t, y11t;

	t_x0 = NR_MATRIX_DF_TRANSFORM_X (&svlb->transform, x0, y0);
	t_y0 = NR_MATRIX_DF_TRANSFORM_Y (&svlb->transform, x0, y0);
	t_x1 = NR_MATRIX_DF_TRANSFORM_X (&svlb->transform, x1, y1);
	t_y1 = NR_MATRIX_DF_TRANSFORM_Y (&svlb->transform, x1, y1);
	t_x2 = NR_MATRIX_DF_TRANSFORM_X (&svlb->transform, x2, y2);
	t_y2 = NR_MATRIX_DF_TRANSFORM_Y (&svlb->transform, x2, y2);
	t_x3 = NR_MATRIX_DF_TRANSFORM_X (&svlb->transform, x3, y3);
	t_y3 = NR_MATRIX_DF_TRANSFORM_Y (&svlb->transform, x3, y3);

	dx1_0 = t_x1 - t_x0;
	dy1_0 = t_y1 - t_y0;
	dx2_0 = t_x2 - t_x0;
	dy2_0 = t_y2 - t_y0;
	dx3_0 = t_x3 - t_x0;
	dy3_0 = t_y3 - t_y0;
	dx2_3 = t_x3 - t_x2;
	dy2_3 = t_y3 - t_y2;
	f2 = flatness * flatness;
	d3_0_2 = dx3_0 * dx3_0 + dy3_0 * dy3_0;
	if (d3_0_2 < f2) {
		double d1_0_2, d2_0_2;
		d1_0_2 = dx1_0 * dx1_0 + dy1_0 * dy1_0;
		d2_0_2 = dx2_0 * dx2_0 + dy2_0 * dy2_0;
		if ((d1_0_2 < f2) && (d2_0_2 < f2)) {
			goto nosubdivide;
		} else {
			goto subdivide;
		}
	}
	f2_q = flatness * flatness * d3_0_2;
	s1_q = dx1_0 * dx3_0 + dy1_0 * dy3_0;
	t1_q = dy1_0 * dx3_0 - dx1_0 * dy3_0;
	s2_q = dx2_0 * dx3_0 + dy2_0 * dy3_0;
	t2_q = dy2_0 * dx3_0 - dx2_0 * dy3_0;
	v2_q = dx2_3 * dx3_0 + dy2_3 * dy3_0;
	if ((t1_q * t1_q) > f2_q) goto subdivide;
	if ((t2_q * t2_q) > f2_q) goto subdivide;
	if ((s1_q < 0.0) && ((s1_q * s1_q) > f2_q)) goto subdivide;
	if ((v2_q < 0.0) && ((v2_q * v2_q) > f2_q)) goto subdivide;
	if (s1_q >= s2_q) goto subdivide;

 nosubdivide:
	nr_svl_build_lineto (svlb, (float) x3, (float) y3);
	return;

 subdivide:
	x00t = (x0 + x1) * 0.5;
	y00t = (y0 + y1) * 0.5;
	x0tt = (x0 + 2 * x1 + x2) * 0.25;
	y0tt = (y0 + 2 * y1 + y2) * 0.25;
	x1tt = (x1 + 2 * x2 + x3) * 0.25;
	y1tt = (y1 + 2 * y2 + y3) * 0.25;
	x11t = (x2 + x3) * 0.5;
	y11t = (y2 + y3) * 0.5;
	xttt = (x0tt + x1tt) * 0.5;
	yttt = (y0tt + y1tt) * 0.5;

	nr_svl_build_curveto (svlb, x0, y0, x00t, y00t, x0tt, y0tt, xttt, yttt, flatness);
	nr_svl_build_curveto (svlb, xttt, yttt, x1tt, y1tt, x11t, y11t, x3, y3, flatness);
}

NRSVL *
nr_bpath_stroke (const NRBPath *path, NRMatrixF *transform, float width, float flatness)
{
	NRSVLBuild svlb;
	ArtBpath *bp;
	double x, y, sx, sy;

	/* Initialize NRSVLBuilds */
	svlb.svl = NULL;
	svlb.flats = NULL;
	if (transform) {
		svlb.transform = *transform;
	} else {
		nr_matrix_f_set_identity (&svlb.transform);
	}
	svlb.closed = FALSE;
	svlb.npoints = 0;
	nr_rect_f_set_empty (&svlb.bbox);
	svlb.left.vx = NULL;
	svlb.left.dir = 0;
	svlb.left.sx = svlb.left.sy = 0.0;
	svlb.right.vx = NULL;
	svlb.right.dir = 0;
	svlb.right.sx = svlb.left.sy = 0.0;

	x = y = 0.0;
	sx = sy = 0.0;

	for (bp = path->path; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO:
			nr_svl_build_finish_subpath (&svlb);
			sx = x = bp->x3;
			sy = y = bp->y3;
			nr_svl_build_start_closed_subpath (&svlb, (float) x, (float) y);
			break;
		case ART_MOVETO_OPEN:
			nr_svl_build_finish_subpath (&svlb);
			sx = x = bp->x3;
			sy = y = bp->y3;
			nr_svl_build_start_open_subpath (&svlb, (float) x, (float) y);
			break;
		case ART_LINETO:
			sx = x = bp->x3;
			sy = y = bp->y3;
			nr_svl_build_lineto (&svlb, (float) x, (float) y);
			break;
		case ART_CURVETO:
			x = bp->x3;
			y = bp->y3;
			nr_svl_build_curveto (&svlb, sx, sy, bp->x1, bp->y1, bp->x2, bp->y2, x, y, flatness);
			sx = x;
			sy = y;
			break;
		default:
			/* fixme: free lists */
			return NULL;
			break;
		}
	}
	nr_svl_build_finish_subpath (&svlb);
	nr_svl_build_finish_segment (&svlb);
	if (svlb.svl) {
		/* NRSVL *s; */
		svlb.svl = nr_svl_uncross_full (svlb.svl, svlb.flats, NR_WIND_RULE_NONZERO);
	} else {
		nr_flat_free_list (svlb.flats);
	}
	/* This happnes in uncross */
	/* nr_flat_free_list (flats); */

	return svlb.svl;
}



