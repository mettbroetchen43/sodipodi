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
#define NR_ARENA(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NR_TYPE_ARENA, NRArena))
#define NR_IS_ARENA(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NR_TYPE_ARENA))

#include <libnr/nr-types.h>
#include "nr-object.h"
#include "nr-arena-forward.h"

#include <glib-object.h>

struct _NRArena {
	GObject object;
};

struct _NRArenaClass {
	GObjectClass parent_class;

	/* These may be used for bookkeeping, like snooping highlited item */
	void (* item_added) (NRArena *arena, NRArenaItem *item);
	void (* remove_item) (NRArena *arena, NRArenaItem *item);

	void (* request_update) (NRArena *arena, NRArenaItem *item);
	void (* request_render) (NRArena *arena, NRRectL *area);
};

unsigned int nr_arena_get_type (void);

/* Following are meant stricktly for subclass/item implementations */

void nr_arena_item_added (NRArena *arena, NRArenaItem *item);
void nr_arena_remove_item (NRArena *arena, NRArenaItem *item);

void nr_arena_request_update (NRArena *arena, NRArenaItem *item);
void nr_arena_request_render_rect (NRArena *arena, NRRectL *area);

#endif
