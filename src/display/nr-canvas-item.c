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
	nr_irect_set_empty (&item->bbox);
	item->transform = NULL;
}

static void
nr_canvas_item_destroy (GtkObject * object)
{
	NRCanvasItem * item;

	item = (NRCanvasItem *) object;

	/* I am not sure, whether to allow destroying items directly - maybe one should use unparent() instead */
	if (item->parent) {
		if (((NRCanvasItemClass *) ((GtkObject *) item->parent)->klass)->remove_child) {
			(* ((NRCanvasItemClass *) ((GtkObject *) item->parent)->klass)->remove_child) (item->parent, item);
		} else {
			/* Every container HAS TO implement ::remove_child () */
			g_assert_not_reached ();
		}
	}

	if (item->transform) {
		g_free (item->transform);
		item->transform = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/* To be used only by implementations */

NRCanvasItemState
nr_canvas_item_invoke_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags)
{
	NRCanvasItemState itemstate, newstate;

	g_return_val_if_fail (item != NULL, NR_CANVAS_ITEM_STATE_INITIAL);
	g_return_val_if_fail (NR_IS_CANVAS_ITEM (item), NR_CANVAS_ITEM_STATE_INITIAL);
	g_return_val_if_fail (ctx != NULL, NR_CANVAS_ITEM_STATE_INITIAL);
	g_return_val_if_fail (((NRCanvasItemClass *) ((GtkObject *) item)->klass)->update, NR_CANVAS_ITEM_STATE_INITIAL);

	itemstate = NR_CI_GET_STATE (item);

	if ((itemstate < state) || (flags & NR_CANVAS_ITEM_FORCED_UPDATE)) {
		newstate = (* ((NRCanvasItemClass *) ((GtkObject *) item)->klass)->update) (item, ctx, state, flags);
		NR_CI_SET_STATE (item, newstate);
		return newstate;
	}

	return itemstate;
}

void
nr_canvas_item_invoke_render (NRCanvasItem * item, NRDrawingArea * area)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_CANVAS_ITEM (item));
	g_return_if_fail (area != NULL);
	g_return_if_fail (((NRCanvasItemClass *) ((GtkObject *) item)->klass)->render);

	(* ((NRCanvasItemClass *) ((GtkObject *) item)->klass)->render) (item, area);
}

NRCanvasItem *
nr_canvas_item_invoke_pick (NRCanvasItem * item, NRPoint * point)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (NR_IS_CANVAS_ITEM (item), NULL);
	g_return_val_if_fail (point != NULL, NULL);
	g_return_val_if_fail (((NRCanvasItemClass *) ((GtkObject *) item)->klass)->pick, NULL);

	return (* ((NRCanvasItemClass *) ((GtkObject *) item)->klass)->pick) (item, point);
}

void
nr_canvas_item_request_update (NRCanvasItem * item, NRCanvasItemState state, guint32 flags)
{
	NRCanvasItemState itemstate;

	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_CANVAS_ITEM (item));

	itemstate = NR_CI_GET_STATE (item);

	if ((itemstate > state) || (flags & NR_CANVAS_ITEM_FORCED_UPDATE)) {
		if (flags & NR_CANVAS_ITEM_FORCED_UPDATE) {
			GTK_OBJECT_SET_FLAGS (item, NR_CANVAS_ITEM_FORCED_UPDATE);
		}
		NR_CI_SET_STATE (item, state);
		if (item->parent) {
			nr_canvas_item_request_update (item->parent, state, 0);
		} else {
			nr_canvas_invoke_request_update (item->canvas);
		}
	}
}

void
nr_canvas_item_set_transform (NRCanvasItem * item, NRAffine * transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (NR_IS_CANVAS_ITEM (item));

	if (!transform) {
		if (!item->transform) return;
		g_free (item->transform);
		item->transform = NULL;
	} else {
		if (!item->transform) item->transform = g_new (NRAffine, 1);
		*item->transform = *transform;
	}

	nr_canvas_item_request_update (item, NR_CANVAS_ITEM_STATE_INITIAL, NR_CANVAS_ITEM_FORCED_UPDATE);
}


