#define __NR_ARENA_IMAGE_C__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <math.h>
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
static void nr_arena_image_destroy (GtkObject *object);

static guint nr_arena_image_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset);
static guint nr_arena_image_render (NRArenaItem *item, NRIRect *area, NRBuffer *b);
static NRArenaItem *nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky);

static NRArenaItemClass *parent_class;

GtkType
nr_arena_image_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"NRArenaImage",
			sizeof (NRArenaImage),
			sizeof (NRArenaImageClass),
			(GtkClassInitFunc) nr_arena_image_class_init,
			(GtkObjectInitFunc) nr_arena_image_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (NR_TYPE_ARENA_ITEM, &info);
	}
	return type;
}

static void
nr_arena_image_class_init (NRArenaImageClass *klass)
{
	GtkObjectClass *object_class;
	NRArenaItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRArenaItemClass *) klass;

	parent_class = gtk_type_class (NR_TYPE_ARENA_IMAGE);

	object_class->destroy = nr_arena_image_destroy;

	item_class->update = nr_arena_image_update;
	item_class->render = nr_arena_image_render;
	item_class->pick = nr_arena_image_pick;
}

static void
nr_arena_image_init (NRArenaImage *image)
{
	image->pixbuf = NULL;
	/* fixme: affine */
}

static void
nr_arena_image_destroy (GtkObject *object)
{
	NRArenaImage *image;

	image = NR_ARENA_IMAGE (object);

	if (image->pixbuf) {
		gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static guint
nr_arena_image_update (NRArenaItem *item, NRIRect *area, NRGC *gc, guint state, guint reset)
{
	NRArenaImage *image;

	image = NR_ARENA_IMAGE (item);

	/* Copy affine */
	art_affine_invert (image->grid2item, gc->affine);

	/* Calculate bbox */
	if (image->pixbuf) {
		ArtDRect bbox;
		ArtPoint p;
		gint w, h;

		w = gdk_pixbuf_get_width (image->pixbuf);
		h = gdk_pixbuf_get_height (image->pixbuf);

		p.x = 0.0;
		p.y = 0.0;
		art_affine_point (&p, &p, gc->affine);
		bbox.x0 = p.x;
		bbox.y0 = p.y;
		bbox.x1 = p.x;
		bbox.y1 = p.y;
		p.x = w;
		p.y = 0.0;
		art_affine_point (&p, &p, gc->affine);
		bbox.x0 = MIN (bbox.x0, p.x);
		bbox.y0 = MIN (bbox.y0, p.y);
		bbox.x1 = MAX (bbox.x1, p.x);
		bbox.y1 = MAX (bbox.y1, p.y);
		p.x = w;
		p.y = h;
		art_affine_point (&p, &p, gc->affine);
		bbox.x0 = MIN (bbox.x0, p.x);
		bbox.y0 = MIN (bbox.y0, p.y);
		bbox.x1 = MAX (bbox.x1, p.x);
		bbox.y1 = MAX (bbox.y1, p.y);
		p.x = 0.0;
		p.y = h;
		art_affine_point (&p, &p, gc->affine);
		bbox.x0 = MIN (bbox.x0, p.x);
		bbox.y0 = MIN (bbox.y0, p.y);
		bbox.x1 = MAX (bbox.x1, p.x);
		bbox.y1 = MAX (bbox.y1, p.y);

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
#define b2i (image->grid2item)

static guint
nr_arena_image_render (NRArenaItem *item, NRIRect *area, NRBuffer *b)
{
	NRArenaImage *image;
	gint x, y, dw, dh, drs, dx0, dy0;
	gint srs, sw, sh;
	gdouble Fs_x_x, Fs_x_y, Fs_y_x, Fs_y_y, Fs__x, Fs__y;
	gint FFs_x_x, FFs_x_y;
	gint FFs_x_x_S, FFs_x_y_S, FFs_y_x_S, FFs_y_y_S;
	guchar *spx, *dpx;
	guint32 Falpha;

	image = NR_ARENA_IMAGE (item);

	if (!image->pixbuf) return item->state;

	Falpha = (guint32) floor (item->opacity * 255.9999);

	dpx = b->px;
	drs = b->rs;
	dw = area->x1 - area->x0;
	dh = area->y1 - area->y0;
	dx0 = area->x0;
	dy0 = area->y0;

	spx = gdk_pixbuf_get_pixels (image->pixbuf);
	srs = gdk_pixbuf_get_rowstride (image->pixbuf);
	/* Width in bytes */
	sw = gdk_pixbuf_get_width (image->pixbuf);
	sh = gdk_pixbuf_get_height (image->pixbuf);

	Fs_x_x = b2i[0];
	Fs_x_y = b2i[1];
	Fs_y_x = b2i[2];
	Fs_y_y = b2i[3];
	Fs__x = b2i[4];
	Fs__y = b2i[5];

	FFs_x_x = (gint) floor (Fs_x_x * (1 << FBITS) + 0.5);
	FFs_x_y = (gint) floor (Fs_x_y * (1 << FBITS) + 0.5);

	FFs_x_x_S = (gint) floor (Fs_x_x * (1 << FBITS) + 0.5) >> XSAMPLE;
	FFs_x_y_S = (gint) floor (Fs_x_y * (1 << FBITS) + 0.5) >> XSAMPLE;
	FFs_y_x_S = (gint) floor (Fs_y_x * (1 << FBITS) + 0.5) >> YSAMPLE;
	FFs_y_y_S = (gint) floor (Fs_y_y * (1 << FBITS) + 0.5) >> YSAMPLE;

	for (y = 0; y < dh; y++) {
		guchar *s0, *s, *d;
		gdouble Fsx, Fsy;
		gint FFsx0, FFsy0, FFdsx, FFdsy, sx, sy;
		d = b->px + y * drs;
		Fsx = (y + dy0) * Fs_y_x + (0 + dx0) * Fs_x_x + Fs__x;
		Fsy = (y + dy0) * Fs_y_y + (0 + dx0) * Fs_x_y + Fs__y;
		s0 = spx + (gint) Fsy * srs + (gint) Fsx * 4;
		FFsx0 = (gint) (floor (Fsx) * (1 << FBITS) + 0.5);
		FFsy0 = (gint) (floor (Fsy) * (1 << FBITS) + 0.5);
		FFdsx = (gint) ((Fsx - floor (Fsx)) * (1 << FBITS) + 0.5);
		FFdsy = (gint) ((Fsy - floor (Fsy)) * (1 << FBITS) + 0.5);
		for (x = 0; x < dw; x++) {
			gint FFdsx0, FFdsy0;
			gint r, g, b, a;
			gint xx, yy;
			FFdsy0 = FFdsy;
			FFdsx0 = FFdsx;
			r = g = b = a = 0;
			for (yy = 0; yy < (1 << YSAMPLE); yy++) {
				FFdsy = FFdsy0 + yy * (FFs_y_y_S);
				FFdsx = FFdsx0 + yy * (FFs_y_x_S);
				for (xx = 0; xx < (1 << XSAMPLE); xx++) {
					sx = (FFsx0 + FFdsx) >> FBITS;
					sy = (FFsy0 + FFdsy) >> FBITS;
					if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh)) {
						s = spx + sy * srs + sx * 4;
						r += s[0];
						g += s[1];
						b += s[2];
						a += s[3];
					}
					FFdsx += FFs_x_x_S;
					FFdsy += FFs_x_y_S;
				}
			}
			if (a > 0) {
				/* fixme: do it right, plus premul stuff */
				r = r >> (XSAMPLE + YSAMPLE);
				g = g >> (XSAMPLE + YSAMPLE);
				b = b >> (XSAMPLE + YSAMPLE);
				a = a * Falpha >> (XSAMPLE + YSAMPLE);
				d[0] = d[0] + (r - d[0]) * a / 65025;
				d[1] = d[1] + (g - d[1]) * a / 65025;
				d[2] = d[2] + (b - d[2]) * a / 65025;
				d[3] = d[3] + (0xff - d[3]) * a / 65025;
			}
			FFdsy = FFdsy0;
			FFdsx = FFdsx0;
			/* Advance pointers */
			FFdsx += FFs_x_x;
			FFdsy += FFs_x_y;
			/* Advance destination */
			d += 4;
		}
	}

	return item->state;
}

static NRArenaItem *
nr_arena_image_pick (NRArenaItem *item, gdouble x, gdouble y, gboolean sticky)
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
	ix = x * image->grid2item[0] + y * image->grid2item[2] + image->grid2item[4];
	iy = x * image->grid2item[1] + y * image->grid2item[3] + image->grid2item[5];

	if ((ix < 0) || (iy < 0) || (ix >= width) || (iy >= height)) return NULL;

	p = pixels + iy * rowstride + ix * 4;

	return (p[3] > 0) ? item : NULL;
}

/* Utility */

void
nr_arena_image_set_pixbuf (NRArenaImage *image, GdkPixbuf * pixbuf)
{
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
nr_arena_image_set_sensitive (NRArenaImage *image, gboolean sensitive)
{
#if 0
	g_assert (NR_IS_ARENA_IMAGE (image));

	image->sensitive = sensitive;
#endif
}
