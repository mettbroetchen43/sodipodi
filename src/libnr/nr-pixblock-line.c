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

struct _NRPBPathData {
	NRPixBlock *pb;
	unsigned int rgba;
};

static unsigned int
nr_pb_path_draw_lineto (float x0, float y0, float x1, float y1, unsigned int flags, void *data)
{
	struct _NRPBPathData *pbdata;
	pbdata = (struct _NRPBPathData *) data;
	nr_pixblock_draw_line_rgba32 (pbdata->pb,
				      (int) floor (x0 + 0.5), (int) floor (y0 + 0.5),
				      (int) floor (x1 + 0.5), (int) floor (y1 + 0.5),
				      (flags & NR_PATH_FIRST) && !(flags & NR_PATH_CLOSED), pbdata->rgba);
	return TRUE;
}

static unsigned int
nr_pb_path_draw_endpath (float ex, float ey, float sx, float sy, unsigned int flags, void *data)
{
	struct _NRPBPathData *pbdata;
	pbdata = (struct _NRPBPathData *) data;
	if ((flags & NR_PATH_CLOSED) && ((ex != sx) || (ey != sy))) {
		nr_pixblock_draw_line_rgba32 (pbdata->pb,
					      (int) floor (ex + 0.5), (int) floor (ey + 0.5),
					      (int) floor (sx + 0.5), (int) floor (sy + 0.5),
					      FALSE, pbdata->rgba);
	}
	return TRUE;
}

static NRPathGVector pbpgv = {
	NULL,
	nr_pb_path_draw_lineto,
	NULL,
	NULL,
	nr_pb_path_draw_endpath
};

void
nr_pixblock_draw_path_rgba32 (NRPixBlock *pb, NRPath *path, NRMatrixF *transform, unsigned int rgba)
{
	struct _NRPBPathData pbdata;
	pbdata.pb = pb;
	pbdata.rgba = rgba;
	nr_path_forall_flat (path, transform, 0.5, &pbpgv, &pbdata);
}


