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
	dpx = d->px + (clip.y0 - d->area.y0) * d->rs + 4 * (clip.x0 - d->area.x0);
	spx = s->px + (clip.y0 - s->area.y0) * s->rs + 4 * (clip.x0 - s->area.x0);
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

