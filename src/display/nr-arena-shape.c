#define __NR_ARENA_SHAPE_C__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */


#include <math.h>
#include <string.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_rect_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_svp_point.h>
#include <libart_lgpl/art_gray_svp.h>
#include "../helper/nr-buffers.h"
#include "../helper/nr-plain-stuff.h"
#include "../helper/art-utils.h"
#include "../style.h"
#include "nr-arena.h"
#include "nr-arena-shape.h"

static void nr_arena_shape_class_init (NRArenaShapeClass *klass);
static void nr_arena_shape_init (NRArenaShape *shape);
static void nr_arena_shape_destroy (GtkObject *object);

static guint nr_arena_shape_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_shape_render (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static guint nr_arena_shape_clip (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static NRArenaItem *nr_arena_shape_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

static NRArenaItemClass *shape_parent_class;

GtkType
nr_arena_shape_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaShape",
			sizeof (NRArenaShape),
			sizeof (NRArenaShapeClass),
			(GtkClassInitFunc) nr_arena_shape_class_init,
			(GtkObjectInitFunc) nr_arena_shape_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_ITEM, &info);
	}
	return type;
}

static void
nr_arena_shape_class_init (NRArenaShapeClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	shape_parent_class = gtk_type_class (NR_TYPE_ARENA_ITEM);

	object_class->destroy = nr_arena_shape_destroy;

	item_class->update = nr_arena_shape_update;
	item_class->render = nr_arena_shape_render;
	item_class->clip = nr_arena_shape_clip;
	item_class->pick = nr_arena_shape_pick;
}

static void
nr_arena_shape_init (NRArenaShape *shape)
{
	shape->curve = NULL;
	shape->style = NULL;
	shape->paintbox.x0 = shape->paintbox.y0 = 0.0;
	shape->paintbox.x1 = shape->paintbox.y1 = 256.0;

	nr_matrix_d_set_identity (&shape->ctm);
	shape->fill_painter = NULL;
	shape->stroke_painter = NULL;
	shape->fill_svp = NULL;
	shape->stroke_svp = NULL;
}

static void
nr_arena_shape_destroy (GtkObject *object)
{
	NRArenaShape *shape;

	shape = NR_ARENA_SHAPE (object);

	if (shape->fill_svp) {
		art_svp_free (shape->fill_svp);
		shape->fill_svp = NULL;
	}

	if (shape->stroke_svp) {
		art_svp_free (shape->stroke_svp);
		shape->stroke_svp = NULL;
	}

	if (shape->fill_painter) {
		sp_painter_free (shape->fill_painter);
		shape->fill_painter = NULL;
	}

	if (shape->stroke_painter) {
		sp_painter_free (shape->stroke_painter);
		shape->stroke_painter = NULL;
	}

	if (shape->style) {
		sp_style_unref (shape->style);
		shape->style = NULL;
	}

	if (shape->curve) {
		sp_curve_unref (shape->curve);
		shape->curve = NULL;
	}

	if (GTK_OBJECT_CLASS (shape_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (shape_parent_class)->destroy) (object);
}

static guint
nr_arena_shape_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
{
	NRArenaShape *shape;
	ArtBpath *abp;
	ArtVpath *vp, *pvp;
	ArtDRect bbox;

	shape = NR_ARENA_SHAPE (item);

	/* Request repaint old area if needed */
	/* fixme: Think about it a bit (Lauris) */
	if (!nr_rect_l_test_empty (&item->bbox)) {
		nr_arena_request_render_rect (item->arena, &item->bbox);
		nr_rect_l_set_empty (&item->bbox);
	}

	/* Release state data */
	if (TRUE || !nr_matrix_d_test_transform_equal ((NRMatrixD *) gc->affine, &shape->ctm, NR_EPSILON_D)) {
		/* Concept test */
		if (shape->fill_svp) {
			art_svp_free (shape->fill_svp);
			shape->fill_svp = NULL;
		}
	}
	if (shape->stroke_svp) {
		art_svp_free (shape->stroke_svp);
		shape->stroke_svp = NULL;
	}
	if (shape->fill_painter) {
		sp_painter_free (shape->fill_painter);
		shape->fill_painter = NULL;
	}
	if (shape->stroke_painter) {
		sp_painter_free (shape->stroke_painter);
		shape->stroke_painter = NULL;
	}

	if (!shape->curve || !shape->style) return NR_ARENA_ITEM_STATE_ALL;
	if (sp_curve_is_empty (shape->curve)) return NR_ARENA_ITEM_STATE_ALL;
	if ((shape->style->fill.type == SP_PAINT_TYPE_NONE) && (shape->style->stroke.type == SP_PAINT_TYPE_NONE)) return NR_ARENA_ITEM_STATE_ALL;

	/* Build state data */
	if (shape->style->fill.type != SP_PAINT_TYPE_NONE) {
		if ((shape->curve->end > 2) || (shape->curve->bpath[1].code == ART_CURVETO)) {
			if (TRUE || !shape->fill_svp) {
				ArtSVP *svpa, *svpb;
				abp = art_bpath_affine_transform (shape->curve->bpath, gc->affine);
				vp = sp_vpath_from_bpath_closepath (abp, 0.25);
				art_free (abp);
				pvp = art_vpath_perturb (vp);
				art_free (vp);
				svpa = art_svp_from_vpath (pvp);
				art_free (pvp);
				svpb = art_svp_uncross (svpa);
				art_svp_free (svpa);
				shape->fill_svp = art_svp_rewind_uncrossed (svpb, shape->style->fill_rule.value);
				art_svp_free (svpb);
			} else if (!NR_DF_TEST_CLOSE (gc->affine[4], shape->ctm.c[4], NR_EPSILON_D) ||
				   !NR_DF_TEST_CLOSE (gc->affine[5], shape->ctm.c[5], NR_EPSILON_D)) {
				ArtSVP *svpa;
				/* Concept test */
				svpa = art_svp_translate (shape->fill_svp, gc->affine[4] - shape->ctm.c[4], gc->affine[5] - shape->ctm.c[5]);
				art_svp_free (shape->fill_svp);
				shape->fill_svp = svpa;
			}
			memcpy (shape->ctm.c, gc->affine, 6 * sizeof (double));
		}
	}

	if (shape->style->stroke.type != SP_PAINT_TYPE_NONE) {
		gdouble width;
		abp = art_bpath_affine_transform (shape->curve->bpath, gc->affine);
		vp = art_bez_path_to_vec (abp, 0.25);
		art_free (abp);
		pvp = art_vpath_perturb (vp);
		art_free (vp);
		width = sp_distance_d_matrix_d_transform (shape->style->stroke_width.computed, gc->affine);
		width = MAX (width, 0.125);
		shape->stroke_svp = art_svp_vpath_stroke (pvp,
							  shape->style->stroke_linejoin.value,
							  shape->style->stroke_linecap.value,
							  width,
							  shape->style->stroke_miterlimit.value, 0.25);
		art_free (pvp);
	}

	bbox.x0 = bbox.y0 = bbox.x1 = bbox.y1 = 0.0;
	if (shape->stroke_svp) art_drect_svp_union (&bbox, shape->stroke_svp);
	if (shape->fill_svp) art_drect_svp_union (&bbox, shape->fill_svp);
	if (art_drect_empty (&bbox)) return NR_ARENA_ITEM_STATE_ALL;

	item->bbox.x0 = bbox.x0 - 1.0;
	item->bbox.y0 = bbox.y0 - 1.0;
	item->bbox.x1 = bbox.x1 + 1.0;
	item->bbox.y1 = bbox.y1 + 1.0;
	nr_arena_request_render_rect (item->arena, &item->bbox);

	if (shape->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		/* fixme: This is probably not correct as bbox has to be the one of fill */
		shape->fill_painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (shape->style), gc->affine, &shape->paintbox);
	}
	if (shape->style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) {
		/* fixme: This is probably not correct as bbox has to be the one of fill */
		shape->stroke_painter = sp_paint_server_painter_new (SP_STYLE_STROKE_SERVER (shape->style), gc->affine, &shape->paintbox);
	}

	return NR_ARENA_ITEM_STATE_ALL;
}

static guint
nr_arena_shape_render (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	NRArenaShape *shape;
	SPStyle *style;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->curve) return item->state;
	if (!shape->style) return item->state;

	style = shape->style;

	if (shape->fill_svp) {
		NRBuffer *m;
		guint32 rgba;

		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (shape->fill_svp, area->x0, area->y0, area->x1, area->y1, m->px, m->rs);
		m->empty = FALSE;

		switch (style->fill.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->fill.value.color,
							   SP_SCALE24_TO_FLOAT (style->fill_opacity.value) *
							   SP_SCALE24_TO_FLOAT (style->opacity.value));
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (shape->fill_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				shape->fill_painter->fill (shape->fill_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
				pb->empty = FALSE;
				/* Composite */
				nr_render_buf_buf_mask (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0,
							pb, 0, 0,
							m, 0, 0);
				b->empty = FALSE;
				nr_buffer_free (pb);
			}
			break;
		default:
			break;
		}
		nr_buffer_free (m);
	}

	if (shape->stroke_svp) {
		NRBuffer *m;
		guint32 rgba;

		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (shape->stroke_svp, area->x0, area->y0, area->x1, area->y1, m->px, m->rs);
		m->empty = FALSE;

		switch (style->stroke.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->stroke.value.color,
							   SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) *
							   SP_SCALE24_TO_FLOAT (style->opacity.value));
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (shape->stroke_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				shape->stroke_painter->fill (shape->stroke_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
				pb->empty = FALSE;
				/* Composite */
				nr_render_buf_buf_mask (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0,
							pb, 0, 0,
							m, 0, 0);
				b->empty = FALSE;
				nr_buffer_free (pb);
			}
			break;
		default:
			break;
		}
		nr_buffer_free (m);
	}

	return item->state;
}

static guint
nr_arena_shape_clip (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	NRArenaShape *shape;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->curve) return item->state;

	if (shape->fill_svp) {
		art_gray_svp_aa (shape->fill_svp, area->x0, area->y0, area->x1, area->y1, b->px, b->rs);
		b->empty = FALSE;
	}

	return item->state;
}

static NRArenaItem *
nr_arena_shape_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	NRArenaShape *shape;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->curve) return NULL;
	if (!shape->style) return NULL;

	if (shape->fill_svp && (shape->style->fill.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (shape->fill_svp, x, y)) return item;
	}
	if (shape->stroke_svp && (shape->style->stroke.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (shape->stroke_svp, x, y)) return item;
	}
	if (delta > 1e-3) {
		if (shape->fill_svp && (shape->style->fill.type != SP_PAINT_TYPE_NONE)) {
			if (art_svp_point_dist (shape->fill_svp, x, y) <= delta) return item;
		}
		if (shape->stroke_svp && (shape->style->stroke.type != SP_PAINT_TYPE_NONE)) {
			if (art_svp_point_dist (shape->stroke_svp, x, y) <= delta) return item;
		}
	}

	return NULL;
}

void
nr_arena_shape_set_path (NRArenaShape *shape, SPCurve *curve, gboolean private, const gdouble *affine)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE (shape));

	nr_arena_item_request_render (NR_ARENA_ITEM (shape));

	if (shape->curve) {
		sp_curve_unref (shape->curve);
		shape->curve = NULL;
	}

	if (curve) {
		if (affine) {
			ArtBpath *abp;
			abp = art_bpath_affine_transform (curve->bpath, affine);
			curve = sp_curve_new_from_bpath (abp);
			shape->curve = curve;
		} else {
			shape->curve = curve;
			sp_curve_ref (curve);
		}
	}

	nr_arena_item_request_update (NR_ARENA_ITEM (shape), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_shape_set_style (NRArenaShape *shape, SPStyle *style)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE (shape));

	if (style) sp_style_ref (style);
	if (shape->style) sp_style_unref (shape->style);
	shape->style = style;

	nr_arena_item_request_update (NR_ARENA_ITEM (shape), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_shape_set_paintbox (NRArenaShape *shape, const ArtDRect *pbox)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE (shape));
	g_return_if_fail (pbox != NULL);

	if ((pbox->x0 < pbox->x1) && (pbox->y0 < pbox->y1)) {
		memcpy (&shape->paintbox, pbox, sizeof (ArtDRect));
	} else {
		/* fixme: We kill warning, although not sure what to do here (Lauris) */
		shape->paintbox.x0 = shape->paintbox.y0 = 0.0;
		shape->paintbox.x1 = shape->paintbox.y1 = 256.0;
	}

	nr_arena_item_request_update (NR_ARENA_ITEM (shape), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

