#ifndef __SP_RECT_H__
#define __SP_RECT_H__

/*
 * SVG <rect> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-types.h"
#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_RECT (sp_rect_get_type ())
#define SP_RECT(o) (GTK_CHECK_CAST ((o), SP_TYPE_RECT, SPRect))
#define SP_RECT_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_RECT, SPRectClass))
#define SP_IS_RECT(o) (GTK_CHECK_TYPE ((o), SP_TYPE_RECT))
#define SP_IS_RECT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_RECT))

typedef struct _SPRect SPRect;
typedef struct _SPRectClass SPRectClass;

struct _SPRect {
	SPShape shape;

	SPSVGLength x;
	SPSVGLength y;
	SPSVGLength width;
	SPSVGLength height;
	SPSVGLength rx;
	SPSVGLength ry;
};

struct _SPRectClass {
	SPShapeClass parent_class;
};


/* Standard Gtk function */
GtkType sp_rect_get_type (void);

void sp_rect_set (SPRect * rect, gdouble x, gdouble y, gdouble width, gdouble height);


END_GNOME_DECLS

#endif
