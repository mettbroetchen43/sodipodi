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

#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-svp-private.h>
#include <libnr/nr-stroke.h>
#include <libnr/nr-svp-render.h>

#include "sp-canvas.h"
#include "sp-canvas-util.h"

#include "canvas-bpath.h"

static void sp_canvas_bpath_class_init (SPCanvasBPathClass *klass);
static void sp_canvas_bpath_init (SPCanvasBPath *path);
static void sp_canvas_bpath_destroy (GtkObject *object);

static void sp_canvas_bpath_update (SPCanvasItem *item, double *affine, unsigned int flags);
static void sp_canvas_bpath_render (SPCanvasItem *item, SPCanvasBuf *buf);
static double sp_canvas_bpath_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item);
 
static SPCanvasItemClass *parent_class;

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
		type = gtk_type_unique (SP_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_canvas_bpath_class_init (SPCanvasBPathClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = GTK_OBJECT_CLASS (klass);
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_CANVAS_ITEM);

	object_class->destroy = sp_canvas_bpath_destroy;

	item_class->update = sp_canvas_bpath_update;
	item_class->render = sp_canvas_bpath_render;
	item_class->point = sp_canvas_bpath_point;
}

static void
sp_canvas_bpath_init (SPCanvasBPath * bpath)
{
	bpath->fill_rgba = 0x000000ff;
	bpath->fill_rule = NR_WIND_RULE_EVENODD;

	bpath->stroke_rgba = 0x00000000;
	bpath->stroke_width = 1.0;
	bpath->stroke_linejoin = NR_STROKE_JOIN_MITER;
	bpath->stroke_linecap = NR_STROKE_CAP_BUTT;
	bpath->stroke_miterlimit = 11.0;

	bpath->fill_svp = NULL;
	bpath->stroke_svp = NULL;
}

static void
sp_canvas_bpath_destroy (GtkObject *object)
{
	SPCanvasBPath *cbp;

	cbp = SP_CANVAS_BPATH (object);

	if (cbp->fill_svp) {
		nr_svp_free (cbp->fill_svp);
		cbp->fill_svp = NULL;
	}

	if (cbp->stroke_svp) {
		nr_svp_free (cbp->stroke_svp);
		cbp->stroke_svp = NULL;
	}

	if (cbp->curve) {
		cbp->curve = sp_curve_unref (cbp->curve);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_bpath_update (SPCanvasItem *item, double *affine, unsigned int flags)
{
	SPCanvasBPath *cbp;
	NRRectF bbox;

	cbp = SP_CANVAS_BPATH (item);

	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	if (((SPCanvasItemClass *) parent_class)->update)
		((SPCanvasItemClass *) parent_class)->update (item, affine, flags);

	sp_canvas_item_reset_bounds (item);

	if (cbp->fill_svp) {
		nr_svp_free (cbp->fill_svp);
		cbp->fill_svp = NULL;
	}

	if (cbp->stroke_svp) {
		nr_svp_free (cbp->stroke_svp);
		cbp->stroke_svp = NULL;
	}

	if (!cbp->curve) return;

	nr_rect_f_set_empty (&bbox);

	if ((cbp->fill_rgba & 0xff) || (cbp->stroke_rgba & 0xff)) {
		NRMatrixF ctmf;
		nr_matrix_f_from_d (&ctmf, NR_MATRIX_D_FROM_DOUBLE (affine));
		if ((cbp->fill_rgba & 0xff) && (cbp->curve->end > 2)) {
			NRSVL *svl;
			svl = nr_svl_from_art_bpath (cbp->curve->bpath, &ctmf, NR_WIND_RULE_EVENODD, TRUE, 0.25);
			cbp->fill_svp = nr_svp_from_svl (svl, NULL);
			nr_svl_free_list (svl);
			nr_svp_bbox (cbp->fill_svp, &bbox, FALSE);
		}

		if ((cbp->stroke_rgba & 0xff) && (cbp->curve->end > 1)) {
			NRBPath bpath;
			NRSVL *svl;
			bpath.path = cbp->curve->bpath;
			svl = nr_bpath_stroke (&bpath, &ctmf, cbp->stroke_width,
								   cbp->stroke_linecap, cbp->stroke_linejoin,
								   cbp->stroke_miterlimit * M_PI / 180.0,
								   0.25);
			cbp->stroke_svp = nr_svp_from_svl (svl, NULL);
			nr_svl_free_list (svl);
			nr_svp_bbox (cbp->stroke_svp, &bbox, FALSE);
		}
	}

	item->x1 = (int) floor (bbox.x0);
	item->y1 = (int) floor (bbox.y0);
	item->x2 = (int) ceil (bbox.x1);
	item->y2 = (int) ceil (bbox.y1);

	sp_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

static void
sp_canvas_bpath_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
	SPCanvasBPath *cbp;

	cbp = SP_CANVAS_BPATH (item);

	sp_canvas_buf_ensure_buf (buf);

	if (cbp->fill_svp) {
		nr_pixblock_render_svp_rgba (&buf->pixblock, cbp->fill_svp, cbp->fill_rgba);
	}

	if (cbp->stroke_svp) {
		nr_pixblock_render_svp_rgba (&buf->pixblock, cbp->stroke_svp, cbp->stroke_rgba);
	}
}

static double
sp_canvas_bpath_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item)
{
	SPCanvasBPath *cbp;

	cbp = SP_CANVAS_BPATH (item);

	if (cbp->fill_svp) {
		if (nr_svp_point_wind (cbp->fill_svp, (float) x, (float) y)) {
			*actual_item = item;
			return 0.0;
		}
	}
	if (cbp->stroke_svp) {
		if (nr_svp_point_wind (cbp->stroke_svp, (float) x, (float) y)) {
			*actual_item = item;
			return 0.0;
		}
	}
	if (cbp->stroke_svp) {
		return nr_svp_point_distance (cbp->stroke_svp, (float) x, (float) y);
	}
	if (cbp->fill_svp) {
		return nr_svp_point_distance (cbp->fill_svp, (float) x, (float) y);
	}

	return NR_HUGE_F;
}

SPCanvasItem *
sp_canvas_bpath_new (SPCanvasGroup *parent, SPCurve *curve)
{
	SPCanvasItem *item;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (SP_IS_CANVAS_GROUP (parent), NULL);

	item = sp_canvas_item_new (parent, SP_TYPE_CANVAS_BPATH, NULL);

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

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cbp));
}

void
sp_canvas_bpath_set_fill (SPCanvasBPath *cbp, guint32 rgba, unsigned int rule)
{
	g_return_if_fail (cbp != NULL);
	g_return_if_fail (SP_IS_CANVAS_BPATH (cbp));

	cbp->fill_rgba = rgba;
	cbp->fill_rule = rule;

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cbp));
}

void
sp_canvas_bpath_set_stroke (SPCanvasBPath *cbp, guint32 rgba, gdouble width, unsigned int join, unsigned int cap)
{
	g_return_if_fail (cbp != NULL);
	g_return_if_fail (SP_IS_CANVAS_BPATH (cbp));

	cbp->stroke_rgba = rgba;
	cbp->stroke_width = MAX (width, 0.1);
	cbp->stroke_linejoin = join;
	cbp->stroke_linecap = cap;

	sp_canvas_item_request_update (SP_CANVAS_ITEM (cbp));
}

