#ifndef __SP_CANVAS_GRID_H__
#define __SP_CANVAS_GRID_H__

/*
 * Infinite visual grid
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "sp-canvas.h"

G_BEGIN_DECLS

#define SP_TYPE_CGRID            (sp_cgrid_get_type ())
#define SP_CGRID(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CGRID, SPCGrid))
#define SP_CGRID_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CGRID, SPCGridClass))
#define SP_IS_CGRID(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CGRID))
#define SP_IS_CGRID_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CGRID))


typedef struct _SPCGrid SPCGrid;
typedef struct _SPCGridClass SPCGridClass;

struct _SPCGrid {
	SPCanvasItem item;

	NRPointF origin;
	NRPointF spacing;
	guint32 color;

	NRPointF ow, sw;
};

struct _SPCGridClass {
	SPCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_cgrid_get_type (void);

G_END_DECLS

#endif
