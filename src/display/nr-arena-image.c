#define __NR_ARENA_IMAGE_C__

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

#include <math.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-compose-transform.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_affine.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "nr-arena-image.h"

gint nr_arena_image_x_sample = 1;
gint nr_arena_image_y_sample = 1;

/* fixme: This should go to common header */
#define NR_ARENA_STICKY_FLAG (1 << 16)

/*
 * NRArenaCanvasImage
 *
 */

static void nr_arena_image_class_init (NRArenaImageClass *klass);
static void nr_arena_image_init (NRArenaImage *image);
static void nr_arena_image_dispose (GObject *object);

static guint nr_arena_image_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_image_render (NRArenaItem *item, NRRectL *area, NRBuffer *b);
static NRArenaItem *nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

static NRArenaItemClass *parent_class;

GType
nr_arena_image_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (NRArenaImageClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) nr_arena_image_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (NRArenaImage),
			16,	/* n_preallocs */
			(GInstanceInitFunc) nr_arena_image_init,
		};
		type = g_type_register_static (NR_TYPE_ARENA_ITEM, "NRArenaImage", &info, 0);
	}
	return type;
}

static void
nr_arena_image_class_init (NRArenaImageClass *klass)
{
	GObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	parent_class = g_type_class_ref (NR_TYPE_ARENA_ITEM);

	object_class->dispose = nr_arena_image_dispose;

	item_class->update = nr_arena_image_update;
	item_class->render = nr_arena_image_render;
	item_class->pick = nr_arena_image_pick;
}

static void
nr_arena_image_init (NRArenaImage *image)
{
	image->pixbuf = NULL;
	image->x = image->y = 0.0;
	image->width = image->height = 1.0;

	nr_matrix_f_set_identity (&image->grid2px);
}

static void
nr_arena_image_dispose (GObject *object)
{
	NRArenaImage *image;

	image = NR_ARENA_IMAGE (object);

	if (image->pixbuf) {
		gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->dispose)
		G_OBJECT_CLASS (parent_class)->dispose (object);
}

static guint
nr_arena_image_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
{
	NRArenaImage *image;
	gdouble hscale, vscale;
	gdouble grid2px[6];

	image = NR_ARENA_IMAGE (item);

	/* Request render old */
	nr_arena_item_request_render (item);

	/* Copy affine */
	art_affine_invert (grid2px, gc->affine);
	if (image->pixbuf) {
		hscale = (fabs (image->width > 1e-9)) ? gdk_pixbuf_get_width (image->pixbuf) / image->width : 1e9;
		vscale = (fabs (image->height > 1e-9)) ? gdk_pixbuf_get_height (image->pixbuf) / image->height : 1e9;
	} else {
		hscale = 1e9;
		vscale = 1e9;
	}

	image->grid2px.c[0] = grid2px[0] * hscale;
	image->grid2px.c[2] = grid2px[2] * hscale;
	image->grid2px.c[4] = grid2px[4] * hscale;
	image->grid2px.c[1] = grid2px[1] * vscale;
	image->grid2px.c[3] = grid2px[3] * vscale;
	image->grid2px.c[5] = grid2px[5] * vscale;

	image->grid2px.c[4] -= image->x * hscale;
	image->grid2px.c[5] -= image->y * vscale;

	/* Calculate bbox */
	if (image->pixbuf) {
		ArtDRect dim, bbox;

		dim.x0 = image->x;
		dim.y0 = image->y;
		dim.x1 = image->x + image->width;
		dim.y1 = image->y + image->height;
		art_drect_affine_transform (&bbox, &dim, gc->affine);

		item->bbox.x0 = (gint) floor (bbox.x0);
		item->bbox.y0 = (gint) floor (bbox.y0);
		item->bbox.x1 = (gint) ceil (bbox.x1);
		item->bbox.y1 = (gint) ceil (bbox.y1);
	} else {
		item->bbox.x0 = (gint) gc->affine[4];
		item->bbox.y0 = (gint) gc->affine[5];
		item->bbox.x1 = item->bbox.x0 - 1;
		item->bbox.y1 = item->bbox.y0 - 1;
	}

	nr_arena_item_request_render (item);

	return NR_ARENA_ITEM_STATE_ALL;
}

#define FBITS 12
#define XSAMPLE nr_arena_image_x_sample
#define YSAMPLE nr_arena_image_y_sample
#define b2i (image->grid2px)

static guint
nr_arena_image_render (NRArenaItem *item, NRRectL *area, NRBuffer *buf)
{
	NRArenaImage *image;
	guint32 Falpha;
	guchar *spx, *dpx;
	gint dw, dh, drs, sw, sh, srs;
	NRMatrixF d2s;

	image = NR_ARENA_IMAGE (item);

	if (!image->pixbuf) return item->state;

	Falpha = (guint32) floor (item->opacity * 255.9999);
	if (Falpha < 1) return item->state;

	dpx = buf->px;
	drs = buf->rs;
	dw = area->x1 - area->x0;
	dh = area->y1 - area->y0;

	spx = gdk_pixbuf_get_pixels (image->pixbuf);
	srs = gdk_pixbuf_get_rowstride (image->pixbuf);
	/* Width in bytes */
	sw = gdk_pixbuf_get_width (image->pixbuf);
	sh = gdk_pixbuf_get_height (image->pixbuf);

	d2s.c[0] = b2i.c[0];
	d2s.c[1] = b2i.c[1];
	d2s.c[2] = b2i.c[2];
	d2s.c[3] = b2i.c[3];
	d2s.c[4] = b2i.c[0] * area->x0 + b2i.c[2] * area->y0 + b2i.c[4];
	d2s.c[5] = b2i.c[1] * area->x0 + b2i.c[3] * area->y0 + b2i.c[5];

	if (buf->premul) {
		nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM (dpx, dw, dh, drs, spx, sw, sh, srs, &d2s, Falpha, XSAMPLE, YSAMPLE);
	} else {
		nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_TRANSFORM (dpx, dw, dh, drs, spx, sw, sh, srs, &d2s, Falpha, XSAMPLE, YSAMPLE);
	}

	buf->empty = FALSE;

	return item->state;
}

static NRArenaItem *
nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	NRArenaImage *image;
	art_u8 *p;
	gint ix, iy;
	guchar *pixels;
	gint width, height, rowstride;

	image = NR_ARENA_IMAGE (item);

	if (!image->pixbuf) return NULL;

	pixels = gdk_pixbuf_get_pixels (image->pixbuf);
	width = gdk_pixbuf_get_width (image->pixbuf);
	height = gdk_pixbuf_get_height (image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (image->pixbuf);
	ix = x * image->grid2px.c[0] + y * image->grid2px.c[2] + image->grid2px.c[4];
	iy = x * image->grid2px.c[1] + y * image->grid2px.c[3] + image->grid2px.c[5];

	if ((ix < 0) || (iy < 0) || (ix >= width) || (iy >= height)) return NULL;

	p = pixels + iy * rowstride + ix * 4;

	return (p[3] > 0) ? item : NULL;
}

/* Utility */

void
nr_arena_image_set_pixbuf (NRArenaImage *image, GdkPixbuf * pixbuf)
{
	g_return_if_fail (image != NULL);
	g_return_if_fail (NR_IS_ARENA_IMAGE (image));

	if (pixbuf != image->pixbuf) {
		gdk_pixbuf_ref (pixbuf);
		if (image->pixbuf) {
			gdk_pixbuf_unref (image->pixbuf);
			image->pixbuf = NULL;
		}
		image->pixbuf = pixbuf;
		nr_arena_item_request_update (NR_ARENA_ITEM (image), NR_ARENA_ITEM_STATE_ALL, FALSE);
	}
}

void
nr_arena_image_set_geometry (NRArenaImage *image, gdouble x, gdouble y, gdouble width, gdouble height)
{
	g_return_if_fail (image != NULL);
	g_return_if_fail (NR_IS_ARENA_IMAGE (image));

	image->x = x;
	image->y = y;
	image->width = width;
	image->height = height;

	nr_arena_item_request_update (NR_ARENA_ITEM (image), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

