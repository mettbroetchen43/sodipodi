#ifndef SP_ELLIPSE_H
#define SP_ELLIPSE_H

#include "sp-shape.h"

#define SP_TYPE_ELLIPSE            (sp_ellipse_get_type ())
#define SP_ELLIPSE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ELLIPSE, SPEllipse))
#define SP_ELLIPSE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ELLIPSE, SPEllipseClass))
#define SP_IS_ELLIPSE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ELLIPSE))
#define SP_IS_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ELLIPSE))

typedef struct _SPEllipse SPEllipse;
typedef struct _SPEllipseClass SPEllipseClass;

struct _SPEllipse {
	SPShape shape;

	double x, y;
	double rx, ry;
	double start, end;
	gint closed;
	ArtBpath * bpath;
};

struct _SPEllipseClass {
	SPShapeClass parent_class;
};


/* Standard Gtk function */
GtkType sp_ellipse_get_type (void);

#endif
