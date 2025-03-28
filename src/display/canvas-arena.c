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

#include <libnr/nr-blit.h>
#include <libnr/nr-matrix.h>

#include <gtk/gtksignal.h>
#include "../helper/sp-canvas.h"
#include "../helper/sp-canvas-util.h"
#include "../helper/sp-marshal.h"
#include "nr-arena.h"
#include "nr-arena-group.h"
#include "canvas-arena.h"

enum {
	ARENA_EVENT,
	LAST_SIGNAL
};

gdouble nr_arena_global_delta = 1.0;

static void sp_canvas_arena_class_init (SPCanvasArenaClass * klass);
static void sp_canvas_arena_init (SPCanvasArena * group);
static void sp_canvas_arena_destroy (GtkObject * object);

static void sp_canvas_arena_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags);
static void sp_canvas_arena_render (SPCanvasItem *item, SPCanvasBuf *buf);
static double sp_canvas_arena_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item);
static gint sp_canvas_arena_event (SPCanvasItem *item, GdkEvent *event);

static gint sp_canvas_arena_send_event (SPCanvasArena *arena, GdkEvent *event);

static void sp_canvas_arena_item_removed (NRArena *arena, NRArenaItem *item, NRArenaItem *child, void *data);
static void sp_canvas_arena_request_update (NRArena *arena, NRArenaItem *item, void *data);
static void sp_canvas_arena_request_render (NRArena *arena, NRRectL *area, void *data);

NRArenaEventVector carenaev = {
	{NULL},
	NULL,
	sp_canvas_arena_item_removed,
	sp_canvas_arena_request_update,
	sp_canvas_arena_request_render
};

static SPCanvasItemClass *parent_class;
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
		type = gtk_type_unique (SP_TYPE_CANVAS_ITEM, &info);
	}
	return type;
}

static void
sp_canvas_arena_class_init (SPCanvasArenaClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_CANVAS_ITEM);

	signals[ARENA_EVENT] = gtk_signal_new ("arena_event",
					       GTK_RUN_LAST,
					       GTK_CLASS_TYPE(object_class),
					       GTK_SIGNAL_OFFSET (SPCanvasArenaClass, arena_event),
					       sp_marshal_INT__POINTER_POINTER,
					       GTK_TYPE_INT, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

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

	arena->arena = (NRArena *) nr_object_new (NR_TYPE_ARENA);
	arena->root = nr_arena_item_new (arena->arena, NR_TYPE_ARENA_GROUP);
	nr_arena_group_set_transparent (NR_ARENA_GROUP (arena->root), TRUE);

	arena->active = NULL;

	nr_active_object_add_listener ((NRActiveObject *) arena->arena,
				       (NRObjectEventVector *) &carenaev, sizeof (carenaev),
				       arena);
}

static void
sp_canvas_arena_destroy (GtkObject *object)
{
	SPCanvasArena *arena;

	arena = SP_CANVAS_ARENA (object);

	if (arena->active) {
		nr_object_unref ((NRObject *) arena->active);
		arena->active = NULL;
	}

	if (arena->root) {
		nr_arena_item_unref (arena->root);
		arena->root = NULL;
	}

	if (arena->arena) {
		nr_active_object_remove_listener_by_data ((NRActiveObject *) arena->arena, arena);

		nr_object_unref ((NRObject *) arena->arena);
		arena->arena = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_arena_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags)
{
	SPCanvasArena *arena;
	guint reset;

	arena = SP_CANVAS_ARENA (item);

	if (((SPCanvasItemClass *) parent_class)->update)
		(* ((SPCanvasItemClass *) parent_class)->update) (item, ctm, flags);

	arena->gc.transform = *ctm;

	if (flags & SP_CANVAS_UPDATE_AFFINE) {
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
			ec.time = GDK_CURRENT_TIME;
			ec.x = arena->cx;
			ec.y = arena->cy;
			/* fixme: */
			if (arena->active) {
				ec.type = GDK_LEAVE_NOTIFY;
				sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
			/* fixme: This is not optimal - better track ::destroy (Lauris) */
			if (arena->active) nr_object_unref ((NRObject *) arena->active);
			arena->active = new;
			if (arena->active) nr_object_ref ((NRObject *) arena->active);
			if (arena->active) {
				ec.type = GDK_ENTER_NOTIFY;
				sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
			}
		}
	}
}

static void
sp_canvas_arena_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
	SPCanvasArena *arena;
	unsigned int reqflags;
	int bw, bh, sw, sh;
	int x, y;

	arena = SP_CANVAS_ARENA (item);

	reqflags = NR_ARENA_ITEM_STATE_BBOX | NR_ARENA_ITEM_STATE_RENDER;

	if ((arena->root->state & reqflags) != reqflags) {
		nr_arena_item_invoke_update (arena->root, NULL, &arena->gc, reqflags, NR_ARENA_ITEM_STATE_NONE);
	}

	sp_canvas_buf_ensure_buf (buf);

	bw = buf->pixblock.area.x1 - buf->pixblock.area.x0;
	bh = buf->pixblock.area.y1 - buf->pixblock.area.y0;
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

/* fixme: RGB transformed bitmap blit is not implemented (Lauris) */
/* And even if it would be, unless it uses MMX there is little reason to go RGB */
#define STRICT_RGBA

	for (y = buf->pixblock.area.y0; y < buf->pixblock.area.y1; y += sh) {
		for (x = buf->pixblock.area.x0; x < buf->pixblock.area.x1; x += sw) {
			NRRectL area;
#ifdef STRICT_RGBA
			NRPixBlock pb;
#endif
			/* NRPixBlock cb; */

			area.x0 = x;
			area.y0 = y;
			area.x1 = MIN (x + sw, buf->pixblock.area.x1);
			area.y1 = MIN (y + sh, buf->pixblock.area.y1);

#ifdef STRICT_RGBA
			nr_pixblock_setup_fast (&pb, NR_PIXBLOCK_MODE_R8G8B8A8P, area.x0, area.y0, area.x1, area.y1, TRUE);
			/* fixme: */
			pb.empty = FALSE;
#endif

#if 0
			nr_pixblock_setup_extern (&cb, NR_PIXBLOCK_MODE_R8G8B8, area.x0, area.y0, area.x1, area.y1,
						  buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + 3 * (x - buf->rect.x0),
						  buf->buf_rowstride,
						  FALSE, FALSE);
#endif

#ifdef STRICT_RGBA
			nr_arena_item_invoke_render (arena->root, &area, &pb, 0);
#if 0
			nr_blit_pixblock_pixblock (&cb, &pb);
#else
			nr_blit_pixblock_pixblock (&buf->pixblock, &pb);
#endif
			nr_pixblock_release (&pb);
#else
			nr_arena_item_invoke_render (arena->root, &area, &cb, 0);
#endif

			/* nr_pixblock_release (&cb); */
		}
	}
}

static double
sp_canvas_arena_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item)
{
	SPCanvasArena *arena;
	NRArenaItem *picked;
	unsigned int reqflags;

	arena = SP_CANVAS_ARENA (item);

	reqflags = NR_ARENA_ITEM_STATE_BBOX | NR_ARENA_ITEM_STATE_PICK;

	if ((arena->root->state & reqflags) != reqflags) {
		nr_arena_item_invoke_update (arena->root, NULL, &arena->gc, reqflags, NR_ARENA_ITEM_STATE_NONE);
	}

	picked = nr_arena_item_invoke_pick (arena->root, x, y, nr_arena_global_delta, arena->sticky);

	arena->picked = picked;

	if (picked) {
		*actual_item = item;
		return 0.0;
	}

	return 1e18;
}

static gint
sp_canvas_arena_event (SPCanvasItem *item, GdkEvent *event)
{
	SPCanvasArena *arena;
	NRArenaItem *new;
	unsigned int reqflags;
	int ret;
	/* fixme: This sucks, we have to handle enter/leave notifiers */

	arena = SP_CANVAS_ARENA (item);
	reqflags = NR_ARENA_ITEM_STATE_BBOX | NR_ARENA_ITEM_STATE_PICK;

	ret = FALSE;

	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		if (!arena->cursor) {
			if (arena->active) {
#ifdef DEBUG_ARENA
				g_warning ("Cursor entered to arena with already active item");
#endif
				nr_object_unref ((NRObject *) arena->active);
			}
			arena->cursor = TRUE;
			arena->cx = event->crossing.x;
			arena->cy = event->crossing.y;
			if ((arena->root->state & reqflags) != reqflags) {
				nr_arena_item_invoke_update (arena->root, NULL, &arena->gc,
							     reqflags, NR_ARENA_ITEM_STATE_NONE);
			}
			arena->active = nr_arena_item_invoke_pick (arena->root, arena->cx, arena->cy,
								   nr_arena_global_delta, arena->sticky);
			if (arena->active) nr_object_ref ((NRObject *) arena->active);
			ret = sp_canvas_arena_send_event (arena, event);
		}
		break;
	case GDK_LEAVE_NOTIFY:
		if (arena->cursor) {
			ret = sp_canvas_arena_send_event (arena, event);
			if (arena->active) nr_object_unref ((NRObject *) arena->active);
			arena->active = NULL;
			arena->cursor = FALSE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		arena->cx = event->motion.x;
		arena->cy = event->motion.y;
		if ((arena->root->state & reqflags) != reqflags) {
			nr_arena_item_invoke_update (arena->root, NULL, &arena->gc,
						     reqflags, NR_ARENA_ITEM_STATE_NONE);
		}
		new = nr_arena_item_invoke_pick (arena->root, arena->cx, arena->cy,
						 nr_arena_global_delta, arena->sticky);
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
			if (arena->active) nr_object_unref ((NRObject *) arena->active);
			arena->active = new;
			if (arena->active) nr_object_ref ((NRObject *) arena->active);
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

	/* Send event to arena */
	gtk_signal_emit (GTK_OBJECT (arena), signals[ARENA_EVENT], arena->active, event, &ret);

	return ret;
}

static void
sp_canvas_arena_item_removed (NRArena *arena, NRArenaItem *item, NRArenaItem *child, void *data)
{
	SPCanvasArena *ca;
	ca = (SPCanvasArena *) data;
	if (child == ca->active) {
		ca->active = NULL;
	}
}

static void
sp_canvas_arena_request_update (NRArena *arena, NRArenaItem *item, void *data)
{
	sp_canvas_item_request_update (SP_CANVAS_ITEM (data));
}

static void
sp_canvas_arena_request_render (NRArena *arena, NRRectL *area, void *data)
{
	sp_canvas_request_redraw (SP_CANVAS_ITEM (data)->canvas, area->x0, area->y0, area->x1, area->y1);
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

void
sp_canvas_arena_render_pixblock (SPCanvasArena *ca, NRPixBlock *pb)
{
	NRRectL area;

	g_return_if_fail (ca != NULL);
	g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

	/* fixme: */
	pb->empty = FALSE;

	area.x0 = pb->area.x0;
	area.y0 = pb->area.y0;
	area.x1 = pb->area.x1;
	area.y1 = pb->area.y1;

	nr_arena_item_invoke_render (ca->root, &area, pb, 0);
}

