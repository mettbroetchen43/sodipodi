#define __NR_GRADIENT_C__

/*
 * Gradient rendering utilities
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <string.h>

#include <libart_lgpl/art_affine.h>

#include "nr-plain-stuff.h"
#include "nr-gradient.h"

static void nr_lgradient_render_r8g8b8a8 (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs);
static void nr_lgradient_render_r8g8b8 (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs);

NRLGradientRenderer *
nr_lgradient_renderer_setup_r8g8b8a8 (NRLGradientRenderer *lgr, NRGradientSpreadType spread, gdouble *n2b)
{
	gdouble b2n[6];

	g_return_val_if_fail (lgr != NULL, NULL);
	g_return_val_if_fail (n2b != NULL, NULL);
	g_return_val_if_fail (lgr->vector != NULL, NULL);

	lgr->render = nr_lgradient_render_r8g8b8a8;
	lgr->spread = spread;

	art_affine_invert (b2n, n2b);

	lgr->x0 = n2b[4];
	lgr->y0 = n2b[5];
	lgr->dx = b2n[0];
	lgr->dy = b2n[2];

	return lgr;
}

NRLGradientRenderer *
nr_lgradient_renderer_setup_r8g8b8 (NRLGradientRenderer *lgr, NRGradientSpreadType spread, gdouble *n2b)
{
	gdouble b2n[6];

	g_return_val_if_fail (lgr != NULL, NULL);
	g_return_val_if_fail (n2b != NULL, NULL);
	g_return_val_if_fail (lgr->vector != NULL, NULL);

	lgr->render = nr_lgradient_render_r8g8b8;
	lgr->spread = spread;

	art_affine_invert (b2n, n2b);

	lgr->x0 = n2b[4];
	lgr->y0 = n2b[5];
	lgr->dx = b2n[0];
	lgr->dy = b2n[2];

	return lgr;
}

void
nr_lgradient_render (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs)
{
	lgr->render (lgr, px, x0, y0, width, height, rs);
}

#define NRG_MASK (NR_GRADIENT_VECTOR_LENGTH - 1)
#define NRG_2MASK (2 * NR_GRADIENT_VECTOR_LENGTH - 1)

static void
nr_lgradient_render_r8g8b8a8 (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs)
{
	gint x, y;
	guchar *d;
	gdouble pos;

	for (y = 0; y < height; y++) {
		d = px + y * rs;
		pos = (y + y0 - lgr->y0) * lgr->dy + (0 + x0 - lgr->x0) * lgr->dx;
		for (x = 0; x < width; x++) {
			gint ip, idx;
			guint ca;
			guchar *s;
			ip = (gint) (pos * NR_GRADIENT_VECTOR_LENGTH);
			switch (lgr->spread) {
			case NR_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, NRG_MASK);
				break;
			case NR_GRADIENT_SPREAD_REFLECT:
				idx = ip & NRG_2MASK;
				if (idx > NRG_MASK) idx = NRG_2MASK - idx;
				break;
			case NR_GRADIENT_SPREAD_REPEAT:
				idx = ip % NRG_MASK;
				break;
			default:
				g_assert_not_reached ();
				idx = 0;
				break;
			}
			/* Full composition */
			s = lgr->vector + 4 * idx;
			ca = 65025 - (255 - s[3]) * (255 - d[3]);
			d[0] = COMPOSENNN_A7 (s[0], s[3], d[0], d[3], ca);
			d[1] = COMPOSENNN_A7 (s[1], s[3], d[1], d[3], ca);
			d[2] = COMPOSENNN_A7 (s[2], s[3], d[2], d[3], ca);
			d[3] = (ca + 127) / 255;
			d += 4;
			pos += lgr->dx;
		}
	}
}

static void
nr_lgradient_render_r8g8b8 (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs)
{
	gint x, y;
	guchar *d;
	gdouble pos;

	for (y = 0; y < height; y++) {
		d = px + y * rs;
		pos = (y + y0 - lgr->y0) * lgr->dy + (0 + x0 - lgr->x0) * lgr->dx;
		for (x = 0; x < width; x++) {
			gint ip, idx;
			guchar *s;
			ip = (gint) (pos * NR_GRADIENT_VECTOR_LENGTH);
			switch (lgr->spread) {
			case NR_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, NRG_MASK);
				break;
			case NR_GRADIENT_SPREAD_REFLECT:
				idx = ip & NRG_2MASK;
				if (idx > NRG_MASK) idx = NRG_2MASK - idx;
				break;
			case NR_GRADIENT_SPREAD_REPEAT:
				idx = ip % NRG_MASK;
				break;
			default:
				g_assert_not_reached ();
				idx = 0;
				break;
			}
			g_assert (idx >= 0);
			g_assert (idx <= NRG_MASK);
			/* Full composition */
			s = lgr->vector + 4 * idx;
			d[0] = COMPOSEN11 (s[0], s[3], d[0]);
			d[1] = COMPOSEN11 (s[1], s[3], d[1]);
			d[2] = COMPOSEN11 (s[2], s[3], d[2]);
			d += 3;
			pos += lgr->dx;
		}
	}
}

