#define __SP_ARENA_C__

/*
 * SPArena
 *
 * Antialiased drawing engine for Sodipodi
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 */

#include "sp-arena.h"

static void sp_arena_class_init (SPArenaClass * klass);
static void sp_arena_init (SPArena * arena);
static void sp_arena_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

GtkType
sp_arena_get_type (void)
{
	static GtkType arena_type = 0;
	if (!arena_type) {
		GtkTypeInfo arena_info = {
			"SPArena",
			sizeof (SPArena),
			sizeof (SPArenaClass),
			(GtkClassInitFunc) sp_arena_class_init,
			(GtkObjectInitFunc) sp_arena_init,
			NULL, NULL, NULL
		};
		arena_type = gtk_type_unique (GTK_TYPE_OBJECT, &arena_info);
	}
	return arena_type;
}

static void
sp_arena_class_init (SPArenaClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	object_class->destroy = sp_arena_destroy;
}

static void
sp_arena_init (SPArena * arena)
{
	arena->root = sp_arena_item_new (SP_TYPE_ARENA_ITEM, NULL);
}

static void
sp_arena_destroy (GtkObject * object)
{
	SPArena * arena;

	arena = (SPArena *) object;

	if (arena->root) {
		gtk_object_unref (GTK_OBJECT (arena->root));
		arena->root = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/* Public methods */

SPArena *
sp_arena_new (void)
{
	SPArena * arena;

	arena = gtk_type_new (SP_TYPE_ARENA);

	return arena;
}



