#ifndef SP_IMAGE_H
#define SP_IMAGE_H

#if 0
#include <gtk/gtkpacker.h> /* why the hell is GtkAnchorType here and not in gtkenums.h? */
#endif
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "sp-item.h"

#define SP_TYPE_IMAGE            (sp_image_get_type ())
#define SP_IMAGE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_IMAGE, SPImage))
#define SP_IMAGE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_IMAGE, SPImageClass))
#define SP_IS_IMAGE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_IMAGE))
#define SP_IS_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_IMAGE))


typedef struct _SPImage SPImage;
typedef struct _SPImageClass SPImageClass;

struct _SPImage {
	SPItem item;
	GdkPixbuf *pixbuf;
};

struct _SPImageClass {
	SPItemClass parent_class;
};

/* Standard Gtk function */
GtkType sp_image_get_type (void);

#endif
