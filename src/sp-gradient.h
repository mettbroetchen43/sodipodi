#ifndef SP_GRADIENT_H
#define SP_GRADIENT_H

/*
 * SPGradient
 *
 * TODO: Implement radial & other fancy gradients
 * TODO: Implement linking attributes
 *
 * Copyright (C) Lauris Kaplinski 2000
 * Released under GNU GPL
 */

#include "sp-object.h"

typedef enum {
	SP_GRADIENT_UNITS_USERSPACE,
	SP_GRADIENT_UNITS_USERSPACEONUSE,
	SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX
} SPGradientUnits;

typedef enum {
	SP_GRADIENT_SPREAD_PAD,
	SP_GRADIENT_SPREAD_REFLECT,
	SP_GRADIENT_SPREAD_REPEAT
} SPGradientSpread;

/*
 * Gradient Stop
 */

typedef struct _SPStop SPStop;
typedef struct _SPStopClass SPStopClass;

#define SP_TYPE_STOP            (sp_lineargradient_get_type ())
#define SP_STOP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_STOP, SPStop))
#define SP_STOP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_STOP, SPStopClass))
#define SP_IS_STOP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_STOP))
#define SP_IS_STOP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_STOP))

struct _SPStop {
	SPObject object;
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

typedef struct _SPLinearGradient SPLinearGradient;
typedef struct _SPLinearGradientClass SPLinearGradientClass;

#define SP_TYPE_LINEARGRADIENT            (sp_lineargradient_get_type ())
#define SP_LINEARGRADIENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_LINEARGRADIENT, SPLinearGradient))
#define SP_LINEARGRADIENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_LINEARGRADIENT, SPLinearGradientClass))
#define SP_IS_LINEARGRADIENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_LINEARGRADIENT))
#define SP_IS_LINEARGRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_LINEARGRADIENT))

struct _SPLinearGradient {
	SPObject object;
	SPGradientUnits units;
	gdouble transform[6];
	ArtDRect box;
	SPGradientSpread spread;
	GSList * stops;
};

struct _SPLinearGradientClass {
	SPObjectClass parent_class;
};

GtkType sp_lineargradient_get_type (void);

#endif
