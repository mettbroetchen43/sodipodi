#define __NR_BLIT_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "nr-rect.h"
#include "nr-pixops.h"
#include "nr-compose.h"
#include "nr-blit.h"

void
nr_blit_pixblock_pixblock (NRPixBlock *d, NRPixBlock *s)
{
	NRRectS clip;
	unsigned char *dpx, *spx;
	int w, h;

	if (s->empty) return;
	/* fixme: */
	if (s->mode == NR_PIXBLOCK_MODE_A8) return;
	/* fixme: */
	if (s->mode == NR_PIXBLOCK_MODE_R8G8B8) return;

	/*
	 * Possible variants as of now:
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

	if (!nr_rect_s_test_intersect (&d->area, &s->area)) return;

	nr_rect_s_intersect (&clip, &d->area, &s->area);

	/* Pointers */
	dpx = NR_PIXBLOCK_PX (d) + (clip.y0 - d->area.y0) * d->rs + 4 * (clip.x0 - d->area.x0);
	spx = NR_PIXBLOCK_PX (s) + (clip.y0 - s->area.y0) * s->rs + 4 * (clip.x0 - s->area.x0);
	w = clip.x1 - clip.x0;
	h = clip.y1 - clip.y0;

	if (d->empty) {
		if (d->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
			if (s->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
				/* Case 8 */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* Case C */
				nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		} else {
			if (s->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
				/* Case 9 */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* Case D */
				nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		}
		d->empty = 0;
	} else {
		if (d->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
			if (s->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
				/* case A */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (dpx, w, h, d->rs, spx, s->rs, 255);
			} else {
				/* case E */
				nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (dpx, w, h, d->rs, spx, s->rs, 255);
			}
		} else {
			if (s->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
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
nr_blit_pixblock_mask_rgba32 (NRPixBlock *d, NRPixBlock *m, unsigned long rgba)
{
	if (!(rgba & 0xff)) return;

	if (m) {
		NRRectS clip;
		unsigned char *dpx, *mpx;
		int w, h;

		if (m->mode != NR_PIXBLOCK_MODE_A8) return;

		if (!nr_rect_s_test_intersect (&d->area, &m->area)) return;

		nr_rect_s_intersect (&clip, &d->area, &m->area);

		/* Pointers */
		dpx = NR_PIXBLOCK_PX (d) + (clip.y0 - d->area.y0) * d->rs + NR_PIXBLOCK_BPP (d) * (clip.x0 - d->area.x0);
		mpx = NR_PIXBLOCK_PX (m) + (clip.y0 - m->area.y0) * m->rs + (clip.x0 - m->area.x0);
		w = clip.x1 - clip.x0;
		h = clip.y1 - clip.y0;

		if (d->empty) {
			if (d->mode == NR_PIXBLOCK_MODE_R8G8B8) {
				nr_R8G8B8_EMPTY_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			} else if (d->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
				nr_R8G8B8A8_P_EMPTY_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			} else {
				nr_R8G8B8A8_N_EMPTY_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			}
			d->empty = 0;
		} else {
			if (d->mode == NR_PIXBLOCK_MODE_R8G8B8) {
				nr_R8G8B8_R8G8B8_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			} else if (d->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
				nr_R8G8B8A8_P_R8G8B8A8_P_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			} else {
				nr_R8G8B8A8_N_R8G8B8A8_N_A8_RGBA32 (dpx, w, h, d->rs, mpx, m->rs, rgba);
			}
		}
	} else {
		unsigned int r, g, b, a;
		int x, y;
		r = NR_RGBA32_R (rgba);
		g = NR_RGBA32_G (rgba);
		b = NR_RGBA32_B (rgba);
		a = NR_RGBA32_A (rgba);
		for (y = d->area.y0; y < d->area.y1; y++) {
			unsigned char *p;
			p = NR_PIXBLOCK_PX (d) + (y - d->area.y0) * d->rs;
			for (x = d->area.x0; x < d->area.x1; x++) {
				unsigned int da;
				switch (d->mode) {
				case NR_PIXBLOCK_MODE_R8G8B8:
					p[0] = NR_COMPOSEN11 (r, a, p[0]);
					p[1] = NR_COMPOSEN11 (g, a, p[1]);
					p[2] = NR_COMPOSEN11 (b, a, p[2]);
					p += 3;
					break;
				case NR_PIXBLOCK_MODE_R8G8B8A8P:
					p[0] = NR_COMPOSENPP (r, a, p[0], p[3]);
					p[1] = NR_COMPOSENPP (g, a, p[1], p[3]);
					p[2] = NR_COMPOSENPP (b, a, p[2], p[3]);
					p[3] = (65025 - (255 - a) * (255 - p[3]) + 127) / 255;
					p += 4;
					break;
				case NR_PIXBLOCK_MODE_R8G8B8A8N:
					da = 65025 - (255 - a) * (255 - p[3]);
					p[0] = NR_COMPOSENNN_A7 (r, a, p[0], p[3], da);
					p[1] = NR_COMPOSENNN_A7 (g, a, p[1], p[3], da);
					p[2] = NR_COMPOSENNN_A7 (b, a, p[2], p[3], da);
					p[3] = (da + 127) / 255;
					p += 4;
					break;
				default:
					break;
				}
			}
		}
	}
}

