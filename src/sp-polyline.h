#ifndef SP_POLYLINE_H
#define SP_POLYLINE_H

#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_POLYLINE            (sp_polyline_get_type ())
#define SP_POLYLINE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_POLYLINE, SPPolyLine))
#define SP_POLYLINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYLINE, SPPolyLineClass))
#define SP_IS_POLYLINE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_POLYLINE))
#define SP_IS_POLYLINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYLINE))

typedef struct _SPPolyLine SPPolyLine;
typedef struct _SPPolyLineClass SPPolyLineClass;

struct _SPPolyLine {
	SPShape shape;
};

struct _SPPolyLineClass {
	SPShapeClass parent_class;
};

GtkType sp_polyline_get_type (void);

END_GNOME_DECLS

#endif
