#define __SP_CANVAS_BPATH_C__

/*
 * Simple bezier bpath CanvasItem for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_svp_point.h>
#include <libart_lgpl/art_rect_svp.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "canvas-bpath.h"

static void sp_canvas_bpath_class_init (SPCanvasBPathClass *klass);
static void sp_canvas_bpath_init (SPCanvasBPath *path);
static void sp_canvas_bpath_destroy (GtkObject *object);

static void sp_canvas_bpath_update (GnomeCanvasItem *item, gdouble *affine, ArtSVP *clip_canvas_bpath, gint flags);
static void sp_canvas_bpath_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);
static double sp_canvas_bpath_point (GnomeCanvasItem *item, gdouble x, gdouble y, gint cx, gint cy, GnomeCanvasItem **actual_item);
 
static GnomeCanvasItemClass *parent_class;

GtkType
sp_canvas_bpath_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPCanvasBPath",
			sizeof (SPCanvasBPath),
			sizeof (SPCanvasBPathClass),
			(GtkClassInitFunc) sp_canvas_bpath_class_init,
			(GtkObjectInitFunc) sp_canvas_bpath_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GNOME_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_canvas_bpath_class_init (SPCanvasBPathClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = GTK_OBJECT_CLASS (klass);
	item_class = GNOME_CANVAS_ITEM_CLASS (klass);

	parent_class = gtk_type_class (GNOME_TYPE_CANVAS_ITEM);

	object_class->destroy = sp_canvas_bpath_destroy;

	item_class->update = sp_canvas_bpath_update;
	item_class->render = sp_canvas_bpath_render;
	item_class->point = sp_canvas_bpath_point;
}

static void
sp_canvas_bpath_init (SPCanvasBPath * bpath)
{
	bpath->width = 1.0;
	bpath->join = ART_PATH_STROKE_JOIN_MITER;
	bpath->cap = ART_PATH_STROKE_CAP_BUTT;
	bpath->miter = 11.0;
	bpath->rgba = 0x0000007f;

	bpath->svp = NULL;
}

static void
sp_canvas_bpath_destroy (GtkObject *object)
{
	SPCanvasBPath *cbp;

	cbp = SP_CANVAS_BPATH (object);

	if (cbp->svp) {
		art_svp_free (cbp->svp);
		cbp->svp = NULL;
	}

	if (cbp->curve) {
		cbp->curve = sp_curve_unref (cbp->curve);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_bpath_update (GnomeCanvasItem *item, gdouble *affine, ArtSVP *clip_path, gint flags)
{
	SPCanvasBPath *cbp;
	ArtBpath *bp;
	ArtVpath *vp, *pp;
	ArtDRect dbox;
	ArtIRect ibox;

	cbp = SP_CANVAS_BPATH (item);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	if (((GnomeCanvasItemClass *) parent_class)->update)
		((GnomeCanvasItemClass *) parent_class)->update (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	if (cbp->svp) {
		art_svp_free (cbp->svp);
		cbp->svp = NULL;
	}

	if (!cbp->curve) return;

	/* Build SVP */

	bp = art_bpath_affine_transform (cbp->curve->bpath, affine);
	vp = art_bez_path_to_vec (bp, 0.25);
	art_free (bp);
	pp = art_vpath_perturb (vp);
	art_free (vp);
	cbp->svp = art_svp_vpath_stroke (pp, cbp->join, cbp->cap, cbp->width, cbp->miter, 0.25);
	art_free (pp);

	art_drect_svp (&dbox, cbp->svp);

	art_drect_to_irect (&ibox, &dbox);

	item->x1 = ibox.x0;
	item->y1 = ibox.y0;
	item->x2 = ibox.x1;
	item->y2 = ibox.y1;

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

static void
sp_canvas_bpath_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCanvasBPath *cbp;

	cbp = SP_CANVAS_BPATH (item);

	if (!cbp->svp) return;

	gnome_canvas_render_svp (buf, cbp->svp, cbp->rgba);
}

#define BIGVAL 1e18

static double
sp_canvas_bpath_point (GnomeCanvasItem *item, gdouble x, gdouble y, gint cx, gint cy, GnomeCanvasItem **actual_item)
{
	SPCanvasBPath *cbp;
	gint wind;
	gdouble dist;

	cbp = SP_CANVAS_BPATH (item);

	if (!cbp->svp) return BIGVAL;

	wind = art_svp_point_wind (cbp->svp, cx, cy);
	if (wind) {
		*actual_item = item;
		return 0.0;
	}

	dist = art_svp_point_dist (cbp->svp, cx, cy);

	return dist;
}

GnomeCanvasItem *
sp_canvas_bpath_new (GnomeCanvasGroup *parent, SPCurve *curve)
{
	GnomeCanvasItem *item;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);

	item = gnome_canvas_item_new (parent, SP_TYPE_CANVAS_BPATH, NULL);

	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (item), curve);

	return item;
}

void
sp_canvas_bpath_set_bpath (SPCanvasBPath *cbp, SPCurve *curve)
{
	g_return_if_fail (cbp != NULL);
	g_return_if_fail (SP_IS_CANVAS_BPATH (cbp));

	if (cbp->curve) {
		cbp->curve = sp_curve_unref (cbp->curve);
	}

	if (curve) {
		cbp->curve = sp_curve_ref (curve);
	}

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (cbp));
}

void
sp_canvas_bpath_set_style (SPCanvasBPath *cbp, gdouble width, ArtPathStrokeJoinType join, ArtPathStrokeCapType cap, guint32 rgba)
{
	g_return_if_fail (cbp != NULL);
	g_return_if_fail (SP_IS_CANVAS_BPATH (cbp));

	cbp->width = width;
	cbp->join = join;
	cbp->cap = cap;
	cbp->rgba = rgba;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (cbp));
}

