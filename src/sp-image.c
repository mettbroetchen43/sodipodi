#define SP_IMAGE_C

#include <ctype.h>
#include <gnome.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include "helper/art-rgba-rgba-affine.h"
#include "display/nr-arena-image.h"
#include "style.h"
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
static GSList * sp_image_snappoints (SPItem * item, GSList * points);
static NRArenaItem *sp_image_show (SPItem *item, NRArena *arena);

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
	item_class->snappoints = sp_image_snappoints;
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

	if (image->pixbuf) {
		gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
	}

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
		/* Free old pixbuf */
		if (image->pixbuf) {
			gdk_pixbuf_unref (image->pixbuf);
			image->pixbuf = NULL;
		}
		pixbuf = sp_image_repr_read_image (object->repr);
		pixbuf = sp_image_pixbuf_force_rgba (pixbuf);
		image->pixbuf = pixbuf;
		sp_image_update_canvas_image (image);
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);


	if (!strcmp (key, "style")) {
#if 0
		/* fixme: */
		sp_image_update_canvas_image (image);
#endif
	}
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

static void
sp_image_print (SPItem * item, GnomePrintContext * gpc)
{
	SPObject *object;
	SPImage *image;
	double affine[6];
	guchar * pixels;
	gint width, height, rowstride;

	object = SP_OBJECT (item);
	image = SP_IMAGE (item);

	if (!image->pixbuf) return;

	pixels = gdk_pixbuf_get_pixels (image->pixbuf);
	width = gdk_pixbuf_get_width (image->pixbuf);
	height = gdk_pixbuf_get_height (image->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (image->pixbuf);

	gnome_print_gsave (gpc);

	art_affine_scale (affine, width, -height);
	gnome_print_concat (gpc, affine);
	art_affine_translate (affine, 0.0, -1.0);
	gnome_print_concat (gpc, affine);

	if (object->style->opacity != 1.0) {
		guchar *px, *d, *s;
		gint x, y;
		guint32 alpha;
		alpha = (guint32) floor (object->style->opacity * 255.9999);
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

	if (image->pixbuf == NULL) return g_strdup (_("Broken bitmap"));
	return g_strdup (_("Color bitmap"));
}

static NRArenaItem *
sp_image_show (SPItem *item, NRArena *arena)
{
	SPObject *object;
	SPImage * image;
	NRArenaItem *arenaitem;

	object = SP_OBJECT (item);
	image = (SPImage *) item;

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_IMAGE);

	nr_arena_image_set_pixbuf (NR_ARENA_IMAGE (arenaitem), image->pixbuf);

	return arenaitem;
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
	SPObject *object;
	SPItem *item;
	SPItemView *v;

	object = SP_OBJECT (image);
	item = SP_ITEM (image);

	if (!image->pixbuf) return;

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_image_set_pixbuf (NR_ARENA_IMAGE (v->arenaitem), image->pixbuf);
	}
}

static GSList * 
sp_image_snappoints (SPItem * item, GSList * points)
{
  ArtPoint * p, p1, p2, p3, p4;
  gdouble affine[6], w, h;
  SPImage * image;

  image = SP_IMAGE (item);

  w = gdk_pixbuf_get_width (image->pixbuf);
  h = gdk_pixbuf_get_height (image->pixbuf);

  /* we use corners of image only */
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = w;
  p2.y = 0.0;
  p3.x = 0.0;
  p3.y = h;
  p4.x = w;
  p4.y = h;
  sp_item_i2d_affine (item, affine);

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
