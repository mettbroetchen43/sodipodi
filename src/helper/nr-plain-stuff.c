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
#include <time.h>
#include "nr-plain-stuff.h"

#define NR_DEFAULT_CHECKERSIZEP2 3
#define NR_DEFAULT_CHECKERCOLOR0 0xbfbfbfff
#define NR_DEFAULT_CHECKERCOLOR1 0x7f7f7fff

/* Pixel ops - FINAL DST SRC */

static void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);
static void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha);

/* Pixel ops - FINAL DST SRC MASK */

static void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs);
static void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs);
static void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs);
static void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs);
static void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_A8 (guchar *p, gint w, gint h, gint rs, const guchar *s, gint srs, const guchar *m, gint mrs);
static void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P_A8 (guchar *p, gint w, gint h, gint rs, const guchar *s, gint srs, const guchar *m, gint mrs);
static void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_A8 (guchar *p, gint w, gint h, gint rs, const guchar *s, gint srs, const guchar *m, gint mrs);
static void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P_A8 (guchar *p, gint w, gint h, gint rs, const guchar *s, gint srs, const guchar *m, gint mrs);

static void nr_R8G8B8A8_N_EMPTY_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba);
static void nr_R8G8B8A8_P_EMPTY_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba);
static void nr_R8G8B8A8_N_R8G8B8A8_N_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba);
static void nr_R8G8B8A8_P_R8G8B8A8_P_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba);

static void nr_R8G8B8_R8G8B8_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs);
static void nr_R8G8B8_R8G8B8_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs);

#if 0
static void nr_R8G8B8A8_P_R8G8B8A8_N_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs);
static void nr_R8G8B8A8_P_R8G8B8A8_N_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs);
#endif

void
nr_render_buf_buf (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy)
{
	guchar *dpx, *spx;

	g_return_if_fail (d != NULL);
	g_return_if_fail (s != NULL);
	g_return_if_fail (s->mode == NR_IMAGE_R8G8B8A8);
	g_return_if_fail (d->mode == NR_IMAGE_R8G8B8A8);

	/*
	 * Possible variants:
	 *
	 * 0. SRC EP - DST EP *
	 * 1. SRC EP - DST EN *
	 * 2. SRC EP - DST  P *
	 * 3. SRC EP - DST  N *
	 * 4. SRC EN - DST EP *
	 * 5. SRC EN - DST EN *
	 * 6. SRC EN - DST  P *
	 * 7. SRC EN - DST  N *
	 * 8. SRC  P - DST EP *
	 * 9. SRC  P - DST EN *
	 * A. SRC  P - DST  P *
	 * B. SRC  P - DST  N *
	 * C. SRC  N - DST EP *
	 * D. SRC  N - DST EN *
	 * E. SRC  N - DST  P *
	 * F. SRC  N - DST  N *
	 *
	 */

	/* Cases 0 - 7 NOP */
	if (s->empty) return;

	/* Following need clipping */
	/* Clip start */
	if (x < 0) {
		sx -= x;
		x = 0;
	}
	if (y < 0) {
		sy -= y;
		y = 0;
	}
	if (sx < 0) {
		x -= sx;
		sx = 0;
	}
	if (sy < 0) {
		y -= sy;
		sy = 0;
	}
	if ((x >= d->w) || (y >= d->h) || (sx >= s->w) || (sy >= s->h)) return;
	/* Clip size */
	if ((x + w) > d->w) {
		w = d->w - x;
	}
	if ((y + h) > d->h) {
		h = d->h - y;
	}
	if ((sx + w) > s->w) {
		w = s->w - sx;
	}
	if ((sy + h) > s->h) {
		h = s->h - sy;
	}
	if ((w <= 0) || (h <= 0)) return;
	
	/* Pointers */
	dpx = d->px + y * d->rs + 4 * x;
	spx = s->px + sy * s->rs + 4 * sx;

	if (d->empty) {
		if (d->premul) {
			if (s->premul) {
				/* Case 8 */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* Case C */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		} else {
			if (s->premul) {
				/* Case 9 */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* Case D */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		}
		d->empty = FALSE;
	} else {
		if (d->premul) {
			if (s->premul) {
				/* case A */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* case E */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		} else {
			if (s->premul) {
				/* case B */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* case F */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		}
	}
}

void
nr_render_buf_buf_alpha (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy, guint alpha)
{
	guchar *dpx, *spx;

	g_return_if_fail (d != NULL);
	g_return_if_fail (s != NULL);
	g_return_if_fail (s->mode == NR_IMAGE_R8G8B8A8);
	g_return_if_fail (d->mode == NR_IMAGE_R8G8B8A8);

	if (alpha == 0) return;

	/*
	 * Possible variants:
	 *
	 * 0. SRC EP - DST EP *
	 * 1. SRC EP - DST EN *
	 * 2. SRC EP - DST  P *
	 * 3. SRC EP - DST  N *
	 * 4. SRC EN - DST EP *
	 * 5. SRC EN - DST EN *
	 * 6. SRC EN - DST  P *
	 * 7. SRC EN - DST  N *
	 * 8. SRC  P - DST EP *
	 * 9. SRC  P - DST EN *
	 * A. SRC  P - DST  P *
	 * B. SRC  P - DST  N *
	 * C. SRC  N - DST EP *
	 * D. SRC  N - DST EN *
	 * E. SRC  N - DST  P *
	 * F. SRC  N - DST  N *
	 *
	 */

	/* Cases 0 - 7 NOP */
	if (s->empty) return;

	/* Following need clipping */
	/* Clip start */
	if (x < 0) {
		sx -= x;
		x = 0;
	}
	if (y < 0) {
		sy -= y;
		y = 0;
	}
	if (sx < 0) {
		x -= sx;
		sx = 0;
	}
	if (sy < 0) {
		y -= sy;
		sy = 0;
	}
	if ((x >= d->w) || (y >= d->h) || (sx >= s->w) || (sy >= s->h)) return;
	/* Clip size */
	if ((x + w) > d->w) {
		w = d->w - x;
	}
	if ((y + h) > d->h) {
		h = d->h - y;
	}
	if ((sx + w) > s->w) {
		w = s->w - sx;
	}
	if ((sy + h) > s->h) {
		h = s->h - sy;
	}
	if ((w <= 0) || (h <= 0)) return;
	
	/* Pointers */
	dpx = d->px + y * d->rs + 4 * x;
	spx = s->px + sy * s->rs + 4 * sx;

	if (d->empty) {
		if (d->premul) {
			if (s->premul) {
				/* Case 8 */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, alpha);
			} else {
				/* Case C */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, alpha);
			}
		} else {
			if (s->premul) {
				/* Case 9 */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, alpha);
			} else {
				/* Case D */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, alpha);
			}
		}
		d->empty = FALSE;
	} else {
		if (d->premul) {
			if (s->premul) {
				/* case A */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, alpha);
			} else {
				/* case E */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, alpha);
			}
		} else {
			if (s->premul) {
				/* case B */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, alpha);
			} else {
				/* case F */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, alpha);
			}
		}
	}
}

void
nr_render_buf_buf_mask (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy, NRBuffer *m, gint mx, gint my)
{
	guchar *dpx, *spx, *mpx;

	g_return_if_fail (d != NULL);
	g_return_if_fail (s != NULL);
	g_return_if_fail (m != NULL);
	g_return_if_fail (s->mode == NR_IMAGE_R8G8B8A8);
	g_return_if_fail (d->mode == NR_IMAGE_R8G8B8A8);
	g_return_if_fail (m->mode == NR_IMAGE_A8);

	if (m->empty) return;

	/*
	 * Possible variants:
	 *
	 * 0. SRC EP - DST EP *
	 * 1. SRC EP - DST EN *
	 * 2. SRC EP - DST  P *
	 * 3. SRC EP - DST  N *
	 * 4. SRC EN - DST EP *
	 * 5. SRC EN - DST EN *
	 * 6. SRC EN - DST  P *
	 * 7. SRC EN - DST  N *
	 * 8. SRC  P - DST EP *
	 * 9. SRC  P - DST EN *
	 * A. SRC  P - DST  P *
	 * B. SRC  P - DST  N *
	 * C. SRC  N - DST EP *
	 * D. SRC  N - DST EN *
	 * E. SRC  N - DST  P *
	 * F. SRC  N - DST  N *
	 *
	 */

	/* Cases 0 - 7 NOP */
	if (s->empty) return;

	/* Following need clipping */
	/* Clip start */
	if (x < 0) {
		sx -= x;
		mx -= x;
		x = 0;
	}
	if (y < 0) {
		sy -= y;
		my -= y;
		y = 0;
	}
	if (sx < 0) {
		x -= sx;
		mx -= sx;
		sx = 0;
	}
	if (sy < 0) {
		y -= sy;
		my -= sy;
		sy = 0;
	}
	if (mx < 0) {
		x -= mx;
		sx -= mx;
		mx = 0;
	}
	if (my < 0) {
		y -= my;
		sy -= my;
		my = 0;
	}
	if ((x >= d->w) || (y >= d->h) || (sx >= s->w) || (sy >= s->h) || (mx >= m->w) || (my >= m->h)) return;
	/* Clip size */
	if ((x + w) > d->w) {
		w = d->w - x;
	}
	if ((y + h) > d->h) {
		h = d->h - y;
	}
	if ((sx + w) > s->w) {
		w = s->w - sx;
	}
	if ((sy + h) > s->h) {
		h = s->h - sy;
	}
	if ((mx + w) > m->w) {
		w = m->w - mx;
	}
	if ((my + h) > m->h) {
		h = m->h - my;
	}
	if ((w <= 0) || (h <= 0)) return;
	
	/* Pointers */
	dpx = d->px + y * d->rs + 4 * x;
	spx = s->px + sy * s->rs + 4 * sx;
	mpx = m->px + my * m->rs + 1 * mx;

	if (d->empty) {
		if (d->premul) {
			if (s->premul) {
				/* Case 8 */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			} else {
				/* Case C */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			}
		} else {
			if (s->premul) {
				/* Case 9 */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			} else {
				/* Case D */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			}
		}
		d->empty = FALSE;
	} else {
		if (d->premul) {
			if (s->premul) {
				/* case A */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			} else {
				/* case E */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			}
		} else {
			if (s->premul) {
				/* case B */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			} else {
				/* case F */
				nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_A8 (dpx, w, h, d->rs, spx, s->rs, mpx, m->rs);
			}
		}
	}
}

void
nr_render_buf_mask_rgba32 (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *m, gint sx, gint sy, guint32 rgba)
{
	guchar *dpx, *mpx;

	g_return_if_fail (d != NULL);
	g_return_if_fail (m != NULL);
	g_return_if_fail (d->mode == NR_IMAGE_R8G8B8A8);
	g_return_if_fail (m->mode == NR_IMAGE_A8);

	/*
	 * Possible variants:
	 *
	 * 0. MSK  E - DST EP *
	 * 1. MSK  E - DST EN *
	 * 2. MSK  E - DST  P *
	 * 3. MSK  E - DST  N *
	 * 4. MSK  N - DST EP *
	 * 5. MSK  N - DST EN *
	 * 6. MSK  N - DST  P *
	 * 7. MSK  N - DST  N *
	 *
	 */

	/* Cases 0 - 3 NOP */
	if (m->empty) return;

	/* Following need clipping */
	/* Clip start */
	if (x < 0) {
		sx -= x;
		x = 0;
	}
	if (y < 0) {
		sy -= y;
		y = 0;
	}
	if (sx < 0) {
		x -= sx;
		sx = 0;
	}
	if (sy < 0) {
		y -= sy;
		sy = 0;
	}
	if ((x >= d->w) || (y >= d->h) || (sx >= m->w) || (sy >= m->h)) return;
	/* Clip size */
	if ((x + w) > d->w) {
		w = d->w - x;
	}
	if ((y + h) > d->h) {
		h = d->h - y;
	}
	if ((sx + w) > m->w) {
		w = m->w - sx;
	}
	if ((sy + h) > m->h) {
		h = m->h - sy;
	}
	if ((w <= 0) || (h <= 0)) return;
	
	/* Pointers */
	dpx = d->px + y * d->rs + 4 * x;
	mpx = m->px + sy * m->rs + sx;

	if (d->empty) {
		if (d->premul) {
			/* Case 4 */
			nr_R8G8B8A8_P_EMPTY_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
		} else {
			/* Case 5 */
			nr_R8G8B8A8_N_EMPTY_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
		}
		d->empty = FALSE;
	} else {
		if (d->premul) {
			/* Case 6 */
			nr_R8G8B8A8_P_R8G8B8A8_P_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
		} else {
			/* Case 7 */
			nr_R8G8B8A8_N_R8G8B8A8_N_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
		}
	}
}

void
nr_render_r8g8b8_buf (guchar *px, gint rs, gint w, gint h, NRBuffer *s, gint sx, gint sy)
{
	g_return_if_fail (px != NULL);
	g_return_if_fail (s != NULL);
	g_return_if_fail (s->mode == NR_IMAGE_R8G8B8A8);

	if (s->empty) return;

	/* Clip */
	if (sx < 0) {
		px -= 4 * sx;
		sx = 0;
	}
	if (sy < 0) {
		px -= sy * rs;
		sy = 0;
	}
	if ((sx >= s->w) || (sy >= s->h)) return;
	/* Clip size */
	if ((sx + w) > s->w) {
		w = s->w - sx;
	}
	if ((sy + h) > s->h) {
		h = s->h - sy;
	}
	if ((w <= 0) || (h <= 0)) return;

	if (s->premul) {
		nr_R8G8B8_R8G8B8_R8G8B8A8_P (px, w, h, rs, s->px + sy * s->rs + 4 * sx, s->rs);
	} else {
		nr_R8G8B8_R8G8B8_R8G8B8A8_N (px, w, h, rs, s->px + sy * s->rs + 4 * sx, s->rs);
	}
}

#define COMPOSE4(bc,fc,ba,fa,da) (((255 - fa) * (bc * ba) + fa * 255 * fc) / da)

static void
nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint x, y;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			d[3] = (s[3] * alpha + 127) / 255;
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint x, y;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			guint a;
			a = (s[3] * alpha + 127) / 255;
			if (a == 0) {
				d[3] = 0;
			} else {
				d[0] = (s[0] * 255 + (a >> 1)) / a;
				d[1] = (s[1] * 255 + (a >> 1)) / a;
				d[2] = (s[2] * 255 + (a >> 1)) / a;
				d[3] = a;
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			guint a;
			a = (s[3] * alpha + 127) / 255;
			d[0] = (s[0] * a + 127) / 255;
			d[1] = (s[1] * a + 127) / 255;
			d[2] = (s[2] * a + 127) / 255;
			d[3] = a;
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			if (alpha == 255) {
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = s[3];
			} else {
				d[0] = PREMUL (s[0], alpha);
				d[1] = PREMUL (s[1], alpha);
				d[2] = PREMUL (s[2], alpha);
				d[3] = PREMUL (s[3], alpha);
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], alpha);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = a;
			} else {
				guint ca;
				/* Full composition */
				ca = 65025 - (255 - a) * (255 - d[3]);
				d[0] = COMPOSENNN_A7 (s[0], a, d[0], d[3], ca);
				d[1] = COMPOSENNN_A7 (s[1], a, d[1], d[3], ca);
				d[2] = COMPOSENNN_A7 (s[2], a, d[2], d[3], ca);
				d[3] = (ca + 127) / 255;
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], alpha);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Full coverage, demul src */
				d[0] = (s[0] * 255 + (s[3] >> 1)) / s[3];
				d[1] = (s[1] * 255 + (s[3] >> 1)) / s[3];
				d[2] = (s[2] * 255 + (s[3] >> 1)) / s[3];
				d[3] = a;
			} else {
				if (alpha == 255) {
					guint ca;
					/* Full composition */
					ca = 65025 - (255 - s[3]) * (255 - d[3]);
					d[0] = COMPOSEPNN_A7 (s[0], s[3], d[0], d[3], ca);
					d[1] = COMPOSEPNN_A7 (s[1], s[3], d[1], d[3], ca);
					d[2] = COMPOSEPNN_A7 (s[2], s[3], d[2], d[3], ca);
					d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				} else {
					guint ca, c;
					/* fixme: Full composition */
					ca = 65025 - (255 - s[3]) * (255 - d[3]);
					c = PREMUL (s[0], alpha);
					d[0] = COMPOSEPNN_A7 (c, a, d[0], d[3], ca);
					c = PREMUL (s[1], alpha);
					d[1] = COMPOSEPNN_A7 (c, a, d[1], d[3], ca);
					c = PREMUL (s[2], alpha);
					d[2] = COMPOSEPNN_A7 (c, a, d[2], d[3], ca);
					d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
				}
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], alpha);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Transparent BG, premul src */
				d[0] = PREMUL (s[0], a);
				d[1] = PREMUL (s[1], a);
				d[2] = PREMUL (s[2], a);
				d[3] = a;
			} else {
				d[0] = COMPOSENPP (s[0], a, d[0], d[3]);
				d[1] = COMPOSENPP (s[1], a, d[1], d[3]);
				d[2] = COMPOSENPP (s[2], a, d[2], d[3]);
				d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint alpha)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], alpha);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Transparent BG, COPY */
				d[0] = PREMUL (s[0], alpha);
				d[1] = PREMUL (s[1], alpha);
				d[2] = PREMUL (s[2], alpha);
				d[3] = PREMUL (s[3], alpha);
			} else {
				if (alpha == 255) {
					/* Simple */
					d[0] = COMPOSEPPP (s[0], s[3], d[0], d[3]);
					d[1] = COMPOSEPPP (s[1], s[3], d[1], d[3]);
					d[2] = COMPOSEPPP (s[2], s[3], d[2], d[3]);
					d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				} else {
					guint c;
					c = PREMUL (s[0], alpha);
					d[0] = COMPOSEPPP (c, a, d[0], d[3]);
					c = PREMUL (s[1], alpha);
					d[1] = COMPOSEPPP (c, a, d[1], d[3]);
					c = PREMUL (s[2], alpha);
					d[2] = COMPOSEPPP (c, a, d[2], d[3]);
					d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
				}
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

/* Masked operations */

static void
nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint x, y;

	for (y = 0; y < h; y++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (x = 0; x < w; x++) {
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			d[3] = (s[3] * m[0] + 127) / 255;
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint x, y;

	for (y = 0; y < h; y++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (x = 0; x < w; x++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			if (a == 0) {
				d[3] = 0;
			} else {
				d[0] = (s[0] * 255 + (a >> 1)) / a;
				d[1] = (s[1] * 255 + (a >> 1)) / a;
				d[2] = (s[2] * 255 + (a >> 1)) / a;
				d[3] = a;
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			d[0] = PREMUL (s[0], a);
			d[1] = PREMUL (s[1], a);
			d[2] = PREMUL (s[2], a);
			d[3] = a;
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			if (m[0] == 255) {
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = s[3];
			} else {
				d[0] = PREMUL (s[0], m[0]);
				d[1] = PREMUL (s[1], m[0]);
				d[2] = PREMUL (s[2], m[0]);
				d[3] = PREMUL (s[3], m[0]);
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = a;
			} else {
				guint ca;
				/* Full composition */
				ca = 65025 - (255 - a) * (255 - d[3]);
				d[0] = COMPOSENNN_A7 (s[0], a, d[0], d[3], ca);
				d[1] = COMPOSENNN_A7 (s[1], a, d[1], d[3], ca);
				d[2] = COMPOSENNN_A7 (s[2], a, d[2], d[3], ca);
				d[3] = (ca + 127) / 255;
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Full coverage, demul src */
				d[0] = (s[0] * 255 + (s[3] >> 1)) / s[3];
				d[1] = (s[1] * 255 + (s[3] >> 1)) / s[3];
				d[2] = (s[2] * 255 + (s[3] >> 1)) / s[3];
				d[3] = a;
			} else {
				if (m[0] == 255) {
					guint ca;
					/* Full composition */
					ca = 65025 - (255 - s[3]) * (255 - d[3]);
					d[0] = COMPOSEPNN_A7 (s[0], s[3], d[0], d[3], ca);
					d[1] = COMPOSEPNN_A7 (s[1], s[3], d[1], d[3], ca);
					d[2] = COMPOSEPNN_A7 (s[2], s[3], d[2], d[3], ca);
					d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				} else {
					guint ca, c;
					/* fixme: Full composition */
					ca = 65025 - (255 - s[3]) * (255 - d[3]);
					c = PREMUL (s[0], m[0]);
					d[0] = COMPOSEPNN_A7 (c, a, d[0], d[3], ca);
					c = PREMUL (s[1], m[0]);
					d[1] = COMPOSEPNN_A7 (c, a, d[1], d[3], ca);
					c = PREMUL (s[2], m[0]);
					d[2] = COMPOSEPNN_A7 (c, a, d[2], d[3], ca);
					d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
				}
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Transparent BG, premul src */
				d[0] = PREMUL (s[0], a);
				d[1] = PREMUL (s[1], a);
				d[2] = PREMUL (s[2], a);
				d[3] = a;
			} else {
				d[0] = COMPOSENPP (s[0], a, d[0], d[3]);
				d[1] = COMPOSENPP (s[1], a, d[1], d[3]);
				d[2] = COMPOSENPP (s[2], a, d[2], d[3]);
				d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P_A8 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, const guchar *mpx, gint mrs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s, *m;
		d = (guchar *) px;
		s = (guchar *) spx;
		m = (guchar *) mpx;
		for (c = 0; c < w; c++) {
			guint a;
			a = PREMUL (s[3], m[0]);
			if (a == 0) {
				/* Transparent FG, NOP */
			} else if ((a == 255) || (d[3] == 0)) {
				/* Transparent BG, COPY */
				d[0] = PREMUL (s[0], m[0]);
				d[1] = PREMUL (s[1], m[0]);
				d[2] = PREMUL (s[2], m[0]);
				d[3] = PREMUL (s[3], m[0]);
			} else {
				if (m[0] == 255) {
					/* Simple */
					d[0] = COMPOSEPPP (s[0], s[3], d[0], d[3]);
					d[1] = COMPOSEPPP (s[1], s[3], d[1], d[3]);
					d[2] = COMPOSEPPP (s[2], s[3], d[2], d[3]);
					d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				} else {
					guint c;
					c = PREMUL (s[0], m[0]);
					d[0] = COMPOSEPPP (c, a, d[0], d[3]);
					c = PREMUL (s[1], m[0]);
					d[1] = COMPOSEPPP (c, a, d[1], d[3]);
					c = PREMUL (s[2], m[0]);
					d[2] = COMPOSEPPP (c, a, d[2], d[3]);
					d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
				}
			}
			d += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		spx += srs;
		mpx += mrs;
	}
}


#if 0
static void
nr_R8G8B8A8_P_R8G8B8A8_N_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			if (s[3] == 0) {
				/* Transparent FG, premul dst */
				d[0] = PREMUL (d[0], d[3]);
				d[1] = PREMUL (d[1], d[3]);
				d[2] = PREMUL (d[2], d[3]);
			} else if ((s[3] == 255) || (d[3] == 0)) {
				/* Transparent BG, premul src */
				d[0] = PREMUL (s[0], s[3]);
				d[1] = PREMUL (s[1], s[3]);
				d[2] = PREMUL (s[2], s[3]);
				d[3] = s[3];
			} else {
				d[0] = COMPOSENNP (s[0], s[3], d[0], d[3]);
				d[1] = COMPOSENNP (s[1], s[3], d[1], d[3]);
				d[2] = COMPOSENNP (s[2], s[3], d[2], d[3]);
				d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_N_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			if (s[3] == 0) {
				/* Transparent FG, premul dst */
				d[0] = PREMUL (d[0], d[3]);
				d[1] = PREMUL (d[1], d[3]);
				d[2] = PREMUL (d[2], d[3]);
			} else if ((s[3] == 255) || (d[3] == 0)) {
				/* Transparent BG, copy src */
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = s[3];
			} else {
				d[0] = COMPOSEPNP (s[0], s[3], d[0], d[3]);
				d[1] = COMPOSEPNP (s[1], s[3], d[1], d[3]);
				d[2] = COMPOSEPNP (s[2], s[3], d[2], d[3]);
				d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}
#endif

static void
nr_R8G8B8_R8G8B8_R8G8B8A8_P (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			if (s[3] == 0) {
				/* Transparent FG, NOP */
			} else if (s[3] == 255) {
				/* Opaque FG, COPY */
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
			} else {
				d[0] = COMPOSEP11 (s[0], s[3], d[0]);
				d[1] = COMPOSEP11 (s[1], s[3], d[1]);
				d[2] = COMPOSEP11 (s[2], s[3], d[2]);
			}
			d += 3;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8_R8G8B8_R8G8B8A8_N (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs)
{
	guint r, c;

	for (r = 0; r < h; r++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (c = 0; c < w; c++) {
			if (s[3] == 0) {
				/* Transparent FG, NOP */
			} else if (s[3] == 255) {
				/* Opaque FG, COPY */
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
			} else {
				d[0] = COMPOSEN11 (s[0], s[3], d[0]);
				d[1] = COMPOSEN11 (s[1], s[3], d[1]);
				d[2] = COMPOSEN11 (s[2], s[3], d[2]);
			}
			d += 3;
			s += 4;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_N_EMPTY_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba)
{
	guint r, g, b, a;
	guint x, y;

	r = NR_RGBA32_R (rgba);
	g = NR_RGBA32_G (rgba);
	b = NR_RGBA32_B (rgba);
	a = NR_RGBA32_A (rgba);

	if (a == 0) return;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			d[0] = r;
			d[1] = g;
			d[2] = b;
			d[3] = PREMUL (s[0], a);
			d += 4;
			s += 1;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_EMPTY_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba)
{
	guint r, g, b, a;
	guint x, y;

	r = NR_RGBA32_R (rgba);
	g = NR_RGBA32_G (rgba);
	b = NR_RGBA32_B (rgba);
	a = NR_RGBA32_A (rgba);

	if (a == 0) return;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			guint ca;
			ca = s[0] * a;
			d[0] = (r * ca + 32512) / 65025;
			d[1] = (g * ca + 32512) / 65025;
			d[2] = (b * ca + 32512) / 65025;
			d[3] = (ca + 127) / 255;
			d += 4;
			s += 1;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_N_R8G8B8A8_N_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba)
{
	guint r, g, b, a;
	guint x, y;

	r = NR_RGBA32_R (rgba);
	g = NR_RGBA32_G (rgba);
	b = NR_RGBA32_B (rgba);
	a = NR_RGBA32_A (rgba);

	if (a == 0) return;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			guint ca;
			ca = PREMUL (s[0], a);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if ((ca == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = r;
				d[1] = g;
				d[2] = b;
				d[3] = ca;
			} else {
				guint da;
				/* Full composition */
				da = 65025 - (255 - ca) * (255 - d[3]);
				d[0] = COMPOSENNN_A7 (r, ca, d[0], d[3], da);
				d[1] = COMPOSENNN_A7 (g, ca, d[1], d[3], da);
				d[2] = COMPOSENNN_A7 (b, ca, d[2], d[3], da);
				d[3] = (da + 127) / 255;
			}
			d += 4;
			s += 1;
		}
		px += rs;
		spx += srs;
	}
}

static void
nr_R8G8B8A8_P_R8G8B8A8_P_A8_RGBA32 (guchar *px, gint w, gint h, gint rs, const guchar *spx, gint srs, guint32 rgba)
{
	guint r, g, b, a;
	guint x, y;

	r = NR_RGBA32_R (rgba);
	g = NR_RGBA32_G (rgba);
	b = NR_RGBA32_B (rgba);
	a = NR_RGBA32_A (rgba);

	if (a == 0) return;

	for (y = 0; y < h; y++) {
		guchar *d, *s;
		d = (guchar *) px;
		s = (guchar *) spx;
		for (x = 0; x < w; x++) {
			guint ca;
			ca = PREMUL (s[0], a);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if ((ca == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = PREMUL (r, ca);
				d[1] = PREMUL (g, ca);
				d[2] = PREMUL (b, ca);
				d[3] = ca;
			} else {
				/* Full composition */
				d[0] = COMPOSENPP (r, ca, d[0], d[3]);
				d[1] = COMPOSENPP (g, ca, d[1], d[3]);
				d[2] = COMPOSENPP (b, ca, d[2], d[3]);
				d[3] = (65025 - (255 - ca) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			s += 1;
		}
		px += rs;
		spx += srs;
	}
}

void
nr_render_checkerboard_rgb (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff)
{
	g_return_if_fail (px != NULL);

	nr_render_checkerboard_rgb_custom (px, w, h, rs, xoff, yoff, NR_DEFAULT_CHECKERCOLOR0, NR_DEFAULT_CHECKERCOLOR1, NR_DEFAULT_CHECKERSIZEP2);
}

void
nr_render_checkerboard_rgb_custom (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff, guint32 c0, guint32 c1, gint sizep2)
{
	gint x, y, m;
	guint r0, g0, b0;
	guint r1, g1, b1;

	g_return_if_fail (px != NULL);
	g_return_if_fail (sizep2 >= 0);
	g_return_if_fail (sizep2 <= 8);

	xoff &= 0x1ff;
	yoff &= 0x1ff;
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
			if (((x + xoff) ^ (y + yoff)) & m) {
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
nr_render_rgba32_rgb (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff, guint32 c)
{
	guint32 c0, c1;
	gint a, r, g, b, cr, cg, cb;

	g_return_if_fail (px != NULL);

	r = NR_RGBA32_R (c);
	g = NR_RGBA32_G (c);
	b = NR_RGBA32_B (c);
	a = NR_RGBA32_A (c);

	cr = COMPOSEN11 (r, a, NR_RGBA32_R (NR_DEFAULT_CHECKERCOLOR0));
	cg = COMPOSEN11 (g, a, NR_RGBA32_G (NR_DEFAULT_CHECKERCOLOR0));
	cb = COMPOSEN11 (b, a, NR_RGBA32_B (NR_DEFAULT_CHECKERCOLOR0));
	c0 = (cr << 24) | (cg << 16) | (cb << 8) | 0xff;

	cr = COMPOSEN11 (r, a, NR_RGBA32_R (NR_DEFAULT_CHECKERCOLOR1));
	cg = COMPOSEN11 (g, a, NR_RGBA32_G (NR_DEFAULT_CHECKERCOLOR1));
	cb = COMPOSEN11 (b, a, NR_RGBA32_B (NR_DEFAULT_CHECKERCOLOR1));
	c1 = (cr << 24) | (cg << 16) | (cb << 8) | 0xff;

	nr_render_checkerboard_rgb_custom (px, w, h, rs, xoff, yoff, c0, c1, NR_DEFAULT_CHECKERSIZEP2);
}

#define COMPOSE4(bc,fc,ba,fa,da) (((255 - fa) * (bc * ba) + fa * 255 * fc) / da)

void
nr_render_rgba32_rgba32 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs)
{
	guint x, y;

	g_return_if_fail (px != NULL);
	g_return_if_fail (src != NULL);

	for (y = 0; y < h; y++) {
		const guchar *s;
		guchar *p;
		p = px;
		s = src;
		for (x = 0; x < w; x++) {
			if (s[3] == 0) {
				/* Do nothing */
			} else if ((s[3] == 255) || (p[3] == 0)) {
				/* Opaque */
				p[0] = s[0];
				p[1] = s[1];
				p[2] = s[2];
				p[3] = s[3];
			} else {
				guint da;
				/* Full composition */
				da = 65025 - (255 - s[3]) * (255 - p[3]);
				p[0] = COMPOSE4 (p[0], s[0], p[3], s[3], da);
				p[1] = COMPOSE4 (p[1], s[1], p[3], s[3], da);
				p[2] = COMPOSE4 (p[2], s[2], p[3], s[3], da);
				p[3] = (da + 0x80) / 255;
			}
			p += 4;
			s += 4;
		}
		px += rs;
		src += srcrs;
	}
}

void
nr_render_r8g8b8a8_r8g8b8a8_alpha (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs, guint alpha)
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
			guint fa;
			fa = (s[3] * alpha + 127) / 255;
			if (fa == 0) {
				/* Do nothing */
			} else if ((fa == 255) || (p[3] == 0)) {
				/* Opaque */
				p[0] = s[0];
				p[1] = s[1];
				p[2] = s[2];
				p[3] = fa;
			} else {
				guint da;
				/* Full composition */
				da = 65025 - (255 - fa) * (255 - p[3]);
				p[0] = COMPOSE4 (p[0], s[0], p[3], fa, da);
				p[1] = COMPOSE4 (p[1], s[1], p[3], fa, da);
				p[2] = COMPOSE4 (p[2], s[2], p[3], fa, da);
				p[3] = (da + 0x80) / 255;
			}
			p += 4;
			s += 4;
		}
		px += rs;
		src += srcrs;
	}
}

void
nr_render_r8g8b8a8_rgba32_mask_a8 (guchar *px, gint w, gint h, gint rs, guint32 rgba, const guchar *src, gint srcrs)
{
	guint r, g, b, a;
	guint x, y;

	g_return_if_fail (px != NULL);
	g_return_if_fail (src != NULL);

	r = NR_RGBA32_R (rgba);
	g = NR_RGBA32_G (rgba);
	b = NR_RGBA32_B (rgba);
	a = NR_RGBA32_A (rgba);

	if (a == 0) return;

	for (y = 0; y < h; y++) {
		const guchar *s;
		guchar *p;
		p = px;
		s = src;
		for (x = 0; x < w; x++) {
			guint fa;
			fa = (s[0] * a + 127) / 255;
			if (fa == 0) {
				/* Do nothing */
			} else if (fa == 255) {
				/* Opaque */
				p[0] = r;
				p[1] = g;
				p[2] = b;
				p[3] = 255;
			} else {
				guint da;
				/* Full composition */
				da = 65025 - (255 - fa) * (255 - p[3]); /* 255 * da */
				p[0] = ((255 - fa) * (p[0] * p[3]) + fa * 255 * r) / da;
				p[1] = ((255 - fa) * (p[1] * p[3]) + fa * 255 * g) / da;
				p[2] = ((255 - fa) * (p[2] * p[3]) + fa * 255 * b) / da;
				p[3] = da / 255;
			}
			p += 4;
			s += 1;
		}
		px += rs;
		src += srcrs;
	}
}

void
nr_render_r8g8b8a8_r8g8b8a8_mask_a8 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs, const guchar *mask, gint maskrs)
{
	gint x, y;

	g_return_if_fail (px != NULL);
	g_return_if_fail (src != NULL);

	for (y = 0; y < h; y++) {
		const guchar *s, *m;
		guchar *p;
		p = px;
		s = src;
		m = mask;
		for (x = 0; x < w; x++) {
			guint fa;
			fa = (s[3] * mask[0] + 127) / 255;
			if (fa == 0) {
				/* Do nothing */
			} else if ((fa == 255) || (p[0] == 0)) {
				/* Opaque */
				p[0] = s[0];
				p[1] = s[1];
				p[2] = s[2];
				p[3] = fa;
			} else {
				guint da;
				/* Full composition */
				da = 65025 - (255 - fa) * (255 - p[3]);
				p[0] = COMPOSE4 (p[0], s[0], p[3], fa, da);
				p[1] = COMPOSE4 (p[1], s[1], p[3], fa, da);
				p[2] = COMPOSE4 (p[2], s[2], p[3], fa, da);
				p[3] = (da + 0x80) / 255;
			}
			p += 4;
			s += 4;
			m += 1;
		}
		px += rs;
		src += srcrs;
	}
}

void
nr_render_r8g8b8_r8g8b8a8 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs)
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
			p[0] = ((255 - s[3]) * p[0] + (s[0] * s[3]) + 127) / 255;
			p[1] = ((255 - s[3]) * p[1] + (s[1] * s[3]) + 127) / 255;
			p[2] = ((255 - s[3]) * p[2] + (s[2] * s[3]) + 127) / 255;
			p += 3;
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

void
nr_render_r8g8b8a8_gray_garbage (guchar *px, gint w, gint h, gint rs)
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
			*p++ = 0xff;
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

