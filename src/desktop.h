#ifndef SP_DESKTOP_H
#define SP_DESKTOP_H

/*
 * SPDesktop
 *
 * A per view object
 *
 */

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include "application.h"
#include "document.h"
#include "selection.h"

#define SP_TYPE_DESKTOP            (sp_desktop_get_type ())
#define SP_DESKTOP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DESKTOP, SPDesktop))
#define SP_DESKTOP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DESKTOP, SPDesktopClass))
#define SP_IS_DESKTOP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DESKTOP))
#define SP_IS_DESKTOP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DESKTOP))

typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;

struct _SPDesktop {
	GtkObject object;
	SPApp * app;
	SPDocument * document;
	SPSelection * selection;
	GtkWidget * window;
	GtkWidget * statusbar;
	GtkScrollbar * hscrollbar, * vscrollbar;
	GtkRuler * hruler, * vruler;
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
	GtkObjectClass parent_class;
};

/* Standard Gtk function */

GtkType sp_desktop_get_type (void);

/* Constructor */

SPDesktop * sp_desktop_new (SPApp * app, SPDocument * document);

/* Utility macros */

#define SP_WIDGET_DESKTOP(w) sp_desktop_widget_desktop(w)
#define SP_CANVAS_ITEM_DESKTOP(i) sp_desktop_widget_desktop(GTK_WIDGET(i->canvas))


#define SP_DT_DOCUMENT(d) (SP_DESKTOP(d)->document)
#define SP_DT_SELECTION(d) (SP_DESKTOP(d)->selection)
#define SP_DT_CANVAS(d) (SP_DESKTOP(d)->canvas)
#define SP_DT_ACETATE(d) (SP_DESKTOP(d)->acetate)
#define SP_DT_MAIN(d) (SP_DESKTOP(d)->main)
#define SP_DT_GRID(d) (SP_DESKTOP(d)->grid)
#define SP_DT_GUIDES(d) (SP_DESKTOP(d)->guides)
#define SP_DT_DRAWING(d) (SP_DESKTOP(d)->drawing)
#define SP_DT_SKETCH(d) (SP_DESKTOP(d)->sketch)
#define SP_DT_CONTROLS(d) (SP_DESKTOP(d)->controls)

SPDesktop * sp_desktop_widget_desktop (GtkWidget * widget);

/* Zooming, viewport, position & similar */

void sp_desktop_set_position (SPDesktop * desktop, gdouble x, gdouble y);
void sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy);
void sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1);

#if 0

void sp_desktop_set_position (double x, double y);

void sp_desktop_zoom (double zoom, double x, double y);
void sp_desktop_show_region (double x0, double y0, double x1, double y1);

void sp_w2d (ArtPoint * d, ArtPoint * s);

/* FIXME reimplement this somehow */

void sp_desktop_set_title (const gchar * title);
void sp_desktop_set_status (const gchar * text);

#endif

#endif
