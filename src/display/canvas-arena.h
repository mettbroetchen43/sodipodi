#ifndef __SP_CANVAS_ARENA_H__
#define __SP_CANVAS_ARENA_H__

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

typedef struct _SPCanvasArena SPCanvasArena;
typedef struct _SPCanvasArenaClass SPCanvasArenaClass;

#define SP_TYPE_CANVAS_ARENA (sp_canvas_arena_get_type ())
#define SP_CANVAS_ARENA(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_ARENA, SPCanvasArena))
#define SP_CANVAS_ARENA_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_ARENA, SPCanvasArenaClass))
#define SP_IS_CANVAS_ARENA(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_ARENA))
#define SP_IS_CANVAS_ARENA_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_ARENA))

#include <libgnomeui/gnome-canvas.h>
#include "nr-arena-item.h"

struct _SPCanvasArena {
	GnomeCanvasItem item;

	guint cursor : 1;
	guint sticky : 1;
	gdouble cx, cy;

	NRArena *arena;
	NRArenaItem *root;
	NRGC gc;

	NRArenaItem *active;
	/* fixme: */
	NRArenaItem *picked;
};

struct _SPCanvasArenaClass {
	GnomeCanvasItemClass parent_class;

	gint (* arena_event) (SPCanvasArena *carena, NRArenaItem *item, GdkEvent *event);
};

GtkType sp_canvas_arena_get_type (void);

void sp_canvas_arena_set_sticky (SPCanvasArena *ca, gboolean sticky);

#endif
