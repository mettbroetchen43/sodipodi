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

#if 0
#include <glib-object.h>
#include <gtk/gtkmarshal.h>
#endif
#include "nr-arena-item.h"
#include "nr-arena.h"
#include "../libnr/nr-rect.h"

#if 0
enum {
	REQUEST_UPDATE,
	REQUEST_RENDER,
	LAST_SIGNAL
};
#endif

static void nr_arena_class_init (NRArenaClass *klass);
static void nr_arena_init (NRArena *arena);
static void nr_arena_finalize (NRObject *object);

static NRActiveObjectClass *parent_class;
#if 0
static guint signals[LAST_SIGNAL] = {0};
#endif

unsigned int
nr_arena_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		type = nr_object_register_type (NR_TYPE_ACTIVE_OBJECT,
						"NRArena",
						sizeof (NRArenaClass),
						sizeof (NRArena),
						(void (*) (NRObjectClass *)) nr_arena_class_init,
						(void (*) (NRObject *)) nr_arena_init);
	}
	return type;
}

static void
nr_arena_class_init (NRArenaClass *klass)
{
	NRObjectClass * object_class;

	object_class = (NRObjectClass *) klass;

	parent_class = (NRActiveObjectClass *) (((NRObjectClass *) klass)->parent);

#if 0
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
#endif

	object_class->finalize = nr_arena_finalize;
}

static void
nr_arena_init (NRArena *arena)
{
}

static void
nr_arena_finalize (NRObject *object)
{
	NRArena *arena;

	arena = NR_ARENA (object);

	((NRObjectClass *) (parent_class))->finalize (object);
}

#if 0
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
#endif

void
nr_arena_request_update (NRArena *arena, NRArenaItem *item)
{
	NRActiveObject *aobject;
	NRObjectListener *listener;

	aobject = (NRActiveObject *) arena;

	nr_return_if_fail (arena != NULL);
	nr_return_if_fail (NR_IS_ARENA (arena));
	nr_return_if_fail (item != NULL);
	nr_return_if_fail (NR_IS_ARENA_ITEM (item));

	for (listener = aobject->listeners; listener != NULL; listener = listener->next) {
		NRArenaEventVector *avector;
		avector = (NRArenaEventVector *) listener->vector;
		if ((listener->size >= sizeof (NRArenaEventVector)) && avector->request_update) {
			avector->request_update (arena, item, listener->data);
		}
	}

#if 0
	g_signal_emit (G_OBJECT (arena), signals [REQUEST_UPDATE], 0, item);
#endif
}

void
nr_arena_request_render_rect (NRArena *arena, NRRectL *area)
{
	NRActiveObject *aobject;
	NRObjectListener *listener;

	aobject = (NRActiveObject *) arena;

	nr_return_if_fail (arena != NULL);
	nr_return_if_fail (NR_IS_ARENA (arena));
	nr_return_if_fail (area != NULL);

	if (area && !nr_rect_l_test_empty (area)) {
		for (listener = aobject->listeners; listener != NULL; listener = listener->next) {
			NRArenaEventVector *avector;
			avector = (NRArenaEventVector *) listener->vector;
			if ((listener->size >= sizeof (NRArenaEventVector)) && avector->request_render) {
				avector->request_render (arena, area, listener->data);
			}
		}
#if 0
		g_signal_emit (G_OBJECT (arena), signals [REQUEST_RENDER], 0, area);
#endif
	}
}

