#ifndef SP_RECT_H
#define SP_RECT_H

#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_RECT            (sp_rect_get_type ())
#define SP_RECT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_RECT, SPRect))
#define SP_RECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_RECT, SPRectClass))
#define SP_IS_RECT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_RECT))
#define SP_IS_RECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_RECT))

typedef struct _SPRect SPRect;
typedef struct _SPRectClass SPRectClass;

struct _SPRect {
	SPShape shape;

	double x, y;
	double width, height;
	double rx, ry;
	ArtBpath * bpath;
};

struct _SPRectClass {
	SPShapeClass parent_class;
};


/* Standard Gtk function */
GtkType sp_rect_get_type (void);

END_GNOME_DECLS

#endif
