#ifndef SODIPODI_CTRLRECT_H
#define SODIPODI_CTRLRECT_H

/*
 * CtrlRect is basically outlined rectangle, which is immune to affine
 * transforms - i.e. it is always upright
 * In should be xored to canvas, although now we still use libart.
 * Outline width is always - you guess it - in pixels ;-)
 */

#include <libart_lgpl/art_rect.h>
#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_CTRLRECT            (sp_ctrlrect_get_type ())
#define SP_CTRLRECT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CTRLRECT, SPCtrlRect))
#define SP_CTRLRECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CTRLRECT, SPCtrlRectClass))
#define SP_IS_CTRLRECT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRLRECT))
#define SP_IS_CTRLRECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CTRLRECT))


typedef struct _SPCtrlRect SPCtrlRect;
typedef struct _SPCtrlRectClass SPCtrlRectClass;

struct _SPCtrlRect {
	GnomeCanvasItem item;

	ArtDRect rect;		/* Dimensions */
	
	double width;		/* Line width */

	ArtSVP * svp;		/* The SVP for the filled shape */
	ArtSVP * rdsvp;		/* SVP of redraw region :-( */
};

struct _SPCtrlRectClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_ctrlrect_get_type (void);

void
sp_ctrlrect_set_rect (SPCtrlRect * rect, ArtDRect * box);

END_GNOME_DECLS

#endif
