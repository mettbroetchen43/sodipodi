#define SP_IMAGE_C

#include <gnome.h>
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

static void sp_image_update (SPItem * item, gdouble affine[]);
static void sp_image_bbox (SPItem * item, ArtDRect * bbox);
static void sp_image_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_image_description (SPItem * item);
static void sp_image_read (SPItem * item, SPRepr * repr);
static void sp_image_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);
static GnomeCanvasItem * sp_image_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static void sp_image_paint (SPItem * item, ArtPixBuf * pixbuf, gdouble * affine);

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
sp_image_class_init (SPImageClass *class)
{
	GtkObjectClass *object_class;
	SPItemClass * item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_item_get_type ());

	object_class->destroy = sp_image_destroy;

	item_class->update = sp_image_update;
	item_class->bbox = sp_image_bbox;
	item_class->print = sp_image_print;
	item_class->description = sp_image_description;
	item_class->read = sp_image_read;
	item_class->read_attr = sp_image_read_attr;
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
sp_image_update (SPItem * item, gdouble * affine)
{
	if (SP_ITEM_CLASS (parent_class)->update)
		(* SP_ITEM_CLASS (parent_class)->update) (item, affine);
}

static void
sp_image_bbox (SPItem * item, ArtDRect * bbox)
{
	SPImage * image;
	SPItem * i;
	double a[6];
	ArtPoint p;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL) {
		bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;
		return;
	}

	art_affine_identity (a);
	for (i = item; i != NULL; i = SP_ITEM (i->parent))
		art_affine_multiply (a, a, i->affine);

	p.x = 0.0;
	p.y = 0.0;
	art_affine_point (&p, &p, a);
	bbox->x0 = bbox->x1 = p.x;
	bbox->y0 = bbox->y1 = p.y;

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
	art_u8 * buf, * sptr, * dptr;
	gint x, y;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL)
		return;

	abp = image->pixbuf->art_pixbuf;

	gnome_print_gsave (gpc);

	art_affine_scale (affine, abp->width, -abp->height);
	gnome_print_concat (gpc, affine);
	art_affine_translate (affine, 0.0, -1.0);
	gnome_print_concat (gpc, affine);

	buf = abp->pixels;

	if (abp->n_channels == 4) {
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
		gnome_print_rgbimage (gpc, buf,
			abp->width, abp->height, abp->width * 3);
		g_free (buf);
	}
	else if (abp->n_channels == 3) {
		gnome_print_rgbimage (gpc, abp->pixels,
			abp->width, abp->height, abp->rowstride);
	}
	else if (abp->n_channels == 1) {
		gnome_print_grayimage (gpc, abp->pixels,
			abp->width, abp->height, abp->rowstride);
	}

	gnome_print_grestore (gpc);
}

static gchar *
sp_image_description (SPItem * item)
{
	SPImage * image;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL)
		return g_strdup (_("Empty bitmap"));
	if (image->pixbuf->art_pixbuf->n_channels == 1)
		return g_strdup (_("Gray bitmap"));
	return g_strdup (_("Color bitmap"));
}

static void
sp_image_read (SPItem * item, SPRepr * repr)
{
	if (SP_ITEM_CLASS (parent_class)->read)
		SP_ITEM_CLASS (parent_class)->read (item, repr);

	sp_image_read_attr (item, repr, "src");
}

static void
sp_image_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	SPImage * image;
	const char * filename;
	const gchar * docbase;
	gchar * fn;
	GdkPixbuf * pixbuf;

	image = SP_IMAGE (item);

	pixbuf = NULL;

	if (strcmp (attr, "src") == 0) {
		if (image->pixbuf != NULL) gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
		filename = sp_repr_attr (repr, attr);
		if (filename != NULL) {
			if (!g_path_is_absolute (filename)) {
				/* try to load from relative pos */
				docbase = sp_repr_doc_attr (repr, "SP-DOCBASE");
				if (docbase != NULL) {
					fn = g_strconcat (docbase, filename, NULL);
					pixbuf = gdk_pixbuf_new_from_file (fn);
					if (pixbuf != NULL) {
						image->pixbuf = pixbuf;
						g_free (fn);
						sp_item_request_canvas_update (SP_ITEM (image));
						return;
					}
					g_free (fn);
				}
			} else {
				/* load from absolute pos */
				pixbuf = gdk_pixbuf_new_from_file (filename);
				if (pixbuf != NULL) {
					image->pixbuf = pixbuf;
					sp_item_request_canvas_update (SP_ITEM (image));
					return;
				}
			}
		}
		/* at last try to load from sp-absolute-path-name */
		filename = sp_repr_attr (repr, "sp-absolute-path-name");
		if (filename != NULL) {
			pixbuf = gdk_pixbuf_new_from_file (filename);
			if (pixbuf != NULL) {
				image->pixbuf = pixbuf;
				sp_item_request_canvas_update (SP_ITEM (image));
				return;
			}
		}
		/* Nope: We do not find any valid pixmap file :-( */
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const gchar **) brokenimage_xpm);
		g_assert (pixbuf != NULL);
		image->pixbuf = pixbuf;
		sp_item_request_canvas_update (SP_ITEM (image));
		return;
	}

	if (SP_ITEM_CLASS (parent_class)->read_attr)
		SP_ITEM_CLASS (parent_class)->read_attr (item, repr, attr);

}

static GnomeCanvasItem *
sp_image_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	SPImage * image;
	SPCanvasImage * ci;

	image = (SPImage *) item;

	ci = (SPCanvasImage *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_IMAGE, NULL);
	g_assert (ci != NULL);
	sp_canvas_image_set_pixbuf (ci, image->pixbuf->art_pixbuf);

	return (GnomeCanvasItem *) ci;
}

static void
sp_image_paint (SPItem * item, ArtPixBuf * pixbuf, gdouble * affine)
{
	SPImage * image;
	ArtPixBuf * ipb;

	image = SP_IMAGE (item);
	ipb = image->pixbuf->art_pixbuf;

	if (ipb->n_channels != 4) return;

	art_rgba_rgba_affine (pixbuf->pixels,
		0, 0, pixbuf->width, pixbuf->height, pixbuf->rowstride,
		ipb->pixels, ipb->width, ipb->height, ipb->rowstride,
		affine,
		ART_FILTER_NEAREST, NULL);
}

