#ifndef SP_CANVAS_IMAGE_H
#define SP_CANVAS_IMAGE_H

/* SPCanvasImage
 *
 * A simple image for Sodipodi display
 *
 * Has always NW corner at 0,0 user coords
 */

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libgnomeui/gnome-canvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define SP_TYPE_CANVAS_IMAGE            (sp_canvas_image_get_type ())
#define SP_CANVAS_IMAGE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_IMAGE, SPCanvasImage))
#define SP_CANVAS_IMAGE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_IMAGE, SPCanvasImageClass))
#define SP_IS_CANVAS_IMAGE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_IMAGE))
#define SP_IS_CANVAS_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_IMAGE))


typedef struct _SPCanvasImage SPCanvasImage;
typedef struct _SPCanvasImageClass SPCanvasImageClass;

struct _SPCanvasImage {
	GnomeCanvasItem item;

	GdkPixbuf * pixbuf;
	GdkPixbuf * realpixbuf;
	gdouble opacity;
	gdouble realpixbufopacity;
	double affine[6];
	ArtVpath * vpath;
	gboolean sensitive;
};

struct _SPCanvasImageClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_canvas_image_get_type (void);

/* Utility */
void sp_canvas_image_set_pixbuf (SPCanvasImage * image, GdkPixbuf * pixbuf);
void sp_canvas_image_set_sensitive (SPCanvasImage * image, gboolean sensitive);

#endif
