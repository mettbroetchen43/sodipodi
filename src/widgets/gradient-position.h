#ifndef __SP_GRADIENT_POSITION_H__
#define __SP_GRADIENT_POSITION_H__

/*
 * SPGradientPosition
 *
 * A simple preview for gradient vector
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 2001
 *
 */

#include <gtk/gtkwidget.h>
#include "../helper/nr-gradient.h"
#include "../sp-gradient.h"

typedef struct _SPGradientPosition SPGradientPosition;
typedef struct _SPGradientPositionClass SPGradientPositionClass;

#define SP_TYPE_GRADIENT_POSITION (sp_gradient_position_get_type ())
#define SP_GRADIENT_POSITION(o) (GTK_CHECK_CAST ((o), SP_TYPE_GRADIENT_POSITION, SPGradientPosition))
#define SP_GRADIENT_POSITION_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_GRADIENT_POSITION, SPGradientPositionClass))
#define SP_IS_GRADIENT_POSITION(o) (GTK_CHECK_TYPE ((o), SP_TYPE_GRADIENT_POSITION))
#define SP_IS_GRADIENT_POSITION_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_GRADIENT_POSITION))

struct _SPGradientPosition {
	GtkWidget widget;
	guint need_update : 1;
	guint dragging : 1;
	guint position : 2;
	SPGradient *gradient;
	ArtDRect bbox; /* BBox in format, expected by gradient */
	ArtDRect vbox; /* BBox in widget coordinates */
	ArtPoint p0, p1, pold;
	gdouble transform[6];
	GdkGC *gc;
	GdkPixmap *px;

	NRGradientSpreadType spread;
	NRLGradientRenderer *lgr;
};

struct _SPGradientPositionClass {
	GtkWidgetClass parent_class;

	void (* grabbed) (SPGradientPosition *pos);
	void (* dragged) (SPGradientPosition *pos);
	void (* released) (SPGradientPosition *pos);
	void (* changed) (SPGradientPosition *pos);

};

GtkType sp_gradient_position_get_type (void);

GtkWidget *sp_gradient_position_new (SPGradient *gradient);
/* Set vector gradient vector widget */
void sp_gradient_position_set_gradient (SPGradientPosition *pos, SPGradient *gradient);

void sp_gradient_position_set_bbox (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1);
void sp_gradient_position_set_vector (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1);
void sp_gradient_position_set_transform (SPGradientPosition *pos, gdouble transform[]);
void sp_gradient_position_set_spread (SPGradientPosition *pos, NRGradientSpreadType spread);

void sp_gradient_position_get_position_floatv (SPGradientPosition *gp, gfloat *pos);


#endif
