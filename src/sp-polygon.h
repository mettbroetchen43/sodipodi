#ifndef __SP_POLYGON_H__
#define __SP_POLYGON_H__

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_POLYGON            (sp_polygon_get_type ())
#define SP_POLYGON(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_POLYGON, SPPolygon))
#define SP_POLYGON_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYGON, SPPolygonClass))
#define SP_IS_POLYGON(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_POLYGON))
#define SP_IS_POLYGON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYGON))

#define SP_POLYGON_WRITE_POINTS (1 << 3)

struct _SPPolygon {
	SPShape shape;
};

struct _SPPolygonClass {
	SPShapeClass parent_class;
};

GtkType sp_polygon_get_type (void);

END_GNOME_DECLS

#endif
