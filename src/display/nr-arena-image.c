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

	parent_class = gtk_type_class (NR_TYPE_ARENA_ITEM);

	object_class->destroy = nr_arena_image_destroy;

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

	art_affine_identity (image->grid2px);
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
	gdouble hscale, vscale;

	image = NR_ARENA_IMAGE (item);

	/* Copy affine */
	art_affine_invert (image->grid2px, gc->affine);
	if (image->pixbuf) {
		hscale = (fabs (image->width > 1e-9)) ? gdk_pixbuf_get_width (image->pixbuf) / image->width : 1e9;
		vscale = (fabs (image->height > 1e-9)) ? gdk_pixbuf_get_height (image->pixbuf) / image->height : 1e9;
	} else {
		hscale = 1e9;
		vscale = 1e9;
	}

	image->grid2px[0] *= hscale;
	image->grid2px[2] *= hscale;
	image->grid2px[4] *= hscale;
	image->grid2px[1] *= vscale;
	image->grid2px[3] *= vscale;
	image->grid2px[5] *= vscale;

	image->grid2px[4] -= image->x * hscale;
	image->grid2px[5] -= image->y * vscale;

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

#define PREMUL(c,a) (((c) * (a) + 127) / 255)
#define COMPOSENNN_A7(fc,fa,bc,ba,a) (((255 - (fa)) * (bc) * (ba) + (fa) * (fc) * 255 + 127) / a)
#define COMPOSEPNN_A7(fc,fa,bc,ba,a) (((255 - (fa)) * (bc) * (ba) + (fc) * 65025 + 127) / a)
#define COMPOSENNP(fc,fa,bc,ba) (((255 - (fa)) * (bc) * (ba) + (fa) * (fc) * 255 + 32512) / 65025)
#define COMPOSEPNP(fc,fa,bc,ba) (((255 - (fa)) * (bc) * (ba) + (fc) * 65025 + 32512) / 65025)
#define COMPOSENPP(fc,fa,bc,ba) (((255 - (fa)) * (bc) + (fa) * (fc) + 127) / 255)
#define COMPOSEPPP(fc,fa,bc,ba) (((255 - (fa)) * (bc) + (fc) * 255 + 127) / 255)
#define COMPOSEP11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * 255 + 127) / 255)
#define COMPOSEN11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * (fa) + 127) / 255)

static guint
nr_arena_image_render (NRArenaItem *item, NRIRect *area, NRBuffer *buf)
{
	NRArenaImage *image;
	guint xnsample, ynsample, dsample;
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
	if (Falpha < 1) return item->state;

	xnsample = 1 << XSAMPLE;
	ynsample = 1 << YSAMPLE;
	dsample = XSAMPLE + YSAMPLE;

	dpx = buf->px;
	drs = buf->rs;
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
		d = buf->px + y * drs;
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
			for (yy = 0; yy < ynsample; yy++) {
				FFdsy = FFdsy0 + yy * (FFs_y_y_S);
				FFdsx = FFdsx0 + yy * (FFs_y_x_S);
				for (xx = 0; xx < xnsample; xx++) {
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
			if (a > dsample) {
				/* fixme: do it right, plus premul stuff */
				r = r >> dsample;
				g = g >> dsample;
				b = b >> dsample;
				a = ((a * Falpha >> dsample) + 127) / 255;
				if (a > 0) {
					if (buf->premul) {
						if ((a == 255) || (d[3] == 0)) {
							/* Transparent BG, premul src */
							d[0] = PREMUL (r, a);
							d[1] = PREMUL (g, a);
							d[2] = PREMUL (b, a);
							d[3] = a;
						} else {
							d[0] = COMPOSENPP (r, a, d[0], d[3]);
							d[1] = COMPOSENPP (g, a, d[1], d[3]);
							d[2] = COMPOSENPP (b, a, d[2], d[3]);
							d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
						}
					} else {
						if ((a == 255) || (d[3] == 0)) {
							/* Full coverage, COPY */
							d[0] = r;
							d[1] = g;
							d[2] = b;
							d[3] = a;
						} else {
							guint ca;
							/* Full composition */
							ca = 65025 - (255 - a) * (255 - d[3]);
							d[0] = COMPOSENNN_A7 (r, a, d[0], d[3], ca);
							d[1] = COMPOSENNN_A7 (g, a, d[1], d[3], ca);
							d[2] = COMPOSENNN_A7 (b, a, d[2], d[3], ca);
							d[3] = (ca + 127) / 255;
						}
					}
				}
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
	ix = x * image->grid2px[0] + y * image->grid2px[2] + image->grid2px[4];
	iy = x * image->grid2px[1] + y * image->grid2px[3] + image->grid2px[5];

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

