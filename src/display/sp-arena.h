#ifndef _SP_ARENA_H_
#define _SP_ARENA_H_

/*
 * SPArena
 *
 * Drawing widget class featuring NRAACanvas
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _SPArena SPArena;
typedef struct _SPArenaClass SPArenaClass;

#define SP_TYPE_ARENA (sp_arena_get_type ())
#define SP_ARENA(o) (GTK_CHECK_CAST ((o), SP_TYPE_ARENA, SPArena))
#define SP_ARENA_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_ARENA, SPArenaClass))
#define SP_IS_ARENA(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ARENA))
#define SP_IS_ARENA_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_ARENA))

#include <gtk/gtkdrawingarea.h>
#include "nr-uta.h"
#include "nr-aa-canvas.h"

struct _SPArena {
	GtkDrawingArea drawingarea;
	NRAACanvas * aacanvas;
	NRIRect viewport;
	NRUTA * uta;
};

struct _SPArenaClass {
	GtkDrawingAreaClass parent_class;
};

GtkType sp_arena_get_type (void);

GtkWidget * sp_arena_new (void);

#endif
