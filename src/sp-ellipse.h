#ifndef __SP_ELLIPSE_H__
#define __SP_ELLIPSE_H__

/*
 * SPGenericEllipse, SPEllipse, SPCircle
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Ximian, Inc. and author
 *
 * You can distribute and modify that code under terms
 * of GNU GPL. See file COPYING for more information.
 *
 */

#include "sp-shape.h"

/* Common parent class */

#define SP_TYPE_GENERICELLIPSE (sp_genericellipse_get_type ())
#define SP_GENERICELLIPSE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_GENERICELLIPSE, SPGenericEllipse))
#define SP_GENERICELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GENERICELLIPSE, SPGenericEllipseClass))
#define SP_IS_GENERICELLIPSE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_GENERICELLIPSE))
#define SP_IS_GENERICELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GENERICELLIPSE))

typedef struct _SPGenericEllipse SPGenericEllipse;
typedef struct _SPGenericEllipseClass SPGenericEllipseClass;

struct _SPGenericEllipse {
	SPShape shape;
	gdouble x, y;
	gdouble rx, ry;
	gdouble start, end;
	gint closed;
};

struct _SPGenericEllipseClass {
	SPShapeClass parent_class;
};

GtkType sp_genericellipse_get_type (void);

/* SVG <ellipse> element */

#define SP_TYPE_ELLIPSE (sp_ellipse_get_type ())
#define SP_ELLIPSE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ELLIPSE, SPEllipse))
#define SP_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ELLIPSE, SPEllipseClass))
#define SP_IS_ELLIPSE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ELLIPSE))
#define SP_IS_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ELLIPSE))

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

#endif
