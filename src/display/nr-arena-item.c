#define __NR_ARENA_ITEM_C__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noNR_ARENA_ITEM_VERBOSE

#include <string.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-blit.h>
#include <libart_lgpl/art_affine.h>
#include <glib-object.h>
#include <gtk/gtkmarshal.h>
#include "../helper/nr-plain-stuff.h"
#include "nr-arena.h"
#include "nr-arena-item.h"

enum {
	EVENT,
	LAST_SIGNAL
};

static void nr_arena_item_class_init (NRArenaItemClass *klass);
static void nr_arena_item_init (NRArenaItem *item);
static void nr_arena_item_private_dispose (GObject *object);

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GType
nr_arena_item_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (NRArenaItemClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) nr_arena_item_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (NRArenaItem),
			16,	/* n_preallocs */
			(GInstanceInitFunc) nr_arena_item_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "NRArenaItem", &info, 0);
	}
	return type;
}

static void
nr_arena_item_class_init (NRArenaItemClass *klass)
{
	GObjectClass * object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	signals[EVENT] = g_signal_new ("event",
				       G_TYPE_FROM_CLASS (klass),
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET (NRArenaItemClass, event),
				       NULL, NULL,
				       gtk_marshal_INT__POINTER,
				       G_TYPE_INT, 1, G_TYPE_POINTER);

	object_class->dispose = nr_arena_item_private_dispose;
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

	item->px = NULL;
}

static void
nr_arena_item_private_dispose (GObject *object)
{
	NRArenaItem *item;

	item = NR_ARENA_ITEM (object);

	/* Parent has to refcount children */
	g_assert (!item->parent);
	g_assert (!item->prev);
	g_assert (!item->next);

	if (item->px) {
		g_free (item->px);
		item->px = NULL;
	}

	if (item->clip) {
		item->clip = nr_arena_item_detach_unref (item, item->clip);
	}

	if (item->affine) {
		g_free (item->affine);
		item->affine = NULL;
	}

	nr_arena_remove_item (item->arena, item);
	item->arena = NULL;

	if (G_OBJECT_CLASS(parent_class)->dispose)
		G_OBJECT_CLASS(parent_class)->dispose (object);
}

NRArenaItem *
nr_arena_item_children (NRArenaItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

	if (((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->children)
		return ((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->children (item);

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

	if (NR_ARENA_ITEM_VIRTUAL (item, add_child))
		NR_ARENA_ITEM_VIRTUAL (item, add_child) (item, child, ref);
}

void
nr_arena_item_remove_child (NRArenaItem *item, NRArenaItem *child)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (child != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (child));
	g_return_if_fail (child->parent == item);

	if (NR_ARENA_ITEM_VIRTUAL (item, remove_child))
		NR_ARENA_ITEM_VIRTUAL (item, remove_child) (item, child);
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

	if (NR_ARENA_ITEM_VIRTUAL (item, set_child_position))
		NR_ARENA_ITEM_VIRTUAL (item, set_child_position) (item, child, ref);
}

guint
nr_arena_item_invoke_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
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

	if (!(item->state & NR_ARENA_ITEM_STATE_IMAGE)) {
		/* Concept test */
		if (item->px) {
			g_free (item->px);
			item->px = NULL;
		}
	}

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
		newstate = NR_ARENA_ITEM_VIRTUAL (item, update) (item, area, &newgc, NR_ARENA_ITEM_STATE_BBOX, reset);
		g_return_val_if_fail (newstate & NR_ARENA_ITEM_STATE_BBOX, newstate);
		item->state = newstate;
	}

	if (item->clip) {
		/* We have do do intersect with clip bbox */
		nr_rect_l_intersect (&item->bbox, &item->bbox, &item->clip->bbox);
	}

	if ((~item->state & state) && (!area || nr_rect_l_test_intersect (area, &item->bbox))) {
		/* Need update to given state */
		newstate = NR_ARENA_ITEM_VIRTUAL (item, update) (item, area, &newgc, state, reset);
#if 0
		g_return_val_if_fail (!(~newstate & state), newstate);
#endif
		item->state = newstate;
	}

	return item->state;
}

guint
nr_arena_item_invoke_render (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	g_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_BBOX, item->state);
#if 0
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_RENDER, item->state);
#endif
	g_return_val_if_fail (b->w >= (area->x1 - area->x0), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (b->h >= (area->y1 - area->y0), NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_VERBOSE
	g_print ("Invoke render %p: %d %d - %d %d\n", item, area->x0, area->y0, area->x1, area->y1);
#endif

	if (nr_rect_l_test_intersect (area, &item->bbox)) {
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
			ret = NR_ARENA_ITEM_VIRTUAL (item, render) (item, area, nb);
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
			if (!item->px && ((item->bbox.x1 - item->bbox.x0) * (item->bbox.y1 - item->bbox.y0) < 4096)) {
				NRBuffer b;
				gint ret;
				item->px = g_new (guchar, 4 * (item->bbox.x1 - item->bbox.x0) * (item->bbox.y1 - item->bbox.y0));
				memset (item->px, 0x0, 4 * (item->bbox.x1 - item->bbox.x0) * (item->bbox.y1 - item->bbox.y0));
				/* Hack for concept testing */
				b.size = NR_SIZE_BIG;
				b.mode = NR_IMAGE_R8G8B8A8;
				b.empty = 1;
				b.premul = 1;
				b.w = item->bbox.x1 - item->bbox.x0;
				b.h = item->bbox.y1 - item->bbox.y0;
				b.rs = 4 * b.w;
				b.px = item->px;
				ret = ((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->render (item, &item->bbox, &b);
				if (!(ret & NR_ARENA_ITEM_STATE_RENDER)) return ret;
			}
			if (item->px) {
				NRPixBlock pbb, pbi;
				guint bmode;
				/* Concept test */
				/* Item px it placed at item bbox 0,0, 4 * width rowstride, premultiplied */
				if (b->premul) {
					bmode = NR_PIXBLOCK_MODE_R8G8B8A8P;
				} else {
					bmode = NR_PIXBLOCK_MODE_R8G8B8A8N;
				}
				nr_pixblock_setup_extern (&pbb, bmode, area->x0, area->y0, area->x1, area->y1, b->px, b->rs, b->empty, FALSE);
				nr_pixblock_setup_extern (&pbi, NR_PIXBLOCK_MODE_R8G8B8A8P,
							  /* fixme: This probably cannot overflow, because we render only if visible */
							  /* fixme: and pixel cache is there only for small items */
							  /* fixme: But this still needs extra check (Lauris) */
							  item->bbox.x0, item->bbox.y0,
							  item->bbox.x1, item->bbox.y1,
							  item->px, 4 * (item->bbox.x1 - item->bbox.x0), FALSE, FALSE);
				nr_blit_pixblock_pixblock (&pbb, &pbi);
				nr_pixblock_release (&pbb);
				nr_pixblock_release (&pbi);
				b->empty = FALSE;
				return item->state;
			} else {
				return ((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->render (item, area, b);
			}
		}
	}

	return item->state;
}

guint
nr_arena_item_invoke_clip (NRArenaItem *item, NRRectL *area, NRBuffer *b)
{
	g_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (NR_IS_ARENA_ITEM (item), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_CLIP, item->state);
	g_return_val_if_fail (b->w >= (area->x1 - area->x0), NR_ARENA_ITEM_STATE_INVALID);
	g_return_val_if_fail (b->h >= (area->y1 - area->y0), NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_VERBOSE
	g_print ("Invoke render %p: %d %d - %d %d\n", item, area->x0, area->y0, area->x1, area->y1);
#endif

	if (nr_rect_l_test_intersect (area, &item->bbox)) {
		/* Need render that item */
		if (((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->clip)
			return ((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->clip (item, area, b);
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
		if (((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->pick)
			return ((NRArenaItemClass *) G_OBJECT_GET_CLASS(item))->pick (item, x, y, delta, sticky);
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

	g_signal_emit (G_OBJECT (item), signals[EVENT], 0, event, &ret);

	return ret;
}

void
nr_arena_item_request_update (NRArenaItem *item, guint reset, gboolean propagate)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));
	g_return_if_fail (!(reset & NR_ARENA_ITEM_STATE_INVALID));

	if (propagate && !item->propagate) item->propagate = TRUE;

	if (item->state & reset) {
		item->state &= ~reset;
		if ((reset & NR_ARENA_ITEM_STATE_IMAGE) && item->px) {
			/* Concept test */
			/* Clear buffer */
			g_free (item->px);
			item->px = NULL;
		}
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
nr_arena_item_new (NRArena *arena, GType type)
{
	NRArenaItem *item;

	g_return_val_if_fail (arena != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA (arena), NULL);
	g_return_val_if_fail (g_type_is_a (type, NR_TYPE_ARENA_ITEM), NULL);

	item = (NRArenaItem *)g_object_new (type, NULL);

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
nr_arena_item_set_transform (NRArenaItem *item, const NRMatrixF *transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	nr_arena_item_request_render (item);

	if (nr_matrix_f_test_identity (transform, NR_EPSILON_F)) {
		/* Set to identity affine */
		if (item->affine) g_free (item->affine);
		item->affine = NULL;
	} else {
		if (!item->affine) item->affine = g_new (gdouble, 6);
		nr_matrix_d_from_f (NR_MATRIX_D_FROM_DOUBLE (item->affine), transform);
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


