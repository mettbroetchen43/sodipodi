#ifndef __SP_SPIRAL_H__
#define __SP_SPIRAL_H__

/*
 * SPStar
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Mitsuru Oka
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include "sp-shape.h"

BEGIN_GNOME_DECLS


#define SP_TYPE_SPIRAL            (sp_spiral_get_type ())
#define SP_SPIRAL(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SPIRAL, SPSpiral))
#define SP_SPIRAL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SPIRAL, SPSpiralClass))
#define SP_IS_SPIRAL(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SPIRAL))
#define SP_IS_SPIRAL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SPIRAL))

typedef struct _SPSpiral SPSpiral;
typedef struct _SPSpiralClass SPSpiralClass;

struct _SPSpiral {
	SPShape shape;
	
	/*
	 * Spiral shape is defined as:
	 * x(t) = rad * t^exp cos(2 * Pi * revo*t + arg) + cx
	 * y(t) = rad * t^exp sin(2 * Pi * revo*t + arg) + cy
	 * where spiral curve is drawn for {t | t0 <= t <= 1}.
	 * rad and arg parameters can also be represented by
	 * transformation. shoud I remove these attributes?
	 */
	gdouble  cx, cy;
	gdouble  exp;           /* Spiral expansion factor */
	gdouble  revo;		/* Spiral revolution factor */
	gdouble  rad;		/* Spiral radius */
	gdouble  arg;		/* Spiral argument */
	gdouble  t0;
};

struct _SPSpiralClass {
	SPShapeClass parent_class;
};


/* Standard Gtk function */
GtkType sp_spiral_get_type  (void);

/* Lowlevel interface */
void    sp_spiral_set		(SPSpiral      *spiral,
				 gdouble	cx,
				 gdouble	cy,
				 gdouble	exp,
				 gdouble	revo,
				 gdouble	rad,
				 gdouble	arg,
				 gdouble	t0);

void    sp_spiral_get_xy	(SPSpiral      *spiral,
				 gdouble	t,
				 ArtPoint      *p);
void    sp_spiral_get_polar	(SPSpiral      *spiral,
				 gdouble	t,
				 gdouble       *rad,
				 gdouble       *arg);
gboolean sp_spiral_is_invalid   (SPSpiral      *spiral);


END_GNOME_DECLS

#endif
