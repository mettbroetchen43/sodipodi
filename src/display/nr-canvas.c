#define _NR_CANVAS_C_

/*
 * NRCanvas
 *
 * Abstract base class for display caches
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include "nr-canvas.h"

static void nr_canvas_class_init (NRCanvasClass * klass);
static void nr_canvas_init (NRCanvas * object);
static void nr_canvas_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

GtkType
nr_canvas_get_type (void)
{
	static GtkType canvas_type = 0;
	if (!canvas_type) {
		GtkTypeInfo canvas_info = {
			"NRCanvas",
			sizeof (NRCanvas),
			sizeof (NRCanvasClass),
			(GtkClassInitFunc) nr_canvas_class_init,
			(GtkObjectInitFunc) nr_canvas_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		canvas_type = gtk_type_unique (gtk_object_get_type (), &canvas_info);
	}
	return canvas_type;
}

static void
nr_canvas_class_init (NRCanvasClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = nr_canvas_destroy;
}

static void
nr_canvas_init (NRCanvas * canvas)
{
}

static void
nr_canvas_destroy (GtkObject * object)
{
	NRCanvas * canvas;

	canvas = (NRCanvas *) object;

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

