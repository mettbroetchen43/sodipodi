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

#include "sp-canvas-util.h"

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

