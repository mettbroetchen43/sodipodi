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
static void nr_canvas_init (NRCanvas * canvas);
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
	/* Only for reference */
	canvas->root = NULL;
}

static void
nr_canvas_destroy (GtkObject * object)
{
	NRCanvas * canvas;

	canvas = (NRCanvas *) object;

	if (canvas->root) {
		gtk_object_unref (GTK_OBJECT (canvas->root));
		canvas->root = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

NRCanvasItem *
nr_canvas_get_root (NRCanvas * canvas)
{
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (NR_IS_CANVAS (canvas), NULL);

	return canvas->root;
}

NRCanvasItem *
nr_canvas_item_new (NRCanvasItem * parent, GtkType type)
{
	NRCanvas * canvas;
	NRCanvasItem * item;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (NR_IS_CANVAS_ITEM (parent), NULL);
	g_return_val_if_fail (parent->canvas != NULL, NULL);
	g_return_val_if_fail (NR_IS_CANVAS (parent->canvas), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, NR_TYPE_CANVAS_ITEM), NULL);

	canvas = parent->canvas;

	if (((NRCanvasClass *) ((GtkObject *) canvas)->klass)->create_item) {
		item = (* ((NRCanvasClass *) ((GtkObject *) canvas)->klass)->create_item) (canvas, parent, type);
	} else {
		/* fixme: */
		item = gtk_type_new (type);
	}

	if (((NRCanvasItemClass *) ((GtkObject *) parent)->klass)->add_child) {
		(* ((NRCanvasItemClass *) ((GtkObject *) parent)->klass)->add_child) (parent, item, -1);
	} else {
		/* Every container HAS TO implement ::add_child () */
		g_assert_not_reached ();
	}

	item->canvas = canvas;
	/* fixme: */
	gtk_object_unref ((GtkObject *) item);

	return item;
}

/* To be used solely by Canvas implementations */

void
nr_canvas_invoke_request_update (NRCanvas * canvas)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (NR_IS_CANVAS (canvas));

	if (((NRCanvasClass *) ((GtkObject *) canvas)->klass)->request_update) {
		(* ((NRCanvasClass *) ((GtkObject *) canvas)->klass)->request_update) (canvas);
	} else {
		/* fixme: */
		g_assert_not_reached ();
	}
}

/* Returns new highest state common for all canvas items */

NRCanvasItemState
nr_canvas_invoke_update (NRCanvas * canvas, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags)
{
	g_return_val_if_fail (canvas != NULL, NR_CANVAS_ITEM_STATE_INITIAL);
	g_return_val_if_fail (NR_IS_CANVAS (canvas), NR_CANVAS_ITEM_STATE_INITIAL);
	g_return_val_if_fail (ctx != NULL, NR_CANVAS_ITEM_STATE_INITIAL);

	return nr_canvas_item_invoke_update (canvas->root, ctx, state, flags);
}

/* To be used solely by Canvas implementations */

void
nr_canvas_invoke_render (NRCanvas * canvas, NRDrawingArea * area)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (NR_IS_CANVAS (canvas));
	g_return_if_fail (area != NULL);

	nr_canvas_item_invoke_render (canvas->root, area);
}

/* To be used solely by Canvas implementations */

NRCanvasItem *
nr_canvas_invoke_pick (NRCanvas * canvas, NRPoint * point)
{
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (NR_IS_CANVAS (canvas), NULL);
	g_return_val_if_fail (point != NULL, NULL);

	return nr_canvas_item_invoke_pick (canvas->root, point);
}


