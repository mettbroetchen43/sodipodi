#define SP_CANVAS_IMAGE_C

#include <config.h>
#include <math.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_uta_vpath.h>
#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "canvas-bgroup.h"
#include "canvas-image.h"

gint sp_canvas_image_x_sample = 1;
gint sp_canvas_image_y_sample = 1;

/* fixme: This should go to common header */
#define SP_CANVAS_STICKY_FLAG (1 << 16)

/*
 * SPCanvasCanvasImage
 *
 */

static void sp_canvas_image_class_init (SPCanvasImageClass * class);
static void sp_canvas_image_init (SPCanvasImage * canvas_image);
static void sp_canvas_image_destroy (GtkObject * object);

static void sp_canvas_image_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_canvas_image_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);
static double sp_canvas_image_point (GnomeCanvasItem *item, double x, double y,
			      int cx, int cy, GnomeCanvasItem **actual_item);

static GnomeCanvasItemClass * parent_class;

GtkType
sp_canvas_image_get_type (void)
{
	static GtkType canvas_image_type = 0;

	if (!canvas_image_type) {
		GtkTypeInfo canvas_image_info = {
			"SPCanvasImage",
			sizeof (SPCanvasImage),
			sizeof (SPCanvasImageClass),
			(GtkClassInitFunc) sp_canvas_image_class_init,
			(GtkObjectInitFunc) sp_canvas_image_init,
			NULL, NULL, NULL
		};

		canvas_image_type = gtk_type_unique (gnome_canvas_item_get_type (), &canvas_image_info);
	}

	return canvas_image_type;
}

static void
sp_canvas_image_class_init (SPCanvasImageClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	object_class->destroy = sp_canvas_image_destroy;

	item_class->update = sp_canvas_image_update;
	item_class->render = sp_canvas_image_render;
	item_class->point = sp_canvas_image_point;
}

static void
sp_canvas_image_init (SPCanvasImage * canvas_image)
{
	canvas_image->opacity = 1.0;
	canvas_image->pixbuf = NULL;
	canvas_image->vpath = NULL;
	canvas_image->sensitive = TRUE;
}

static void
sp_canvas_image_destroy (GtkObject *object)
{
	SPCanvasImage *canvas_image;

	canvas_image = (SPCanvasImage *) object;

	if (canvas_image->pixbuf) gdk_pixbuf_unref (canvas_image->pixbuf);

	if (canvas_image->vpath) art_free (canvas_image->vpath);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_image_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCanvasImage *canvas_image;
	ArtVpath vp[6], * vpath;
	ArtUta * uta;
	ArtDRect bbox;
	gint i;
	gint width, height;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->update)
		(* GNOME_CANVAS_ITEM_CLASS (parent_class)->update) (item, affine, clip_path, flags);

	for (i = 0; i < 6; i++) canvas_image->affine[i] = affine[i];

	if (canvas_image->vpath) {
		uta = art_uta_from_vpath (canvas_image->vpath);
		gnome_canvas_request_redraw_uta (item->canvas, uta);
		art_free (canvas_image->vpath);
		canvas_image->vpath = NULL;
	}

	if (canvas_image->pixbuf == NULL) return;

	width = gdk_pixbuf_get_width (canvas_image->pixbuf);
	height = gdk_pixbuf_get_height (canvas_image->pixbuf);

	vp[0].code = ART_MOVETO;
	vp[0].x = 0.0;
	vp[0].y = 0.0;
	vp[1].code = ART_LINETO;
	vp[1].x = 0.0;
	vp[1].y = height;
	vp[2].code = ART_LINETO;
	vp[2].x = width;
	vp[2].y = height;
	vp[3].code = ART_LINETO;
	vp[3].x = width;
	vp[3].y = 0.0;
	vp[4].code = ART_LINETO;
	vp[4].x = 0.0;
	vp[4].y = 0.0;
	vp[5].code = ART_END;
	vp[5].x = 0.0;
	vp[5].y = 0.0;

	vpath = art_vpath_affine_transform (vp, affine);
	canvas_image->vpath = vpath;

	uta = art_uta_from_vpath (vpath);
	gnome_canvas_request_redraw_uta (item->canvas, uta);

	art_vpath_bbox_drect (vpath, &bbox);
	item->x1 = MIN (bbox.x0, bbox.x1);
	item->y1 = MIN (bbox.y0, bbox.y1);
	item->x2 = MAX (bbox.x0, bbox.x1);
	item->y2 = MAX (bbox.y0, bbox.y1);
}

static double
sp_canvas_image_point (GnomeCanvasItem *item, double x, double y,
			  int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCanvasImage *canvas_image;
	art_u8 * p;
	gint ix, iy;
	guchar * pixels;
	gint width, height, rowstride;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (!canvas_image->sensitive && !(GTK_OBJECT_FLAGS (item->canvas) & SP_CANVAS_STICKY_FLAG)) return 1e18;

	if (canvas_image->pixbuf == NULL) return 1e18;

	pixels = gdk_pixbuf_get_pixels (canvas_image->pixbuf);
	width = gdk_pixbuf_get_width (canvas_image->pixbuf);
	height = gdk_pixbuf_get_height (canvas_image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (canvas_image->pixbuf);
	ix = (gint) x;
	iy = (gint) y;

	if ((ix < 0) || (iy < 0) || (ix >= width) || (iy >= height)) return 1e18;

	p = pixels + iy * rowstride + ix * 4;

	if (* (p + 3) < 16) return 1e18;

	*actual_item = item;

	return 0.0;
}

#define FBITS 12
#define XSAMPLE sp_canvas_image_x_sample
#define YSAMPLE sp_canvas_image_y_sample

static void
sp_canvas_image_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
#if 1
	SPCanvasImage *image;
	gdouble b2i[6];
	gint x, y, dw, dh, drs, dx0, dy0;
	gint srs, sw, sh;
	gdouble Fs_x_x, Fs_x_y, Fs_y_x, Fs_y_y, Fs__x, Fs__y;
	gint FFs_x_x, FFs_x_y;
	gint FFs_x_x_S, FFs_x_y_S, FFs_y_x_S, FFs_y_y_S;
	guchar *spx, *dpx;
	guint32 Falpha;

	image = SP_CANVAS_IMAGE (item);

	if (!image->pixbuf) return;

        gnome_canvas_buf_ensure_buf (buf);

	art_affine_invert (b2i, image->affine);

	Falpha = (guint32) floor (image->opacity * 255.9999);

	dpx = buf->buf;
	drs = buf->buf_rowstride;
	dw = buf->rect.x1 - buf->rect.x0;
	dh = buf->rect.y1 - buf->rect.y0;
	dx0 = buf->rect.x0;
	dy0 = buf->rect.y0;

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
		d = buf->buf + y * drs;
		Fsx = (y + dy0) * Fs_y_x + (0 + dx0) * Fs_x_x + Fs__x;
		Fsy = (y + dy0) * Fs_y_y + (0 + dx0) * Fs_x_y + Fs__y;
		s0 = spx + (gint) Fsy * srs + (gint) Fsx * 4;
		FFsx0 = (gint) (floor (Fsx) * (1 << FBITS) + 0.5);
		FFsy0 = (gint) (floor (Fsy) * (1 << FBITS) + 0.5);
		FFdsx = (gint) ((Fsx - floor (Fsx)) * (1 << FBITS) + 0.5);
		FFdsy = (gint) ((Fsy - floor (Fsy)) * (1 << FBITS) + 0.5);
		for (x = 0; x < dw; x++) {
			gint FFdsx0, FFdsy0;
			guint32 r, g, b, a;
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
				guint32 bg_r, bg_g, bg_b;
				guint32 Fc;
				a = (a >> (XSAMPLE + YSAMPLE)) * Falpha / 255;
				bg_r = d[0];
				bg_g = d[1];
				bg_b = d[2];
				Fc = ((r >> (XSAMPLE + YSAMPLE)) - bg_r) * a;
				d[0] = bg_r + ((Fc + (Fc >> 8) + 0x80) >> 8);
				Fc = ((g >> (XSAMPLE + YSAMPLE)) - bg_g) * a;
				d[1] = bg_g + ((Fc + (Fc >> 8) + 0x80) >> 8);
				Fc = ((b >> (XSAMPLE + YSAMPLE)) - bg_b) * a;
				d[2] = bg_b + ((Fc + (Fc >> 8) + 0x80) >> 8);
			}
			FFdsy = FFdsy0;
			FFdsx = FFdsx0;
			/* Advance pointers */
			FFdsx += FFs_x_x;
			FFdsy += FFs_x_y;
			/* Advance destination */
			d += 3;
		}
	}


	buf->is_bg = 0;
#else
	SPCanvasImage *image;
	gdouble b2i[6];
	gint x, y, dw, dh, drs, dx0, dy0;
	gint srs, sw, sh;
	gdouble Fs_x_x, Fs_x_y, Fs_y_x, Fs_y_y, Fs__x, Fs__y;
	gdouble Fs_x_x_2, Fs_x_y_2;
	gdouble Fsx, Fsy, Fsw, Fsh;
	guchar *spx, *dpx, *s, *d;
	guint32 Falpha;

	image = SP_CANVAS_IMAGE (item);

	if (!image->pixbuf) return;

        gnome_canvas_buf_ensure_buf (buf);

	art_affine_invert (b2i, image->affine);

	Falpha = (guint32) floor (image->opacity * 255.9999);

	dpx = buf->buf;
	drs = buf->buf_rowstride;
	dw = buf->rect.x1 - buf->rect.x0;
	dh = buf->rect.y1 - buf->rect.y0;
	dx0 = buf->rect.x0;
	dy0 = buf->rect.y0;

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

	Fs_x_x_2 = Fs_x_x * 0.5;
	Fs_x_y_2 = Fs_x_y * 0.5;

	Fsw = sw;
	Fsh = sh;

	for (y = 0; y < dh; y++) {
		d = buf->buf + y * drs;
		Fsx = (y + dy0) * Fs_y_x + (0 + dx0) * Fs_x_x + Fs__x;
		Fsy = (y + dy0) * Fs_y_y + (0 + dx0) * Fs_x_y + Fs__y;
		s = spx + (gint) Fsy * srs + (gint) Fsx * 4;
		for (x = 0; x < dw; x++) {
			guint32 r, g, b, a;
			gdouble Fsx2, Fsy2;
			/* Oversampling */
			r = g = b = a = 0x0;
			if ((Fsx >= 0) && (Fsx < Fsw) && (Fsy >= 0) && (Fsy < Fsh)) {
				r = s[0];
				g = s[1];
				b = s[2];
				a = s[3];
			}
			Fsx2 = Fsx + Fs_x_x_2;
			Fsy2 = Fsy;
			if ((Fsx2 >= 0) && (Fsx2 < Fsw) && (Fsy2 >= 0) && (Fsy2 < Fsh)) {
				s = spx + (gint) Fsy2 * srs + (gint) Fsx2 * 4;
				r += s[0];
				g += s[1];
				b += s[2];
				a += s[3];
			}
			Fsx2 = Fsx + Fs_x_x_2;
			Fsy2 = Fsy + Fs_x_y_2;
			if ((Fsx2 >= 0) && (Fsx2 < Fsw) && (Fsy2 >= 0) && (Fsy2 < Fsh)) {
				s = spx + (gint) Fsy2 * srs + (gint) Fsx2 * 4;
				r += s[0];
				g += s[1];
				b += s[2];
				a += s[3];
			}
			Fsx2 = Fsx;
			Fsy2 = Fsy + Fs_x_y_2;
			if ((Fsx2 >= 0) && (Fsx2 < Fsw) && (Fsy2 >= 0) && (Fsy2 < Fsh)) {
				s = spx + (gint) Fsy2 * srs + (gint) Fsx2 * 4;
				r += s[0];
				g += s[1];
				b += s[2];
				a += s[3];
			}
			/* Do real thing */
			if (a > 0) {
				guint32 Fc;
				guint32 bg_r, bg_g, bg_b;
				r = r >> 2;
				g = g >> 2;
				b = b >> 2;
				a = a >> 2;
				a = a * Falpha / 255;
				bg_r = d[0];
				bg_g = d[1];
				bg_b = d[2];
				Fc = (r - bg_r) * a;
				d[0] = bg_r + ((Fc + (Fc >> 8) + 0x80) >> 8);
				Fc = (g - bg_g) * a;
				d[1] = bg_g + ((Fc + (Fc >> 8) + 0x80) >> 8);
				Fc = (b - bg_b) * a;
				d[2] = bg_b + ((Fc + (Fc >> 8) + 0x80) >> 8);
			}
			/* Advance pointers */
			Fsx += Fs_x_x;
			Fsy += Fs_x_y;
			s = spx + (gint) Fsy * srs + (gint) Fsx * 4;
			/* Advance destination */
			d += 3;
		}
	}


	buf->is_bg = 0;
#endif
}

/* Utility */

void
sp_canvas_image_set_pixbuf (SPCanvasImage * image, GdkPixbuf * pixbuf, gdouble opacity)
{
	if (pixbuf != image->pixbuf) {
		gdk_pixbuf_ref (pixbuf);
		if (image->pixbuf) {
			gdk_pixbuf_unref (image->pixbuf);
			image->pixbuf = NULL;
		}
		image->pixbuf = pixbuf;
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (image));
	}
	if (opacity != image->opacity) {
		image->opacity = opacity;
		/* fixme: should request redraw here */
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (image));
	}
}

void
sp_canvas_image_set_sensitive (SPCanvasImage * image, gboolean sensitive)
{
	g_assert (SP_IS_CANVAS_IMAGE (image));

	image->sensitive = sensitive;
}
