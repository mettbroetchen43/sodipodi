#define __SODIPODI_CTRLLINE_C__

/*
 * Simple straight line
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

/*
 * TODO:
 * Draw it by hand - we really do not need aa stuff for it
 *
 */

#include <math.h>

#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock-line.h>

#include "sp-canvas.h"
#include "sp-canvas-util.h"
#include "sp-ctrlline.h"

struct _SPCtrlLine {
	SPCanvasItem item;

	guint32 rgba;

	float x0, y0, x1, y1;
	int ix0, iy0, ix1, iy1;
};

struct _SPCtrlLineClass {
	SPCanvasItemClass parent_class;
};

static void sp_ctrlline_class_init (SPCtrlLineClass *klass);
static void sp_ctrlline_init (SPCtrlLine *ctrlline);
static void sp_ctrlline_destroy (GtkObject *object);

static void sp_ctrlline_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags);
static void sp_ctrlline_render (SPCanvasItem *item, SPCanvasBuf *buf);

static SPCanvasItemClass *parent_class;

GtkType
sp_ctrlline_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"SPCtrlLine",
			sizeof (SPCtrlLine),
			sizeof (SPCtrlLineClass),
			(GtkClassInitFunc) sp_ctrlline_class_init,
			(GtkObjectInitFunc) sp_ctrlline_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_ctrlline_class_init (SPCtrlLineClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_CANVAS_ITEM);

	object_class->destroy = sp_ctrlline_destroy;

	item_class->update = sp_ctrlline_update;
	item_class->render = sp_ctrlline_render;
}

static void
sp_ctrlline_init (SPCtrlLine *ctrlline)
{
	ctrlline->rgba = 0x0000ff7f;
}

static void
sp_ctrlline_destroy (GtkObject *object)
{
	SPCtrlLine *ctrlline;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (object));

	ctrlline = SP_CTRLLINE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
sp_ctrlline_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
	SPCtrlLine *cline;

	cline = SP_CTRLLINE (item);

	sp_canvas_buf_ensure_buf (buf);

	nr_pixblock_draw_line_rgba32 (&buf->pixblock, cline->ix0, cline->iy0, cline->ix1, cline->iy1, TRUE, cline->rgba);
}

static void
sp_ctrlline_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags)
{
	SPCtrlLine *cline;

	cline = SP_CTRLLINE (item);

	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	if (parent_class->update) parent_class->update (item, ctm, flags);

	sp_canvas_item_reset_bounds (item);

	cline->ix0 = (int) (NR_MATRIX_DF_TRANSFORM_X (ctm, cline->x0, cline->y0) + 0.5);
	cline->iy0 = (int) (NR_MATRIX_DF_TRANSFORM_Y (ctm, cline->x0, cline->y0) + 0.5);
	cline->ix1 = (int) (NR_MATRIX_DF_TRANSFORM_X (ctm, cline->x1, cline->y1) + 0.5);
	cline->iy1 = (int) (NR_MATRIX_DF_TRANSFORM_Y (ctm, cline->x1, cline->y1) + 0.5);

	item->x1 = MIN (cline->ix0, cline->ix1);
	item->y1 = MIN (cline->iy0, cline->iy1);
	item->x2 = MAX (cline->ix0, cline->ix1) + 1;
	item->y2 = MAX (cline->iy0, cline->iy1) + 1;

	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

void
sp_ctrlline_set_rgba32 (SPCtrlLine *cl, guint32 rgba)
{
	g_return_if_fail (cl != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (cl));

	if (rgba != cl->rgba) {
		SPCanvasItem *item;
		cl->rgba = rgba;
		item = SP_CANVAS_ITEM (cl);
		sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	}
}

#define EPSILON 1e-6
#define DIFFER(a,b) (fabs ((a) - (b)) > EPSILON)

void
sp_ctrlline_set_coords (SPCtrlLine *cline, float x0, float y0, float x1, float y1)
{
	g_return_if_fail (cline != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (cline));

	if ((x0 != cline->x0) || (y0 != cline->y0) || (x1 != cline->x1) || (y1 != cline->y1)) {
		cline->x0 = x0;
		cline->y0 = y0;
		cline->x1 = x1;
		cline->y1 = y1;
		sp_canvas_item_request_update (SP_CANVAS_ITEM (cline));
	}
}
