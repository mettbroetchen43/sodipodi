#ifndef _SP_CANVAS_ITEM_H_
#define _SP_CANVAS_ITEM_H_

/*
 * SPCanvasItem
 *
 * Implementing Gdk specific ::event()
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _SPCanvasItem SPCanvasItem;
typedef struct _SPCanvasItemClass SPCanvasItemClass;

#define SP_TYPE_CANVAS_ITEM (sp_canvas_item_get_type ())
#define SP_CANVAS_ITEM(o) (GTK_CHECK_CAST ((o), SP_TYPE_CANVAS_ITEM, SPCanvasItem))
#define SP_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_CANVAS_ITEM, SPCanvasItemClass))
#define SP_IS_CANVAS_ITEM(o) (GTK_CHECK_TYPE ((o), SP_TYPE_CANVAS_ITEM))
#define SP_IS_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_CANVAS_ITEM))

#include <gdk/gdk.h>
#include "nr-canvas-item.h"

struct _SPCanvasItem {
	NRCanvasItem item;
};

struct _SPCanvasItemClass {
	NRCanvasItemClass parent_class;
	/* Event */
	gboolean (* event) (SPCanvasItem * item, GdkEvent * event);
};

GtkType sp_canvas_item_get_type (void);

gboolean sp_canvas_item_emit_event (SPCanvasItem * item, GdkEvent * event);

#endif
