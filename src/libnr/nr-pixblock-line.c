#define __NR_PIXBLOCK_LINE_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <assert.h>

#include <libnr/nr-pixops.h>
#include <libnr/nr-pixblock-pixel.h>
#include <libnr/nr-pixblock-line.h>

/* #include <glib.h> */

void
nr_pixblock_draw_line_rgba32 (NRPixBlock *d, long x0, long y0, long x1, long y1, short first, unsigned long rgba)
{
	long deltax, deltay, xinc1, xinc2, yinc1, yinc2;
	long den, num0, num, numadd, numpixels;
	long x, y, curpixel;
	/* Pixblock */
	int dbpp;
	NRPixBlock spb;
	unsigned char *spx;
	unsigned int ytrash;

	if ((x0 < d->area.x0) && (x1 < d->area.x0)) return;
	if ((x0 >= d->area.x1) && (x1 >= d->area.x1)) return;
	if ((y0 < d->area.y0) && (y1 < d->area.y0)) return;
	if ((y0 >= d->area.y1) && (y1 >= d->area.y1)) return;

	if (x1 < x0) {
		int t;
		t = x1;
		x1 = x0;
		x0 = t;
		t = y1;
		y1 = y0;
		y0 = t;
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
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	/* v = v0 + ((num + pixels * numadd) / den) * vinc1 + pixels * vinc2 */
	/* pixels = ((v - v0) * den - num + (numadd - 1)) / numadd */

	if ((x0 < d->area.x0) && (x1 >= d->area.x0)) {
		int pixels;
		if (xinc1) {
			/* xinc1 = 1 */
			/* yinc1 = 0 */
			/* deltax <= deltay */
			/* numadd <= den */
			/* alpha = numinitial + pixels * numadd;
			 * area.x0 = x0 + (numinitial + pixels * numadd) / den;
			 * (numinitial + pixels * numadd) / den = area.x0 - x0;
			 * numinitial + pixels * numadd = (area.x0 - x0) * den [+ (alpha % den)];
			 * pixels * numadd = (area.x0 - x0) * den - numinitial + (numadd - 1);
			 * pixels = ((area.x0 - x0) * den - numinitial + (numadd - 1)) / numadd;
			 *
			 * num <- alpha % den;
			 */
			pixels = ((d->area.x0 - x0) * den - num + (numadd - 1)) / numadd;
		} else {
			/* area.x0 = x0 + pixels;
			 */
			pixels = d->area.x0 - x0;
		}
		x0 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
		y0 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
		if (x0 >= d->area.x1) return;
		num = (num + pixels * numadd) % den;
		numpixels -= pixels;
	}

	/* g_print ("line B %d %d %d %d\n", x0, y0, x1, y1); */

	if ((x0 < d->area.x1) && (x1 >= d->area.x1)) {
		int pixels;
		if (xinc1) {
			/* We need LAST pixel before step here */
			pixels = ((d->area.x1 - x0) * den - num + (numadd - 1)) / numadd - 1;
		} else {
			pixels = d->area.x1 - x0 - 1;
		}
		x1 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
		y1 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
		if (x1 <= d->area.x0) return;
		/* num = (num + pixels * numadd) % den; */
		numpixels = pixels;
	}

	/* g_print ("line C %d %d %d %d\n", x0, y0, x1, y1); */

	if ((y0 < d->area.y0) && (y1 < d->area.y0)) return;
	if ((y0 >= d->area.y1) && (y1 >= d->area.y1)) return;

	if (y1 >= y0) {
		if ((y0 < d->area.y0) && (y1 >= d->area.y0)) {
			int pixels;
			if (yinc1) {
				pixels = ((d->area.y0 - y0) * den - num + (numadd - 1)) / numadd;
			} else {
				pixels = d->area.y0 - y0;
			}
			x0 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y0 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x0 >= d->area.x1) return;
			if (y0 >= d->area.y1) return;
			num = (num + pixels * numadd) % den;
			numpixels -= pixels;
		}
		/* g_print ("line D %d %d %d %d\n", x0, y0, x1, y1); */
		if ((y0 < d->area.y1) && (y1 >= d->area.y1)) {
			int pixels;
			if (yinc1) {
				/* We need LAST pixel before step here */
				pixels = ((d->area.y1 - y0) * den - num + (numadd - 1)) / numadd - 1;
			} else {
				pixels = d->area.y1 - y0 - 1;
			}
			x1 = x0 + ((num + pixels * numadd) / den) * xinc1 + pixels * xinc2;
			y1 = y0 + ((num + pixels * numadd) / den) * yinc1 + pixels * yinc2;
			if (x1 <= d->area.x0) return;
			if (y1 <= d->area.y0) return;
			/* num = (num + pixels * numadd) % den; */
			numpixels = pixels;
		}
		/* g_print ("line E %d %d %d %d\n", x0, y0, x1, y1); */
		ytrash = 0;
	} else {
		ytrash = 1;
	}

	/* We can be quite sure 1x1 pixblock is TINY */
	nr_pixblock_setup_fast (&spb, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, 1, 1, 0);
	spb.empty = 0;
	spx = NR_PIXBLOCK_PX (&spb);
	spx[0] = NR_RGBA32_R (rgba);
	spx[1] = NR_RGBA32_G (rgba);
	spx[2] = NR_RGBA32_B (rgba);
	spx[3] = NR_RGBA32_A (rgba);

	dbpp = NR_PIXBLOCK_BPP (d);

	x = x0;
	y = y0;

	num0 = num;
	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
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
		assert (x >= d->area.x0);
		assert (x < d->area.x1);
		if (!ytrash) {
			assert (y >= d->area.y0);
			assert (y < d->area.y1);
		}
		if ((x >= d->area.x0) && (y >= d->area.y0) && (x < d->area.x1) && (y < d->area.y1)) {
			nr_compose_pixblock_pixblock_pixel (d, NR_PIXBLOCK_PX (d) + (y - d->area.y0) * d->rs + (x - d->area.x0) * dbpp, &spb, spx);
		}
		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}

	nr_pixblock_release (&spb);
}

