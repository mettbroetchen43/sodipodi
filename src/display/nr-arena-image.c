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
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-compose-transform.h>
#include "nr-arena-image.h"

gint nr_arena_image_x_sample = 1;
gint nr_arena_image_y_sample = 1;

/*
 * NRArenaCanvasImage
 *
 */

static void nr_arena_image_class_init (NRArenaImageClass *klass);
static void nr_arena_image_init (NRArenaImage *image);
static void nr_arena_image_dispose (GObject *object);

static guint nr_arena_image_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static unsigned int nr_arena_image_render (NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags);
static NRArenaItem *nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

static NRArenaItemClass *parent_class;

GType
nr_arena_image_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (NRArenaImageClass),
			NULL, NULL,
			(GClassInitFunc) nr_arena_image_class_init,
			NULL, NULL,
			sizeof (NRArenaImage),
			16,
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
	image->width = 256.0;
	image->height = 256.0;

	nr_matrix_f_set_identity (&image->grid2px);
}

static void
nr_arena_image_dispose (GObject *object)
{
	NRArenaImage *image;

	image = NR_ARENA_IMAGE (object);

	image->px = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static guint
nr_arena_image_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
{
	NRArenaImage *image;
	gdouble hscale, vscale;
	NRMatrixD grid2px;

	image = NR_ARENA_IMAGE (item);

	/* Request render old */
	nr_arena_item_request_render (item);

	/* Copy affine */
	nr_matrix_d_invert (&grid2px, &gc->transform);
	if (image->px) {
		hscale = image->pxw / image->width;
		vscale = image->pxh / image->height;
	} else {
		hscale = 1.0;
		vscale = 1.0;
	}

	image->grid2px.c[0] = grid2px.c[0] * hscale;
	image->grid2px.c[2] = grid2px.c[2] * hscale;
	image->grid2px.c[4] = grid2px.c[4] * hscale;
	image->grid2px.c[1] = grid2px.c[1] * vscale;
	image->grid2px.c[3] = grid2px.c[3] * vscale;
	image->grid2px.c[5] = grid2px.c[5] * vscale;

	image->grid2px.c[4] -= image->x * hscale;
	image->grid2px.c[5] -= image->y * vscale;

	/* Calculate bbox */
	if (image->px) {
		NRRectD bbox;

		bbox.x0 = image->x;
		bbox.y0 = image->y;
		bbox.x1 = image->x + image->width;
		bbox.y1 = image->y + image->height;
		nr_rect_d_matrix_d_transform (&bbox, &bbox, &gc->transform);

		item->bbox.x0 = (gint) floor (bbox.x0);
		item->bbox.y0 = (gint) floor (bbox.y0);
		item->bbox.x1 = (gint) ceil (bbox.x1);
		item->bbox.y1 = (gint) ceil (bbox.y1);
	} else {
		item->bbox.x0 = (gint) gc->transform.c[4];
		item->bbox.y0 = (gint) gc->transform.c[5];
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

static unsigned int
nr_arena_image_render (NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags)
{
	NRArenaImage *image;
	guint32 Falpha;
	guchar *spx, *dpx;
	gint dw, dh, drs, sw, sh, srs;
	NRMatrixF d2s;

	image = NR_ARENA_IMAGE (item);

	if (!image->px) return item->state;

	Falpha = (guint32) floor (item->opacity * 255.9999);
	if (Falpha < 1) return item->state;

	dpx = NR_PIXBLOCK_PX (pb);
	drs = pb->rs;
	dw = pb->area.x1 - pb->area.x0;
	dh = pb->area.y1 - pb->area.y0;

	spx = image->px;
	srs = image->pxrs;
	sw = image->pxw;
	sh = image->pxh;

	d2s.c[0] = b2i.c[0];
	d2s.c[1] = b2i.c[1];
	d2s.c[2] = b2i.c[2];
	d2s.c[3] = b2i.c[3];
	d2s.c[4] = b2i.c[0] * pb->area.x0 + b2i.c[2] * pb->area.y0 + b2i.c[4];
	d2s.c[5] = b2i.c[1] * pb->area.x0 + b2i.c[3] * pb->area.y0 + b2i.c[5];

	if (pb->mode == NR_PIXBLOCK_MODE_R8G8B8A8P) {
		nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM (dpx, dw, dh, drs, spx, sw, sh, srs, &d2s, Falpha, XSAMPLE, YSAMPLE);
	} else if (pb->mode == NR_PIXBLOCK_MODE_R8G8B8A8N) {
		nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_TRANSFORM (dpx, dw, dh, drs, spx, sw, sh, srs, &d2s, Falpha, XSAMPLE, YSAMPLE);
	}

	pb->empty = FALSE;

	return item->state;
}

static NRArenaItem *
nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky)
{
	NRArenaImage *image;
	unsigned char *p;
	gint ix, iy;
	guchar *pixels;
	gint width, height, rowstride;

	image = NR_ARENA_IMAGE (item);

	if (!image->px) return NULL;

	pixels = image->px;
	width = image->pxw;
	height = image->pxh;
	rowstride = image->pxrs;
	ix = x * image->grid2px.c[0] + y * image->grid2px.c[2] + image->grid2px.c[4];
	iy = x * image->grid2px.c[1] + y * image->grid2px.c[3] + image->grid2px.c[5];

	if ((ix < 0) || (iy < 0) || (ix >= width) || (iy >= height)) return NULL;

	p = pixels + iy * rowstride + ix * 4;

	return (p[3] > 0) ? item : NULL;
}

/* Utility */

void
nr_arena_image_set_pixels (NRArenaImage *image, const unsigned char *px, unsigned int pxw, unsigned int pxh, unsigned int pxrs)
{
	g_return_if_fail (image != NULL);
	g_return_if_fail (NR_IS_ARENA_IMAGE (image));

	image->px = (unsigned char *) px;
	image->pxw = pxw;
	image->pxh = pxh;
	image->pxrs = pxrs;

	nr_arena_item_request_update (NR_ARENA_ITEM (image), NR_ARENA_ITEM_STATE_ALL, FALSE);
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

