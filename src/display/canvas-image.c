#define SP_CANVAS_IMAGE_C

#include <config.h>

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_uta_vpath.h>
#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "canvas-image.h"

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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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
	canvas_image->pixbuf = NULL;
	canvas_image->vpath = NULL;
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


static void
sp_canvas_image_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCanvasImage *canvas_image;
	guchar * pixels;
	gint width, height, rowstride;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (canvas_image->pixbuf == NULL) return;

	pixels = gdk_pixbuf_get_pixels (canvas_image->pixbuf);
	width = gdk_pixbuf_get_width (canvas_image->pixbuf);
	height = gdk_pixbuf_get_height (canvas_image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (canvas_image->pixbuf);

        gnome_canvas_buf_ensure_buf (buf);

	art_rgb_rgba_affine (buf->buf,
			buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
			buf->buf_rowstride,
			pixels,
			width, height, rowstride,
			canvas_image->affine,
			ART_FILTER_NEAREST, NULL);

	buf->is_bg = 0;
}

/* Utility */

void
sp_canvas_image_set_pixbuf (SPCanvasImage * image, GdkPixbuf * pixbuf)
{

	gdk_pixbuf_ref (pixbuf);
	if (image->pixbuf) gdk_pixbuf_unref (image->pixbuf);

	image->pixbuf = pixbuf;

	gnome_canvas_item_request_update ((GnomeCanvasItem *) image);
}

