#define _NR_CANVAS_ITEM_C_

/*
 * NRCanvasItem
 *
 * Base class for NRCanvas items
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include "nr-canvas-item.h"

static void nr_canvas_item_class_init (NRCanvasItemClass * klass);
static void nr_canvas_item_init (NRCanvasItem * item);
static void nr_canvas_item_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

GtkType
nr_canvas_item_get_type (void)
{
	static GtkType item_type = 0;
	if (!item_type) {
		GtkTypeInfo item_info = {
			"NRCanvasItem",
			sizeof (NRCanvasItem),
			sizeof (NRCanvasItemClass),
			(GtkClassInitFunc) nr_canvas_item_class_init,
			(GtkObjectInitFunc) nr_canvas_item_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		item_type = gtk_type_unique (gtk_object_get_type (), &item_info);
	}
	return item_type;
}

static void
nr_canvas_item_class_init (NRCanvasItemClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = nr_canvas_item_destroy;
}

static void
nr_canvas_item_init (NRCanvasItem * item)
{
	/* Only for reference */
	item->canvas = NULL;
	item->parent = NULL;
}

static void
nr_canvas_item_destroy (GtkObject * object)
{
	NRCanvasItem * item;

	item = (NRCanvasItem *) object;

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

