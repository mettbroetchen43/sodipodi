#define __NR_PLAIN_STUFF_C__

/*
 * Miscellaneous simple rendering utilities
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <stdlib.h>
#include <sys/time.h>
#include "nr-plain-stuff.h"

#define NR_DEFAULT_CHECKERSIZEP2 3
#define NR_DEFAULT_CHECKERCOLOR0 0xbfbfbfff
#define NR_DEFAULT_CHECKERCOLOR1 0x7f7f7fff

#define NR_RGBA32_R(v) ((v) >> 24)
#define NR_RGBA32_G(v) (((v) >> 16) & 0xff)
#define NR_RGBA32_B(v) (((v) >> 8) & 0xff)
#define NR_RGBA32_A(v) ((v) & 0xff)

void
nr_render_checkerboard_rgb (guchar *px, gint w, gint h, gint rs)
{
	g_return_if_fail (px != NULL);

	nr_render_checkerboard_rgb_custom (px, w, h, rs, NR_DEFAULT_CHECKERCOLOR0, NR_DEFAULT_CHECKERCOLOR1, NR_DEFAULT_CHECKERSIZEP2);
}

void
nr_render_checkerboard_rgb_custom (guchar *px, gint w, gint h, gint rs, guint32 c0, guint32 c1, gint sizep2)
{
	gint x, y, m;
	guint r0, g0, b0;
	guint r1, g1, b1;

	g_return_if_fail (px != NULL);
	g_return_if_fail (sizep2 >= 0);
	g_return_if_fail (sizep2 <= 8);

	m = 0x1 << sizep2;
	r0 = NR_RGBA32_R (c0);
	g0 = NR_RGBA32_G (c0);
	b0 = NR_RGBA32_B (c0);
	r1 = NR_RGBA32_R (c1);
	g1 = NR_RGBA32_G (c1);
	b1 = NR_RGBA32_B (c1);

	for (y = 0; y < h; y++) {
		guchar *p;
		p = px;
		for (x = 0; x < w; x++) {
			if ((x ^ y) & m) {
				*p++ = r0;
				*p++ = g0;
				*p++ = b0;
			} else {
				*p++ = r1;
				*p++ = g1;
				*p++ = b1;
			}
		}
		px += rs;
	}
}

void
nr_render_rgba32_rgb (guchar *px, gint w, gint h, gint rs, guint32 c)
{
	guint32 c0, c1;
	gint a, r, g, b, cr, cg, cb;

	g_return_if_fail (px != NULL);

	r = NR_RGBA32_R (c);
	g = NR_RGBA32_G (c);
	b = NR_RGBA32_B (c);
	a = NR_RGBA32_A (c);

	cr = r + (r - NR_RGBA32_R (NR_DEFAULT_CHECKERCOLOR0)) * a / 255;
	cg = g + (g - NR_RGBA32_G (NR_DEFAULT_CHECKERCOLOR0)) * a / 255;
	cb = b + (b - NR_RGBA32_B (NR_DEFAULT_CHECKERCOLOR0)) * a / 255;
	c0 = (r << 24) | (g << 16) | (b << 8) | 0xff;

	cr = r + (r - NR_RGBA32_R (NR_DEFAULT_CHECKERCOLOR1)) * a / 255;
	cg = g + (g - NR_RGBA32_G (NR_DEFAULT_CHECKERCOLOR1)) * a / 255;
	cb = b + (b - NR_RGBA32_B (NR_DEFAULT_CHECKERCOLOR1)) * a / 255;
	c1 = (r << 24) | (g << 16) | (b << 8) | 0xff;

	nr_render_checkerboard_rgb_custom (px, w, h, rs, c0, c1, NR_DEFAULT_CHECKERSIZEP2);
}

void
nr_render_rgba32_rgba32 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs)
{
	gint x, y;

	g_return_if_fail (px != NULL);
	g_return_if_fail (src != NULL);

	for (y = 0; y < h; y++) {
		const guchar *s;
		guchar *p;
		p = px;
		s = src;
		for (x = 0; x < w; x++) {
			p[0] = p[0] + (p[0] - s[0]) * s[3] / 255;
			p[1] = p[1] + (p[1] - s[1]) * s[3] / 255;
			p[2] = p[2] + (p[2] - s[2]) * s[3] / 255;
			p[3] = p[3] + (0xff - p[3]) * s[3] / 255;
			p += 4;
			s += 4;
		}
		px += rs;
		src += srcrs;
	}
}

#define NR_GARBAGE_SIZE 1024

void
nr_render_gray_garbage_rgb (guchar *px, gint w, gint h, gint rs)
{
	static guchar *garbage = NULL;
	static guint seed = 0;
	gint x, y, v;

	g_return_if_fail (px != NULL);

	if (!garbage) {
		gint i;
		/* Create grabage buffer */
		garbage = g_new (guchar, NR_GARBAGE_SIZE);
		srand (time (NULL));
		for (i = 0; i < NR_GARBAGE_SIZE; i++) {
			garbage[i] = (rand () / (RAND_MAX >> 8)) & 0xff;
		}
	}

	v = (rand () / (RAND_MAX >> 8)) & 0xff;

	for (y = 0; y < h; y++) {
		guchar *p;
		p = px;
		for (x = 0; x < w; x++) {
			v = v ^ garbage[seed];
			*p++ = v;
			*p++ = v;
			*p++ = v;
			seed += 1;
			if (seed >= NR_GARBAGE_SIZE) {
				gint i;
				i = (rand () / (RAND_MAX / NR_GARBAGE_SIZE)) % NR_GARBAGE_SIZE;
				garbage[i] ^= v;
				seed = i % (NR_GARBAGE_SIZE >> 2);
			}
		}
		px += rs;
	}
}

