#define __SP_CANVAS_ARENA_C__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <gtk/gtksignal.h>
#include "../helper/sp-canvas-util.h"
#include "../helper/nr-buffers.h"
#include "../helper/nr-plain-stuff.h"
#include "nr-arena.h"
#include "nr-arena-group.h"
#include "canvas-arena.h"

enum {
	ARENA_EVENT,
	LAST_SIGNAL
};

gdouble nr_arena_global_delta = 1.0;

static void sp_marshal_INT__POINTER_POINTER (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args);

static void sp_canvas_arena_class_init (SPCanvasArenaClass * klass);
static void sp_canvas_arena_init (SPCanvasArena * group);
static void sp_canvas_arena_destroy (GtkObject * object);

static void sp_canvas_arena_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_canvas_arena_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);
static double sp_canvas_arena_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);
static gint sp_canvas_arena_event (GnomeCanvasItem *item, GdkEvent *event);

static gint sp_canvas_arena_send_event (SPCanvasArena *arena, GdkEvent *event);

static void sp_canvas_arena_item_added (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca);
static void sp_canvas_arena_remove_item (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca);
static void sp_canvas_arena_request_update (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca);
static void sp_canvas_arena_request_render (NRArena *arena, NRIRect *area, SPCanvasArena *ca);

static GnomeCanvasItemClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
sp_canvas_arena_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPCanvasArena",
			sizeof (SPCanvasArena),
			sizeof (SPCanvasArenaClass),
			(GtkClassInitFunc) sp_canvas_arena_class_init,
			(GtkObjectInitFunc) sp_canvas_arena_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GNOME_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_canvas_arena_class_init (SPCanvasArenaClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (GNOME_TYPE_CANVAS_ITEM);

	signals[ARENA_EVENT] = gtk_signal_new ("arena_event",
					       GTK_RUN_LAST,
					       object_class->type,
					       GTK_SIGNAL_OFFSET (SPCanvasArenaClass, arena_event),
					       sp_marshal_INT__POINTER_POINTER,
					       GTK_TYPE_INT, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = sp_canvas_arena_destroy;

	item_class->update = sp_canvas_arena_update;
	item_class->render = sp_canvas_arena_render;
	item_class->point = sp_canvas_arena_point;
	item_class->event = sp_canvas_arena_event;
}

static void
sp_canvas_arena_init (SPCanvasArena *arena)
{
	arena->sticky = FALSE;

	arena->arena = gtk_type_new (NR_TYPE_ARENA);
	arena->root = nr_arena_item_new (arena->arena, NR_TYPE_ARENA_GROUP);
	nr_arena_group_set_transparent (NR_ARENA_GROUP (arena->root), TRUE);

	arena->active = NULL;

	gtk_signal_connect (GTK_OBJECT (arena->arena), "item_added",
			    GTK_SIGNAL_FUNC (sp_canvas_arena_item_added), arena);
	gtk_signal_connect (GTK_OBJECT (arena->arena), "remove_item",
			    GTK_SIGNAL_FUNC (sp_canvas_arena_remove_item), arena);
	gtk_signal_connect (GTK_OBJECT (arena->arena), "request_update",
			    GTK_SIGNAL_FUNC (sp_canvas_arena_request_update), arena);
	gtk_signal_connect (GTK_OBJECT (arena->arena), "request_render",
			    GTK_SIGNAL_FUNC (sp_canvas_arena_request_render), arena);
}

static void
sp_canvas_arena_destroy (GtkObject *object)
{
	SPCanvasArena *arena;

	arena = SP_CANVAS_ARENA (object);

	if (arena->active) {
		gtk_object_unref (GTK_OBJECT (arena->active));
		arena->active = NULL;
	}

	if (arena->root) {
		nr_arena_item_unref (arena->root);
		arena->root = NULL;
	}

	if (arena) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (arena->arena), arena);
		gtk_object_unref (GTK_OBJECT (arena->arena));
		arena->arena = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_arena_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCanvasArena *arena;
	guint reset;

	arena = SP_CANVAS_ARENA (item);

	if (((GnomeCanvasItemClass *) parent_class)->update)
		(* ((GnomeCanvasItemClass *) parent_class)->update) (item, affine, clip_path, flags);

	memcpy (arena->gc.affine, affine, 6 * sizeof (double));

	if (flags & GNOME_CANVAS_UPDATE_AFFINE) {
		reset = NR_ARENA_ITEM_STATE_ALL;
	} else {
		reset = NR_ARENA_ITEM_STATE_NONE;
	}

	nr_arena_item_invoke_update (arena->root, NULL, &arena->gc, NR_ARENA_ITEM_STATE_ALL, reset);

	item->x1 = arena->root->bbox.x0 - 1;
	item->y1 = arena->root->bbox.y0 - 1;
	item->x2 = arena->root->bbox.x1 + 1;
	item->y2 = arena->root->bbox.y1 + 1;

	if (arena->cursor) {
		NRArenaItem *new;
		/* Mess with enter/leave notifiers */
		new = nr_arena_item_invoke_pick (arena->root, arena->cx, arena->cy, nr_arena_global_delta, arena->sticky);
		if (new != arena->active) {
			GdkEventCrossing ec;
			ec.window = GTK_WIDGET (item->canvas)->window;
			ec.send_event = TRUE;
			ec.subwindow = ec.window;
			ec.time = gdk_time_get ();
			gnome_canvas_c2w (item->canvas, arena->cx, arena->cy, &ec.x, &ec.y);
			/* fixme: */
			if (arena->active) {
				ec.type = GDK_LEAVE_NOTIFY;
				sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
			/* fixme: This is not optimal - better track ::destroy (Lauris) */
			if (arena->active) gtk_object_unref (GTK_OBJECT (arena->active));
			arena->active = new;
			if (arena->active) gtk_object_ref (GTK_OBJECT (arena->active));
			if (arena->active) {
				ec.type = GDK_ENTER_NOTIFY;
				sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
		}
	}
}

static void
sp_canvas_arena_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCanvasArena *arena;
	gint bw, bh, sw, sh;
	gint x, y;

	arena = SP_CANVAS_ARENA (item);

	nr_arena_item_invoke_update (arena->root, NULL, &arena->gc, NR_ARENA_ITEM_STATE_RENDER, NR_ARENA_ITEM_STATE_NONE);

	if (buf->is_bg) {
		gnome_canvas_clear_buffer (buf);
		buf->is_bg = FALSE;
		buf->is_buf = TRUE;
	}

	bw = buf->rect.x1 - buf->rect.x0;
	bh = buf->rect.y1 - buf->rect.y0;
	if ((bw < 1) || (bh < 1)) return;

	/* 65536 is max cached buffer and we need 4 channels */
	if (bw * bh < 16384) {
		/* We can go with single buffer */
		sw = bw;
		sh = bh;
	} else if (bw <= 2048) {
		/* Go with row buffer */
		sw = bw;
		sh = 16384 / bw;
	} else if (bh <= 2048) {
		/* Go with column buffer */
		sw = 16384 / bh;
		sh = bh;
	} else {
		sw = 128;
		sh = 128;
	}

	for (y = buf->rect.y0; y < buf->rect.y1; y += sh) {
		for (x = buf->rect.x0; x < buf->rect.x1; x += sw) {
			NRIRect area;
			NRBuffer *b;

			area.x0 = x;
			area.y0 = y;
			area.x1 = MIN (x + sw, buf->rect.x1);
			area.y1 = MIN (y + sh, buf->rect.y1);

			b = nr_buffer_get (NR_IMAGE_R8G8B8A8, area.x1 - area.x0, area.y1 - area.y0, TRUE, TRUE);
			/* fixme: */
			b->empty = FALSE;

			nr_arena_item_invoke_render (arena->root, &area, b);
			nr_render_r8g8b8_buf (buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + 3 * (x - buf->rect.x0),
					      buf->buf_rowstride,
					      area.x1 - area.x0, area.y1 - area.y0, b, 0, 0);
			nr_buffer_free (b);
		}
	}
}

static double
sp_canvas_arena_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCanvasArena *arena;
	NRArenaItem *picked;

	arena = SP_CANVAS_ARENA (item);

	nr_arena_item_invoke_update (arena->root, NULL, &arena->gc, NR_ARENA_ITEM_STATE_PICK, NR_ARENA_ITEM_STATE_NONE);

	picked = nr_arena_item_invoke_pick (arena->root, cx, cy, nr_arena_global_delta, arena->sticky);

	arena->picked = picked;

	if (picked) {
		*actual_item = item;
		return 0.0;
	}

	return 1e18;
}

static gint
sp_canvas_arena_event (GnomeCanvasItem *item, GdkEvent *event)
{
	SPCanvasArena *arena;
	NRArenaItem *new;
	gint ret;
	/* fixme: This sucks, we have to handle enter/leave notifiers */

	arena = SP_CANVAS_ARENA (item);

	ret = FALSE;

	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		if (!arena->cursor) {
			if (arena->active) {
				g_warning ("Cursor entered to arena with already active item");
				gtk_object_unref (GTK_OBJECT (arena->active));
			}
			arena->cursor = TRUE;
			gnome_canvas_w2c_d (item->canvas, event->crossing.x, event->crossing.y, &arena->cx, &arena->cy);
			arena->active = nr_arena_item_invoke_pick (arena->root, arena->cx, arena->cy, nr_arena_global_delta, arena->sticky);
			if (arena->active) gtk_object_ref (GTK_OBJECT (arena->active));
			ret = sp_canvas_arena_send_event (arena, event);
		}
		break;
	case GDK_LEAVE_NOTIFY:
		if (arena->cursor) {
			ret = sp_canvas_arena_send_event (arena, event);
			if (arena->active) gtk_object_unref (GTK_OBJECT (arena->active));
			arena->active = NULL;
			arena->cursor = FALSE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		gnome_canvas_w2c_d (item->canvas, event->motion.x, event->motion.y, &arena->cx, &arena->cy);
		new = nr_arena_item_invoke_pick (arena->root, arena->cx, arena->cy, nr_arena_global_delta, arena->sticky);
		if (new != arena->active) {
			GdkEventCrossing ec;
			ec.window = event->motion.window;
			ec.send_event = event->motion.send_event;
			ec.subwindow = event->motion.window;
			ec.time = event->motion.time;
			ec.x = event->motion.x;
			ec.y = event->motion.y;
			/* fixme: */
			if (arena->active) {
				ec.type = GDK_LEAVE_NOTIFY;
				ret = sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
			if (arena->active) gtk_object_unref (GTK_OBJECT (arena->active));
			arena->active = new;
			if (arena->active) gtk_object_ref (GTK_OBJECT (arena->active));
			if (arena->active) {
				ec.type = GDK_ENTER_NOTIFY;
				ret = sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
		}
		ret = sp_canvas_arena_send_event (arena, event);
		break;
	default:
		/* Just send event */
		ret = sp_canvas_arena_send_event (arena, event);
		break;
	}

	return ret;
}

static gint
sp_canvas_arena_send_event (SPCanvasArena *arena, GdkEvent *event)
{
	gint ret;

	ret = FALSE;

	if (arena->active) {
		/* Send event to active item */
		ret = nr_arena_item_emit_event (arena->active, (NREvent *) event);
	}
	if (!ret) {
		/* Send event to arena */
		gtk_signal_emit (GTK_OBJECT (arena), signals[ARENA_EVENT], arena->active, event, &ret);
	}

	return ret;
}

static void
sp_canvas_arena_item_added (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca)
{
}

static void
sp_canvas_arena_remove_item (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca)
{
}

static void
sp_canvas_arena_request_update (NRArena *arena, NRArenaItem *item, SPCanvasArena *ca)
{
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (ca));
}

static void
sp_canvas_arena_request_render (NRArena *arena, NRIRect *area, SPCanvasArena *ca)
{
	gnome_canvas_request_redraw (GNOME_CANVAS_ITEM (ca)->canvas, area->x0, area->y0, area->x1, area->y1);
}

void
sp_canvas_arena_set_pick_delta (SPCanvasArena *ca, gdouble delta)
{
	g_return_if_fail (ca != NULL);
	g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

	/* fixme: repick? */
	ca->delta = delta;
}

void
sp_canvas_arena_set_sticky (SPCanvasArena *ca, gboolean sticky)
{
	g_return_if_fail (ca != NULL);
	g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

	/* fixme: repick? */
	ca->sticky = sticky;
}

typedef gint (* SPSignal_INT__POINTER_POINTER) (GtkObject *object, gpointer arg1, gpointer arg2, gpointer user_data);

static void
sp_marshal_INT__POINTER_POINTER (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	SPSignal_INT__POINTER_POINTER rfunc;
	gint *return_val;

	return_val = GTK_RETLOC_INT (args[2]);

	rfunc = (SPSignal_INT__POINTER_POINTER) func;

	*return_val =  (* rfunc) (object, GTK_VALUE_POINTER (args[0]), GTK_VALUE_POINTER (args[1]), func_data);
}


