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
 * Copyright (C) 200-2001 Lauris Kaplinski and Ximian, Inc.
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
	SPStop *next;
	gdouble offset;
	guint32 color;
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

struct _SPGradient {
	SPPaintServer paint_server;
	SPGradientSpread spread;
	guint spread_set : 1;
	SPGradient *href;
	/* Gradient stops */
	SPObject *stops;
	/* Color array */
	guint32 color[1024];
	/* Length of vector */
	gdouble len;
};

struct _SPGradientClass {
	SPPaintServerClass parent_class;
};

GtkType sp_gradient_get_type (void);

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

#endif
