#ifndef SP_SHAPE_H
#define SP_SHAPE_H

#include <libart_lgpl/art_bpath.h>
#include "display/fill.h"
#include "display/stroke.h"
#include "sp-path.h"

#define SP_TYPE_SHAPE            (sp_shape_get_type ())
#define SP_SHAPE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SHAPE, SPShape))
#define SP_SHAPE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SHAPE, SPShapeClass))
#define SP_IS_SHAPE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SHAPE))
#define SP_IS_SHAPE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SHAPE))

struct _SPShape {
	SPPath path;
};

struct _SPShapeClass {
	SPPathClass parent_class;
};


/* Standard Gtk function */
GtkType sp_shape_get_type (void);

/* Utility */

#if 0
void sp_shape_clear (SPShape * shape);

void sp_shape_add_bpath (SPShape * shape, ArtBpath * bpath, double affine[]);
void sp_shape_add_bpath_identity (SPShape * shape, ArtBpath * bpath);

void sp_shape_set_bpath (SPShape * shape, ArtBpath * bpath, double affine[]);
void sp_shape_set_bpath_identity (SPShape * shape, ArtBpath * bpath);
#endif

#endif
