#define _NR_CANVAS_GROUP_C_

/*
 * NRCanvasGroup
 *
 * Base class for NRCanvas groups
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include "nr-canvas-group.h"

static void nr_canvas_group_class_init (NRCanvasGroupClass * klass);
static void nr_canvas_group_init (NRCanvasGroup * group);
static void nr_canvas_group_destroy (GtkObject * object);

static void nr_canvas_group_add_child (NRCanvasItem * item, NRCanvasItem * child, gint position);
static void nr_canvas_group_remove_child (NRCanvasItem * item, NRCanvasItem * child);
static NRCanvasItemState nr_canvas_group_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags);

static NRCanvasItemClass * parent_class;

GtkType
nr_canvas_group_get_type (void)
{
	static GtkType group_type = 0;
	if (!group_type) {
		GtkTypeInfo group_info = {
			"NRCanvasGroup",
			sizeof (NRCanvasGroup),
			sizeof (NRCanvasGroupClass),
			(GtkClassInitFunc) nr_canvas_group_class_init,
			(GtkObjectInitFunc) nr_canvas_group_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		group_type = gtk_type_unique (nr_canvas_item_get_type (), &group_info);
	}
	return group_type;
}

static void
nr_canvas_group_class_init (NRCanvasGroupClass * klass)
{
	GtkObjectClass * object_class;
	NRCanvasItemClass * item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRCanvasItemClass *) klass;

	parent_class = gtk_type_class (nr_canvas_item_get_type ());

	object_class->destroy = nr_canvas_group_destroy;

	item_class->add_child = nr_canvas_group_add_child;
	item_class->remove_child = nr_canvas_group_remove_child;
	item_class->update = nr_canvas_group_update;
}

static void
nr_canvas_group_init (NRCanvasGroup * group)
{
	/* Only for reference */
	group->children = NULL;
	group->last = NULL;
}

static void
nr_canvas_group_destroy (GtkObject * object)
{
	NRCanvasGroup * group;

	group = (NRCanvasGroup *) object;

	/* Unref children */

	while (group->children) {
		NRCanvasItem * child;
		child = (NRCanvasItem *) group->children->data;
		/* fixme: Should we use unparent here? */
		child->parent = NULL;
		group->children = g_slist_remove (group->children, child);
		gtk_object_unref ((GtkObject *) child);
	}
	group->last = NULL;

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
nr_canvas_group_add_child (NRCanvasItem * item, NRCanvasItem * child, gint position)
{
	NRCanvasGroup * group;

	group = (NRCanvasGroup *) item;

	gtk_object_ref ((GtkObject *) child);

	if (!group->children) {
		g_assert (!group->last);
		group->last = group->children = g_slist_prepend (NULL, child);
	} else {
		g_assert (group->last);
		if (position == 0) {
			group->children = g_slist_prepend (group->children, child);
		} else if (position < 0) {
			g_slist_append (group->last, child);
			group->last = group->last->next;
		} else {
			GSList * l;
			l = g_slist_nth (group->children, position - 1);
			l->next = g_slist_prepend (l->next, child);
			if (l == group->last) group->last = l->next;
		}
	}

	child->parent = item;
}

static void
nr_canvas_group_remove_child (NRCanvasItem * item, NRCanvasItem * child)
{
	NRCanvasGroup * group;
	GSList * p, * l;

	group = (NRCanvasGroup *) item;

	p = NULL;
	for (l = group->children; l != NULL; l = l->next) {
		if (l->data == child) {
			child->parent = NULL;
			if (l == group->last) {
				group->last = p;
			}
			if (p != NULL) {
				p->next = g_slist_remove (l, child);
			} else {
				group->children = g_slist_remove (group->children, child);
			}
			gtk_object_unref ((GtkObject *) child);
			return;
		}
		p = l;
	}
}

static NRCanvasItemState
nr_canvas_group_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags)
{
	NRCanvasGroup * group;
	NRCanvasItemState beststate;
	GSList * l;

	group = (NRCanvasGroup *) item;

	beststate = NR_CANVAS_ITEM_STATE_COMPLETE;

	for (l = group->children; l != NULL; l = l->next) {
		NRCanvasItem * child;
		NRCanvasItemState childstate;
		child = (NRCanvasItem *) l->data;
		childstate = NR_CI_GET_STATE (child);
		if ((childstate < state) || (flags & NR_CANVAS_ITEM_FORCED_UPDATE)) {
			/* fixme: state mangling */
			childstate = nr_canvas_item_invoke_update (child, ctx, NR_CANVAS_ITEM_STATE_RENDER, flags);
			if (childstate < beststate) beststate = childstate;
		}
	}

	return beststate;
}

