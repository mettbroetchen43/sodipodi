#ifndef _NR_CANVAS_ITEM_H_
#define _NR_CANVAS_ITEM_H_

/*
 * NRCanvasItem
 *
 * Base class for NRCanvas items
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRCanvasItem NRCanvasItem;
typedef struct _NRCanvasItemClass NRCanvasItemClass;

#define NR_TYPE_CANVAS_ITEM (nr_canvas_item_get_type ())
#define NR_CANVAS_ITEM(o) (GTK_CHECK_CAST ((o), NR_TYPE_CANVAS_ITEM, NRCanvasItem))
#define NR_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_CANVAS_ITEM, NRCanvasItemClass))
#define NR_IS_CANVAS_ITEM(o) (GTK_CHECK_TYPE ((o), NR_TYPE_CANVAS_ITEM))
#define NR_IS_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_CANVAS_ITEM))

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include "nr-primitives.h"
#include "nr-canvas.h"

struct _NRCanvasItem {
	GtkObject object;
	NRCanvas * canvas;
	NRCanvasItem * parent;
	NRIRect bbox;
	NRAffine * transform;
};

struct _NRCanvasItemClass {
	GtkObjectClass parent_class;
	/* To be used by NRCanvas */
	void (* add_child) (NRCanvasItem * item, NRCanvasItem * child, gint position);
	void (* remove_child) (NRCanvasItem * item, NRCanvasItem * child);
};

GtkType nr_canvas_item_get_type (void);

void nr_canvas_item_set_transform (NRCanvasItem * item, NRAffine * transform);

#endif
