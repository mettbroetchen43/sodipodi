#define SP_IMAGE_C

#include <gnome.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include "helper/art-rgba-rgba-affine.h"
#include "display/canvas-image.h"
#include "brokenimage.xpm"
#include "sp-image.h"

/*
 * SPImage
 */

static void sp_image_class_init (SPImageClass * class);
static void sp_image_init (SPImage * image);
static void sp_image_destroy (GtkObject * object);

static void sp_image_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_image_read_attr (SPObject * object, const gchar * key);

static void sp_image_update (SPItem * item, gdouble affine[]);
static void sp_image_bbox (SPItem * item, ArtDRect * bbox);
static void sp_image_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_image_description (SPItem * item);
static GnomeCanvasItem * sp_image_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static gboolean sp_image_paint (SPItem * item, ArtPixBuf * pixbuf, gdouble * affine);

GdkPixbuf * sp_image_repr_read_image (SPRepr * repr);
static GdkPixbuf * sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf);

static SPItemClass *parent_class;

GtkType
sp_image_get_type (void)
{
	static GtkType image_type = 0;

	if (!image_type) {
		GtkTypeInfo image_info = {
			"SPImage",
			sizeof (SPImage),
			sizeof (SPImageClass),
			(GtkClassInitFunc) sp_image_class_init,
			(GtkObjectInitFunc) sp_image_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		image_type = gtk_type_unique (sp_item_get_type (), &image_info);
	}

	return image_type;
}

static void
sp_image_class_init (SPImageClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (sp_item_get_type ());

	gtk_object_class->destroy = sp_image_destroy;

	sp_object_class->build = sp_image_build;
	sp_object_class->read_attr = sp_image_read_attr;

	item_class->update = sp_image_update;
	item_class->bbox = sp_image_bbox;
	item_class->print = sp_image_print;
	item_class->description = sp_image_description;
	item_class->show = sp_image_show;
	item_class->paint = sp_image_paint;
}

static void
sp_image_init (SPImage *image)
{
	image->pixbuf = NULL;
}

static void
sp_image_destroy (GtkObject *object)
{
	SPImage *image;

	image = SP_IMAGE (object);

	if (image->pixbuf) gdk_pixbuf_unref (image->pixbuf);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_image_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (SP_OBJECT_CLASS (parent_class)->build)
		SP_OBJECT_CLASS (parent_class)->build (object, document, repr);

	sp_image_read_attr (object, "src");
}

static void
sp_image_read_attr (SPObject * object, const gchar * key)
{
	SPImage * image;
	GdkPixbuf * pixbuf;

	image = SP_IMAGE (object);

	pixbuf = NULL;

	if (strcmp (key, "src") == 0) {
		pixbuf = sp_image_repr_read_image (object->repr);
		pixbuf = sp_image_pixbuf_force_rgba (pixbuf);
		g_return_if_fail (pixbuf != NULL);

		if (image->pixbuf != NULL)
			gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = pixbuf;

		sp_item_request_canvas_update (SP_ITEM (image));
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, key);

}

static void
sp_image_update (SPItem * item, gdouble * affine)
{
	if (SP_ITEM_CLASS (parent_class)->update)
		(* SP_ITEM_CLASS (parent_class)->update) (item, affine);
}

static void
sp_image_bbox (SPItem * item, ArtDRect * bbox)
{
	SPImage * image;
	double a[6];
	ArtPoint p;

	image = SP_IMAGE (item);

	sp_item_i2d_affine (item, a);

	p.x = 0.0;
	p.y = 0.0;
	art_affine_point (&p, &p, a);
	bbox->x0 = bbox->x1 = p.x;
	bbox->y0 = bbox->y1 = p.y;

	if (image->pixbuf == NULL) return;

	p.x = 0.0;
	p.y = image->pixbuf->art_pixbuf->height;
	art_affine_point (&p, &p, a);
	bbox->x0 = MIN (bbox->x0, p.x);
	bbox->y0 = MIN (bbox->y0, p.y);
	bbox->x1 = MAX (bbox->x1, p.x);
	bbox->y1 = MAX (bbox->y1, p.y);

	p.x = image->pixbuf->art_pixbuf->width;
	p.y = image->pixbuf->art_pixbuf->height;
	art_affine_point (&p, &p, a);
	bbox->x0 = MIN (bbox->x0, p.x);
	bbox->y0 = MIN (bbox->y0, p.y);
	bbox->x1 = MAX (bbox->x1, p.x);
	bbox->y1 = MAX (bbox->y1, p.y);

	p.x = image->pixbuf->art_pixbuf->width;
	p.y = 0.0;
	art_affine_point (&p, &p, a);
	bbox->x0 = MIN (bbox->x0, p.x);
	bbox->y0 = MIN (bbox->y0, p.y);
	bbox->x1 = MAX (bbox->x1, p.x);
	bbox->y1 = MAX (bbox->y1, p.y);
}

static void sp_image_print (SPItem * item, GnomePrintContext * gpc)
{
	SPImage * image;
	ArtPixBuf * abp;
	double affine[6];
	art_u8 * buf, * tbuf, * sptr, * dptr;
	gint x, y;
	gboolean has_alpha;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL) return;

	abp = image->pixbuf->art_pixbuf;
	tbuf = NULL;	/* Keep gcc happy ;-) */

	gnome_print_gsave (gpc);

	art_affine_scale (affine, abp->width, -abp->height);
	gnome_print_concat (gpc, affine);
	art_affine_translate (affine, 0.0, -1.0);
	gnome_print_concat (gpc, affine);

	has_alpha = FALSE;

	for (y = 0; y < abp->height; y++) {
		sptr = abp->pixels + y * abp->rowstride + 3;
		for (x = 0; x < abp->width; x++) {
			if (*sptr != 255) {has_alpha = TRUE; break;}
			sptr += 3;
		}
		if (has_alpha) break;
	}

	if (has_alpha) {
		gdouble a[6], aa[6];

		/* Chemistry ;-( */
		sp_item_i2doc_affine (item, a);
		art_affine_invert (aa, a);

		tbuf = art_new (art_u8, abp->width * abp->height * 4);
		memset (tbuf, 255, abp->width * abp->height * 4);
		abp = art_pixbuf_new_rgba (tbuf, abp->width, abp->height, abp->width * 4);

		item->stop_paint = TRUE;
		sp_item_paint (SP_ITEM (SP_OBJECT (item)->document->root), abp, aa);
		item->stop_paint = FALSE;

		art_affine_identity (a);
		art_rgba_rgba_affine (abp->pixels,
			0, 0, abp->width, abp->height, abp->rowstride,
			image->pixbuf->art_pixbuf->pixels, abp->width, abp->height,
			image->pixbuf->art_pixbuf->rowstride,
			a,
			ART_FILTER_NEAREST, NULL);
	}

	buf = g_new (art_u8, abp->width * abp->height * 3);

	for (y = 0; y < abp->height; y++) {
		sptr = abp->pixels + y * abp->rowstride;
		dptr = buf + y * abp->width * 3;
		for (x = 0; x < abp->width; x++) {
			*dptr++ = *sptr++;
			*dptr++ = *sptr++;
			*dptr++ = *sptr++;
			sptr++;
		}
	}

	gnome_print_rgbimage (gpc, buf, abp->width, abp->height, abp->width * 3);

	g_free (buf);

	if (has_alpha) {
		art_pixbuf_free (abp);
	}

	gnome_print_grestore (gpc);
}

static gchar *
sp_image_description (SPItem * item)
{
	SPImage * image;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL)
		return g_strdup (_("Broken bitmap"));
	return g_strdup (_("Color bitmap"));
}

static GnomeCanvasItem *
sp_image_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	SPImage * image;
	SPCanvasImage * ci;

	image = (SPImage *) item;

	ci = (SPCanvasImage *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_IMAGE, NULL);
	g_return_val_if_fail (ci != NULL, NULL);
	sp_canvas_image_set_pixbuf (ci, image->pixbuf->art_pixbuf);

	return (GnomeCanvasItem *) ci;
}

static gboolean
sp_image_paint (SPItem * item, ArtPixBuf * pixbuf, gdouble * affine)
{
	SPImage * image;
	ArtPixBuf * ipb;

	image = SP_IMAGE (item);
	ipb = image->pixbuf->art_pixbuf;

	if (ipb->n_channels != 4) return FALSE;

	if (pixbuf->n_channels == 3) {
		art_rgb_rgba_affine (pixbuf->pixels,
			0, 0, pixbuf->width, pixbuf->height, pixbuf->rowstride,
			ipb->pixels, ipb->width, ipb->height, ipb->rowstride,
			affine,
			ART_FILTER_NEAREST, NULL);
	} else {
		art_rgba_rgba_affine (pixbuf->pixels,
			0, 0, pixbuf->width, pixbuf->height, pixbuf->rowstride,
			ipb->pixels, ipb->width, ipb->height, ipb->rowstride,
			affine,
			ART_FILTER_NEAREST, NULL);
	}

	return FALSE;
}

/*
 * utility function to try loading image from src
 *
 * docbase/relative_src
 * absolute_src
 *
 */

GdkPixbuf *
sp_image_repr_read_image (SPRepr * repr)
{
	const gchar * filename, * docbase;
	gchar * fullname;
	GdkPixbuf * pixbuf;

	filename = sp_repr_attr (repr, "src");

	if (filename != NULL) {
		if (!g_path_is_absolute (filename)) {
			/* try to load from relative pos */
			docbase = sp_repr_doc_attr (repr, "docbase");
			if (docbase != NULL) {
				fullname = g_strconcat (docbase, filename, NULL);
				pixbuf = gdk_pixbuf_new_from_file (fullname);
				g_free (fullname);
				if (pixbuf != NULL) return pixbuf;
			}
		} else {
			/* try absolute filename */
			pixbuf = gdk_pixbuf_new_from_file (filename);
			if (pixbuf != NULL) return pixbuf;
		}
	}
	/* at last try to load from sp absolute path name */
	filename = sp_repr_attr (repr, "absref");
	if (filename != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (filename);
		if (pixbuf != NULL) return pixbuf;
	}
	/* Nope: We do not find any valid pixmap file :-( */
	pixbuf = gdk_pixbuf_new_from_xpm_data ((const gchar **) brokenimage_xpm);

	/* It should be included xpm, so if it still does not does load, */
	/* our libraries are broken */
	g_assert (pixbuf != NULL);

	return pixbuf;
}

static GdkPixbuf *
sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf)
{
	GdkPixbuf * newbuf;
	gint x, y;
	art_u8 * s, * d;

	if (pixbuf->art_pixbuf->n_channels == 4) return pixbuf;
	g_return_val_if_fail (pixbuf->art_pixbuf->n_channels == 3, NULL);

	newbuf = gdk_pixbuf_new (ART_PIX_RGB, TRUE, 8,
		pixbuf->art_pixbuf->width, pixbuf->art_pixbuf->height);

	for (y = 0; y < pixbuf->art_pixbuf->height; y++) {
		s = pixbuf->art_pixbuf->pixels + y * pixbuf->art_pixbuf->rowstride;
		d = newbuf->art_pixbuf->pixels + y * newbuf->art_pixbuf->rowstride;
		for (x = 0; x < pixbuf->art_pixbuf->width; x++) {
			* d++ = * s++;
			* d++ = * s++;
			* d++ = * s++;
			* d++ = 255;
		}
	}

	gdk_pixbuf_unref (pixbuf);

	return newbuf;
}

