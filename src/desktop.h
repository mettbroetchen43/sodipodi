#ifndef __SP_DESKTOP_H__
#define __SP_DESKTOP_H__

/*
 * Editable view and widget implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 *
 */

typedef struct _SPDesktopWidget SPDesktopWidget;
typedef struct _SPDesktopWidgetClass SPDesktopWidgetClass;

#define SP_TYPE_DESKTOP_WIDGET (sp_desktop_widget_get_type ())
#define SP_DESKTOP_WIDGET(o) (GTK_CHECK_CAST ((o), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidget))
#define SP_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidgetClass))
#define SP_IS_DESKTOP_WIDGET(o) (GTK_CHECK_TYPE ((o), SP_TYPE_DESKTOP_WIDGET))
#define SP_IS_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_DESKTOP_WIDGET))

#include "helper/sp-canvas.h"
#include "forward.h"
#include "view.h"

struct _SPDesktop {
	SPView view;

#if 1
	SPDesktopWidget *owner;
#endif

	SPNamedView *namedview;
	SPSelection *selection;
	SPEventContext *event_context;

	GnomeCanvasItem *acetate;
	GnomeCanvasGroup *main;
	GnomeCanvasGroup *grid;
  	GnomeCanvasGroup *guides;
	GnomeCanvasGroup *drawing;
	GnomeCanvasGroup *sketch;
	GnomeCanvasGroup *controls;
	GnomeCanvasItem *page;
	gdouble d2w[6], w2d[6], doc2dt[6];

        gint number;
	gboolean active;
	/* Normalized snap distances */
	gdouble gridsnap;
	gdouble guidesnap;
	/* fixme: This has to be implemented in different way */
	guint guides_active : 1;
};

struct _SPDesktopClass {
	GtkEventBoxClass parent_class;

	void (* activate) (SPDesktop *desktop);
	void (* desactivate) (SPDesktop *desktop);
	void (* modified) (SPDesktop *desktop, guint flags);
};

void sp_desktop_set_active (SPDesktop *desktop, gboolean active);

#ifndef __SP_DESKTOP_C__
extern gboolean SPShowFullFielName;
#else
gboolean SPShowFullFielName = TRUE;
#endif

/* Show/hide rulers & scrollbars */

void sp_desktop_activate_guides (SPDesktop *desktop, gboolean activate);

void sp_desktop_change_document (SPDesktop *desktop, SPDocument * document);

/* Zooming, viewport, position & similar */
#define SP_DESKTOP_SCROLL_LIMIT 10000.0
#define SP_DESKTOP_ZOOM_INC     1.5
#define SP_DESKTOP_ZOOM_MAX     10.0
#define SP_DESKTOP_ZOOM_MIN     0.01

gdouble sp_desktop_zoom_factor (SPDesktop * desktop);

void sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy);
ArtDRect *sp_desktop_get_visible_area (SPDesktop * desktop, ArtDRect * area);
void sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint border);
void sp_desktop_zoom_relative (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);
void sp_desktop_zoom_absolute (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy);

/* fixme: */
void sp_desktop_toggle_borders (GtkWidget * widget);

/* Context */
void sp_desktop_set_event_context (SPDesktop *desktop, GtkType type, const guchar *config);

#define SP_COORDINATES_UNDERLINE_X (1 << 0)
#define SP_COORDINATES_UNDERLINE_Y (1 << 1)

void sp_desktop_set_coordinate_status (SPDesktop *desktop, gdouble x, gdouble y, guint underline);

struct _SPDesktopWidget {
	SPViewWidget viewwidget;

	SPDesktop *desktop;

	gint decorations : 1;
	GtkWidget *table;
	GtkWidget *hscrollbar, *vscrollbar;

	/* Rulers */
	GtkWidget *hruler, *vruler;
	double dt2r;
	double rx0, ry0;

        GtkWidget *active;
        GtkWidget *inactive;
        GtkWidget *menubutton;   
        GtkWidget *select_status, *coord_status;

        gint coord_status_id, select_status_id;
        GtkWidget *zoom;

	GnomeCanvas *canvas;
};

struct _SPDesktopWidgetClass {
	SPViewWidgetClass parent_class;
};

GtkType sp_desktop_widget_get_type (void);

/* Constructor */

SPViewWidget *sp_desktop_widget_new (SPNamedView *namedview);

gint sp_desktop_widget_set_focus (GtkWidget *widget, GdkEvent *event, SPDesktopWidget  *dtw);

void sp_desktop_widget_show_decorations (SPDesktopWidget *dtw, gboolean show);

#endif
