#define __SP_CANVAS_UTILS_C__

/*
 * Helper stuff for SPCanvas
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_rgb_svp.h>
#include "sp-canvas-util.h"

#if 0
/*
 * Grabs canvas item, but unlike the original method does not pass
 * illegal key event mask to canvas, who passes it ahead to Gdk, but
 * instead sets event mask in canvas struct by hand
 */

int
sp_canvas_item_grab (GnomeCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime)
{
	int ret;

	g_return_val_if_fail (item != NULL, -1);
	g_return_val_if_fail (GNOME_IS_CANVAS_ITEM (item), -1);

	ret = gnome_canvas_item_grab (item,
				      event_mask & (~(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK)),
				      cursor,
				      etime);
			
	/* fixme: Top hack (Lauris) */
	/* fixme: If we add key masks to event mask, Gdk will abort (Lauris) */
	/* fixme: But Canvas actualle does get key events, so all we need is routing these here */
	item->canvas->grabbed_event_mask = event_mask;

	return ret;
}
#endif

void
sp_canvas_update_bbox (SPCanvasItem *item, int x1, int y1, int x2, int y2)
{
	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->x1 = x1;
	item->y1 = y1;
	item->x2 = x2;
	item->y2 = y2;
	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

void
sp_canvas_item_reset_bounds (SPCanvasItem *item)
{
	item->x1 = 0.0;
	item->y1 = 0.0;
	item->x2 = 0.0;
	item->y2 = 0.0;
}

void
sp_canvas_buf_ensure_buf (SPCanvasBuf *buf)
{
	unsigned int r, g, b;
	if (!buf->pixblock.empty) return;
	r = buf->bgcolor >> 16;
	g = (buf->bgcolor >> 8) & 0xff;
	b = buf->bgcolor & 0xff;
	if ((r != g) || (r != b)) {
		int x, y;
		for (y = buf->pixblock.area.y0; y < buf->pixblock.area.y1; y++) {
			unsigned char *p;
			p = NR_PIXBLOCK_PX (&buf->pixblock) + (y - buf->pixblock.area.y0) * buf->pixblock.rs;
			for (x = buf->pixblock.area.x0; x < buf->pixblock.area.x1; x++) {
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
	} else {
		int y;
		for (y = buf->pixblock.area.y0; y < buf->pixblock.area.y1; y++) {
			unsigned char *p;
			p = NR_PIXBLOCK_PX (&buf->pixblock) + (y - buf->pixblock.area.y0) * buf->pixblock.rs;
			memset (p, r, 3 * (buf->pixblock.area.x1 - buf->pixblock.area.x0));
		}
	}
	buf->pixblock.empty = 0;
}

#if 0
void
sp_canvas_clear_buffer (SPCanvasBuf *buf)
{
	unsigned char r, g, b;

	r = (buf->bg_color >> 16) & 0xff;
	g = (buf->bg_color >> 8) & 0xff;
	b = buf->bg_color & 0xff;

	if ((r != g) || (r != b)) {
		int x, y;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			unsigned char *p;
			p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride;
			for (x = buf->rect.x0; x < buf->rect.x1; x++) {
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
	} else {
		int y;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			memset (buf->buf + (y - buf->rect.y0) * buf->buf_rowstride, r, 3 * (buf->rect.x1 - buf->rect.x0));
		}
	}
}
#endif

#if 0
void
sp_canvas_render_svp_translated (SPCanvasBuf * buf, ArtSVP * svp,
	guint32 rgba, gint x, gint y)
{
	guint32 fg_color, bg_color;
	int alpha;

	sp_canvas_buf_ensure_buf (buf);

	if (buf->is_bg) {
		bg_color = buf->bg_color;
		alpha = rgba & 0xff;
		if (alpha == 0xff)
			fg_color = rgba >> 8;
		else {
			/* composite over background color */
			int bg_r, bg_g, bg_b;
			int fg_r, fg_g, fg_b;
			int tmp;

			bg_r = (bg_color >> 16) & 0xff;
			fg_r = (rgba >> 24) & 0xff;
			tmp = (fg_r - bg_r) * alpha;
			fg_r = bg_r + ((tmp + (tmp >> 8) + 0x80) >> 8);

			bg_g = (bg_color >> 8) & 0xff;
			fg_g = (rgba >> 16) & 0xff;
			tmp = (fg_g - bg_g) * alpha;
			fg_g = bg_g + ((tmp + (tmp >> 8) + 0x80) >> 8);

			bg_b = bg_color & 0xff;
			fg_b = (rgba >> 8) & 0xff;
			tmp = (fg_b - bg_b) * alpha;
			fg_b = bg_b + ((tmp + (tmp >> 8) + 0x80) >> 8);

			fg_color = (fg_r << 16) | (fg_g << 8) | fg_b;
		}
		art_rgb_svp_aa (svp,
				buf->rect.x0 - x, buf->rect.y0 - y,
				buf->rect.x1 - x, buf->rect.y1 - y,
				fg_color, bg_color,
				buf->buf, buf->buf_rowstride,
				NULL);
		buf->is_bg = 0;
		buf->is_buf = 1;
	} else {
		art_rgb_svp_alpha (svp,
				   buf->rect.x0 - x, buf->rect.y0 - y,
				   buf->rect.x1 - x, buf->rect.y1 - y,
				   rgba,
				   buf->buf, buf->buf_rowstride,
				   NULL);
	}
}
#endif

#if 0
void sp_canvas_item_i2p_affine (SPCanvasItem * item, double affine[])
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (affine != NULL);

	if (item->xform == NULL) {
		art_affine_identity (affine);
		return;
	}

	affine[0] = item->xform[0];
	affine[1] = item->xform[1];
	affine[2] = item->xform[2];
	affine[3] = item->xform[3];
	affine[4] = item->xform[4];
	affine[5] = item->xform[5];
}

void sp_canvas_item_i2i_affine (SPCanvasItem * from, SPCanvasItem * to, double affine[])
{
	double f2w[6], t2w[6], w2t[6];

	g_return_if_fail (from != NULL);
	g_return_if_fail (to != NULL);
	g_return_if_fail (affine != NULL);

	sp_canvas_item_i2w_affine (from, f2w);
	sp_canvas_item_i2w_affine (to, t2w);
	art_affine_invert (w2t, t2w);

	art_affine_multiply (affine, f2w, w2t);
}

void sp_canvas_item_set_i2w_affine (SPCanvasItem * item, double i2w[])
{
	double p2w[6],w2p[6],i2p[6];
	
	g_return_if_fail (item != NULL);
	g_return_if_fail (i2w != NULL);

	sp_canvas_item_i2w_affine (item->parent, p2w);
	art_affine_invert (w2p, p2w);
	art_affine_multiply (i2p, i2w, w2p);
	sp_canvas_item_affine_absolute (item, i2p);
}
#endif

void sp_canvas_item_move_to_z (SPCanvasItem * item, gint z)
{
	gint current_z;

	g_assert (item != NULL);

	current_z = sp_canvas_item_order (item);

	if (z == current_z)
		return;

	if (z > current_z)
		sp_canvas_item_raise (item, z - current_z);

	sp_canvas_item_lower (item, current_z - z);
}

gint
sp_canvas_item_compare_z (SPCanvasItem * a, SPCanvasItem * b)
{
	gint o_a, o_b;

	o_a = sp_canvas_item_order (a);
	o_b = sp_canvas_item_order (b);

	if (a > b) return -1;
	if (a < b) return 1;
	return 0;
}

#define RGBA_R(v) ((v) >> 24)
#define RGBA_G(v) (((v) >> 16) & 0xff)
#define RGBA_B(v) (((v) >> 8) & 0xff)
#define RGBA_A(v) ((v) & 0xff)
#define COMPOSE(b,f,a) (((255 - (a)) * b + (f * a) + 127) / 255)

void
sp_render_hline (NRPixBlock *pb, gint y, gint xs, gint xe, guint32 rgba)
{
	if ((y >= pb->area.y0) && (y < pb->area.y1)) {
		unsigned int r, g, b, a;
		int x0, x1, x;
		unsigned char *p;
		r = RGBA_R (rgba);
		g = RGBA_G (rgba);
		b = RGBA_B (rgba);
		a = RGBA_A (rgba);
		x0 = MAX (pb->area.x0, xs);
		x1 = MIN (pb->area.x1, xe + 1);
		p = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs + (x0 - pb->area.x0) * 3;
		for (x = x0; x < x1; x++) {
			p[0] = COMPOSE (p[0], r, a);
			p[1] = COMPOSE (p[1], g, a);
			p[2] = COMPOSE (p[2], b, a);
			p += 3;
		}
	}
}

void
sp_render_vline (NRPixBlock *pb, gint x, gint ys, gint ye, guint32 rgba)
{
	if ((x >= pb->area.x0) && (x < pb->area.x1)) {
		guint r, g, b, a;
		gint y0, y1, y;
		guchar *p;
		r = RGBA_R (rgba);
		g = RGBA_G (rgba);
		b = RGBA_B (rgba);
		a = RGBA_A (rgba);
		y0 = MAX (pb->area.y0, ys);
		y1 = MIN (pb->area.y1, ye + 1);
		p = NR_PIXBLOCK_PX (pb) + (y0 - pb->area.y0) * pb->rs + (x - pb->area.x0) * 3;
		for (y = y0; y < y1; y++) {
			p[0] = COMPOSE (p[0], r, a);
			p[1] = COMPOSE (p[1], g, a);
			p[2] = COMPOSE (p[2], b, a);
			p += pb->rs;
		}
	}
}

void
sp_render_area (NRPixBlock *pb, gint xs, gint ys, gint xe, gint ye, guint32 rgba)
{
	guint r, g, b, a;
	gint x0, x1, x;
	gint y0, y1, y;
	guchar *p;
	r = RGBA_R (rgba);
	g = RGBA_G (rgba);
	b = RGBA_B (rgba);
	a = RGBA_A (rgba);
	x0 = MAX (pb->area.x0, xs);
	x1 = MIN (pb->area.x1, xe + 1);
	y0 = MAX (pb->area.y0, ys);
	y1 = MIN (pb->area.y1, ye + 1);
	for (y = y0; y < y1; y++) {
		p = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs + (x0 - pb->area.x0) * 3;
		for (x = x0; x < x1; x++) {
			p[0] = COMPOSE (p[0], r, a);
			p[1] = COMPOSE (p[1], g, a);
			p[2] = COMPOSE (p[2], b, a);
			p += 3;
		}
	}
}

