#ifndef __SP_ELLIPSE_H__
#define __SP_ELLIPSE_H__

/*
 * SVG <ellipse> and related implementations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-types.h"
#include "sp-shape.h"

/* Common parent class */

#define SP_TYPE_GENERICELLIPSE (sp_genericellipse_get_type ())
#define SP_GENERICELLIPSE(o) (GTK_CHECK_CAST ((o), SP_TYPE_GENERICELLIPSE, SPGenericEllipse))
#define SP_GENERICELLIPSE_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_GENERICELLIPSE, SPGenericEllipseClass))
#define SP_IS_GENERICELLIPSE(o) (GTK_CHECK_TYPE ((o), SP_TYPE_GENERICELLIPSE))
#define SP_IS_GENERICELLIPSE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_GENERICELLIPSE))

typedef struct _SPGenericEllipse SPGenericEllipse;
typedef struct _SPGenericEllipseClass SPGenericEllipseClass;

struct _SPGenericEllipse {
	SPShape shape;
	SPSVGLength cx;
	SPSVGLength cy;
	SPSVGLength rx;
	SPSVGLength ry;

	unsigned int closed : 1;
	double start, end;
};

struct _SPGenericEllipseClass {
	SPShapeClass parent_class;
};

GtkType sp_genericellipse_get_type (void);

/* SVG <ellipse> element */

#define SP_TYPE_ELLIPSE (sp_ellipse_get_type ())
#define SP_ELLIPSE(o) (GTK_CHECK_CAST ((o), SP_TYPE_ELLIPSE, SPEllipse))
#define SP_ELLIPSE_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_ELLIPSE, SPEllipseClass))
#define SP_IS_ELLIPSE(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ELLIPSE))
#define SP_IS_ELLIPSE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_ELLIPSE))

struct _SPEllipse {
	SPGenericEllipse genericellipse;
};

struct _SPEllipseClass {
	SPGenericEllipseClass parent_class;
};

GtkType sp_ellipse_get_type (void);

void sp_ellipse_set (SPEllipse * ellipse, gdouble x, gdouble y, gdouble rx, gdouble ry);

/* SVG <circle> element */

#define SP_TYPE_CIRCLE (sp_circle_get_type ())
#define SP_CIRCLE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CIRCLE, SPCircle))
#define SP_CIRCLE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CIRCLE, SPCircleClass))
#define SP_IS_CIRCLE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CIRCLE))
#define SP_IS_CIRCLE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CIRCLE))

struct _SPCircle {
	SPGenericEllipse genericellipse;
};

struct _SPCircleClass {
	SPGenericEllipseClass parent_class;
};

GtkType sp_circle_get_type (void);

/* <path sodipodi:type="arc"> element */

#define SP_TYPE_ARC (sp_arc_get_type ())
#define SP_ARC(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ARC, SPArc))
#define SP_ARC_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ARC, SPArcClass))
#define SP_IS_ARC(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ARC))
#define SP_IS_ARC_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ARC))

struct _SPArc {
	SPGenericEllipse genericellipse;
};

struct _SPArcClass {
	SPGenericEllipseClass parent_class;
};

GtkType sp_arc_get_type (void);
void sp_arc_set (SPArc *arc, gdouble x, gdouble y, gdouble rx, gdouble ry);
void sp_arc_get_xy (SPArc *ge, gdouble arg, ArtPoint *p);

#endif
