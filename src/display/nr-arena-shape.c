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
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_point.h>
#include <libart_lgpl/art_gray_svp.h>
#include "../helper/nr-buffers.h"
#include "../helper/nr-plain-stuff.h"
#include "../style.h"
#include "nr-arena.h"
#include "nr-arena-shape.h"

static void nr_arena_shape_class_init (NRArenaShapeClass *klass);
static void nr_arena_shape_init (NRArenaShape *shape);
static void nr_arena_shape_destroy (GtkObject *object);

static guint nr_arena_shape_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_shape_render (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static guint nr_arena_shape_clip (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static NRArenaItem *nr_arena_shape_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky);

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
	shape->style = NULL;
	shape->comp = NULL;
	shape->painter = NULL;
}

static void
nr_arena_shape_destroy (GtkObject *object)
{
	NRArenaShape *shape;

	shape = NR_ARENA_SHAPE (object);

	if (shape->painter) {
		sp_painter_free (shape->painter);
		shape->painter = NULL;
	}

	while (shape->comp) {
		sp_cpath_comp_unref (shape->comp);
		shape->comp = NULL;
	}

	if (shape->style) {
		sp_style_unref (shape->style);
		shape->style = NULL;
	}

	if (GTK_OBJECT_CLASS (shape_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (shape_parent_class)->destroy) (object);
}

static guint
nr_arena_shape_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset)
{
	NRArenaShape * shape;

	shape = NR_ARENA_SHAPE (item);

	nr_irect_set_empty (&item->bbox);

	if (shape->painter) {
		sp_painter_free (shape->painter);
		shape->painter = NULL;
	}

	if (shape->comp && shape->comp->archetype) {
		SPCPathComp *comp;
		NRIRect ibox;
		comp = shape->comp;
		ibox.x0 = comp->bbox.x0 - comp->stroke_width - 1.0;
		ibox.y0 = comp->bbox.y0 - comp->stroke_width - 1.0;
		ibox.x1 = comp->bbox.x1 + comp->stroke_width + 1.0;
		ibox.y1 = comp->bbox.y1 + comp->stroke_width + 1.0;
		nr_arena_request_render_rect (item->arena, &ibox);
	}

	if (!shape->style) return NR_ARENA_ITEM_STATE_ALL;

	if (shape->comp) {
		SPCPathComp *comp;
		comp = shape->comp;
		comp->rule = shape->style->fill_rule;
		if (shape->style->stroke.type != SP_PAINT_TYPE_NONE) {
			gdouble wx, wy;
			wx = gc->affine[0] + gc->affine[2];
			wy = gc->affine[1] + gc->affine[3];
			comp->stroke_width = shape->style->user_stroke_width * sqrt (wx * wx + wy * wy) * 0.707106781;
		} else {
			comp->stroke_width = 0.0;
		}
		comp->join = shape->style->stroke_linejoin;
		comp->cap = shape->style->stroke_linecap;
		sp_cpath_comp_update (comp, gc->affine);
	}

	if (shape->comp && shape->comp->archetype) {
		SPCPathComp *comp;
		NRIRect ibox;
		comp = shape->comp;
		ibox.x0 = comp->bbox.x0 - comp->stroke_width - 1.0;
		ibox.y0 = comp->bbox.y0 - comp->stroke_width - 1.0;
		ibox.x1 = comp->bbox.x1 + comp->stroke_width + 1.0;
		ibox.y1 = comp->bbox.y1 + comp->stroke_width + 1.0;
		nr_arena_request_render_rect (item->arena, &ibox);
	}

	if (shape->comp) {
		SPCPathComp *comp;
		comp = shape->comp;
		item->bbox.x0 = comp->bbox.x0;
		item->bbox.y0 = comp->bbox.y0;
		item->bbox.x1 = comp->bbox.x1;
		item->bbox.y1 = comp->bbox.y1;
	}

	if (shape->comp && shape->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		ArtDRect bbox;
		bbox.x0 = item->bbox.x0;
		bbox.y0 = item->bbox.y0;
		bbox.x1 = item->bbox.x1;
		bbox.y1 = item->bbox.y1;
		shape->painter = sp_paint_server_painter_new (shape->style->fill.server, gc->affine, shape->style->opacity, &bbox);
	}

	return NR_ARENA_ITEM_STATE_ALL;
}

static guint
nr_arena_shape_render (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaShape *shape;
	SPCPathComp *comp;
	SPStyle *style;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->comp) return item->state;
	if (!shape->style) return item->state;

	style = shape->style;

	comp = shape->comp;
	if (comp->closed) {
		NRBuffer *m;
		guint32 rgba;

		m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		art_gray_svp_aa (comp->archetype->svp,
				 area->x0 - comp->cx, area->y0 - comp->cy,
				 area->x1 - comp->cx, area->y1 - comp->cy,
				 m->px, m->rs);
		m->empty = FALSE;

		switch (style->fill.type) {
		case SP_PAINT_TYPE_COLOR:
			rgba = sp_color_get_rgba32_falpha (&style->fill.color, style->fill_opacity * style->opacity);
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (shape->painter) {
				NRBuffer *pb;
				/* Need separate gradient buffer */
				pb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, FALSE);
				shape->painter->fill (shape->painter, pb->px, area->x0, area->y0, pb->w, pb->h, pb->rs);
				pb->empty = FALSE;
				/* Composite */
				nr_render_buf_buf_mask (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0,
							pb, 0, 0,
							m, 0, 0);
				b->empty = FALSE;
				nr_buffer_free (pb);
				break;
			default:
				break;
			}
		}
		nr_buffer_free (m);
	}

	switch (style->stroke.type) {
	case SP_PAINT_TYPE_COLOR:
		if (comp->archetype->stroke) {
			guint32 rgba;
			NRBuffer *m;

			m = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
			art_gray_svp_aa (comp->archetype->stroke,
					 area->x0 - comp->cx, area->y0 - comp->cy,
					 area->x1 - comp->cx, area->y1 - comp->cy,
					 m->px, m->rs);
			m->empty = FALSE;
			rgba = sp_color_get_rgba32_falpha (&style->stroke.color, style->stroke_opacity * style->real_opacity);
			nr_render_buf_mask_rgba32 (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0, m, 0, 0, rgba);
			b->empty = FALSE;
			nr_buffer_free (m);
		}
		break;
	default:
		break;
	}

	return item->state;
}

static guint
nr_arena_shape_clip (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaShape *shape;
	SPCPathComp *comp;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->comp) return item->state;

	comp = shape->comp;
	if (comp->closed) {

		art_gray_svp_aa (comp->archetype->svp,
				 area->x0 - comp->cx, area->y0 - comp->cy,
				 area->x1 - comp->cx, area->y1 - comp->cy,
				 b->px, b->rs);
		b->empty = FALSE;
	}

	return item->state;
}

static NRArenaItem *
nr_arena_shape_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky)
{
	NRArenaShape *shape;
	SPCPathComp *comp;

	shape = NR_ARENA_SHAPE (item);

	if (!shape->comp) return NULL;
	if (!shape->style) return NULL;

	comp = shape->comp;
	if (!comp->archetype) return NULL;

	if (comp->closed && comp->archetype->svp && (shape->style->fill.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (comp->archetype->svp, x - comp->cx, y - comp->cy)) return item;
	}
	if (comp->archetype->stroke && (shape->style->stroke.type != SP_PAINT_TYPE_NONE)) {
		if (art_svp_point_wind (comp->archetype->stroke, x - comp->cx, y - comp->cy)) return item;
	}

	return NULL;
}

void
nr_arena_shape_set_path (NRArenaShape *shape, SPCurve *curve, gboolean private, const gdouble *affine)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE (shape));

	nr_arena_item_request_render (NR_ARENA_ITEM (shape));

	if (shape->comp) {
		sp_cpath_comp_unref (shape->comp);
		shape->comp = NULL;
	}

	if (curve) {
		shape->comp = sp_cpath_comp_new (curve, private, (gdouble *) affine, 0.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);
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


static void nr_arena_shape_group_class_init (NRArenaShapeGroupClass *klass);
static void nr_arena_shape_group_init (NRArenaShapeGroup *group);
static void nr_arena_shape_group_destroy (GtkObject *object);

static guint nr_arena_shape_group_render (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static NRArenaItem *nr_arena_shape_group_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky);

static NRArenaGroupClass *group_parent_class;

GtkType
nr_arena_shape_group_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaShapeGroup",
			sizeof (NRArenaShapeGroup),
			sizeof (NRArenaShapeGroupClass),
			(GtkClassInitFunc) nr_arena_shape_group_class_init,
			(GtkObjectInitFunc) nr_arena_shape_group_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_GROUP, &info);
	}
	return type;
}

static void
nr_arena_shape_group_class_init (NRArenaShapeGroupClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	group_parent_class = gtk_type_class (NR_TYPE_ARENA_GROUP);

	object_class->destroy = nr_arena_shape_group_destroy;

	item_class->render = nr_arena_shape_group_render;
	item_class->pick = nr_arena_shape_group_pick;
}

static void
nr_arena_shape_group_init (NRArenaShapeGroup *group)
{
	group->style = NULL;
}

static void
nr_arena_shape_group_destroy (GtkObject *object)
{
	NRArenaShapeGroup *group;

	group = NR_ARENA_SHAPE_GROUP (object);

	if (group->style) {
		sp_style_unref (group->style);
		group->style = NULL;
	}

	if (GTK_OBJECT_CLASS (group_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (group_parent_class)->destroy) (object);
}

/* This sucks - as soon, as we have inheritable renderprops, do something with that opacity */

static guint
nr_arena_shape_group_render (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaGroup *group;
	NRArenaItem *child;
	guint ret;

	group = NR_ARENA_GROUP (item);

	ret = item->state;

	for (child = group->children; child != NULL; child = child->next) {
		ret = nr_arena_item_invoke_render (child, area, b);
		if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) break;
	}

	g_return_val_if_fail (ret & NR_ARENA_ITEM_STATE_RENDER, ret);

	return ret;
}

static NRArenaItem *
nr_arena_shape_group_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky)
{
	NRArenaGroup *group;
	NRArenaItem *picked;

	group = NR_ARENA_GROUP (item);

	picked = NULL;

	if (((NRArenaItemClass *) group_parent_class)->pick)
		picked = ((NRArenaItemClass *) group_parent_class)->pick (item, x, y, sticky);

	if (picked) picked = item;

	return picked;
}

void
nr_arena_shape_group_clear (NRArenaShapeGroup *sg)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (sg);

	nr_arena_item_request_render (NR_ARENA_ITEM (group));

	while (group->children) {
		nr_arena_item_remove_child (NR_ARENA_ITEM (group), group->children);
	}
}

void
nr_arena_shape_group_add_component (NRArenaShapeGroup *sg, SPCurve *curve, gboolean private, const gdouble *affine)
{
	NRArenaGroup *group;
	NRArenaItem *new;

	group = NR_ARENA_GROUP (sg);

	nr_arena_item_request_render (NR_ARENA_ITEM (group));

	new = nr_arena_item_new (NR_ARENA_ITEM (group)->arena, NR_TYPE_ARENA_SHAPE);
	nr_arena_item_append_child (NR_ARENA_ITEM (group), new);
	nr_arena_shape_set_path (NR_ARENA_SHAPE (new), curve, private, affine);
	nr_arena_shape_set_style (NR_ARENA_SHAPE (new), sg->style);
}

void
nr_arena_shape_group_set_component (NRArenaShapeGroup *sg, SPCurve *curve, gboolean private, const gdouble *affine)
{
	g_return_if_fail (sg != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE_GROUP (sg));

	nr_arena_shape_group_clear (sg);
	nr_arena_shape_group_add_component (sg, curve, private, affine);
}

void
nr_arena_shape_group_set_style (NRArenaShapeGroup *sg, SPStyle *style)
{
	NRArenaGroup *group;
	NRArenaItem *child;

	g_return_if_fail (sg != NULL);
	g_return_if_fail (NR_IS_ARENA_SHAPE_GROUP (sg));

	group = NR_ARENA_GROUP (sg);

	if (style) sp_style_ref (style);
	if (sg->style) sp_style_unref (sg->style);
	sg->style = style;

	for (child = group->children; child != NULL; child = child->next) {
		g_return_if_fail (NR_IS_ARENA_SHAPE (child));
		nr_arena_shape_set_style (NR_ARENA_SHAPE (child), sg->style);
	}
}

