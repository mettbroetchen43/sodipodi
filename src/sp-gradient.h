#ifndef __SP_GRADIENT_H__
#define __SP_GRADIENT_H__

/*
 * SPGradient
 *
 * TODO: Implement radial & other fancy gradients
 * TODO: Implement linking attributes
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include "forward.h"
#include "style.h"
#include "sp-paint-server.h"

/*
 * Gradient Stop
 */

#define SP_TYPE_STOP            (sp_stop_get_type ())
#define SP_STOP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_STOP, SPStop))
#define SP_STOP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_STOP, SPStopClass))
#define SP_IS_STOP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_STOP))
#define SP_IS_STOP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_STOP))

struct _SPStop {
	SPObject object;
	gdouble offset;
	SPColor color;
	gfloat opacity;
};

struct _SPStopClass {
	SPObjectClass parent_class;
};

GtkType sp_stop_get_type (void);

/*
 * Gradient
 *
 * Implement spread, stops list
 * fixme: Implement more here (Lauris)
 */

typedef struct _SPGradientStop SPGradientStop;
typedef struct _SPGradientVector SPGradientVector;

struct _SPGradientStop {
	gdouble offset;
	SPColor color;
	gfloat opacity;
};

struct _SPGradientVector {
	gint nstops;
	gdouble start, end;
	SPGradientStop stops[1];
};

typedef enum {
	SP_GRADIENT_STATE_UNKNOWN,
	SP_GRADIENT_STATE_VECTOR,
	SP_GRADIENT_STATE_PRIVATE
} SPGradientState;

typedef enum {
	SP_GRADIENT_UNITS_USERSPACEONUSE,
	SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX
} SPGradientUnits;

typedef enum {
	SP_GRADIENT_SPREAD_PAD,
	SP_GRADIENT_SPREAD_REFLECT,
	SP_GRADIENT_SPREAD_REPEAT
} SPGradientSpread;

#define SP_TYPE_GRADIENT (sp_gradient_get_type ())
#define SP_GRADIENT(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_GRADIENT, SPGradient))
#define SP_GRADIENT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GRADIENT, SPGradientClass))
#define SP_IS_GRADIENT(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_GRADIENT))
#define SP_IS_GRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GRADIENT))

/* fixme: I am absolutely not sure, whether <use> may mess things up */
#define SP_GRADIENT_IS_PRIVATE(g) (SP_OBJECT_HREFCOUNT (g) == 1)

struct _SPGradient {
	SPPaintServer paint_server;
	/* Reference (href) */
	SPGradient *href;
	/* State in Sodipodi gradient system */
	guint state : 2;
	SPGradientSpread spread;
	guint spread_set : 1;
	/* Gradient stops */
	SPObject *stops;
	guint has_stops : 1;
	/* Composed vector */
	SPGradientVector *vector;
	/* Rendered color array (4 * 1024 bytes at moment) */
	guchar *color;
	/* Length of vector */
	gdouble len;
};

struct _SPGradientClass {
	SPPaintServerClass parent_class;
};

GtkType sp_gradient_get_type (void);

/* Forces vector to be built, if not present (i.e. changed) */

void sp_gradient_ensure_vector (SPGradient *gradient);

void sp_gradient_set_vector (SPGradient *gradient, SPGradientVector *vector);

/*
 * Renders gradient vector to buffer
 *
 * len, width, height, rowstride - buffer parameters (1 or 2 dimensional)
 * span - full integer width of requested gradient
 * pos - buffer starting position in span
 *
 * RGB buffer background should be set up before
 */
void sp_gradient_render_vector_line_rgba (SPGradient *gradient, guchar *buf, gint len, gint pos, gint span);
void sp_gradient_render_vector_line_rgb (SPGradient *gradient, guchar *buf, gint len, gint pos, gint span);
void sp_gradient_render_vector_block_rgba (SPGradient *gradient, guchar *buf, gint width, gint height, gint rowstride,
					   gint pos, gint span, gboolean horizontal);
void sp_gradient_render_vector_block_rgb (SPGradient *gradient, guchar *buf, gint width, gint height, gint rowstride,
					  gint pos, gint span, gboolean horizontal);

/*
 * Linear Gradient
 */

#define SP_TYPE_LINEARGRADIENT            (sp_lineargradient_get_type ())
#define SP_LINEARGRADIENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_LINEARGRADIENT, SPLinearGradient))
#define SP_LINEARGRADIENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_LINEARGRADIENT, SPLinearGradientClass))
#define SP_IS_LINEARGRADIENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_LINEARGRADIENT))
#define SP_IS_LINEARGRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_LINEARGRADIENT))

struct _SPLinearGradient {
	SPGradient gradient;
	SPGradientUnits units;
	guint units_set : 1;
	gdouble transform[6];
	guint transform_set : 1;
	SPDistance x1;
	guint x1_set : 1;
	SPDistance y1;
	guint y1_set : 1;
	SPDistance x2;
	guint x2_set : 1;
	SPDistance y2;
	guint y2_set : 1;
};

struct _SPLinearGradientClass {
	SPGradientClass parent_class;
};

GtkType sp_lineargradient_get_type (void);

void sp_lineargradient_set_position (SPLinearGradient *lg, gdouble x1, gdouble y1, gdouble x2, gdouble y2);

/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *sp_lineargradient_build_repr (SPLinearGradient *lg, gboolean vector);

/*
 * Radial Gradient
 */

#define SP_TYPE_RADIALGRADIENT (sp_radialgradient_get_type ())
#define SP_RADIALGRADIENT(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_RADIALGRADIENT, SPRadialGradient))
#define SP_RADIALGRADIENT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_RADIALGRADIENT, SPRadialGradientClass))
#define SP_IS_RADIALGRADIENT(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_RADIALGRADIENT))
#define SP_IS_RADIALGRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_RADIALGRADIENT))

struct _SPRadialGradient {
	SPGradient gradient;
	SPGradientUnits units;
	guint units_set : 1;
	gdouble transform[6];
	guint transform_set : 1;
	SPDistance cx;
	guint cx_set : 1;
	SPDistance cy;
	guint cy_set : 1;
	SPDistance r;
	guint r_set : 1;
	SPDistance fx;
	guint fx_set : 1;
	SPDistance fy;
	guint fy_set : 1;
};

struct _SPRadialGradientClass {
	SPGradientClass parent_class;
};

GtkType sp_radialgradient_get_type (void);

/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *sp_radialgradient_build_repr (SPRadialGradient *lg, gboolean vector);

#endif
