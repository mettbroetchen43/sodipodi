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
#include "sp-canvas.h"
#include "sp-canvas-util.h"
#include "sp-ctrlline.h"

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_rect_svp.h>

static void sp_ctrlline_class_init (SPCtrlLineClass *klass);
static void sp_ctrlline_init (SPCtrlLine *ctrlline);
static void sp_ctrlline_destroy (GtkObject *object);

static void sp_ctrlline_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_ctrlline_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

#if 0
static double sp_ctrlline_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);
#endif

static GnomeCanvasItemClass *parent_class;

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
		type = gtk_type_unique (GNOME_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_ctrlline_class_init (SPCtrlLineClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	object_class->destroy = sp_ctrlline_destroy;

	item_class->update = sp_ctrlline_update;
	item_class->render = sp_ctrlline_render;
#if 0
	item_class->point = sp_ctrlline_point;
#endif
}

static void
sp_ctrlline_init (SPCtrlLine *ctrlline)
{
	ctrlline->rgba = 0x0000ff7f;
	ctrlline->s.x = ctrlline->s.y = ctrlline->e.x = ctrlline->e.y = 0.0;
	ctrlline->svp = NULL;
}

static void
sp_ctrlline_destroy (GtkObject *object)
{
	SPCtrlLine *ctrlline;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (object));

	ctrlline = SP_CTRLLINE (object);

	if (ctrlline->svp)
		art_svp_free (ctrlline->svp);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrlline_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCtrlLine *ctrlline;

	ctrlline = SP_CTRLLINE (item);

	if (ctrlline->svp) {
		gnome_canvas_buf_ensure_buf (buf);
		art_rgb_svp_alpha (ctrlline->svp, buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1, ctrlline->rgba,
				   buf->buf, buf->buf_rowstride,
				   NULL);
	}
}

static void
sp_ctrlline_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCtrlLine *cl;
	ArtPoint p;
	ArtVpath vpath[3];
	ArtSVP *svp;
	ArtDRect dbox;
	ArtIRect ibox;

	cl = SP_CTRLLINE (item);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	p.x = cl->s.x;
	p.y = cl->s.y;
	art_affine_point (&p, &p, affine);

	vpath[0].code = ART_MOVETO_OPEN;
	vpath[0].x = p.x;
	vpath[0].y = p.y;

	p.x = cl->e.x;
	p.y = cl->e.y;
	art_affine_point (&p, &p, affine);

	vpath[1].code = ART_LINETO;
	vpath[1].x = p.x;
	vpath[1].y = p.y;

	vpath[2].code = ART_END;

	svp = art_svp_vpath_stroke (vpath, ART_PATH_STROKE_CAP_BUTT, ART_PATH_STROKE_JOIN_MITER, 1, 4, 0.25);
	cl->svp = svp;

	art_drect_svp (&dbox, cl->svp);
	art_drect_to_irect (&ibox, &dbox);

	item->x1 = ibox.x0;
	item->y1 = ibox.y0;
	item->x2 = ibox.x1;
	item->y2 = ibox.y1;

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

#if 0
static double
sp_ctrlline_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	return 1000.0;
}
#endif

void
sp_ctrlline_set_rgba32 (SPCtrlLine *cl, guint32 rgba)
{
	g_return_if_fail (cl != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (cl));

	if (rgba != cl->rgba) {
		GnomeCanvasItem *item;
		cl->rgba = rgba;
		item = GNOME_CANVAS_ITEM (cl);
		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	}
}

#define EPSILON 1e-6
#define DIFFER(a,b) (fabs ((a) - (b)) > EPSILON)

void
sp_ctrlline_set_coords (SPCtrlLine *cl, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	g_return_if_fail (cl != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (cl));

	if (DIFFER (x0, cl->s.x) || DIFFER (y0, cl->s.y) || DIFFER (x1, cl->e.x) || DIFFER (y1, cl->e.y)) {
		cl->s.x = x0;
		cl->s.y = y0;
		cl->e.x = x1;
		cl->e.y = y1;
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (cl));
	}
}
