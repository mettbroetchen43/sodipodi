#define __NR_ARENA_C__

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

#include <glib-object.h>
#include <gtk/gtkmarshal.h>
#include "nr-arena-item.h"
#include "nr-arena.h"
#include "../libnr/nr-rect.h"

enum {
	ITEM_ADDED,
	REMOVE_ITEM,
	REQUEST_UPDATE,
	REQUEST_RENDER,
	LAST_SIGNAL
};

static void nr_arena_class_init (NRArenaClass *klass);
static void nr_arena_init (NRArena *arena);
static void nr_arena_dispose (GObject *object);

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

unsigned int
nr_arena_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (NRArenaClass),
			NULL, NULL,
			(GClassInitFunc) nr_arena_class_init,
			NULL, NULL,
			sizeof (NRArena),
			16,
			(GInstanceInitFunc) nr_arena_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "NRArena", &info, 0);
	}
	return type;
}

static void
nr_arena_class_init (NRArenaClass *klass)
{
	GObjectClass * object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	signals[ITEM_ADDED] = g_signal_new ("item_added",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET(NRArenaClass, item_added),
					    NULL, NULL,
					    gtk_marshal_NONE__POINTER,
					    G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[REMOVE_ITEM] = g_signal_new ("remove_item",
					     G_TYPE_FROM_CLASS (klass),
					     G_SIGNAL_RUN_FIRST,
					     G_STRUCT_OFFSET (NRArenaClass, remove_item),
					     NULL, NULL,
					     gtk_marshal_NONE__POINTER,
					     G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[REQUEST_UPDATE] = g_signal_new ("request_update",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (NRArenaClass, request_update),
						NULL, NULL,
						gtk_marshal_NONE__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[REQUEST_RENDER] = g_signal_new ("request_render",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (NRArenaClass, request_render),
						NULL, NULL,
						gtk_marshal_NONE__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);

	object_class->dispose = nr_arena_dispose;
}

static void
nr_arena_init (NRArena *arena)
{
}

static void
nr_arena_dispose (GObject *object)
{
	NRArena *arena;

	arena = NR_ARENA (object);

	if (G_OBJECT_CLASS (parent_class)->dispose)
		G_OBJECT_CLASS (parent_class)->dispose(object);
}

void
nr_arena_item_added (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	g_signal_emit (G_OBJECT (arena), signals [ITEM_ADDED], 0, item);
}

void
nr_arena_remove_item (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	g_signal_emit (G_OBJECT (arena), signals [REMOVE_ITEM], 0, item);
}

void
nr_arena_request_update (NRArena *arena, NRArenaItem *item)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_ARENA_ITEM (item));

	g_signal_emit (G_OBJECT (arena), signals [REQUEST_UPDATE], 0, item);
}

void
nr_arena_request_render_rect (NRArena *arena, NRRectL *area)
{
	g_return_if_fail (arena != NULL);
	g_return_if_fail (NR_IS_ARENA (arena));
	g_return_if_fail (area != NULL);

	if (area && !nr_rect_l_test_empty (area)) {
		g_signal_emit (G_OBJECT (arena), signals [REQUEST_RENDER], 0, area);
	}
}

