#define __NR_PIXBLOCK_LINE_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <math.h>
#include <assert.h>

#include <libnr/nr-pixops.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock-pixel.h>
#include <libnr/nr-pixblock-line.h>

void
nr_pixblock_draw_line_rgba32 (NRPixBlock *pb, int x0, int y0, int x1, int y1,
			      unsigned int first, unsigned int rgba)
{
	int deltax, deltay;
	int xinc1, xinc2, yinc1, yinc2, dstep1, dstep2;
	int den, num0, num, numadd, numpixels;
	int x, y, curpixel;
	/* Pixblock */
	int dbpp;
	NRPixBlock spb;
	unsigned char *dpx, *spx;
	unsigned int last;

	if ((x0 < pb->area.x0) && (x1 < pb->area.x0)) return;
	if ((x0 >= pb->area.x1) && (x1 >= pb->area.x1)) return;
	if ((y0 < pb->area.y0) && (y1 < pb->area.y0)) return;
	if ((y0 >= pb->area.y1) && (y1 >= pb->area.y1)) return;

	last = 1;

	if (x1 < x0) {
		int t;
		t = x1;
		x1 = x0;
		x0 = t;
		t = y1;
		y1 = y0;
		y0 = t;
		last = first;
		first = 1;
	}

	deltax = x1 - x0;
	xinc1 = 1;
	xinc2 = 1;

	/* g_print ("line A %d %d %d %d\n", x0, y0, x1, y1); */

	if (y1 >= y0) {
		deltay = y1 - y0;
		yinc1 = 1;
		yinc2 = 1;
	} else {
		deltay = y0 - y1;
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax + 1;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay + 1;
	}

	/* v = v0 + ((num + pixels * numadd) / den) * vinc1 + pixels * vinc2 */
	/* pixels = ((v - v0) * den - num + (numadd - 1)) / numadd */

	if ((x0 < pb->area.x0) && (x1 >= pb->area.x0)) {
		int pixels;
		if (xinc1) {
			pixels = ((pb->area.x0 - x0) * den - num + (numadd - 1)) / numadd;
		} else {
			pixels = pb->area.x0 - x0;
		}
		x0 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
		y0 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
		if (x0 >= pb->area.x1) return;
		num = (num + pixels * numadd) % den;
		numpixels -= pixels;
		first = 1;
	}

	if ((x0 < pb->area.x1) && (x1 >= pb->area.x1)) {
		int pixels;
		if (xinc1) {
			/* We need LAST pixel before step here */
			pixels = ((pb->area.x1 - x0) * den - num + (numadd - 1)) / numadd - 1;
		} else {
			pixels = pb->area.x1 - x0 - 1;
		}
		x1 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
		y1 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
		if (x1 <= pb->area.x0) return;
		numpixels = pixels + 1;
		last = 1;
	}

	if ((y0 < pb->area.y0) && (y1 < pb->area.y0)) return;
	if ((y0 >= pb->area.y1) && (y1 >= pb->area.y1)) return;

	if (y1 >= y0) {
		if ((y0 < pb->area.y0) && (y1 >= pb->area.y0)) {
			int pixels;
			if (yinc1) {
				pixels = ((pb->area.y0 - y0) * den - num + (numadd - 1)) / numadd;
			} else {
				pixels = pb->area.y0 - y0;
			}
			x0 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y0 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x0 >= pb->area.x1) return;
			if (y0 >= pb->area.y1) return;
			num = (num + pixels * numadd) % den;
			numpixels -= pixels;
			first = 1;
		}
		if ((y0 < pb->area.y1) && (y1 >= pb->area.y1)) {
			int pixels;
			if (yinc1) {
				/* We need LAST pixel before step here */
				pixels = ((pb->area.y1 - y0) * den - num + (numadd - 1)) / numadd - 1;
			} else {
				pixels = pb->area.y1 - y0 - 1;
			}
			x1 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y1 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x1 <= pb->area.x0) return;
			if (y1 <= pb->area.y0) return;
			numpixels = pixels + 1;
			last = 1;
		}
	} else {
		if ((y0 >= pb->area.y1) && (y1 < pb->area.y1)) {
			int pixels;
			if (yinc1) {
				pixels = ((y0 - (pb->area.y1 - 1)) * den - num + (numadd - 1)) / numadd;
			} else {
				pixels = y0 - (pb->area.y1 - 1);
			}
			x0 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y0 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x0 >= pb->area.x1) return;
			if (y0 < pb->area.y0) return;
			num = (num + pixels * numadd) % den;
			numpixels -= pixels;
			first = 1;
		}
		if ((y0 >= pb->area.y0) && (y1 < pb->area.y0)) {
			int pixels;
			if (yinc1) {
				/* We need LAST pixel before step here */
				pixels = ((y0 - (pb->area.y0 - 1)) * den - num + (numadd - 1)) / numadd - 1;
			} else {
				pixels = y0 - (pb->area.y0 - 1) - 1;
			}
			x1 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y1 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x1 < pb->area.x0) return;
			if (y1 >= pb->area.y1) return;
			numpixels = pixels + 1;
			last = 1;
		}
	}

	/* We can be quite sure 1x1 pixblock is TINY */
	nr_pixblock_setup_fast (&spb, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, 1, 1, 0);
	spb.empty = 0;
	spx = NR_PIXBLOCK_PX (&spb);
	spx[0] = NR_RGBA32_R (rgba);
	spx[1] = NR_RGBA32_G (rgba);
	spx[2] = NR_RGBA32_B (rgba);
	spx[3] = NR_RGBA32_A (rgba);

	dbpp = NR_PIXBLOCK_BPP (pb);

	x = x0;
	y = y0;

	num0 = num;
	dpx = NR_PIXBLOCK_PX (pb) + (y0 - pb->area.y0) * pb->rs + (x0 - pb->area.x0) * NR_PIXBLOCK_BPP (pb);
	dstep1 = yinc1 * pb->rs + xinc1 * NR_PIXBLOCK_BPP (pb);
	dstep2 = yinc2 * pb->rs + xinc2 * NR_PIXBLOCK_BPP (pb);
	for (curpixel = 0; curpixel < numpixels; curpixel++) {
		int cx, cy;
		if (den != 0) {
			cx = x0 + ((num0 + curpixel * numadd) / den) * xinc1 + curpixel * xinc2;
			cy = y0 + ((num0 + curpixel * numadd) / den) * yinc1 + curpixel * yinc2;
		} else {
			assert (num == 0);
			assert (numadd == 0);
			cx = x0 + curpixel * xinc2;
			cy = y0 + curpixel * yinc2;
		}
		assert (cx == x);
		assert (cy == y);
		assert (x >= pb->area.x0);
		assert (x < pb->area.x1);
		assert (y >= pb->area.y0);
		assert (y < pb->area.y1);
		if ((first || (curpixel > 0)) && (last || (curpixel < (numpixels - 1)))) {
			nr_compose_pixblock_pixblock_pixel (pb, dpx, &spb, spx);
		}
		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
			dpx += dstep1;
		}
		x += xinc2;
		y += yinc2;
		dpx += dstep2;
	}

	nr_pixblock_release (&spb);
}

#define flatness 0.5

static void
nr_pixblock_draw_curve_rgba32 (NRPixBlock *pb,
			       double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
			       unsigned int first, unsigned int rgba)
{
	double dx1_0, dy1_0, dx2_0, dy2_0, dx3_0, dy3_0, dx2_3, dy2_3, d3_0_2;
	double s1_q, t1_q, s2_q, t2_q, v2_q;
	double f2, f2_q;
	double x00t, y00t, x0tt, y0tt, xttt, yttt, x1tt, y1tt, x11t, y11t;

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
	nr_pixblock_draw_line_rgba32 (pb,
				      (int) floor (x0 + 0.5), (int) floor (y0 + 0.5),
				      (int) floor (x3 + 0.5), (int) floor (y3 + 0.5),
				      first, rgba);
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

	nr_pixblock_draw_curve_rgba32 (pb, x0, y0, x00t, y00t, x0tt, y0tt, xttt, yttt, first, rgba);
	nr_pixblock_draw_curve_rgba32 (pb, xttt, yttt, x1tt, y1tt, x11t, y11t, x3, y3, 0, rgba);
}

void
nr_pixblock_draw_path_rgba32 (NRPixBlock *pb, NRPath *path, NRMatrixF *transform, unsigned int rgba)
{
	double x, y, sx, sy, spsx, spsy;
	unsigned int sidx;

	x = y = 0.0;
	sx = sy = 0.0;

	for (sidx = 0; sidx < path->nelements; sidx += path->elements[sidx].code.length) {
		NRPathElement *sel;
		unsigned int idx, first;
		/* Start new path */
		sel = path->elements + sidx;
		if (transform) {
			spsx = sx = x = NR_MATRIX_DF_TRANSFORM_X (transform, sel[1].value, sel[2].value);
			spsy = sy = y = NR_MATRIX_DF_TRANSFORM_Y (transform, sel[1].value, sel[2].value);
		} else {
			spsx = sx = x = sel[1].value;
			spsy = sy = y = sel[2].value;
		}
		first = !sel[0].code.closed;
		idx = 3;
		while (idx < sel[0].code.length) {
			int nmulti, i;
			nmulti = sel[idx].code.length;
			if (sel[idx].code.code == NR_PATH_LINETO) {
				idx += 1;
				for (i = 0; i < nmulti; i++) {
					if (transform) {
						x = NR_MATRIX_DF_TRANSFORM_X (transform, sel[idx].value, sel[idx + 1].value);
						y = NR_MATRIX_DF_TRANSFORM_Y (transform, sel[idx].value, sel[idx + 1].value);
					} else {
						x = sel[idx].value;
						y = sel[idx + 1].value;
					}
					nr_pixblock_draw_line_rgba32 (pb, sx, sy, x, y, first, rgba);
					first = 0;
					sx = x;
					sy = y;
					idx += 2;
				}
			} else {
				idx += 1;
				for (i = 0; i < nmulti; i++) {
					if (transform) {
						double x1, y1, x2, y2;
						x = NR_MATRIX_DF_TRANSFORM_X (transform, sel[idx + 4].value, sel[idx + 5].value);
						y = NR_MATRIX_DF_TRANSFORM_Y (transform, sel[idx + 4].value, sel[idx + 5].value);
						x1 = NR_MATRIX_DF_TRANSFORM_X (transform, sel[idx].value, sel[idx + 1].value);
						y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, sel[idx].value, sel[idx + 1].value);
						x2 = NR_MATRIX_DF_TRANSFORM_X (transform, sel[idx + 2].value, sel[idx + 3].value);
						y2 = NR_MATRIX_DF_TRANSFORM_Y (transform, sel[idx + 2].value, sel[idx + 3].value);
						nr_pixblock_draw_curve_rgba32 (pb, sx, sy, x1, y1, x2, y2, x, y, first, rgba);
					} else {
						x = sel[idx + 4].value;
						y = sel[idx + 5].value;
						nr_pixblock_draw_curve_rgba32 (pb,
									       sx, sy,
									       sel[idx].value, sel[idx + 1].value,
									       sel[idx + 2].value, sel[idx + 3].value,
									       x, y, first, rgba);
					}
					idx += 6;
					sx = x;
					sy = y;
					first = 0;
				}
			}
		}
		/* Close if needed */
		if (sel[0].code.closed && ((x != spsx) || (y != spsy))) {
			nr_pixblock_draw_line_rgba32 (pb, x, y, spsx, spsy, first, rgba);
		}
	}
}


