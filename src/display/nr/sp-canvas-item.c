#define _SP_CANVAS_ITEM_C_

/*
 * SPCanvasItem
 *
 * Implementing Gdk specific ::event()
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include <gtk/gtksignal.h>
#include "nr-aa-canvas.h" /* For AA stuff */
#include "sp-canvas-item.h"

enum {EVENT, LAST_SIGNAL};

static void sp_canvas_item_class_init (SPCanvasItemClass * klass);
static void sp_canvas_item_init (SPCanvasItem * spci);
static void sp_canvas_item_destroy (GtkObject * object);

static NRCanvasItemState sp_canvas_item_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags);

typedef gboolean (* SPCanvasItemEvent) (GtkObject * item, gpointer arg, gpointer data);

static void sp_canvas_item_marshal_event (GtkObject * object, GtkSignalFunc func, gpointer data, GtkArg * args);

static guint item_signals[LAST_SIGNAL] = {0};
static NRCanvasItemClass * parent_class;

GtkType
sp_canvas_item_get_type (void)
{
	static GtkType item_type = 0;
	if (!item_type) {
		GtkTypeInfo item_info = {
			"SPCanvasItem",
			sizeof (SPCanvasItem),
			sizeof (SPCanvasItemClass),
			(GtkClassInitFunc) sp_canvas_item_class_init,
			(GtkObjectInitFunc) sp_canvas_item_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		item_type = gtk_type_unique (nr_canvas_item_get_type (), &item_info);
	}
	return item_type;
}

static void
sp_canvas_item_class_init (SPCanvasItemClass * klass)
{
	GtkObjectClass * object_class;
	NRCanvasItemClass * item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRCanvasItemClass *) klass;

	parent_class = gtk_type_class (nr_canvas_item_get_type ());

	item_signals[EVENT] = gtk_signal_new ("event", GTK_RUN_LAST,
					      object_class->type,
					      GTK_SIGNAL_OFFSET (SPCanvasItemClass, event),
					      sp_canvas_item_marshal_event,
					      GTK_TYPE_BOOL, 1,
					      GTK_TYPE_GDK_EVENT);

	gtk_object_class_add_signals (object_class, item_signals, LAST_SIGNAL);

	object_class->destroy = sp_canvas_item_destroy;

	item_class->update = sp_canvas_item_update;
}

static void
sp_canvas_item_init (SPCanvasItem * spci)
{
	NRCanvasItem * item;
	NRAAGraphicCtx * aactx;

	item = (NRCanvasItem *) spci;

	aactx = g_new (NRAAGraphicCtx, 1);
	nr_affine_set_identity (&item->ctx->transform);
	aactx->opacity = 1.0;

	item->ctx = (NRGraphicCtx *) aactx;
}

static void
sp_canvas_item_destroy (GtkObject * object)
{
	NRCanvasItem * item;

	item = (NRCanvasItem *) object;

	if (item->ctx) {
		g_free (item->ctx);
		item->ctx = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static NRCanvasItemState
sp_canvas_item_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags)
{
	g_assert (item->ctx);

	*((NRAAGraphicCtx *) item->ctx) = *((NRAAGraphicCtx *) ctx);

	return state;
}

static void
sp_canvas_item_marshal_event (GtkObject * object, GtkSignalFunc func, gpointer data, GtkArg * args)
{
	SPCanvasItemEvent rfunc;
	gint * ret;

	rfunc = (SPCanvasItemEvent) func;
	ret = GTK_RETLOC_BOOL (args[1]);

	*ret = (* rfunc) (object, GTK_VALUE_BOXED (args[0]), data);
}

gboolean
sp_canvas_item_emit_event (SPCanvasItem * item, GdkEvent * event)
{
	gboolean finished;

	while ((item) && (!finished)) {
		NRCanvasItem * parent;
		gtk_object_ref (GTK_OBJECT (item));
		gtk_signal_emit (GTK_OBJECT (item), item_signals[EVENT], event, &finished);
		parent = NR_CANVAS_ITEM (item)->parent;
		gtk_object_unref (GTK_OBJECT (item));
		item = SP_CANVAS_ITEM (parent);
	}

	return finished;
}


