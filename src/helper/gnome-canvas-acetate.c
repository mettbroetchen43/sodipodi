#define __GNOME_CANVAS_ACETATE_C__

/*
 * Infinite invisible canvas item
 *
 * Author:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@acm.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1998-1999 The Free Software Foundation
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "gnome-canvas-acetate.h"

static void gnome_canvas_acetate_class_init (GnomeCanvasAcetateClass *class);
static void gnome_canvas_acetate_init (GnomeCanvasAcetate *acetate);
static void gnome_canvas_acetate_destroy (GtkObject *object);

static void gnome_canvas_acetate_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static double gnome_canvas_acetate_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);

static GnomeCanvasItemClass *parent_class;

GtkType
gnome_canvas_acetate_get_type (void)
{
	static GtkType acetate_type = 0;
	if (!acetate_type) {
		GtkTypeInfo acetate_info = {
			"GnomeCanvasAcetate",
			sizeof (GnomeCanvasAcetate),
			sizeof (GnomeCanvasAcetateClass),
			(GtkClassInitFunc) gnome_canvas_acetate_class_init,
			(GtkObjectInitFunc) gnome_canvas_acetate_init,
			NULL, NULL, NULL
		};
		acetate_type = gtk_type_unique (gnome_canvas_item_get_type (), &acetate_info);
	}
	return acetate_type;
}

static void
gnome_canvas_acetate_class_init (GnomeCanvasAcetateClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	object_class->destroy = gnome_canvas_acetate_destroy;

	item_class->update = gnome_canvas_acetate_update;
	item_class->point = gnome_canvas_acetate_point;
}

static void
gnome_canvas_acetate_init (GnomeCanvasAcetate *acetate)
{
	/* Nothing here */
}

static void
gnome_canvas_acetate_destroy (GtkObject *object)
{
	GnomeCanvasAcetate *acetate;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ACETATE (object));

	acetate = GNOME_CANVAS_ACETATE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_canvas_acetate_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	item->x1 = -G_MAXINT;
	item->y1 = -G_MAXINT;
	item->x2 = G_MAXINT;
	item->y2 = G_MAXINT;
}

static double
gnome_canvas_acetate_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	*actual_item = item;
	return 0.0;
}

