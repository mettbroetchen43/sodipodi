#define __NR_ARENA_ITEM_C__

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

#define noNR_ARENA_ITEM_VERBOSE

#include <string.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include "../helper/nr-plain-stuff.h"
#include "nr-arena.h"
#include "nr-arena-item.h"

enum {
	EVENT,
	LAST_SIGNAL
};

static void nr_arena_item_class_init (NRArenaItemClass *klass);
static void nr_arena_item_init (NRArenaItem *item);
static void nr_arena_item_private_destroy (GtkObject *object);

static GtkObjectClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
nr_arena_item_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaItem",
			sizeof (NRArenaItem),
			sizeof (NRArenaItemClass),
			(GtkClassInitFunc) nr_arena_item_class_init,
			(GtkObjectInitFunc) nr_arena_item_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_OBJECT, &info);
	}
	return type;
}

static void
nr_arena_item_class_init (NRArenaItemClass *klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	signals[EVENT] = gtk_signal_new ("event",
					 GTK_RUN_LAST,
					 object_class->type,
					 GTK_SIGNAL_OFFSET (NRArenaItemClass, event),
					 gtk_marshal_INT__POINTER,
					 GTK_TYPE_INT, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = nr_arena_item_private_destroy;
}

static void
nr_arena_item_init (NRArenaItem *item)
{
	item->arena = NULL;
	item->parent = NULL;
	item->next = NULL;
	item->prev = NULL;

	item->state = NR_ARENA_ITEM_STATE_NONE;
	item->sensitive = TRUE;
	/* fixme: Initialize bbox */
	item->affine = NULL;
	item->opacity = 1.0;

	item->clip = NULL;
}

static void
nr_arena_item_private_destroy (GtkObject *object)
{
	NRArenaItem *item;

	item = NR_ARENA_ITEM (object);

	/* Parent has to refcount children */
	g_assert (!item->parent);
	g_assert (!item->prev);
	g_assert (!item->next);

	nr_arena_remove_item (item->arena, item);

	if (item->affine) {
		g_free (item->affine);
		item->affine = NULL;
	}

	if (item->clip) {
		item->clip = nr_arena_item_detach_unref (item, item->clip);
	}

	item->arena = NULL;

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

NRArenaItem *
nr_arena_item_children (NRArenaItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

	if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->children)
		return ((NRArenaItemClass *) ((GtkObject *) item)->klass)->children (item);

	return NULL;
}

void
nr_arena_item_add_child (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (child != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (child));
	g_return_if_fail (child->parent == NULL);
	g_return_if_fail (child->prev == NULL);
	g_return_if_fail (child->next == NULL);
	g_return_if_fail (child->arena == item->arena);
	g_return_if_fail (child != ref);
	g_return_if_fail (!ref || NR_IS_ARENA_ITEM (ref));
	g_return_if_fail (!ref || (ref->parent == item));

	if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->add_child)
		((NRArenaItemClass *) ((GtkObject *) item)->klass)->add_child (item, child, ref);
}

void
nr_arena_item_remove_child (NRArenaItem *item, NRArenaItem *child)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (child != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (child));
	g_return_if_fail (child->parent == item);

	if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->remove_child)
		((NRArenaItemClass *) ((GtkObject *) item)->klass)->remove_child (item, child);
}

void
nr_arena_item_set_child_position (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (child != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (child));
	g_return_if_fail (child->parent == item);
	g_return_if_fail (!ref || NR_IS_ARENA_ITEM (ref));
	g_return_if_fail (!ref || (ref->parent == item));

	if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->set_child_position)
		((NRArenaItemClass *) ((GtkObject *) item)->klass)->set_child_position (item, child, ref);
}

guint
nr_arena_item_invoke_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset)
{
	guint newstate;
	NRGC newgc;

	g_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (!(state & NR_ARENA_ITEM_STATE_INVALID), NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_VERBOSE
	g_print ("Invoke update %p: %d %d\n", item, state, reset);
#endif

	/* Propagation means to clear children at least to our state */
	if (item->propagate) {
		reset |= ~item->state;
		item->propagate = FALSE;
	}

	/* Reset requested flags on our state */
	item->state &= ~reset;

	/* Set up local gc */
	if (item->affine) {
		art_affine_multiply (newgc.affine, item->affine, gc->affine);
	} else {
		memcpy (newgc.affine, gc->affine, 6 * sizeof (gdouble));
	}

	if (item->clip) {
		/* Update our clip */
		newstate = nr_arena_item_invoke_update (item->clip, area, &newgc, state, reset);
		g_return_val_if_fail (!(~newstate & state), newstate);
	}

	if (!(item->state & NR_ARENA_ITEM_STATE_BBOX)) {
		/* BBox state not set - need bbox level update before continuing */
		newstate = ((NRArenaItemClass *) ((GtkObject *) item)->klass)->update (item, area, &newgc, NR_ARENA_ITEM_STATE_BBOX, reset);
		g_return_val_if_fail (newstate & NR_ARENA_ITEM_STATE_BBOX, newstate);
		item->state = newstate;
	}

	if (item->clip) {
		/* We have do do intersect with clip bbox */
		nr_irect_intersection (&item->bbox, &item->bbox, &item->clip->bbox);
	}

	if ((~item->state & state) && (!area || nr_irect_do_intersect (area, &item->bbox))) {
		/* Need update to given state */
		newstate = ((NRArenaItemClass *) ((GtkObject *) item)->klass)->update (item, area, &newgc, state, reset);
		g_return_val_if_fail (!(~newstate & state), newstate);
		item->state = newstate;
	}

	return item->state;
}

guint
nr_arena_item_invoke_render (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	g_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_RENDER, item->state);
	g_return_val_if_fail (b->w >= (area->x1 - area->x0), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (b->h >= (area->y1 - area->y0), NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_VERBOSE
	g_print ("Invoke render %p: %d %d - %d %d\n", item, area->x0, area->y0, area->x1, area->y1);
#endif

	if (nr_irect_do_intersect (area, &item->bbox)) {
		/* Need render that item */
		/* fixme: clip updating etc. needs serious contemplation */
		if (item->clip) {
			NRBuffer *nb, *cb;
			guint ret;
			nb = nr_buffer_get (NR_IMAGE_R8G8B8A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
			cb = nr_buffer_get (NR_IMAGE_A8, area->x1 - area->x0, area->y1 - area->y0, TRUE, TRUE);
			ret = nr_arena_item_invoke_clip (item->clip, area, cb);
			/* fixme: */
			cb->empty = FALSE;
			ret = ((NRArenaItemClass *) ((GtkObject *) item)->klass)->render (item, area, nb);
			/* fixme: */
			nb->empty = FALSE;
			if (!(ret & NR_ARENA_ITEM_STATE_INVALID)) {
				/* Compose */
				nr_render_buf_buf_mask (b, 0, 0, area->x1 - area->x0, area->y1 - area->y0,
							nb, 0, 0,
							cb, 0, 0);
			}
			nr_buffer_free (cb);
			nr_buffer_free (nb);
		} else {
			return ((NRArenaItemClass *) ((GtkObject *) item)->klass)->render (item, area, b);
		}
	}

	return item->state;
}

guint
nr_arena_item_invoke_clip (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	g_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_CLIP, item->state);
	g_return_val_if_fail (b->w >= (area->x1 - area->x0), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (b->h >= (area->y1 - area->y0), NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_VERBOSE
	g_print ("Invoke render %p: %d %d - %d %d\n", item, area->x0, area->y0, area->x1, area->y1);
#endif

	if (nr_irect_do_intersect (area, &item->bbox)) {
		/* Need render that item */
		if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->clip)
			return ((NRArenaItemClass *) ((GtkObject *) item)->klass)->clip (item, area, b);
	}

	return item->state;
}

NRArenaItem *
nr_arena_item_invoke_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	gint ix, iy;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_BBOX, NULL);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_PICK, NULL);

	if (!sticky && !item->sensitive) return NULL;

	ix = (gint) x;
	iy = (gint) y;

	if (((x + delta) >= item->bbox.x0) &&
	    ((x - delta) <  item->bbox.x1) &&
	    ((y + delta) >= item->bbox.y0) &&
	    ((y - delta) <  item->bbox.y1)) {
		if (((NRArenaItemClass *) ((GtkObject *) item)->klass)->pick)
			return ((NRArenaItemClass *) ((GtkObject *) item)->klass)->pick (item, x, y, delta, sticky);
	}

	return NULL;
}

gint
nr_arena_item_emit_event (NRArenaItem *item, NREvent *event)
{
	gint ret;

	g_return_val_if_fail (item != NULL, TRUE);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), TRUE);

	ret = FALSE;

	gtk_signal_emit (GTK_OBJECT (item), signals[EVENT], event, &ret);

	return ret;
}

void
nr_arena_item_request_update (NRArenaItem *item, guint reset, gboolean propagate)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (!(reset & NR_ARENA_ITEM_STATE_INVALID));

	if (propagate && !item->propagate) item->propagate = propagate;

	if (item->state & reset) {
		item->state &= ~reset;
		if (item->parent) {
			nr_arena_item_request_update (item->parent, reset, FALSE);
		} else {
			nr_arena_request_update (item->arena, item);
		}
	}
}

void
nr_arena_item_request_render (NRArenaItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	nr_arena_request_render_rect (item->arena, &item->bbox);
}

/* Public */

NRArenaItem *
nr_arena_item_new (NRArena *arena, GtkType type)
{
	NRArenaItem *item;

	g_return_val_if_fail (arena != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA (arena), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, NR_TYPE_ARENA_ITEM), NULL);

	item = gtk_type_new (type);

	item->arena = arena;

	nr_arena_item_added (arena, item);

	return item;
}

NRArenaItem *
nr_arena_item_destroy (NRArenaItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

	nr_arena_item_request_render (item);

	if (item->parent) {
		nr_arena_item_remove_child (item->parent, item);
	} else {
		nr_arena_item_unref (item);
	}

	return NULL;
}

void
nr_arena_item_append_child (NRArenaItem *parent, NRArenaItem *child)
{
	NRArenaItem *ref;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (parent));
	g_return_if_fail (child != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (child));
	g_return_if_fail (parent->arena == child->arena);
	g_return_if_fail (child->parent == NULL);
	g_return_if_fail (child->prev == NULL);
	g_return_if_fail (child->next == NULL);

	ref = nr_arena_item_children (parent);
	if (ref) while (ref->next) ref = ref->next;

	nr_arena_item_add_child (parent, child, ref);
}

void
nr_arena_item_set_transform (NRArenaItem *item, const gdouble *transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	nr_arena_item_request_render (item);

	if (!transform || nr_affine_is_identity (transform)) {
		/* Set to identity affine */
		if (item->affine) g_free (item->affine);
		item->affine = NULL;
	} else {
		if (!item->affine) item->affine = g_new (gdouble, 6);
		memcpy (item->affine, transform, 6 * sizeof (gdouble));
	}

	nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, TRUE);
}

void
nr_arena_item_set_opacity (NRArenaItem *item, gdouble opacity)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	nr_arena_item_request_render (item);

	item->opacity = opacity;
}

void
nr_arena_item_set_sensitive (NRArenaItem *item, gboolean sensitive)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	/* fixme: mess with pick/repick... */

	item->sensitive = sensitive;
}

void
nr_arena_item_set_clip (NRArenaItem *item, NRArenaItem *clip)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (!clip || NR_IS_ARENA_ITEM (clip));

	nr_arena_item_request_render (item);

	if (item->clip) {
		item->clip = nr_arena_item_detach_unref (item, item->clip);
	}

	if (clip) {
		item->clip = nr_arena_item_attach_ref (item, clip, NULL, NULL);
	}

	nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, TRUE);
}

void
nr_arena_item_set_order (NRArenaItem *item, gint order)
{
	NRArenaItem *children, *child, *ref;
	gint pos;

	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	if (!item->parent) return;

	children = nr_arena_item_children (item->parent);

	ref = NULL;
	pos = 0;
	for (child = children; child != NULL; child = child->next) {
		if (pos >= order) break;
		if (child != item) {
			ref = child;
			pos += 1;
		}
	}

	nr_arena_item_set_child_position (item->parent, item, ref);
}

/* Helpers */

NRArenaItem *
nr_arena_item_attach_ref (NRArenaItem *parent, NRArenaItem *child, NRArenaItem *prev, NRArenaItem *next)
{
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (parent), NULL);
	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (child), NULL);
	g_return_val_if_fail (child->parent == NULL, NULL);
	g_return_val_if_fail (child->prev == NULL, NULL);
	g_return_val_if_fail (child->next == NULL, NULL);
	g_return_val_if_fail (!prev || NR_IS_ARENA_ITEM (prev), NULL);
	g_return_val_if_fail (!prev || (prev->parent == parent), NULL);
	g_return_val_if_fail (!prev || (prev->next == next), NULL);
	g_return_val_if_fail (!next || NR_IS_ARENA_ITEM (next), NULL);
	g_return_val_if_fail (!next || (next->parent == parent), NULL);
	g_return_val_if_fail (!next || (next->prev == prev), NULL);

	nr_arena_item_ref (child);

	child->parent = parent;
	child->prev = prev;
	child->next = next;

	if (prev) prev->next = child;
	if (next) next->prev = child;

	return child;
}

NRArenaItem *
nr_arena_item_detach_unref (NRArenaItem *parent, NRArenaItem *child)
{
	NRArenaItem *prev, *next;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (parent), NULL);
	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (child), NULL);
	g_return_val_if_fail (child->parent == parent, NULL);

	prev = child->prev;
	next = child->next;

	child->parent = NULL;
	child->prev = NULL;
	child->next = NULL;

	nr_arena_item_unref (child);

	if (prev) prev->next = next;
	if (next) next->prev = prev;

	return next;
}


