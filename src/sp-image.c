#define __SP_IMAGE_C__

/*
 * SVG <image> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include "display/nr-arena-image.h"
#include "svg/svg.h"
#include "style.h"
#include "brokenimage.xpm"
#include "document.h"
#include "dialogs/object-attributes.h"
#include "sp-image.h"

/*
 * SPImage
 */

static void sp_image_class_init (SPImageClass * class);
static void sp_image_init (SPImage * image);

static void sp_image_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_image_release (SPObject * object);
static void sp_image_read_attr (SPObject * object, const gchar * key);
static SPRepr *sp_image_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_image_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static void sp_image_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_image_description (SPItem * item);
static GSList * sp_image_snappoints (SPItem * item, GSList * points);
static NRArenaItem *sp_image_show (SPItem *item, NRArena *arena);
static void sp_image_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);
static void sp_image_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_image_image_properties (GtkMenuItem *menuitem, SPAnchor *anchor);
#ifdef ENABLE_AUTOTRACE
static void sp_image_autotrace (GtkMenuItem *menuitem, SPAnchor *anchor);
static void autotrace_dialog(SPImage * img);
#endif /* Def: ENABLE_AUTOTRACE */

GdkPixbuf * sp_image_repr_read_image (SPRepr * repr);
static GdkPixbuf *sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf);
static void sp_image_update_canvas_image (SPImage *image);
static GdkPixbuf * sp_image_repr_read_dataURI (const gchar * uri_data);
static GdkPixbuf * sp_image_repr_read_b64 (const gchar * uri_data);

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
			NULL, NULL, NULL
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

	sp_object_class->build = sp_image_build;
	sp_object_class->release = sp_image_release;
	sp_object_class->read_attr = sp_image_read_attr;
	sp_object_class->write = sp_image_write;

	item_class->bbox = sp_image_bbox;
	item_class->print = sp_image_print;
	item_class->description = sp_image_description;
	item_class->show = sp_image_show;
	item_class->snappoints = sp_image_snappoints;
	item_class->write_transform = sp_image_write_transform;
	item_class->menu = sp_image_menu;
}

static void
sp_image_init (SPImage *image)
{
	sp_svg_length_unset (&image->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&image->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&image->width, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&image->height, SP_SVG_UNIT_NONE, 0.0, 0.0);

	image->href = NULL;

	image->pixbuf = NULL;
}

static void
sp_image_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

	sp_image_read_attr (object, "xlink:href");
	sp_image_read_attr (object, "x");
	sp_image_read_attr (object, "y");
	sp_image_read_attr (object, "width");
	sp_image_read_attr (object, "height");

	/* Register */
	sp_document_add_resource (document, "image", object);
}

static void
sp_image_release (SPObject *object)
{
	SPImage *image;

	image = SP_IMAGE (object);

	if (SP_OBJECT_DOCUMENT (object)) {
		/* Unregister ourselves */
		sp_document_remove_resource (SP_OBJECT_DOCUMENT (object), "image", SP_OBJECT (object));
	}

	if (image->href) {
		g_free (image->href);
		image->href = NULL;
	}

	if (image->pixbuf) {
		gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_image_read_attr (SPObject * object, const gchar *key)
{
	SPImage *image;
	const guchar *str;
	gulong unit;

	image = SP_IMAGE (object);

	str = sp_repr_attr (SP_OBJECT_REPR (object), key);

	if (!strcmp (key, "xlink:href")) {
		if (image->href) {
			g_free (image->href);
			image->href = NULL;
		}
		if (image->pixbuf) {
			gdk_pixbuf_unref (image->pixbuf);
			image->pixbuf = NULL;
		}
		if (str) {
			GdkPixbuf *pixbuf;
			image->href = g_strdup (str);
			pixbuf = sp_image_repr_read_image (object->repr);
			if (pixbuf) {
				pixbuf = sp_image_pixbuf_force_rgba (pixbuf);
				image->pixbuf = pixbuf;
			}
		}
		sp_image_update_canvas_image (image);
	} else if (!strcmp (key, "x")) {
		if (sp_svg_length_read_lff (str, &unit, &image->x.value, &image->x.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT)) {
			image->x.set = TRUE;
			image->x.unit = unit;
		} else {
			image->x.set = FALSE;
			image->x.unit = SP_SVG_UNIT_NONE;
			image->x.computed = 0.0;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		/* fixme: Do async (Lauris) */
		sp_image_update_canvas_image (image);
	} else if (!strcmp (key, "y")) {
		if (sp_svg_length_read_lff (str, &unit, &image->y.value, &image->y.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT)) {
			image->y.set = TRUE;
			image->y.unit = unit;
		} else {
			image->y.set = FALSE;
			image->y.unit = SP_SVG_UNIT_NONE;
			image->y.computed = 0.0;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		/* fixme: Do async (Lauris) */
		sp_image_update_canvas_image (image);
	} else if (!strcmp (key, "width")) {
		if (sp_svg_length_read_lff (str, &unit, &image->width.value, &image->width.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT)) {
			image->width.set = TRUE;
			image->width.unit = unit;
		} else {
			image->width.set = FALSE;
			image->width.unit = SP_SVG_UNIT_NONE;
			image->width.computed = 0.0;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		/* fixme: Do async (Lauris) */
		sp_image_update_canvas_image (image);
	} else if (!strcmp (key, "height")) {
		if (sp_svg_length_read_lff (str, &unit, &image->height.value, &image->height.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (unit != SP_SVG_UNIT_PERCENT)) {
			image->height.set = TRUE;
			image->height.unit = unit;
		} else {
			image->height.set = FALSE;
			image->height.unit = SP_SVG_UNIT_NONE;
			image->height.computed = 0.0;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		/* fixme: Do async (Lauris) */
		sp_image_update_canvas_image (image);
	} else if (((SPObjectClass *) (parent_class))->read_attr) {
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
	}
}

static SPRepr *
sp_image_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPImage *image;

	image = SP_IMAGE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("image");
	}

	sp_repr_set_attr (repr, "xlink:href", image->href);
	/* fixme: Reset attribute if needed (Lauris) */
	if (image->x.set) sp_repr_set_double (repr, "x", image->x.computed);
	if (image->y.set) sp_repr_set_double (repr, "y", image->y.computed);
	if (image->width.set) sp_repr_set_double (repr, "width", image->width.computed);
	if (image->height.set) sp_repr_set_double (repr, "height", image->height.computed);

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_image_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPImage *image;

	image = SP_IMAGE (item);

	if ((image->width.computed > 0.0) && (image->height.computed > 0.0)) {
		ArtDRect dim;

		dim.x0 = image->x.computed;
		dim.y0 = image->y.computed;
		dim.x1 = dim.x0 + image->width.computed;
		dim.y1 = dim.y0 + image->height.computed;

		art_drect_affine_transform (bbox, &dim, transform);
	}
}

static void
sp_image_print (SPItem *item, GnomePrintContext *gpc)
{
	SPObject *object;
	SPImage *image;
	guchar *pixels;
	gint width, height, rowstride;

	object = SP_OBJECT (item);
	image = SP_IMAGE (item);

	if (!image->pixbuf) return;
	if ((image->width.computed <= 0.0) || (image->height.computed <= 0.0)) return;

	pixels = gdk_pixbuf_get_pixels (image->pixbuf);
	width = gdk_pixbuf_get_width (image->pixbuf);
	height = gdk_pixbuf_get_height (image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (image->pixbuf);

	gnome_print_gsave (gpc);

	gnome_print_translate (gpc, image->x.computed, image->y.computed);
	gnome_print_scale (gpc, image->width.computed, -image->height.computed);
	gnome_print_translate (gpc, 0.0, -1.0);

	if (object->style->opacity.value != SP_SCALE24_MAX) {
		guchar *px, *d, *s;
		gint x, y;
		guint32 alpha;
		alpha = (guint32) floor (SP_SCALE24_TO_FLOAT (object->style->opacity.value) * 255.9999);
		px = g_new (guchar, width * height * 4);
		for (y = 0; y < height; y++) {
			s = pixels + y * rowstride;
			d = px + y * width * 4;
			memcpy (d, s, width * 4);
			for (x = 0; x < width; x++) {
				d[3] = (s[3] * alpha) / 255;
				s += 4;
				d += 4;
			}
		}
		gnome_print_rgbaimage (gpc, px, width, height, width * 4);
		g_free (px);
	} else {
		gnome_print_rgbaimage (gpc, pixels, width, height, rowstride);
	}

	gnome_print_grestore (gpc);
}

static gchar *
sp_image_description (SPItem * item)
{
	SPImage * image;

	image = SP_IMAGE (item);

	if (image->pixbuf == NULL) {
		return g_strdup_printf (_("Image with bad reference: %s"), image->href);
	} else {
		return g_strdup_printf (_("Color image %d x %d: %s"),
					  gdk_pixbuf_get_width (image->pixbuf),
					  gdk_pixbuf_get_height (image->pixbuf),
					  image->href);
	}
}

static NRArenaItem *
sp_image_show (SPItem *item, NRArena *arena)
{
	SPImage * image;
	NRArenaItem *ai;

	image = (SPImage *) item;

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_IMAGE);

	nr_arena_image_set_pixbuf (NR_ARENA_IMAGE (ai), image->pixbuf);
	nr_arena_image_set_geometry (NR_ARENA_IMAGE (ai), image->x.computed, image->y.computed, image->width.computed, image->height.computed);

	return ai;
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
	if (filename == NULL) filename = sp_repr_attr (repr, "href"); /* FIXME */
	if (filename != NULL) {
		if (strncmp (filename,"data:",5) == 0) {
			/* data URI - embedded image */
			filename += 5;
			pixbuf = sp_image_repr_read_dataURI (filename);
			if (pixbuf != NULL) return pixbuf;
		}
		else if (!g_path_is_absolute (filename)) {
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

	if (gdk_pixbuf_get_has_alpha (pixbuf)) return pixbuf;

	newbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
	gdk_pixbuf_unref (pixbuf);

	return newbuf;
}

/* We assert that realpixbuf is either NULL or identical size to pixbuf */

static void
sp_image_update_canvas_image (SPImage *image)
{
	SPItem *item;
	SPItemView *v;

	item = SP_ITEM (image);

	if (image->pixbuf) {
		/* fixme: We are slightly violating spec here (Lauris) */
		if (!image->width.set) {
			image->width.computed = gdk_pixbuf_get_width (image->pixbuf);
		}
		if (!image->height.set) {
			image->height.computed = gdk_pixbuf_get_height (image->pixbuf);
		}
	}

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_image_set_pixbuf (NR_ARENA_IMAGE (v->arenaitem), image->pixbuf);
		nr_arena_image_set_geometry (NR_ARENA_IMAGE (v->arenaitem),
					     image->x.computed, image->y.computed,
					     image->width.computed, image->height.computed);
	}
}

static GSList * 
sp_image_snappoints (SPItem * item, GSList * points)
{
	ArtPoint *p, p1, p2, p3, p4;
	gdouble affine[6];
	SPImage *image;

	image = SP_IMAGE (item);

	sp_item_i2d_affine (item, affine);

	/* we use corners of image only */
	p1.x = image->x.computed;
	p1.y = image->y.computed;
	p2.x = p1.x + image->width.computed;
	p2.y = p1.y;
	p3.x = p1.x;
	p3.y = p1.y + image->height.computed;
	p4.x = p2.x;
	p4.y = p3.y;

	p = g_new (ArtPoint,1);
	art_affine_point (p, &p1, affine);
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p2, affine);
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p3, affine);
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p4, affine);
	points = g_slist_append (points, p);

	return points;
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static void
sp_image_write_transform (SPItem *item, SPRepr *repr, gdouble *transform)
{
	SPImage *image;
	gdouble rev[6];
	gdouble px, py, sw, sh;
	guchar t[80];

	image = SP_IMAGE (item);

	/* Calculate text start in parent coords */
	px = transform[0] * image->x.computed + transform[2] * image->y.computed + transform[4];
	py = transform[1] * image->x.computed + transform[3] * image->y.computed + transform[5];

	/* Clear translation */
	transform[4] = 0.0;
	transform[5] = 0.0;

	/* Scalers */
	sw = sqrt (transform[0] * transform[0] + transform[1] * transform[1]);
	sh = sqrt (transform[2] * transform[2] + transform[3] * transform[3]);
	if (sw > 1e-9) {
		transform[0] = transform[0] / sw;
		transform[1] = transform[1] / sw;
	} else {
		transform[0] = 1.0;
		transform[1] = 0.0;
	}
	if (sh > 1e-9) {
		transform[2] = transform[2] / sh;
		transform[3] = transform[3] / sh;
	} else {
		transform[2] = 0.0;
		transform[3] = 1.0;
	}
	sp_repr_set_double_attribute (repr, "width", image->width.computed * sw);
	sp_repr_set_double_attribute (repr, "height", image->height.computed * sh);

	/* Find start in item coords */
	art_affine_invert (rev, transform);
	sp_repr_set_double_attribute (repr, "x", px * rev[0] + py * rev[2]);
	sp_repr_set_double_attribute (repr, "y", px * rev[1] + py * rev[3]);

	if (sp_svg_write_affine (t, 80, transform)) {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
	} else {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	}
}

/* Generate context menu item section */

static void
sp_image_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Image"));
	m = gtk_menu_new ();
	/* Link dialog */
	w = gtk_menu_item_new_with_label (_("Image Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_image_image_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);

#ifdef ENABLE_AUTOTRACE
	/* Autotrace dialog */
	w = gtk_menu_item_new_with_label (_("Trace"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_image_autotrace), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
#endif /* Def: ENABLE_AUTOTRACE */

	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

static void
sp_image_image_properties (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	sp_object_attributes_dialog (SP_OBJECT (anchor), "SPImage");
}

#ifdef ENABLE_AUTOTRACE
static void
sp_image_autotrace (GtkMenuItem *menuitem, SPAnchor *anchor)
{
  autotrace_dialog (SP_IMAGE(anchor));
}
#endif /* Def: ENABLE_AUTOTRACE */

static GdkPixbuf *
sp_image_repr_read_dataURI (const gchar * uri_data)
{	GdkPixbuf * pixbuf = NULL;

	gint data_is_image = 0;
	gint data_is_base64 = 0;

	const gchar * data = uri_data;

	while (*data) {
		if (strncmp (data,"base64",6) == 0) {
			/* base64-encoding */
			data_is_base64 = 1;
			data += 6;
		}
		else if (strncmp (data,"image/png",9) == 0) {
			/* PNG image */
			data_is_image = 1;
			data += 9;
		}
		else if (strncmp (data,"image/jpg",9) == 0) {
			/* JPEG image */
			data_is_image = 1;
			data += 9;
		}
		else if (strncmp (data,"image/jpeg",10) == 0) {
			/* JPEG image */
			data_is_image = 1;
			data += 10;
		}
		else { /* unrecognized option; skip it */
			while (*data) {
				if (((*data) == ';') || ((*data) == ',')) break;
				data++;
			}
		}
		if ((*data) == ';') {
			data++;
			continue;
		}
		if ((*data) == ',') {
			data++;
			break;
		}
	}

	if ((*data) && data_is_image && data_is_base64) {
		pixbuf = sp_image_repr_read_b64 (data);
	}

	return pixbuf;
}

static GdkPixbuf *
sp_image_repr_read_b64 (const gchar * uri_data)
{	GdkPixbuf * pixbuf = NULL;
	GdkPixbufLoader * loader = NULL;

	gint j;
	gint k;
	gint l;
	gint b;
	gint len;
	gint eos = 0;
	gint failed = 0;

	guint32 bits;

	static const gchar B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const gchar* btr = uri_data;

	gchar ud[4];

	guchar bd[57];

	loader = gdk_pixbuf_loader_new ();

	if (loader == NULL) return NULL;

	while (eos == 0) {
		l = 0;
		for (j = 0; j < 19; j++) {
			len = 0;
			for (k = 0; k < 4; k++) {
				while (isspace ((int) (*btr))) {
					if ((*btr) == '\0') break;
					btr++;
				}
				if (eos) {
					ud[k] = 0;
					continue;
				}
				if (((*btr) == '\0') || ((*btr) == '=')) {
					eos = 1;
					ud[k] = 0;
					continue;
				}
				ud[k] = 64;
				for (b = 0; b < 64; b++) { /* There must a faster way to do this... ?? */
					if (B64[b] == (*btr)) {
						ud[k] = (gchar) b;
						break;
					}
				}
				if (ud[k] == 64) { /* data corruption ?? */
					eos = 1;
					ud[k] = 0;
					continue;
				}
				btr++;
				len++;
			}
			bits = (guint32) ud[0];
			bits = (bits << 6) | (guint32) ud[1];
			bits = (bits << 6) | (guint32) ud[2];
			bits = (bits << 6) | (guint32) ud[3];
			bd[l++] = (guchar) ((bits & 0xff0000) >> 16);
			if (len > 2) {
				bd[l++] = (guchar) ((bits & 0xff00) >>  8);
			}
			if (len > 3) {
				bd[l++] = (guchar)  (bits & 0xff);
			}
		}

		if (!gdk_pixbuf_loader_write (loader, (const guchar *) bd, (size_t) l)) {
			failed = 1;
			break;
		}
	}

	gdk_pixbuf_loader_close (loader);

	if (!failed) pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

	return pixbuf;
}

#ifdef ENABLE_AUTOTRACE
#define GDK_PIXBUF_TO_AT_BITMAP_DEBUG 0
#include <autotrace/autotrace.h>
#include <frontline/frontline.h>

#include "view.h"
#include "desktop.h"
#include "interface.h"

/* getpid and umask */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

/* g_strdup_printf */
#include <glib.h>

/* GnomeMessageBox */
#include <libgnomeui/libgnomeui.h>

static at_bitmap_type * gdk_pixbuf_to_at_bitmap (GdkPixbuf * pixbuf);
static void load_trace_result(FrontlineDialog * fl_dialog, gpointer user_data);
static void load_splines(at_splines_type * splines);
static void load_file(const guchar * filename);
static void handle_msg(at_string msg, at_msg_type msg_type, at_address client_data);
static GtkWidget *  build_header_area(SPRepr *repr);

static void
autotrace_dialog (SPImage * img)
{
	gchar *title = _("Trace");

	GtkWidget *trace_dialog;
	GtkWidget * header_area;
	GtkWidget * header_sep;
	at_bitmap_type * bitmap;

	trace_dialog = frontline_dialog_new();
	gtk_window_set_title (GTK_WINDOW (trace_dialog), title);

	gtk_signal_connect_object_while_alive (GTK_OBJECT (img), 
					       "release",
					       GTK_SIGNAL_FUNC (gtk_widget_destroy),
					       GTK_OBJECT (trace_dialog));
	gtk_signal_connect_object (GTK_OBJECT(FRONTLINE_DIALOG(trace_dialog)->close_button),
				   "clicked",
				   GTK_SIGNAL_FUNC(gtk_widget_destroy), 
				   GTK_OBJECT(trace_dialog));
	gtk_signal_connect(GTK_OBJECT(trace_dialog),
			   "trace_done",
			   GTK_SIGNAL_FUNC (load_trace_result), 
			   trace_dialog);
	
	header_area = build_header_area(SP_OBJECT_REPR(img));
	gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(trace_dialog)->header_area),
				    header_area);
	gtk_widget_show(header_area);
	
	header_sep = gtk_hseparator_new();
	gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(trace_dialog)->header_area), 
				    header_sep);
	gtk_widget_show(header_sep);

	
	bitmap = gdk_pixbuf_to_at_bitmap(img->pixbuf);
	frontline_dialog_set_bitmap(FRONTLINE_DIALOG(trace_dialog), bitmap);
	
	gtk_widget_show (trace_dialog);
}

static at_bitmap_type *
gdk_pixbuf_to_at_bitmap (GdkPixbuf * pixbuf)
{
	guchar *    datum;
	at_bitmap_type * bitmap;
	unsigned short width, height;
	int i, j;

	if (GDK_PIXBUF_TO_AT_BITMAP_DEBUG)
		g_message("%d:channel %d:width %d:height %d:bits_per_sample %d:has_alpha %d:rowstride\n", 
			  gdk_pixbuf_get_n_channels(pixbuf),
			  gdk_pixbuf_get_width(pixbuf),
			  gdk_pixbuf_get_height(pixbuf),
			  gdk_pixbuf_get_bits_per_sample(pixbuf),
			  gdk_pixbuf_get_has_alpha(pixbuf),
			  gdk_pixbuf_get_rowstride(pixbuf));

	datum   = gdk_pixbuf_get_pixels(pixbuf);
	width   = gdk_pixbuf_get_width(pixbuf);
	height  = gdk_pixbuf_get_height(pixbuf);

	bitmap = at_bitmap_new(width, height, 3);

	if (gdk_pixbuf_get_has_alpha(pixbuf)) {
		j = 0;
		for(i = 0; i < width * height * 4; i++)
			if (3 != (i % 4))
				bitmap->bitmap[j++] = datum[i];
	}
	else
		memmove(bitmap->bitmap, datum, width * height * 3);
	return bitmap;
}

static void
load_trace_result(FrontlineDialog * fl_dialog, gpointer user_data)
{
	FrontlineDialog * trace_dialog;
	trace_dialog = FRONTLINE_DIALOG(user_data);
	
	if (!trace_dialog->splines) 
	  return;
	if (fl_ask(GTK_WINDOW(trace_dialog), trace_dialog->splines))
	  load_splines(trace_dialog->splines);
}


static void
load_splines(at_splines_type * splines)
{
  	static int serial_num = 0;
	FILE * tmp_fp;
	at_output_write_func writer;
	gchar * filename;

	mode_t old_mask;

  	filename = g_strdup_printf("/tmp/at-%s-%d-%d.svg", 
				   g_get_user_name(),
				   getpid(), 
				   serial_num++);
	
	/* Make the mode of temporary svg file 
	   "readable and writable by the user only". */
	old_mask = umask(066);
	tmp_fp = fopen(filename, "w");
	umask(old_mask);
  
	writer = at_output_get_handler_by_suffix ("svg");
	at_splines_write (writer, tmp_fp, filename, NULL, splines,
			  handle_msg, filename);
	fclose(tmp_fp);
  
	load_file (filename);

	unlink(filename);
	g_free(filename);
}


static void
load_file (const guchar *filename)
{
	SPDocument * doc;
	SPViewWidget *dtw;
  
	/* fixme: Either use file:: method, or amke this private (Lauris) */
	/* fixme: In latter case we may want to publish it on save (Lauris) */
	doc = sp_document_new (filename, TRUE);
	dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
	sp_document_unref (doc);
	sp_create_window (dtw, TRUE);
}

static void
handle_msg(at_string msg, at_msg_type msg_type, at_address client_data)
{
	GtkWidget *dialog;
	guchar * long_msg;
	guchar * target = client_data;
	
	if (msg_type == AT_MSG_FATAL) {
		long_msg = g_strdup_printf(_("Error to write %s: %s"), 
					   target, msg);	
		dialog = gnome_message_box_new(long_msg,
					       GNOME_MESSAGE_BOX_ERROR,
					       _("Ok"), NULL);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gnome_dialog_run(GNOME_DIALOG(dialog));
		g_free(long_msg);
	}
	else {
		long_msg = g_strdup_printf("%s: %s", msg, target);	
		g_warning("%s", long_msg);
		g_free(long_msg);
	}
}

static GtkWidget *
build_header_area(SPRepr *repr)
{
	const gchar * doc_name, * img_uri;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * label;
	GtkWidget * entry;
	
	vbox = gtk_vbox_new(TRUE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
	
	/* Doc name */
	doc_name = sp_repr_attr(sp_repr_document_root(sp_repr_document(repr)), 
				"sodipodi:docname");
	if (!doc_name)
		doc_name = _("Untitled");
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
	gtk_widget_show(hbox);
	
	label = gtk_label_new(_("Document Name:"));
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), doc_name);
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 4);
	gtk_widget_show(label);
	gtk_widget_show(entry);
	gtk_widget_set_sensitive(entry, FALSE);

	/* Image URI */
	img_uri = sp_repr_attr(repr, "xlink:href");
	if (!img_uri)
		img_uri = _("Unknown");

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
	gtk_widget_show(hbox);
	
	label = gtk_label_new(_("Image URI:"));
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), img_uri);
	gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 4);
	gtk_widget_show(label);
	gtk_widget_show(entry);
	gtk_widget_set_sensitive(entry, FALSE);
	
	return vbox;
}

#endif /* Def: ENABLE_AUTOTRACE */
