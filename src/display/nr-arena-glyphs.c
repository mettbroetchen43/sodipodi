#define __NR_ARENA_GLYPHS_C__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 *
 */


#include <math.h>
#include <string.h>
#include <libnr/nr-rect.h>
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
#include "nr-arena-glyphs.h"

static void nr_arena_glyphs_class_init (NRArenaGlyphsClass *klass);
static void nr_arena_glyphs_init (NRArenaGlyphs *glyphs);
static void nr_arena_glyphs_destroy (GtkObject *object);

static guint nr_arena_glyphs_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_glyphs_render (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static guint nr_arena_glyphs_clip (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static NRArenaItem *nr_arena_glyphs_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

static NRArenaItemClass *glyphs_parent_class;

GtkType
nr_arena_glyphs_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaGlyphs",
			sizeof (NRArenaGlyphs),
			sizeof (NRArenaGlyphsClass),
			(GtkClassInitFunc) nr_arena_glyphs_class_init,
			(GtkObjectInitFunc) nr_arena_glyphs_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_ITEM, &info);
	}
	return type;
}

static void
nr_arena_glyphs_class_init (NRArenaGlyphsClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	glyphs_parent_class = gtk_type_class (NR_TYPE_ARENA_ITEM);

	object_class->destroy = nr_arena_glyphs_destroy;

	item_class->update = nr_arena_glyphs_update;
	item_class->render = nr_arena_glyphs_render;
	item_class->clip = nr_arena_glyphs_clip;
	item_class->pick = nr_arena_glyphs_pick;
}

static void
nr_arena_glyphs_init (NRArenaGlyphs *glyphs)
{
	glyphs->curve = NULL;
	glyphs->style = NULL;
#if 0
	glyphs->fill_painter = NULL;
	glyphs->stroke_painter = NULL;
#endif
	glyphs->fill_svp = NULL;
	glyphs->stroke_svp = NULL;
}

static void
nr_arena_glyphs_destroy (GtkObject *object)
{
	NRArenaGlyphs *glyphs;

	glyphs = NR_ARENA_GLYPHS (object);

	if (glyphs->fill_svp) {
		art_svp_free (glyphs->fill_svp);
		glyphs->fill_svp = NULL;
	}

	if (glyphs->stroke_svp) {
		art_svp_free (glyphs->stroke_svp);
		glyphs->stroke_svp = NULL;
	}

#if 0
	if (glyphs->fill_painter) {
		sp_painter_free (glyphs->fill_painter);
		glyphs->fill_painter = NULL;
	}

	if (glyphs->stroke_painter) {
		sp_painter_free (glyphs->stroke_painter);
		glyphs->stroke_painter = NULL;
	}
#endif

	if (glyphs->style) {
		sp_style_unref (glyphs->style);
		glyphs->style = NULL;
	}

	if (glyphs->curve) {
		sp_curve_unref (glyphs->curve);
		glyphs->curve = NULL;
	}

	if (GTK_OBJECT_CLASS (glyphs_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (glyphs_parent_class)->destroy) (object);
}

static guint
nr_arena_glyphs_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
{
	NRArenaGlyphs *glyphs;
	ArtBpath *abp;
	ArtVpath *vp, *pvp;
	ArtDRect bbox;

	glyphs = NR_ARENA_GLYPHS (item);

	/* Request repaint old area if needed */
	/* fixme: Think about it a bit (Lauris) */
	if (!nr_rect_l_test_empty (&item->bbox)) {
		nr_arena_request_render_rect (item->arena, &item->bbox);
		nr_rect_l_set_empty (&item->bbox);
	}

	/* Release state data */
	if (glyphs->fill_svp) {
		art_svp_free (glyphs->fill_svp);
		glyphs->fill_svp = NULL;
	}
	if (glyphs->stroke_svp) {
		art_svp_free (glyphs->stroke_svp);
		glyphs->stroke_svp = NULL;
	}
#if 0
	if (glyphs->fill_painter) {
		sp_painter_free (glyphs->fill_painter);
		glyphs->fill_painter = NULL;
	}
	if (glyphs->stroke_painter) {
		sp_painter_free (glyphs->stroke_painter);
		glyphs->stroke_painter = NULL;
	}
#endif

	if (!glyphs->curve || !glyphs->style) return NR_ARENA_ITEM_STATE_ALL;
	if ((glyphs->style->fill.type == SP_PAINT_TYPE_NONE) && (glyphs->style->stroke.type == SP_PAINT_TYPE_NONE)) return NR_ARENA_ITEM_STATE_ALL;

	/* Build state data */
	abp = art_bpath_affine_transform (glyphs->curve->bpath, gc->affine);
	vp = art_bez_path_to_vec (abp, 0.25);
	art_free (abp);
	pvp = art_vpath_perturb (vp);
	art_free (vp);

	if (glyphs->style->fill.type != SP_PAINT_TYPE_NONE) {
		ArtSVP *svpa, *svpb;
		svpa = art_svp_from_vpath (pvp);
		svpb = art_svp_uncross (svpa);
		art_svp_free (svpa);
		glyphs->fill_svp = art_svp_rewind_uncrossed (svpb, glyphs->style->fill_rule.value);
		art_svp_free (svpb);
	}

	if (glyphs->style->stroke.type != SP_PAINT_TYPE_NONE) {
		gdouble width;
		width = sp_distance_d_matrix_d_transform (glyphs->style->stroke_width.computed, gc->affine);
		if (width > 0.125) {
			glyphs->stroke_svp = art_svp_vpath_stroke (pvp,
								   glyphs->style->stroke_linejoin.value,
								   glyphs->style->stroke_linecap.value,
								   width,
								   glyphs->style->stroke_miterlimit.value, 0.25);
		}
	}

	art_free (pvp);

	bbox.x0 = bbox.y0 = bbox.x1 = bbox.y1 = 0.0;
	if (glyphs->stroke_svp) art_drect_svp_union (&bbox, glyphs->stroke_svp);
	if (glyphs->fill_svp) art_drect_svp_union (&bbox, glyphs->fill_svp);
	if (art_drect_empty (&bbox)) return NR_ARENA_ITEM_STATE_ALL;

	item->bbox.x0 = bbox.x0 - 1.0;
	item->bbox.y0 = bbox.y0 - 1.0;
	item->bbox.x1 = bbox.x1 + 1.0;
	item->bbox.y1 = bbox.y1 + 1.0;
	nr_arena_request_render_rect (item->arena, &item->bbox);

#if 0
	if (glyphs->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		/* fixme: This is probably not correct as bbox has to be the one of fill */
		g_warning ("Glyphs gradient fill: Implement this");
		glyphs->fill_painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (glyphs->style), gc->affine, &bbox);
	}
	if (glyphs->style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) {
		/* fixme: This is probably not correct as bbox has to be the one of fill */
		glyphs->stroke_painter = sp_paint_server_painter_new (SP_STYLE_STROKE_SERVER (glyphs->style), gc->affine, &bbox);
	}
#endif

	return NR_ARENA_ITEM_STATE_ALL;
}

static guint
nr_arena_glyphs_render (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
#if 0
	NRArenaGlyphs *glyphs;
	SPStyle *style;

	glyphs = NR_ARENA_GLYPHS (item);

	if (!glyphs->curve) return item->state;
	if (!glyphs->style) return item->state;

	style = glyphs->style;

	if (glyphs->fill_svp) {
		NRBuffer *m;
		guint32 rgba;

		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (glyphs->fill_svp, area->x0, area->y0, area->x1, area->y1, m->px, m->rs);
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
			if (glyphs->fill_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				glyphs->fill_painter->fill (glyphs->fill_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
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

	if (glyphs->stroke_svp) {
		NRBuffer *m;
		guint32 rgba;

		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (glyphs->stroke_svp, area->x0, area->y0, area->x1, area->y1, m->px, m->rs);
		m->empty = FALSE;

		switch (style->stroke.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->stroke.value.color,
							   SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) *
							   SP_SCALE24_TO_FLOAT (style->opacity.value));
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			nr_buffer_free (m);
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (glyphs->stroke_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				glyphs->stroke_painter->fill (glyphs->stroke_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
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
#endif

	return item->state;
}

static guint
nr_arena_glyphs_clip (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	NRArenaGlyphs *glyphs;

	glyphs = NR_ARENA_GLYPHS (item);

	if (!glyphs->curve) return item->state;

	if (glyphs->fill_svp) {
		art_gray_svp_aa (glyphs->fill_svp, area->x0, area->y0, area->x1, area->y1, b->px, b->rs);
		b->empty = FALSE;
	}

	return item->state;
}

static NRArenaItem *
nr_arena_glyphs_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	NRArenaGlyphs *glyphs;

	glyphs = NR_ARENA_GLYPHS (item);

	if (!glyphs->curve) return NULL;
	if (!glyphs->style) return NULL;

	if (glyphs->fill_svp && (glyphs->style->fill.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (glyphs->fill_svp, x, y)) return item;
	}
	if (glyphs->stroke_svp && (glyphs->style->stroke.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (glyphs->stroke_svp, x, y)) return item;
	}
	if (delta > 1e-3) {
		if (glyphs->fill_svp && (glyphs->style->fill.type != SP_PAINT_TYPE_NONE)) {
			if (art_svp_point_dist (glyphs->fill_svp, x, y) <= delta) return item;
		}
		if (glyphs->stroke_svp && (glyphs->style->stroke.type != SP_PAINT_TYPE_NONE)) {
			if (art_svp_point_dist (glyphs->stroke_svp, x, y) <= delta) return item;
		}
	}

	return NULL;
}

void
nr_arena_glyphs_set_path (NRArenaGlyphs *glyphs, SPCurve *curve, gboolean private, const gdouble *affine)
{
	g_return_if_fail (glyphs != NULL);
	g_return_if_fail (NR_IS_ARENA_GLYPHS (glyphs));

	nr_arena_item_request_render (NR_ARENA_ITEM (glyphs));

	if (glyphs->curve) {
		sp_curve_unref (glyphs->curve);
		glyphs->curve = NULL;
	}

	if (curve) {
		if (affine) {
			ArtBpath *abp;
			abp = art_bpath_affine_transform (curve->bpath, affine);
			curve = sp_curve_new_from_bpath (abp);
			glyphs->curve = curve;
		} else {
			glyphs->curve = curve;
			sp_curve_ref (curve);
		}
	}

	nr_arena_item_request_update (NR_ARENA_ITEM (glyphs), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_glyphs_set_style (NRArenaGlyphs *glyphs, SPStyle *style)
{
	g_return_if_fail (glyphs != NULL);
	g_return_if_fail (NR_IS_ARENA_GLYPHS (glyphs));

	if (style) sp_style_ref (style);
	if (glyphs->style) sp_style_unref (glyphs->style);
	glyphs->style = style;

	nr_arena_item_request_update (NR_ARENA_ITEM (glyphs), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

static guint
nr_arena_glyphs_fill_mask (NRArenaGlyphs *glyphs, NRRectL *area, NRBuffer *b)
{
	NRArenaItem *item;

	item = NR_ARENA_ITEM (glyphs);

	if (glyphs->fill_svp && nr_rect_l_test_intersect (area, &item->bbox)) {
		NRBuffer *gb;
		gint x, y;
		gb = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (glyphs->fill_svp, area->x0, area->y0, area->x1, area->y1, gb->px, gb->rs);
		for (y = area->y0; y < area->y1; y++) {
			guchar *d, *s;
			d = b->px + (y - area->y0) * b->rs;
			s = gb->px + (y - area->y0) * b->rs;
			for (x = area->x0; x < area->x1; x++) {
				*d = (*d) + ((255 - *d) * (*s) / 255);
				d += 1;
				s += 1;
			}
		}
		nr_buffer_free (gb);
		b->empty = FALSE;
	}

	return item->state;
}

static guint
nr_arena_glyphs_stroke_mask (NRArenaGlyphs *glyphs, NRRectL *area, NRBuffer *b)
{
	NRArenaItem *item;

	item = NR_ARENA_ITEM (glyphs);

	if (glyphs->stroke_svp && nr_rect_l_test_intersect (area, &item->bbox)) {
		NRBuffer *gb;
		gint x, y;
		gb = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (glyphs->stroke_svp, area->x0, area->y0, area->x1, area->y1, gb->px, gb->rs);
		for (y = area->y0; y < area->y1; y++) {
			guchar *d, *s;
			d = b->px + (y - area->y0) * b->rs;
			s = gb->px + (y - area->y0) * b->rs;
			for (x = area->x0; x < area->x1; x++) {
				*d = (*d) + ((255 - *d) * (*s) / 255);
				d += 1;
				s += 1;
			}
		}
		nr_buffer_free (gb);
		b->empty = FALSE;
	}

	return item->state;
}

static void nr_arena_glyphs_group_class_init (NRArenaGlyphsGroupClass *klass);
static void nr_arena_glyphs_group_init (NRArenaGlyphsGroup *group);
static void nr_arena_glyphs_group_destroy (GtkObject *object);

static guint nr_arena_glyphs_group_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_glyphs_group_render (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static NRArenaItem *nr_arena_glyphs_group_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

static NRArenaGroupClass *group_parent_class;

GtkType
nr_arena_glyphs_group_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaGlyphsGroup",
			sizeof (NRArenaGlyphsGroup),
			sizeof (NRArenaGlyphsGroupClass),
			(GtkClassInitFunc) nr_arena_glyphs_group_class_init,
			(GtkObjectInitFunc) nr_arena_glyphs_group_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_GROUP, &info);
	}
	return type;
}

static void
nr_arena_glyphs_group_class_init (NRArenaGlyphsGroupClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	group_parent_class = gtk_type_class (NR_TYPE_ARENA_GROUP);

	object_class->destroy = nr_arena_glyphs_group_destroy;

	item_class->update = nr_arena_glyphs_group_update;
	item_class->render = nr_arena_glyphs_group_render;
	item_class->pick = nr_arena_glyphs_group_pick;
}

static void
nr_arena_glyphs_group_init (NRArenaGlyphsGroup *group)
{
	group->style = NULL;
	group->paintbox.x0 = group->paintbox.y0 = 0.0;
	group->paintbox.x1 = group->paintbox.y1 = 1.0;

	group->fill_painter = NULL;
	group->stroke_painter = NULL;
}

static void
nr_arena_glyphs_group_destroy (GtkObject *object)
{
	NRArenaGlyphsGroup *group;

	group = NR_ARENA_GLYPHS_GROUP (object);

	if (group->fill_painter) {
		sp_painter_free (group->fill_painter);
		group->fill_painter = NULL;
	}

	if (group->stroke_painter) {
		sp_painter_free (group->stroke_painter);
		group->stroke_painter = NULL;
	}

	if (group->style) {
		sp_style_unref (group->style);
		group->style = NULL;
	}

	if (GTK_OBJECT_CLASS (group_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (group_parent_class)->destroy) (object);
}

static guint
nr_arena_glyphs_group_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
{
	NRArenaGlyphsGroup *group;

	group = NR_ARENA_GLYPHS_GROUP (item);

	if (group->fill_painter) {
		sp_painter_free (group->fill_painter);
		group->fill_painter = NULL;
	}

	if (group->stroke_painter) {
		sp_painter_free (group->stroke_painter);
		group->stroke_painter = NULL;
	}

	if (group->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		group->fill_painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (group->style), gc->affine, &group->paintbox);
	}

	if (group->style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) {
		group->stroke_painter = sp_paint_server_painter_new (SP_STYLE_STROKE_SERVER (group->style), gc->affine, &group->paintbox);
	}

	if (((NRArenaItemClass *) group_parent_class)->update)
		return ((NRArenaItemClass *) group_parent_class)->update (item, area, gc, state, reset);

	return NR_ARENA_ITEM_STATE_ALL;
}

/* This sucks - as soon, as we have inheritable renderprops, do something with that opacity */

static guint
nr_arena_glyphs_group_render (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	NRArenaGroup *group;
	NRArenaGlyphsGroup *ggroup;
	NRArenaItem *child;
	SPStyle *style;
	guint ret;

	group = NR_ARENA_GROUP (item);
	ggroup = NR_ARENA_GLYPHS_GROUP (item);
	style = ggroup->style;

	ret = item->state;

	/* Fill */
	if (style->fill.type != SP_PAINT_TYPE_NONE) {
		NRBuffer *m;
		guint32 rgba;
		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		/* Render children fill mask */
		for (child = group->children; child != NULL; child = child->next) {
			ret = nr_arena_glyphs_fill_mask (NR_ARENA_GLYPHS (child), area, m);
			if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) {
				nr_buffer_free (m);
				return ret;
			}
		}
		/* Composite into buffer */
		switch (style->fill.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->fill.value.color,
							   SP_SCALE24_TO_FLOAT (style->fill_opacity.value) *
							   SP_SCALE24_TO_FLOAT (style->opacity.value));
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (ggroup->fill_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				ggroup->fill_painter->fill (ggroup->fill_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
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

	/* Stroke */
	if (style->stroke.type != SP_PAINT_TYPE_NONE) {
		NRBuffer *m;
		guint32 rgba;
		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		/* Render children stroke mask */
		for (child = group->children; child != NULL; child = child->next) {
			ret = nr_arena_glyphs_stroke_mask (NR_ARENA_GLYPHS (child), area, m);
			if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) {
				nr_buffer_free (m);
				return ret;
			}
		}
		/* Composite into buffer */
		switch (style->stroke.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->stroke.value.color,
							   SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) *
							   SP_SCALE24_TO_FLOAT (style->opacity.value));
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (ggroup->stroke_painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				ggroup->stroke_painter->fill (ggroup->stroke_painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
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

	return ret;
}

static NRArenaItem *
nr_arena_glyphs_group_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	NRArenaGroup *group;
	NRArenaItem *picked;

	group = NR_ARENA_GROUP (item);

	picked = NULL;

	if (((NRArenaItemClass *) group_parent_class)->pick)
		picked = ((NRArenaItemClass *) group_parent_class)->pick (item, x, y, delta, sticky);

	if (picked) picked = item;

	return picked;
}

void
nr_arena_glyphs_group_clear (NRArenaGlyphsGroup *sg)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (sg);

	nr_arena_item_request_render (NR_ARENA_ITEM (group));

	while (group->children) {
		nr_arena_item_remove_child (NR_ARENA_ITEM (group), group->children);
	}
}

void
nr_arena_glyphs_group_add_component (NRArenaGlyphsGroup *sg, SPCurve *curve, gboolean private, const gdouble *affine)
{
	NRArenaGroup *group;
	NRArenaItem *new;

	group = NR_ARENA_GROUP (sg);

	nr_arena_item_request_render (NR_ARENA_ITEM (group));

	new = nr_arena_item_new (NR_ARENA_ITEM (group)->arena, NR_TYPE_ARENA_GLYPHS);
	nr_arena_item_append_child (NR_ARENA_ITEM (group), new);
	nr_arena_item_unref (new);
	nr_arena_glyphs_set_path (NR_ARENA_GLYPHS (new), curve, private, affine);
	nr_arena_glyphs_set_style (NR_ARENA_GLYPHS (new), sg->style);
}

void
nr_arena_glyphs_group_set_component (NRArenaGlyphsGroup *sg, SPCurve *curve, gboolean private, const gdouble *affine)
{
	g_return_if_fail (sg != NULL);
	g_return_if_fail (NR_IS_ARENA_GLYPHS_GROUP (sg));

	nr_arena_glyphs_group_clear (sg);
	nr_arena_glyphs_group_add_component (sg, curve, private, affine);
}

void
nr_arena_glyphs_group_set_style (NRArenaGlyphsGroup *sg, SPStyle *style)
{
	NRArenaGroup *group;
	NRArenaItem *child;

	g_return_if_fail (sg != NULL);
	g_return_if_fail (NR_IS_ARENA_GLYPHS_GROUP (sg));

	group = NR_ARENA_GROUP (sg);

	if (style) sp_style_ref (style);
	if (sg->style) sp_style_unref (sg->style);
	sg->style = style;

	for (child = group->children; child != NULL; child = child->next) {
		g_return_if_fail (NR_IS_ARENA_GLYPHS (child));
		nr_arena_glyphs_set_style (NR_ARENA_GLYPHS (child), sg->style);
	}

	nr_arena_item_request_update (NR_ARENA_ITEM (sg), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_glyphs_group_set_paintbox (NRArenaGlyphsGroup *gg, const ArtDRect *pbox)
{
	g_return_if_fail (gg != NULL);
	g_return_if_fail (NR_IS_ARENA_GLYPHS_GROUP (gg));
	g_return_if_fail (pbox != NULL);
	g_return_if_fail (pbox->x1 > pbox->x0);
	g_return_if_fail (pbox->y1 > pbox->y0);

	memcpy (&gg->paintbox, pbox, sizeof (ArtDRect));

	nr_arena_item_request_update (NR_ARENA_ITEM (gg), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

