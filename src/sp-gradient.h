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

#include "style.h"
#include "sp-paint-server.h"

/*
 * Gradient Stop
 */

typedef struct _SPStop SPStop;
typedef struct _SPStopClass SPStopClass;

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
 * Linear Gradient
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

typedef struct _SPLinearGradient SPLinearGradient;
typedef struct _SPLinearGradientClass SPLinearGradientClass;

#define SP_TYPE_LINEARGRADIENT            (sp_lineargradient_get_type ())
#define SP_LINEARGRADIENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_LINEARGRADIENT, SPLinearGradient))
#define SP_LINEARGRADIENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_LINEARGRADIENT, SPLinearGradientClass))
#define SP_IS_LINEARGRADIENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_LINEARGRADIENT))
#define SP_IS_LINEARGRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_LINEARGRADIENT))

struct _SPLinearGradient {
	SPPaintServer paint_server;
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
	SPGradientSpread spread;
	guint spread_set : 1;
	SPLinearGradient *href;
	/* Gradient stops */
	SPStop *stops;
	/* Color array */
	guint32 color[256];
	/* Length of vector */
	gdouble len;
};

struct _SPLinearGradientClass {
	SPPaintServerClass parent_class;
};

GtkType sp_lineargradient_get_type (void);

#endif
