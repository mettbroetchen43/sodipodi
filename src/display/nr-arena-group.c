#define __NR_ARENA_GROUP_C__

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
#include "../helper/nr-plain-stuff.h"
#include "../helper/nr-buffers.h"
#include "nr-arena-group.h"

static void nr_arena_group_class_init (NRArenaGroupClass *klass);
static void nr_arena_group_init (NRArenaGroup *group);
static void nr_arena_group_destroy (GtkObject *object);

static NRArenaItem *nr_arena_group_children (NRArenaItem *item);
static void nr_arena_group_add_child (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);
static void nr_arena_group_remove_child (NRArenaItem *item, NRArenaItem *child);
static void nr_arena_group_set_child_position (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);

static guint nr_arena_group_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_group_render (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static guint nr_arena_group_clip (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static NRArenaItem *nr_arena_group_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky);

static NRArenaItemClass *parent_class;

GtkType
nr_arena_group_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaGroup",
			sizeof (NRArenaGroup),
			sizeof (NRArenaGroupClass),
			(GtkClassInitFunc) nr_arena_group_class_init,
			(GtkObjectInitFunc) nr_arena_group_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_ITEM, &info);
	}
	return type;
}

static void
nr_arena_group_class_init (NRArenaGroupClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	parent_class = gtk_type_class (NR_TYPE_ARENA_ITEM);

	object_class->destroy = nr_arena_group_destroy;

	item_class->children = nr_arena_group_children;
	item_class->add_child = nr_arena_group_add_child;
	item_class->set_child_position = nr_arena_group_set_child_position;
	item_class->remove_child = nr_arena_group_remove_child;
	item_class->update = nr_arena_group_update;
	item_class->render = nr_arena_group_render;
	item_class->clip = nr_arena_group_clip;
	item_class->pick = nr_arena_group_pick;
}

static void
nr_arena_group_init (NRArenaGroup *group)
{
	group->transparent = FALSE;
	group->children = NULL;
	group->last = NULL;
}

static void
nr_arena_group_destroy (GtkObject *object)
{
	NRArenaItem *item;
	NRArenaGroup *group;

	item = NR_ARENA_ITEM (object);
	group = NR_ARENA_GROUP (object);

	while (group->children) {
		group->children = nr_arena_item_detach_unref (item, group->children);
	}

	group->last = NULL;

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static NRArenaItem *
nr_arena_group_children (NRArenaItem *item)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (item);

	return group->children;
}

static void
nr_arena_group_add_child (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (item);

	if (!ref) {
		group->children = nr_arena_item_attach_ref (item, child, NULL, group->children);
	} else {
		ref->next = nr_arena_item_attach_ref (item, child, ref, ref->next);
	}

	if (ref == group->last) group->last = child;

	nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

static void
nr_arena_group_remove_child (NRArenaItem *item, NRArenaItem *child)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (item);

	if (child == group->last) group->last = child->prev;

	if (child->prev) {
		nr_arena_item_detach_unref (item, child);
	} else {
		group->children = nr_arena_item_detach_unref (item, child);
	}

	nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

static void
nr_arena_group_set_child_position (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
	NRArenaGroup *group;

	group = NR_ARENA_GROUP (item);

	nr_arena_item_ref (child);

	if (child == group->last) group->last = child->prev;

	if (child->prev) {
		nr_arena_item_detach_unref (item, child);
	} else {
		group->children = nr_arena_item_detach_unref (item, child);
	}

	if (!ref) {
		group->children = nr_arena_item_attach_ref (item, child, NULL, group->children);
	} else {
		ref->next = nr_arena_item_attach_ref (item, child, ref, ref->next);
	}

	if (ref == group->last) group->last = child;

	nr_arena_item_unref (child);

	nr_arena_item_request_render (child);
}

static guint
nr_arena_group_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset)
{
	NRArenaGroup *group;
	NRArenaItem *child;
	guint newstate, beststate;

	group = NR_ARENA_GROUP (item);

	beststate = NR_ARENA_ITEM_STATE_ALL;

	for (child = group->children; child != NULL; child = child->next) {
		newstate = nr_arena_item_invoke_update (child, area, gc, state, reset);
		g_return_val_if_fail (!(~newstate & state), newstate);
		beststate = beststate & newstate;
	}

	if (beststate & NR_ARENA_ITEM_STATE_BBOX) {
		nr_irect_set_empty (&item->bbox);
		for (child = group->children; child != NULL; child = child->next) {
			nr_irect_union (&item->bbox, &item->bbox, &child->bbox);
		}
	}

	return beststate;
}

static guint
nr_arena_group_render (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaGroup *group;
	NRArenaItem *child;
	guint ret;

	group = NR_ARENA_GROUP (item);

	ret = item->state;

	if (item->opacity == 1.0) {
		/* Just compose children into parent buffer */
		for (child = group->children; child != NULL; child = child->next) {
			ret = nr_arena_item_invoke_render (child, area, b);
			if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) break;
		}
	} else {
		NRBuffer *nb;
		/* Need intermediate composing */
		/* fixme: */
		g_assert ((area->x1 - area->x0) * (area->y1 - area->y0) <= 4096);
		nb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
		/* fixme: */
		nb->empty = FALSE;
		for (child = group->children; child != NULL; child = child->next) {
			ret = nr_arena_item_invoke_render (child, area, nb);
			if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) break;
		}
		if (ret & NR_ARENA_ITEM_STATE_RENDER) {
			/* Compose */
			nr_render_buf_buf_alpha (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0,
						 nb, 0, 0,
						 (guint) floor (item->opacity * 255.9999));
		}
		nr_buffer_free (nb);
	}

	g_return_val_if_fail (ret & NR_ARENA_ITEM_STATE_RENDER, ret);

	return ret;
}

static guint
nr_arena_group_clip (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaGroup *group;
	NRArenaItem *child;
	guint ret;

	group = NR_ARENA_GROUP (item);

	ret = item->state;

	/* Just compose children into parent buffer */
	for (child = group->children; child != NULL; child = child->next) {
		ret = nr_arena_item_invoke_clip (child, area, b);
		if (!(ret & NR_ARENA_ITEM_STATE_CLIP)) break;
	}

	g_return_val_if_fail (ret & NR_ARENA_ITEM_STATE_CLIP, ret);

	return ret;
}

static NRArenaItem *
nr_arena_group_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky)
{
	NRArenaGroup *group;
	NRArenaItem *child, *picked;

	group = NR_ARENA_GROUP (item);

	for (child = group->last; child != NULL; child = child->prev) {
		picked = nr_arena_item_invoke_pick (child, x, y, sticky);
		if (picked) return (group->transparent) ? picked : item;
	}

	return NULL;
}

void
nr_arena_group_set_transparent (NRArenaGroup *group, gboolean transparent)
{
	g_return_if_fail (group != NULL);
	g_return_if_fail (NR_IS_ARENA_GROUP (group));

	group->transparent = transparent;
}


