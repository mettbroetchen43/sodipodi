#ifndef __NR_ARENA_H__
#define __NR_ARENA_H__

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

#define NR_TYPE_ARENA (nr_arena_get_type ())
#define NR_ARENA(o) (GTK_CHECK_CAST ((o), NR_TYPE_ARENA, NRArena))
#define NR_ARENA_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_ARENA, NRArenaClass))
#define NR_IS_ARENA(o) (GTK_CHECK_TYPE ((o), NR_TYPE_ARENA))
#define NR_IS_ARENA_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_ARENA))

#include <libnr/nr-types.h>
#include <gtk/gtkobject.h>
#include "nr-arena-forward.h"

struct _NRArena {
	GtkObject object;
};

struct _NRArenaClass {
	GtkObjectClass parent_class;

	/* These may be used for bookkeeping, like snooping highlited item */
	void (* item_added) (NRArena *arena, NRArenaItem *item);
	void (* remove_item) (NRArena *arena, NRArenaItem *item);

	void (* request_update) (NRArena *arena, NRArenaItem *item);
	void (* request_render) (NRArena *arena, NRRectL *area);
};

GtkType nr_arena_get_type (void);

/* Following are meant stricktly for subclass/item implementations */

void nr_arena_item_added (NRArena *arena, NRArenaItem *item);
void nr_arena_remove_item (NRArena *arena, NRArenaItem *item);

void nr_arena_request_update (NRArena *arena, NRArenaItem *item);
void nr_arena_request_render_rect (NRArena *arena, NRRectL *area);

#endif
