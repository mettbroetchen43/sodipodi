#define __NR_PATHOPS_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "nr-macros.h"

#include "nr-pathops.h"

static double nr_line_point_distance (double Ax, double Ay, double Bx, double By, double Px, double Py, double *t, double tolerance);
static double nr_curve_point_distance (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
				       double Px, double Py,
				       double *t, double best, double tolerance);

static unsigned int nr_path_intersect_inner (double a[], double b[], double ta[], double tb[], double tolerance);
static void nr_path_bbox_loose (double p[], double *x0, double *y0, double *x1, double *y1);
static void nr_path_subdivide (double path[], double a[], double b[], double pos);

int
nr_path_intersect_self (double path[], double pos[], double tolerance)
{
	if (NR_DF_TEST_CLOSE (path[0], path[6], tolerance) &&
	    NR_DF_TEST_CLOSE (path[1], path[7], tolerance)) {
		/* Special case - coincident start and end */
		pos[0] = 0.0;
		pos[1] = 1.0;
		return 2;
	} else {
		double a[8], b[8], ta[1], tb[1];
		int nt;
		/* Use common intersector on subdivided path */
		/* fixme: Test plain case */
		nr_path_subdivide (path, a, b, 0.5);
		nt = nr_path_intersect_inner (a, b, ta, tb, tolerance);
		if (nt > 0) {
			pos[0] = 2.0 * ta[0];
			return 1;
		}
	}
	return 0;
}

static unsigned int
nr_path_intersect_inner (double a[], double b[], double ta[], double tb[], double tolerance)
{
#if 0
	double ax0, ay0, ax1, ay1;
	double bx0, by0, bx1, by1;
	double a0[8], a1[8];
	/* Step 1 - check bounding boxes */
	/* fixme: Use exact bounding boxes */
	nr_path_bbox_loose (a, &ax0, &ay0, &ax1, &ay1);
	nr_path_bbox_loose (b, &bx0, &by0, &bx1, &by1);
	if ((ax0 >= bx1) || (ax1 <= bx0) || (ay0 >= by1) || (ay1 <= by0)) return 0;
	/* Step 2 - check border polygons */
	for (i = 0; i < 4; i++) {
		int aidx;
		aidx = i * 2;
		for (j = 0; j < 4; j++) {
			int bidx;
			bidx = j * 2;
			if (nr_path_line_test_intersect (a[aidx], a[aidx + 1], a[(aidx + 2) % 8], a[(aidx + 3) % 8],
							 b[bidx], b[bidx + 1], b[(bidx + 2) % 8], b[(bidx + 3) % 8],
							 tolerance)) break;
		}
	}
	if ((i == 3) && (j == 3)) return 0;
	/* Now comes the crazy part */
	/* Split first segment */
	nr_path_subdivide (a, a0, a1, 0.5);
	/* Test the midpoint on second segment */
	dist = nr_path_test_point (b, a0[6], a0[7], &t, 2.0 * tolerance);
	if (dist < (2.0 * tolerance)) {
		/* lala */
	}
#endif
	return 0;
}

static void
nr_path_bbox_loose (double p[], double *x0, double *y0, double *x1, double *y1)
{
	*x0 = MIN (p[0], p[2]);
	*x0 = MIN (*x0, p[4]);
	*x0 = MIN (*x0, p[6]);
	*y0 = MIN (p[1], p[3]);
	*y0 = MIN (*y0, p[5]);
	*y0 = MIN (*y0, p[7]);
	*x1 = MAX (p[0], p[2]);
	*x1 = MAX (*x1, p[4]);
	*x1 = MAX (*x1, p[6]);
	*y1 = MAX (p[1], p[3]);
	*y1 = MAX (*y1, p[5]);
	*y1 = MAX (*y1, p[7]);
}

static void
nr_path_subdivide (double path[], double a[], double b[], double t)
{
	double x000, y000, x001, y001, x011, y011, x111, y111;
	double x00t, x0tt, xttt, x1tt, x11t, x01t;
	double y00t, y0tt, yttt, y1tt, y11t, y01t;
	double s;

	x000 = path[0];
	y000 = path[1];
	x001 = path[2];
	y001 = path[3];
	x011 = path[4];
	y011 = path[5];
	x111 = path[6];
	y111 = path[7];

	s = 1.0 - t;

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

	a[0] = x000;
	a[1] = y000;
	a[2] = x00t;
	a[3] = y00t;
	a[4] = x0tt;
	a[5] = y0tt;
	a[6] = xttt;
	a[7] = yttt;

	b[0] = xttt;
	b[1] = yttt;
	b[2] = x1tt;
	b[3] = y1tt;
	b[4] = x11t;
	b[5] = y11t;
	b[6] = x111;
	b[7] = y111;
}

static double
nr_line_point_distance (double Ax, double Ay, double Bx, double By, double Px, double Py, double *t, double tolerance)
{
	double Dx, Dy;
	double dist2;

	Dx = Bx - Ax;
	Dy = By - Ay;

	*t = ((Px - Ax) * Dx + (Py - Ay) * Dy) / (Dx * Dx + Dy * Dy);
	if (*t <= 0.0) {
		dist2 = (Px - Ax) * (Px - Ax) + (Py - Ay) * (Py - Ay);
		*t = 0.0;
	} else if (*t >= 1.0) {
		dist2 = (Px - Bx) * (Px - Bx) + (Py - By) * (Py - By);
		*t = 1.0;
	} else {
		double Qx, Qy;
		Qx = Ax + *t * Dx;
		Qy = Ay + *t * Dy;
		dist2 = (Px - Qx) * (Px - Qx) + (Py - Qy) * (Py - Qy);
	}
	return sqrt (dist2);
}

static double
nr_curve_point_distance (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
			 double Px, double Py,
			 double *t, double best, double tolerance)
{
	double BX0, BY0, BX1, BY1;
	double dist2, best2, bestt;

	/* Find sloppy bbox */
	BX0 = MIN (x0, x1);
	BX0 = MIN (BX0, x2);
	BX0 = MIN (BX0, x3);
	BY0 = MIN (y0, y1);
	BY0 = MIN (BY0, y2);
	BY0 = MIN (BY0, y3);
	BX1 = MAX (x0, x1);
	BX1 = MAX (BX1, x2);
	BX1 = MAX (BX1, x3);
	BY1 = MAX (y0, y1);
	BY1 = MAX (BY1, y2);
	BY1 = MAX (BY1, y3);

	/* Quicly adjust to endpoints */
	best2 = best * best;
	bestt = 0.0;
	dist2 = (x0 - Px) * (x0 - Px) + (y0 - Py) * (y0 - Py);
	if (dist2 < best2) {
		best2 = dist2;
	}
	dist2 = (x1 - Px) * (x1 - Px) + (y1 - Py) * (y1 - Py);
	if (dist2 < best2) {
		best2 = dist2;
		bestt = 1.0;
	}
	best = sqrt (best2);

	if (((BX0 - Px) < best) && ((BY0 - Py) < best) && ((Px - BX1) < best) && ((Py - BY1) < best)) {
		double dx1_0, dy1_0, dx2_0, dy2_0, dx3_0, dy3_0, dx2_3, dy2_3, d3_0_2;
		double s1_q, t1_q, s2_q, t2_q, v2_q;
		double f2, f2_q;
		double x00t, y00t, x0tt, y0tt, xttt, yttt, x1tt, y1tt, x11t, y11t;
		double flatness;
		double subbest, subt;
		/* Point is inside sloppy bbox */
		/* Now we have to decide, whether subdivide */
		flatness = tolerance;
		dx1_0 = x1 - x0;
		dy1_0 = y1 - y0;
		dx2_0 = x2 - x0;
		dy2_0 = y2 - y0;
		dx3_0 = x3 - x0;
		dy3_0 = y3 - y0;
		dx2_3 = x3 - x2;
		dy2_3 = y3 - y2;
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
		return nr_line_point_distance (x0, y0, x3, y3, Px, Py, t, tolerance);

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

		subbest = nr_curve_point_distance (x0, y0, x00t, y00t, x0tt, y0tt, xttt, yttt, Px, Py, &subt, best, tolerance);
		if (subbest < best) {
			best = subbest;
			*t = 0.5 * subt;
		}
		subbest = nr_curve_point_distance (xttt, yttt, x1tt, y1tt, x11t, y11t, x3, y3, Px, Py, &subt, best, tolerance);
		if (subbest < best) {
			best = subbest;
			*t = 0.5 + 0.5 * subt;
		}
	}

	return best;
}

NRPathSeg *
nr_path_seg_new_curve (NRPathSeg *next, unsigned int value, unsigned int isfirst, unsigned int islast,
		       double x0, double y0, double x1, double y1, double x2, double y2)
{
	NRPathSeg *seg;
	seg = nr_new (NRPathSeg, 1);
	seg->value = value;
	seg->iscurve = 1;
	seg->isfirst = isfirst;
	seg->islast = islast;
	seg->x0 = x0;
	seg->y0 = y0;
	seg->x1 = x1;
	seg->y1 = y1;
	seg->x2 = x2;
	seg->y2 = y2;
	return seg;
}

NRPathSeg *
nr_path_seg_new_line (NRPathSeg *next, unsigned int value, unsigned int isfirst, unsigned int islast,
		      double x0, double y0)
{
	NRPathSeg *seg;
	seg = nr_new (NRPathSeg, 1);
	seg->value = value;
	seg->iscurve = 0;
	seg->isfirst = isfirst;
	seg->islast = islast;
	seg->x0 = x0;
	seg->y0 = y0;
	return seg;
}

void
nr_path_seg_free (NRPathSeg *seg)
{
	nr_free (seg);
}

