#ifndef SP_CANVAS_SHAPE_H
#define SP_CANVAS_SHAPE_H

/*
 * CanvasShape
 *
 * A multiline, multicomponent shape, filled and/or stroked
 *
 */

#include <libgnomeui/gnome-canvas.h>
#include "path-archetype.h"
#include "cpath-component.h"
#include "fill.h"
#include "stroke.h"

#define SP_TYPE_CANVAS_SHAPE            (sp_canvas_shape_get_type ())
#define SP_CANVAS_SHAPE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_SHAPE, SPCanvasShape))
#define SP_CANVAS_SHAPE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_SHAPE, SPCanvasShapeClass))
#define SP_IS_CANVAS_SHAPE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_SHAPE))
#define SP_IS_CANVAS_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_SHAPE))

typedef struct _SPCanvasShape SPCanvasShape;
typedef struct _SPCanvasShapeClass SPCanvasShapeClass;

struct _SPCanvasShape {
	GnomeCanvasItem item;
	SPFill * fill;
	SPStroke * stroke;
	GList * comp;
	gboolean sensitive;
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

void sp_canvas_shape_set_fill (SPCanvasShape * shape, SPFill * fill);
void sp_canvas_shape_set_stroke (SPCanvasShape * shape, SPStroke * stroke);

void sp_canvas_shape_set_sensitive (SPCanvasShape * shape, gboolean sensitive);

#endif
