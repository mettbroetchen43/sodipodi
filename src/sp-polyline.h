#ifndef __SP_POLYLINE_H__
#define __SP_POLYLINE_H__

/*
 * SVG <polyline> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-shape.h"

G_BEGIN_DECLS

#define SP_TYPE_POLYLINE            (sp_polyline_get_type ())
#define SP_POLYLINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYLINE, SPPolyLine))
#define SP_POLYLINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYLINE, SPPolyLineClass))
#define SP_IS_POLYLINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYLINE))
#define SP_IS_POLYLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYLINE))

typedef struct _SPPolyLine SPPolyLine;
typedef struct _SPPolyLineClass SPPolyLineClass;

struct _SPPolyLine {
	SPShape shape;
};

struct _SPPolyLineClass {
	SPShapeClass parent_class;
};

GType sp_polyline_get_type (void);

G_END_DECLS

#endif
