#ifndef _NR_CANVAS_H_
#define _NR_CANVAS_H_

/*
 * NRCanvas
 *
 * Abstract base class for display caches
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRCanvas NRCanvas;
typedef struct _NRCanvasClass NRCanvasClass;

#define NR_TYPE_CANVAS (nr_canvas_get_type ())
#define NR_CANVAS(o) (GTK_CHECK_CAST ((o), NR_TYPE_CANVAS, NRCanvas))
#define NR_CANVAS_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_CANVAS, NRCanvasClass))
#define NR_IS_CANVAS(o) (GTK_CHECK_TYPE ((o), NR_TYPE_CANVAS))
#define NR_IS_CANVAS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_CANVAS))

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include "nr-canvas-item.h"

struct _NRCanvas {
	GtkObject object;
	NRCanvasItem * root;
};

struct _NRCanvasClass {
	GtkObjectClass parent_class;
	NRCanvasItem * (* create_item) (NRCanvas * canvas, NRCanvasItem * parent, GtkType type);
};

GtkType nr_canvas_get_type (void);

/* Methods */

NRCanvasItem * nr_canvas_get_root (NRCanvas * canvas);

/* This is here because we use class method internally */

NRCanvasItem * nr_canvas_item_new (NRCanvasItem * parent, GtkType type);

#endif
