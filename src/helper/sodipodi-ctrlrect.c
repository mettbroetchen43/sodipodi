#define __SODIPODI_CTRLRECT_C__

/*
 * Simple non-transformed rectangle, usable for rubberband
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <math.h>
#include "sp-canvas.h"
#include "sp-canvas-util.h"
#include "sodipodi-ctrlrect.h"

/*
 * Currently we do not have point method, as it should always be painted
 * during some transformation, which takes care of events...
 *
 * Corner coords can be in any order - i.e. x1 < x0 is allowed
 */

static void sp_ctrlrect_class_init (SPCtrlRectClass *klass);
static void sp_ctrlrect_init (SPCtrlRect *ctrlrect);
static void sp_ctrlrect_destroy (GtkObject *object);

static void sp_ctrlrect_update (SPCanvasItem *item, double *affine, unsigned int flags);
static void sp_ctrlrect_render (SPCanvasItem *item, SPCanvasBuf *buf);

static SPCanvasItemClass *parent_class;

GtkType
sp_ctrlrect_get_type (void)
{
	static GtkType ctrlrect_type = 0;

	if (!ctrlrect_type) {
		GtkTypeInfo ctrlrect_info = {
			"SPCtrlRect",
			sizeof (SPCtrlRect),
			sizeof (SPCtrlRectClass),
			(GtkClassInitFunc) sp_ctrlrect_class_init,
			(GtkObjectInitFunc) sp_ctrlrect_init,
			NULL, NULL, NULL
		};
		ctrlrect_type = gtk_type_unique (SP_TYPE_CANVAS_ITEM, &ctrlrect_info);
	}
	return ctrlrect_type;
}

static void
sp_ctrlrect_class_init (SPCtrlRectClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (sp_canvas_item_get_type ());

	object_class->destroy = sp_ctrlrect_destroy;

	item_class->update = sp_ctrlrect_update;
	item_class->render = sp_ctrlrect_render;
}

static void
sp_ctrlrect_init (SPCtrlRect *cr)
{
	cr->has_fill = FALSE;

	cr->rect.x0 = cr->rect.y0 = cr->rect.x1 = cr->rect.y1 = 0;

	cr->shadow = 0;

	cr->area.x0 = cr->area.y0 = 0;
	cr->area.x1 = cr->area.y1 = -1;

	cr->shadow_size = 0;

	cr->border_color = 0x000000ff;
	cr->fill_color = 0xffffffff;
	cr->shadow_color = 0x000000ff;
}

static void
sp_ctrlrect_destroy (GtkObject *object)
{
	SPCtrlRect *cr;

	cr = SP_CTRLRECT (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrlrect_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
	SPCtrlRect *cr;
	NRPixBlock *pb;

	cr = SP_CTRLRECT (item);
	pb = &buf->pixblock;
	
	if ((cr->area.x0 < pb->area.x1) &&
	    (cr->area.y0 < pb->area.y1) &&
	    ((cr->area.x1 + cr->shadow_size) >= pb->area.x0) &&
	    ((cr->area.y1 + cr->shadow_size) >= pb->area.y0)) {
		/* Initialize buffer, if needed */
		sp_canvas_buf_ensure_buf (buf);
		/* Top */
		sp_render_hline (pb, cr->area.y0, cr->area.x0, cr->area.x1, cr->border_color);
		/* Bottom */
		sp_render_hline (pb, cr->area.y1, cr->area.x0, cr->area.x1, cr->border_color);
		/* Left */
		sp_render_vline (pb, cr->area.x0, cr->area.y0 + 1, cr->area.y1 - 1, cr->border_color);
		/* Right */
		sp_render_vline (pb, cr->area.x1, cr->area.y0 + 1, cr->area.y1 - 1, cr->border_color);
		if (cr->shadow_size > 0) {
			/* Right shadow */
			sp_render_area (pb, cr->area.x1 + 1, cr->area.y0 + cr->shadow_size,
					cr->area.x1 + cr->shadow_size, cr->area.y1 + cr->shadow_size, cr->shadow_color);
			/* Bottom shadow */
			sp_render_area (pb, cr->area.x0 + cr->shadow_size, cr->area.y1 + 1,
					cr->area.x1, cr->area.y1 + cr->shadow_size, cr->shadow_color);
		}
		if (cr->has_fill) {
			/* Fill */
			sp_render_area (pb, cr->area.x0 + 1, cr->area.y0 + 1,
					cr->area.x1 - 1, cr->area.y1 - 1, cr->fill_color);
		}
	}
}

static void
sp_ctrlrect_update (SPCanvasItem *item, double *affine, unsigned int flags)
{
	SPCtrlRect *cr;
	ArtDRect bbox;

	cr = SP_CTRLRECT (item);

	if (((SPCanvasItemClass *) parent_class)->update)
		((SPCanvasItemClass *) parent_class)->update (item, affine, flags);

	sp_canvas_item_reset_bounds (item);

	/* Request redraw old */
	if (!cr->has_fill) {
		/* Top */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x1 + 1, cr->area.y0 + 1);
		/* Left */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x0 + 1, cr->area.y1 + 1);
		/* Right */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x1 - 1, cr->area.y0 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
		/* Bottom */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y1 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
	} else {
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
	}

	art_drect_affine_transform (&bbox, &cr->rect, affine);

	cr->area.x0 = (int) (bbox.x0 + 0.5);
	cr->area.y0 = (int) (bbox.y0 + 0.5);
	cr->area.x1 = (int) (bbox.x1 + 0.5);
	cr->area.y1 = (int) (bbox.y1 + 0.5);

	cr->shadow_size = cr->shadow;

	/* Request redraw new */
	if (!cr->has_fill) {
		/* Top */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x1 + 1, cr->area.y0 + 1);
		/* Left */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x0 + 1, cr->area.y1 + 1);
		/* Right */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x1 - 1, cr->area.y0 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
		/* Bottom */
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y1 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
	} else {
		sp_canvas_request_redraw (item->canvas,
					     cr->area.x0 - 1, cr->area.y0 - 1,
					     cr->area.x1 + cr->shadow_size + 1, cr->area.y1 + cr->shadow_size + 1);
	}

	item->x1 = cr->area.x0 - 1;
	item->y1 = cr->area.y0 - 1;
	item->x2 = cr->area.x1 + cr->shadow_size + 1;
	item->y2 = cr->area.y1 + cr->shadow_size + 1;
}

void
sp_ctrlrect_set_area (SPCtrlRect *cr, double x0, double y0, double x1, double y1)
{
	g_return_if_fail (cr != NULL);
	g_return_if_fail (SP_IS_CTRLRECT (cr));

	cr->rect.x0 = MIN (x0, x1);
	cr->rect.y0 = MIN (y0, y1);
	cr->rect.x1 = MAX (x0, x1);
	cr->rect.y1 = MAX (y0, y1);

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cr));
}

void
sp_ctrlrect_set_color (SPCtrlRect *cr, guint32 border_color, gboolean has_fill, guint32 fill_color)
{
	g_return_if_fail (cr != NULL);
	g_return_if_fail (SP_IS_CTRLRECT (cr));

	cr->border_color = border_color;
	cr->has_fill = has_fill;
	cr->fill_color = fill_color;

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cr));
}

void
sp_ctrlrect_set_shadow (SPCtrlRect *cr, gint shadow_size, guint32 shadow_color)
{
	g_return_if_fail (cr != NULL);
	g_return_if_fail (SP_IS_CTRLRECT (cr));

	cr->shadow = shadow_size;
	cr->shadow_color = shadow_color;

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cr));
}

void
sp_ctrlrect_set_rect (SPCtrlRect * cr, ArtDRect * r)
{
	cr->rect.x0 = MIN (r->x0, r->x1);
	cr->rect.y0 = MIN (r->y0, r->y1);
	cr->rect.x1 = MAX (r->x0, r->x1);
	cr->rect.y1 = MAX (r->y0, r->y1);

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cr));
}
