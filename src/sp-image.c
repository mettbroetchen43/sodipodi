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

	sp_image_read_attr (object, "xlink:href");
}

static void
sp_image_read_attr (SPObject * object, const gchar * key)
{
	SPImage * image;
	GdkPixbuf * pixbuf;

	image = SP_IMAGE (object);

	pixbuf = NULL;

	if (strcmp (key, "xlink:href") == 0) {
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
	p.y = gdk_pixbuf_get_height (image->pixbuf);
	art_affine_point (&p, &p, a);
	bbox->x0 = MIN (bbox->x0, p.x);
	bbox->y0 = MIN (bbox->y0, p.y);
	bbox->x1 = MAX (bbox->x1, p.x);
	bbox->y1 = MAX (bbox->y1, p.y);

	p.x = gdk_pixbuf_get_width (image->pixbuf);
	p.y = gdk_pixbuf_get_height (image->pixbuf);
	art_affine_point (&p, &p, a);
	bbox->x0 = MIN (bbox->x0, p.x);
	bbox->y0 = MIN (bbox->y0, p.y);
	bbox->x1 = MAX (bbox->x1, p.x);
	bbox->y1 = MAX (bbox->y1, p.y);

	p.x = gdk_pixbuf_get_width (image->pixbuf);
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
	double affine[6];
	guchar * pixels;
	gint width, height, rowstride;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL) return;

	pixels = gdk_pixbuf_get_pixels (image->pixbuf);
	width = gdk_pixbuf_get_width (image->pixbuf);
	height = gdk_pixbuf_get_height (image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (image->pixbuf);

	gnome_print_gsave (gpc);

	art_affine_scale (affine, width, -height);
	gnome_print_concat (gpc, affine);
	art_affine_translate (affine, 0.0, -1.0);
	gnome_print_concat (gpc, affine);

	gnome_print_rgbaimage (gpc, pixels, width, height, rowstride);

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

	sp_canvas_image_set_pixbuf (ci, image->pixbuf);

	return (GnomeCanvasItem *) ci;
}

static gboolean
sp_image_paint (SPItem * item, ArtPixBuf * pixbuf, gdouble * affine)
{
	SPImage * image;
	guchar * pixels;
	gint width, height, rowstride;

	image = SP_IMAGE (item);

	pixels = gdk_pixbuf_get_pixels (image->pixbuf);
	width = gdk_pixbuf_get_width (image->pixbuf);
	height = gdk_pixbuf_get_height (image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (image->pixbuf);

	if (gdk_pixbuf_get_n_channels (image->pixbuf) != 4) return FALSE;

	if (pixbuf->n_channels == 3) {
		art_rgb_rgba_affine (pixbuf->pixels,
			0, 0, pixbuf->width, pixbuf->height, pixbuf->rowstride,
			pixels, width, height, rowstride,
			affine,
			ART_FILTER_NEAREST, NULL);
	} else {
		art_rgba_rgba_affine (pixbuf->pixels,
			0, 0, pixbuf->width, pixbuf->height, pixbuf->rowstride,
			pixels, width, height, rowstride,
			affine,
			ART_FILTER_NEAREST, NULL);
	}

	return FALSE;
}

/*
 * utility function to try loading image from href
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

	filename = sp_repr_attr (repr, "xlink:href");

	if (filename != NULL) {
		if (!g_path_is_absolute (filename)) {
			/* try to load from relative pos */
			docbase = sp_repr_attr (sp_repr_document_root (sp_repr_document (repr)), "sodipodi:docbase");
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
	filename = sp_repr_attr (repr, "sodipodi:absref");
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

	if (gdk_pixbuf_get_has_alpha (pixbuf))
		return pixbuf;

	newbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
	gdk_pixbuf_unref (pixbuf);

	return newbuf;
}

