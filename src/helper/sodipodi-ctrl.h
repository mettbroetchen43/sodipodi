#ifndef SODIPODI_CTRL_H
#define SODIPODI_CTRL_H

/* sodipodi-ctrl
 *
 * It is simply small square, which does not scale nor rotate
 *
 */

#include <gtk/gtkpacker.h>
#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_CTRL            (sp_ctrl_get_type ())
#define SP_CTRL(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CTRL, SPCtrl))
#define SP_CTRL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CTRL, SPCtrlClass))
#define SP_IS_CTRL(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRL))
#define SP_IS_CTRL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CTRL))


typedef struct _SPCtrl SPCtrl;
typedef struct _SPCtrlClass SPCtrlClass;

struct _SPCtrl {
	GnomeCanvasItem item;

	GtkAnchorType anchor;
	double size;			/* Width in canvas pixel coords */
	guint filled  : 1;
	guint stroked : 1;
	guint32 fill_color;
	guint32 stroke_color;
	ArtSVP * fill_svp;		/* The SVP for the filled shape */
	ArtSVP * stroke_svp;
};

struct _SPCtrlClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_ctrl_get_type (void);

void sp_ctrl_moveto (SPCtrl * ctrl, double x, double y);

END_GNOME_DECLS

#endif
