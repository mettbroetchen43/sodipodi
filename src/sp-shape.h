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

#define SP_TYPE_SHAPE (sp_shape_get_type ())
#define SP_SHAPE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_SHAPE, SPShape))
#define SP_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SHAPE, SPShapeClass))
#define SP_IS_SHAPE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_SHAPE))
#define SP_IS_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SHAPE))

#define SP_SHAPE_WRITE_PATH (1 << 2)

#include "sp-path.h"

struct _SPShape {
	SPPath path;
};

struct _SPShapeClass {
	SPPathClass parent_class;

	/* build bpath from extra shape attributes */
	void (* set_shape) (SPShape *shape);
};


/* Standard Gtk function */
GtkType sp_shape_get_type (void);

/* Method */
void sp_shape_set_shape (SPShape *shape);

#endif
