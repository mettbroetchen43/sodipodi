#ifndef _SP_WIDGET_H_
#define _SP_WIDGET_H_

/*
 * SPWidget
 *
 * Abstract base class for sodipodi property control widgets
 *
 * Authors:
 *  Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_WIDGET (sp_widget_get_type ())
#define SP_WIDGET(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_WIDGET, SPWidget))
#define SP_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_WIDGET, SPWidgetClass))
#define SP_IS_WIDGET(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_WIDGET))
#define SP_IS_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_WIDGET))

typedef struct _SPWidget SPWidget;
typedef struct _SPWidgetClass SPWidgetClass;

#include <gtk/gtkbin.h>
#include "../forward.h"

struct _SPWidget {
	GtkBin bin;
	Sodipodi *sodipodi;
	SPDesktop *desktop;
	SPDocument *document;
	guint change_selection_id;
	guint set_selection_id;
};

struct _SPWidgetClass {
	GtkBinClass bin_class;
	void (* change_selection) (SPWidget *spw, SPSelection *selection);
	void (* set_selection) (SPWidget *spw, SPSelection *selection);
};

GtkType sp_widget_get_type (void);

SPWidget *sp_widget_construct (SPWidget *spw, Sodipodi *sodipodi, SPDesktop *desktop, SPDocument *document);

END_GNOME_DECLS

#endif
