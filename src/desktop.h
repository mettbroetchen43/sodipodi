#ifndef SP_DESKTOP_H
#define SP_DESKTOP_H

/*
 * SPDesktop
 *
 * A per view drawing object which implements:
 *   - scrollbars
 *   - postscript coordinates
 *   - comfortable canvas groups
 *
 */

#define SP_TYPE_DESKTOP            (sp_desktop_get_type ())
#define SP_DESKTOP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DESKTOP, SPDesktop))
#define SP_DESKTOP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DESKTOP, SPDesktopClass))
#define SP_IS_DESKTOP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DESKTOP))
#define SP_IS_DESKTOP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DESKTOP))

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-appbar.h>
#include "forward.h"
#include "sp-namedview.h"

struct _SPDesktop {
	GtkEventBox eventbox;
	gint decorations : 1;
	guint guides_active : 1;
	GtkBox * table;
	GtkScrollbar * hscrollbar;
	GtkScrollbar * vscrollbar;
	GtkRuler * hruler;
	GtkRuler * vruler;
        GtkWidget * active;
        GtkWidget * inactive;
        GtkWidget * menubutton;   
        GnomeAppBar * select_status, * coord_status;

        gint coord_status_id, select_status_id;
        GtkWidget * zoom;

	SPDocument * document;
	SPNamedView * namedview;
	SPSelection * selection;
	SPEventContext * event_context;
	GnomeCanvas * canvas;
	GnomeCanvasItem * acetate;
	GnomeCanvasGroup * main;
	GnomeCanvasGroup * grid;
  	GnomeCanvasGroup * guides;
	GnomeCanvasGroup * drawing;
	GnomeCanvasGroup * sketch;
	GnomeCanvasGroup * controls;
	GnomeCanvasItem * page;
	gdouble d2w[6], w2d[6];
        gint number;
};

struct _SPDesktopClass {
	GtkEventBoxClass parent_class;
};

#ifndef SP_DESKTOP_C
extern gboolean SPShowFullFielName;
#else
gboolean SPShowFullFielName = TRUE;
#endif




/* Standard Gtk function */

GtkType sp_desktop_get_type (void);

/* Constructor */

SPDesktop * sp_desktop_new (SPDocument * document, SPNamedView * namedview);

/* Show/hide rulers & scrollbars */

void sp_desktop_show_decorations (SPDesktop * desktop, gboolean show);
void sp_desktop_activate_guides (SPDesktop * desktop, gboolean activate);

void sp_desktop_change_document (SPDesktop * desktop, SPDocument * document);

/* Zooming, viewport, position & similar */
#define SP_DESKTOP_SCROLL_LIMIT 10000.0
#define SP_DESKTOP_ZOOM_INC     1.5
#define SP_DESKTOP_ZOOM_MAX     10.0
#define SP_DESKTOP_ZOOM_MIN     0.01

gdouble sp_desktop_zoom_factor (SPDesktop * desktop);
void sp_desktop_set_position (SPDesktop * desktop, gdouble x, gdouble y);
void sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy);
ArtDRect * sp_desktop_get_visible_area (SPDesktop * desktop, ArtDRect * area);
void sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint border);
void sp_desktop_zoom_relative (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);
void sp_desktop_zoom_absolute (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);
gint sp_desktop_set_focus (GtkWidget * widget, GtkWidget * widget2, SPDesktop * desktop);

/* Context */

void sp_desktop_set_event_context (SPDesktop * desktop, GtkType type);

void sp_desktop_set_title (SPDesktop * desktop);


// statusbars
void sp_desktop_default_status (SPDesktop *desktop, const gchar * text);
void sp_desktop_set_status (SPDesktop * desktop, const gchar * stat);
void sp_desktop_clear_status (SPDesktop * desktop);
void sp_desktop_coordinate_status (SPDesktop * desktop, gdouble x, gdouble y, gint8 underline);


void sp_desktop_connect_item (SPDesktop * desktop, SPItem * item, GnomeCanvasItem * canvasitem);

#endif
