#ifndef SP_POLYGON_H
#define SP_POLYGON_H

#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_POLYGON            (sp_polygon_get_type ())
#define SP_POLYGON(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_POLYGON, SPPolygon))
#define SP_POLYGON_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYGON, SPPolygonClass))
#define SP_IS_POLYGON(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_POLYGON))
#define SP_IS_POLYGON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYGON))

typedef struct _SPPolygon SPPolygon;
typedef struct _SPPolygonClass SPPolygonClass;

struct _SPPolygon {
	SPShape shape;
};

struct _SPPolygonClass {
	SPShapeClass parent_class;
};

GtkType sp_polygon_get_type (void);

END_GNOME_DECLS

#endif
