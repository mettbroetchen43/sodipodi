#ifndef __SP_ARENA_H__
#define __SP_ARENA_H__

/*
 * SPArena
 *
 * Antialiased drawing engine for Sodipodi
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _SPArena SPArena;
typedef struct _SPArenaClass SPArenaClass;

#define SP_TYPE_ARENA (sp_arena_get_type ())
#define SP_ARENA(o) (GTK_CHECK_CAST ((o), SP_TYPE_ARENA, SPArena))
#define SP_ARENA_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_ARENA, SPArenaClass))
#define SP_IS_ARENA(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ARENA))
#define SP_IS_ARENA_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_ARENA))

#include <gtk/gtkobject.h>
#include "sp-arena-item.h"

struct _SPArena {
	GtkObject object;
	SPArenaItem * root;
};

struct _SPArenaClass {
	GtkObjectClass object_class;
};

GtkType sp_arena_get_type (void);

SPArena * sp_arena_new (void);

END_GNOME_DECLS

#endif
