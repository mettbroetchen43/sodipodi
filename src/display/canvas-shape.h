#ifndef SP_CANVAS_SHAPE_H
#define SP_CANVAS_SHAPE_H

/*
 * Basic bezier shape item for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <libgnomeui/gnome-canvas.h>
#include "path-archetype.h"
#include "cpath-component.h"
#include "../forward.h"
#include "../sp-paint-server.h"

#define SP_TYPE_CANVAS_SHAPE            (sp_canvas_shape_get_type ())
#define SP_CANVAS_SHAPE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_SHAPE, SPCanvasShape))
#define SP_CANVAS_SHAPE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_SHAPE, SPCanvasShapeClass))
#define SP_IS_CANVAS_SHAPE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_SHAPE))
#define SP_IS_CANVAS_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_SHAPE))

typedef struct _SPCanvasShape SPCanvasShape;
typedef struct _SPCanvasShapeClass SPCanvasShapeClass;

struct _SPCanvasShape {
	GnomeCanvasItem item;
	SPStyle *style;
	GList * comp;
	gboolean sensitive;
	/* Paint server stuff */
	SPPainter *painter;
};

struct _SPCanvasShapeClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_canvas_shape_get_type (void);

/* Utility functions */

void sp_canvas_shape_clear (SPCanvasShape * canvas_shape);
void sp_canvas_shape_add_component (SPCanvasShape * canvas_shape, SPCurve * curve, gboolean private, gdouble affine[]);
void sp_canvas_shape_set_component (SPCanvasShape * canvas_shape, SPCurve * curve, gboolean private, gdouble affine[]);
/* NB! This works only for single component private shapes */
void sp_canvas_shape_change_bpath (SPCanvasShape * canvas_shape, SPCurve * curve);

#if 0
void sp_canvas_shape_set_fill (SPCanvasShape * shape, SPFill * fill);
void sp_canvas_shape_set_stroke (SPCanvasShape * shape, SPStroke * stroke);
#endif

void sp_canvas_shape_set_style (SPCanvasShape *shape, SPStyle *style);

void sp_canvas_shape_set_sensitive (SPCanvasShape * shape, gboolean sensitive);

#endif
