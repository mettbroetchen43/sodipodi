#define __SP_ARENA_ITEM_C__

/*
 * SPArenaItem
 *
 * Abstract base class for Arena objects
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 */

#include "sp-arena-item.h"

static void sp_arena_item_class_init (SPArenaItemClass * klass);
static void sp_arena_item_init (SPArenaItem * spci);
static void sp_arena_item_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

GtkType
sp_arena_item_get_type (void)
{
	static GtkType item_type = 0;
	if (!item_type) {
		GtkTypeInfo item_info = {
			"SPArenaItem",
			sizeof (SPArenaItem),
			sizeof (SPArenaItemClass),
			(GtkClassInitFunc) sp_arena_item_class_init,
			(GtkObjectInitFunc) sp_arena_item_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		item_type = gtk_type_unique (GTK_TYPE_OBJECT, &item_info);
	}
	return item_type;
}

static void
sp_arena_item_class_init (SPArenaItemClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	object_class->destroy = sp_arena_item_destroy;
}

static void
sp_arena_item_init (SPArenaItem * ai)
{
	ai->arena = NULL;
	ai->parent = NULL;
	ai->children = NULL;
	ai->last = NULL;
	ai->next = NULL;
}

static void
sp_arena_item_destroy (GtkObject * object)
{
	SPArenaItem * ai;

	ai = (SPArenaItem *) object;

	g_assert (ai->arena == NULL);
	g_assert (ai->parent == NULL);
	g_assert (ai->children == NULL);
	g_assert (ai->last == NULL);
	g_assert (ai->next == NULL);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

