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
#define NR_ARENA(o) (NR_CHECK_INSTANCE_CAST ((o), NR_TYPE_ARENA, NRArena))
#define NR_IS_ARENA(o) (NR_CHECK_INSTANCE_TYPE ((o), NR_TYPE_ARENA))

#include <libnr/nr-types.h>
#include <libnr/nr-object.h>
#include "nr-arena-forward.h"

typedef struct _NRArenaEventVector NRArenaEventVector;

struct _NRArenaEventVector {
	NRObjectEventVector object_vector;
	void (* item_added) (NRArena *arena, NRArenaItem *parent, NRArenaItem *child, void *data);
	void (* item_removed) (NRArena *arena, NRArenaItem *parent, NRArenaItem *child, void *data);
	void (* request_update) (NRArena *arena, NRArenaItem *item, void *data);
	void (* request_render) (NRArena *arena, NRRectL *area, void *data);
};

struct _NRArena {
	NRActiveObject object;
};

struct _NRArenaClass {
	NRActiveObjectClass parent_class;
};

unsigned int nr_arena_get_type (void);

/* Following are meant stricktly for subclass/item implementations */
void nr_arena_item_added (NRArena *arena, NRArenaItem *parent, NRArenaItem *child);
void nr_arena_item_removed (NRArena *arena, NRArenaItem *parent, NRArenaItem *child);

void nr_arena_request_update (NRArena *arena, NRArenaItem *item);
void nr_arena_request_render_rect (NRArena *arena, NRRectL *area);

#endif
