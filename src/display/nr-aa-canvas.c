#define _NR_AA_CANVAS_C_

/*
 * NRAACanvas
 *
 * RGB Buffer based canvas implementation
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include "nr-aa-canvas.h"

static void nr_aa_canvas_class_init (NRAACanvasClass * klass);
static void nr_aa_canvas_init (NRAACanvas * canvas);
static void nr_aa_canvas_destroy (GtkObject * object);

static NRCanvasClass * parent_class;

GtkType
nr_aa_canvas_get_type (void)
{
	static GtkType aa_type = 0;
	if (!aa_type) {
		GtkTypeInfo aa_info = {
			"NRAACanvas",
			sizeof (NRAACanvas),
			sizeof (NRAACanvasClass),
			(GtkClassInitFunc) nr_aa_canvas_class_init,
			(GtkObjectInitFunc) nr_aa_canvas_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		aa_type = gtk_type_unique (nr_canvas_get_type (), &aa_info);
	}
	return aa_type;
}

static void
nr_aa_canvas_class_init (NRAACanvasClass * klass)
{
	GtkObjectClass * object_class;
	NRCanvasClass * canvas_class;

	object_class = (GtkObjectClass *) klass;
	canvas_class = (NRCanvasClass *) klass;

	parent_class = gtk_type_class (nr_canvas_get_type ());

	object_class->destroy = nr_aa_canvas_destroy;
}

static void
nr_aa_canvas_init (NRAACanvas * canvas)
{
}

static void
nr_aa_canvas_destroy (GtkObject * object)
{
	NRAACanvas * aacanvas;

	aacanvas = (NRAACanvas *) object;

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

NRAACanvas *
nr_aa_canvas_new (void)
{
	NRAACanvas * aacanvas;

	aacanvas = gtk_type_new (NR_TYPE_AA_CANVAS);

	return aacanvas;
}


