#ifndef __SP_DESKTOP_H__
#define __SP_DESKTOP_H__

/*
 * Editable view and widget implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#define SP_TYPE_DESKTOP (sp_desktop_get_type ())
#define SP_DESKTOP(o) (GTK_CHECK_CAST ((o), SP_TYPE_DESKTOP, SPDesktop))
#define SP_DESKTOP_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_DESKTOP, SPDesktopClass))
#define SP_IS_DESKTOP(o) (GTK_CHECK_TYPE ((o), SP_TYPE_DESKTOP))
#define SP_IS_DESKTOP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_DESKTOP))

typedef struct _SPDesktopWidget SPDesktopWidget;
typedef struct _SPDesktopWidgetClass SPDesktopWidgetClass;

#define SP_TYPE_DESKTOP_WIDGET (sp_desktop_widget_get_type ())
#define SP_DESKTOP_WIDGET(o) (GTK_CHECK_CAST ((o), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidget))
#define SP_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidgetClass))
#define SP_IS_DESKTOP_WIDGET(o) (GTK_CHECK_TYPE ((o), SP_TYPE_DESKTOP_WIDGET))
#define SP_IS_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_DESKTOP_WIDGET))

#include <libgnomeui/gnome-canvas.h>
#include "forward.h"
#include "view.h"

struct _SPDesktop {
	SPView view;

	SPDesktopWidget *owner;

	SPNamedView * namedview;
	SPSelection * selection;
	SPEventContext * event_context;

	GnomeCanvasItem * acetate;
	GnomeCanvasGroup * main;
	GnomeCanvasGroup * grid;
  	GnomeCanvasGroup * guides;
	GnomeCanvasGroup * drawing;
	GnomeCanvasGroup * sketch;
	GnomeCanvasGroup * controls;
	GnomeCanvasItem * page;
	gdouble d2w[6], w2d[6], doc2dt[6];
        gint number;
};

struct _SPDesktopClass {
	GtkEventBoxClass parent_class;

	void (* modified) (SPDesktop *desktop, guint flags);
};

#ifndef __SP_DESKTOP_C__
extern gboolean SPShowFullFielName;
#else
gboolean SPShowFullFielName = TRUE;
#endif

GtkType sp_desktop_get_type (void);

/* Constructor */

SPView *sp_desktop_new (SPDesktopWidget *widget);

/* Show/hide rulers & scrollbars */

void sp_desktop_show_decorations (SPDesktop *desktop, gboolean show);
void sp_desktop_activate_guides (SPDesktop *desktop, gboolean activate);

void sp_desktop_change_document (SPDesktop *desktop, SPDocument * document);

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

// statusbars
void sp_desktop_default_status (SPDesktop *desktop, const gchar * text);
void sp_desktop_set_status (SPDesktop * desktop, const gchar * stat);
void sp_desktop_clear_status (SPDesktop * desktop);
void sp_desktop_coordinate_status (SPDesktop * desktop, gdouble x, gdouble y, gint8 underline);

#include <gtk/gtkscrollbar.h>
#include <gtk/gtkruler.h>
#include <libgnomeui/gnome-appbar.h>

struct _SPDesktopWidget {
	SPViewWidget viewwidget;

	SPDocument *document;
	SPNamedView *namedview;

	SPDesktop *desktop;

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

	GnomeCanvas *canvas;
};

struct _SPDesktopWidgetClass {
	SPViewWidgetClass parent_class;
};

GtkType sp_desktop_widget_get_type (void);

/* Constructor */

SPViewWidget *sp_desktop_widget_new (SPDocument *document, SPNamedView *namedview);

#endif
