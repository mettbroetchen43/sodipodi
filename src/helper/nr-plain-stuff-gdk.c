#define __NR_PLAIN_STUFF_GDK_C__

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

#include "nr-plain-stuff.h"
#include "nr-plain-stuff-gdk.h"

void
nr_gdk_draw_gray_garbage (GdkDrawable *drawable, GdkGC *gc, gint x, gint y, gint w, gint h)
{
	static guchar *px;
	gint xx, yy;

	if (!px) px = g_new (guchar, 3 * 64 * 64);

	for (yy = y; yy < y + h; yy += 64) {
		for (xx = x; xx < x + w; xx += 64) {
			gint ex, ey;
			ex = MIN (xx + 64, x + w);
			ey = MIN (yy + 64, y + h);
			nr_render_gray_garbage_rgb (px, ex - xx, ey - yy, 3 * 64);
			gdk_draw_rgb_image (drawable, gc, xx, yy, ex - xx, ey - yy, GDK_RGB_DITHER_NONE, px, 3 * 64);
		}
	}
}

