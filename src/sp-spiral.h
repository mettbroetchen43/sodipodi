#ifndef SP_SPIRAL_H
#define SP_SPIRAL_H

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
	 * x(t) = radius * t^expansion cos(2 * Pi * revolution*t + argument)
	 * y(t) = radius * t^expansion sin(2 * Pi * revolution*t + argument)
	 * where spiral curve is drawn for {t | t0 <= t <= 1}.
	 * radius and argument parameters can also be represented by
	 * transformation. shoud I remove these attributes?
	 */
	gdouble  cx, cy;              /* Spiral center (cx,xy) */
	gdouble  expansion;           /* Spiral expansion factor */
	gdouble  revolution;          /* Spiral revolution factor */
	gdouble  radius;              /* Spiral radius */
	gdouble  argument;            /* Spiral argument */
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
				 gdouble	expansion,
				 gdouble	revolution,
				 gdouble	radius,
				 gdouble	argument,
				 gdouble	t0);

void    sp_spiral_get_xy	(SPSpiral      *spiral,
				 gdouble	t,
				 ArtPoint      *p);

void    sp_spiral_build_repr	(SPSpiral      *spiral,
				 SPRepr	       *repr);


END_GNOME_DECLS

#endif
