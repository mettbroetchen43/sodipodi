#define __NR_ARENA_C__

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

#include <gtk/gtksignal.h>
#include "nr-arena-item.h"
#include "nr-arena.h"

enum {
	ITEM_ADDED,
	REMOVE_ITEM,
	REQUEST_UPDATE,
	REQUEST_RENDER,
	LAST_SIGNAL
};

static void nr_arena_class_init (NRArenaClass *klass);
static void nr_arena_init (NRArena *arena);
static void nr_arena_destroy (GtkObject *object);

static GtkObjectClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
nr_arena_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArena",
			sizeof (NRArena),
			sizeof (NRArenaClass),
			(GtkClassInitFunc) nr_arena_class_init,
			(GtkObjectInitFunc) nr_arena_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_OBJECT, &info);
	}
	return type;
}

static void
nr_arena_class_init (NRArenaClass *klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	signals[ITEM_ADDED] = gtk_signal_new ("item_added",
						  GTK_RUN_FIRST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (NRArenaClass, item_added),
						  gtk_marshal_NONE__POINTER,
						  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[REMOVE_ITEM] = gtk_signal_new ("remove_item",
						  GTK_RUN_FIRST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (NRArenaClass, remove_item),
						  gtk_marshal_NONE__POINTER,
						  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[REQUEST_UPDATE] = gtk_signal_new ("request_update",
						  GTK_RUN_FIRST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (NRArenaClass, request_update),
						  gtk_marshal_NONE__POINTER,
						  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[REQUEST_RENDER] = gtk_signal_new ("request_render",
						  GTK_RUN_FIRST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (NRArenaClass, request_render),
						  gtk_marshal_NONE__POINTER,
						  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = nr_arena_destroy;
}

static void
nr_arena_init (NRArena *arena)
{
}

static void
nr_arena_destroy (GtkObject *object)
{
	NRArena *arena;

	arena = NR_ARENA (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

void
nr_arena_item_added (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	gtk_signal_emit (GTK_OBJECT (arena), signals [ITEM_ADDED], item);
}

void
nr_arena_remove_item (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	gtk_signal_emit (GTK_OBJECT (arena), signals [REMOVE_ITEM], item);
}

void
nr_arena_request_update (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	gtk_signal_emit (GTK_OBJECT (arena), signals [REQUEST_UPDATE], item);
}

void
nr_arena_request_render_rect (NRArena *arena, NRIRect *area)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (area != NULL);

	if (!nr_irect_is_empty (area)) {
		gtk_signal_emit (GTK_OBJECT (arena), signals [REQUEST_RENDER], area);
	}
}

