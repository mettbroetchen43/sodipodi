#define SP_CANVAS_IMAGE_C

#include <config.h>

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libart_lgpl/art_rgb_pixbuf_affine.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_svp_point.h>
#include <gnome.h>

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
	canvas_image->svp = NULL;
}

static void
sp_canvas_image_destroy (GtkObject *object)
{
	SPCanvasImage *canvas_image;

	canvas_image = (SPCanvasImage *) object;

#if 0
	/* We should not free pixbuf here ;-) */
	if (canvas_image->pixbuf) art_pixbuf_free (canvas_image->pixbuf);
#endif
	if (canvas_image->svp) art_svp_free (canvas_image->svp);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_image_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCanvasImage *canvas_image;
	ArtVpath vpath[6];
	ArtVpath * avpath;
	gint i;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->update)
		(* GNOME_CANVAS_ITEM_CLASS (parent_class)->update) (item, affine, clip_path, flags);

	for (i = 0; i < 6; i++) canvas_image->affine[i] = affine[i];

	if (canvas_image->pixbuf == NULL)
		return;

	vpath[0].code = ART_MOVETO;
	vpath[0].x = 0.0;
	vpath[0].y = 0.0;
	vpath[1].code = ART_LINETO;
	vpath[1].x = 0.0;
	vpath[1].y = canvas_image->pixbuf->height;
	vpath[2].code = ART_LINETO;
	vpath[2].x = canvas_image->pixbuf->width;
	vpath[2].y = canvas_image->pixbuf->height;
	vpath[3].code = ART_LINETO;
	vpath[3].x = canvas_image->pixbuf->width;
	vpath[3].y = 0.0;
	vpath[4].code = ART_LINETO;
	vpath[4].x = 0.0;
	vpath[4].y = 0.0;
	vpath[5].code = ART_END;
	vpath[5].x = 0.0;
	vpath[5].y = 0.0;

	avpath = art_vpath_affine_transform (vpath, affine);
	gnome_canvas_item_update_svp_clip (item, &canvas_image->svp, art_svp_from_vpath (avpath), clip_path);
	art_free (avpath);
}

static double
sp_canvas_image_point (GnomeCanvasItem *item, double x, double y,
			  int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCanvasImage *canvas_image;
	double wind;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (canvas_image->pixbuf == NULL)
		return 1e18;

	*actual_item = item;

	wind = art_svp_point_wind (canvas_image->svp, cx, cy);

	if (wind) return 0.0;

	return art_svp_point_dist (canvas_image->svp, cx, cy);
}


static void
sp_canvas_image_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCanvasImage *canvas_image;

	canvas_image = SP_CANVAS_IMAGE (item);

	if (canvas_image->pixbuf == NULL)
		return;

        gnome_canvas_buf_ensure_buf (buf);

	art_rgb_pixbuf_affine (buf->buf,
			buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
			buf->buf_rowstride,
			canvas_image->pixbuf,
			canvas_image->affine,
			ART_FILTER_NEAREST, NULL);

	buf->is_bg = 0;
}

/* Utility */

void
sp_canvas_image_set_pixbuf (SPCanvasImage * image, ArtPixBuf * pixbuf)
{
	image->pixbuf = pixbuf;
	gnome_canvas_item_request_update ((GnomeCanvasItem *) image);
}

