#define __SP_PAINT_SERVER_C__

/*
 * SPPaintServer
 *
 * Abstract base class for different paint types
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include "helper/nr-plain-stuff.h"
#include "sp-paint-server.h"

static void sp_paint_server_class_init (SPPaintServerClass *klass);
static void sp_paint_server_init (SPPaintServer *ps);

static void sp_paint_server_destroy (GtkObject *object);

static void sp_painter_stale_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride);

static SPObjectClass *parent_class;
static GSList *stale_painters = NULL;

GtkType
sp_paint_server_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPPaintServer",
			sizeof (SPPaintServer),
			sizeof (SPPaintServerClass),
			(GtkClassInitFunc) sp_paint_server_class_init,
			(GtkObjectInitFunc) sp_paint_server_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_OBJECT, &info);
	}
	return type;
}

static void
sp_paint_server_class_init (SPPaintServerClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_OBJECT);

	object_class->destroy = sp_paint_server_destroy;
}

static void
sp_paint_server_init (SPPaintServer *ps)
{
	ps->painters = NULL;
}

static void
sp_paint_server_destroy (GtkObject *object)
{
	SPPaintServer *ps;

	ps = SP_PAINT_SERVER (object);

	while (ps->painters) {
		SPPainter *painter;
		painter = ps->painters;
		ps->painters = painter->next;
		stale_painters = g_slist_prepend (stale_painters, painter);
		painter->next = NULL;
		painter->server = NULL;
		painter->fill = sp_painter_stale_fill;
	}

	if (((GtkObjectClass *) parent_class)->destroy)
		(* ((GtkObjectClass *) parent_class)->destroy) (object);
}

SPPainter *
sp_paint_server_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox)
{
	SPPainter *painter;

	g_return_val_if_fail (ps != NULL, NULL);
	g_return_val_if_fail (SP_IS_PAINT_SERVER (ps), NULL);
	g_return_val_if_fail (affine != NULL, NULL);
	g_return_val_if_fail (bbox != NULL, NULL);

	painter = NULL;
	if (((SPPaintServerClass *) ((GtkObject *) ps)->klass)->painter_new)
		painter = (* ((SPPaintServerClass *) ((GtkObject *) ps)->klass)->painter_new) (ps, affine, opacity, bbox);

	if (painter) {
		painter->next = ps->painters;
		painter->server = ps;
		painter->type = GTK_OBJECT_TYPE (ps);
		ps->painters = painter;
	}

	return painter;
}

static void
sp_paint_server_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPPainter *p, *r;

	g_return_if_fail (ps != NULL);
	g_return_if_fail (SP_IS_PAINT_SERVER (ps));
	g_return_if_fail (painter != NULL);

	r = NULL;
	for (p = ps->painters; p != NULL; p = p->next) {
		if (p == painter) {
			if (r) {
				r->next = p->next;
			} else {
				ps->painters = p->next;
			}
			p->next = NULL;
			if (((SPPaintServerClass *) ((GtkObject *) ps)->klass)->painter_free)
				(* ((SPPaintServerClass *) ((GtkObject *) ps)->klass)->painter_free) (ps, painter);
			return;
		}
		r = p;
	}
	g_assert_not_reached ();
}

SPPainter *
sp_painter_free (SPPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);

	if (painter->server) {
		sp_paint_server_painter_free (painter->server, painter);
	} else {
		if (((SPPaintServerClass *) gtk_type_class (painter->type))->painter_free)
			(* ((SPPaintServerClass *) gtk_type_class (painter->type))->painter_free) (NULL, painter);
		stale_painters = g_slist_remove (stale_painters, painter);
	}

	return NULL;
}

static void
sp_painter_stale_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
	nr_render_r8g8b8a8_gray_garbage (px, width, height, rowstride);
}


