#ifndef __SP_POLYGON_H__
#define __SP_POLYGON_H__

/*
 * SVG <polygon> and <polyline> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _SPPolygon SPPolygon;
typedef struct _SPPolygonClass SPPolygonClass;

#define SP_TYPE_POLYGON (sp_polygon_get_type ())
#define SP_POLYGON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYGON, SPPolygon))
#define SP_IS_POLYGON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYGON))

typedef struct _SPPolyLine SPPolyLine;
typedef struct _SPPolyLineClass SPPolyLineClass;

#define SP_TYPE_POLYLINE (sp_polyline_get_type ())
#define SP_POLYLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYLINE, SPPolyLine))
#define SP_IS_POLYLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYLINE))

#include "sp-shape.h"

/* SPPolygon */

struct _SPPolygon {
	SPShape shape;
};

struct _SPPolygonClass {
	SPShapeClass parent_class;
};

GType sp_polygon_get_type (void);

/* SPPolyline */

struct _SPPolyLine {
	SPShape shape;
};

struct _SPPolyLineClass {
	SPShapeClass parent_class;
};

GType sp_polyline_get_type (void);

#endif
