#ifndef __NR_ARENA_GROUP_H__
#define __NR_ARENA_GROUP_H__

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

#define NR_TYPE_ARENA_GROUP (nr_arena_group_get_type ())
#define NR_ARENA_GROUP(o) (GTK_CHECK_CAST ((o), NR_TYPE_ARENA_GROUP, NRArenaGroup))
#define NR_ARENA_GROUP_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_ARENA_GROUP, NRArenaGroupClass))
#define NR_IS_ARENA_GROUP(o) (GTK_CHECK_TYPE ((o), NR_TYPE_ARENA_GROUP))
#define NR_IS_ARENA_GROUP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_ARENA_GROUP))

#include "nr-arena-item.h"

struct _NRArenaGroup {
	NRArenaItem item;
	guint transparent : 1;
	NRArenaItem *children, *last;
};

struct _NRArenaGroupClass {
	NRArenaItemClass parent_class;
};

GtkType nr_arena_group_get_type (void);

void nr_arena_group_set_transparent (NRArenaGroup *group, gboolean transparent);

#endif
