#ifndef SODIPODI_CTRLLINE_H
#define SODIPODI_CTRLLINE_H

/* sodipodi-ctrlline
 *
 * Simple straight line
 *
 */

#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_CTRLLINE            (sp_ctrlline_get_type ())
#define SP_CTRLLINE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CTRLLINE, SPCtrlLine))
#define SP_CTRLLINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CTRLLINE, SPCtrlLineClass))
#define SP_IS_CTRLLINE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRLLINE))
#define SP_IS_CTRLLINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CTRLLINE))


typedef struct _SPCtrlLine SPCtrlLine;
typedef struct _SPCtrlLineClass SPCtrlLineClass;

struct _SPCtrlLine {
	GnomeCanvasItem item;

	ArtPoint s, e;
	ArtSVP * svp;
};

struct _SPCtrlLineClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_ctrlline_get_type (void);

void sp_ctrlline_set_coords (SPCtrlLine * ctrlline, double x0, double y0, double x2, double y1);

END_GNOME_DECLS

#endif
