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

#ifndef SP_DESKTOP_DEFINED
#define SP_DESKTOP_DEFINED
typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;
#endif

#define SP_TYPE_DESKTOP            (sp_desktop_get_type ())
#define SP_DESKTOP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DESKTOP, SPDesktop))
#define SP_DESKTOP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DESKTOP, SPDesktopClass))
#define SP_IS_DESKTOP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DESKTOP))
#define SP_IS_DESKTOP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DESKTOP))

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-appbar.h>
#include "document.h"
#include "selection.h"
#include "event-context.h"

struct _SPDesktop {
	GtkEventBox eventbox;
	guint decorations :1;
	GtkTable * table;
	GtkScrollbar * hscrollbar;
	GtkScrollbar * vscrollbar;
	GtkRuler * hruler;
	GtkRuler * vruler;
        GtkWidget * active;
        GtkWidget * inactive;

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
	gdouble d2w[6], w2d[6];
};

struct _SPDesktopClass {
	GtkEventBoxClass parent_class;
};

/* Standard Gtk function */

GtkType sp_desktop_get_type (void);

/* Constructor */

SPDesktop * sp_desktop_new (SPDocument * document, SPNamedView * namedview);

/* Show/hide rulers & scrollbars */

void sp_desktop_show_decorations (SPDesktop * desktop, gboolean show);

void sp_desktop_change_document (SPDesktop * desktop, SPDocument * document);

/* Zooming, viewport, position & similar */

void sp_desktop_set_position (SPDesktop * desktop, gdouble x, gdouble y);
void sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy);
ArtDRect * sp_desktop_get_visible_area (SPDesktop * desktop, ArtDRect * area);
void sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1);
void sp_desktop_zoom_relative (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);
void sp_desktop_zoom_absolute (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);
/* Context */

void sp_desktop_set_event_context (SPDesktop * desktop, GtkType type);

#if 0

/* FIXME reimplement this somehow */

void sp_desktop_set_title (const gchar * title);

#endif

void sp_desktop_set_status (SPDesktop *desktop, const gchar * text);

#endif
