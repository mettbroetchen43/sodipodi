#ifndef __SP_SHAPE_H__
#define __SP_SHAPE_H__

/*
 * Base class for shapes, including <path> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "helper/curve.h"

#include "sp-item.h"

#define SP_TYPE_SHAPE (sp_shape_get_type ())
#define SP_SHAPE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_SHAPE, SPShape))
#define SP_SHAPE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_SHAPE, SPShapeClass))
#define SP_IS_SHAPE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_SHAPE))
#define SP_IS_SHAPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_SHAPE))

#define SP_SHAPE_WRITE_PATH (1 << 2)

struct _SPShape {
	SPItem item;

	SPCurve *curve;
};

struct _SPShapeClass {
	SPItemClass item_class;

	/* Build bpath from extra shape attributes */
	void (* set_shape) (SPShape *shape);
};

unsigned int sp_shape_get_type (void);

void sp_shape_set_shape (SPShape *shape);

/* Return duplicate of curve or NULL */
SPCurve *sp_shape_get_curve (SPShape *shape);

void sp_shape_set_curve (SPShape *shape, SPCurve *curve, unsigned int owner);

/* NOT FOR GENERAL PUBLIC UNTIL SORTED OUT (Lauris) */
void sp_shape_set_curve_insync (SPShape *shape, SPCurve *curve, unsigned int owner);

#endif
