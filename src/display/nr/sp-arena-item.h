#ifndef __SP_ARENA_ITEM_H__
#define __SP_ARENA_ITEM_H__

/*
 * SPArenaItem
 *
 * Abstract base class for Arena objects
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _SPArenaItem SPArenaItem;
typedef struct _SPArenaItemClass SPArenaItemClass;

#define SP_TYPE_ARENA_ITEM (sp_arena_item_get_type ())
#define SP_ARENA_ITEM(o) (GTK_CHECK_CAST ((o), SP_TYPE_ARENA_ITEM, SPArenaItem))
#define SP_ARENA_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_ARENA_ITEM, SPArenaItemClass))
#define SP_IS_ARENA_ITEM(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ARENA_ITEM))
#define SP_IS_ARENA_ITEM_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_ARENA_ITEM))

#include <gtk/gtkobject.h>
#include "sp-arena.h"

struct _SPArenaItem {
	GtkObject object;
	SPArena * arena; /* Our Arena */
	SPArenaItem * parent; /* Our parent */
	SPArenaItem * children; /* Tree skeleton */
	SPArenaItem * last; /* Last child */
	SPArenaItem * next; /* Tree skeleton */
};

struct _SPArenaItemClass {
	GtkObjectClass object_class;
};

GtkType sp_arena_item_get_type (void);

SPArenaItem * sp_arena_item_new (GtkType type, SPArenaItem * parent);

END_GNOME_DECLS

#endif
