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
	guint dirty : 1;
	guint autoupdate : 1;
	Sodipodi *sodipodi;
	SPDesktop *desktop;
	SPDocument *document;
};

struct _SPWidgetClass {
	GtkBinClass bin_class;
	/* Selection change handlers */
	void (* modify_selection) (SPWidget *spw, SPSelection *selection, guint flags);
	void (* change_selection) (SPWidget *spw, SPSelection *selection);
	void (* set_selection) (SPWidget *spw, SPSelection *selection);
	/* Signal */
	void (* set_dirty) (SPWidget *spw, gboolean dirty);
};

GtkType sp_widget_get_type (void);

GtkWidget *sp_widget_new (Sodipodi *sodipodi, SPDesktop *desktop, SPDocument *document);
GtkWidget *sp_widget_construct (SPWidget *spw, Sodipodi *sodipodi, SPDesktop *desktop, SPDocument *document);

void sp_widget_set_dirty (SPWidget *spw, gboolean dirty);
void sp_widget_set_autoupdate (SPWidget *spw, gboolean autoupdate);

const GSList *sp_widget_get_item_list (SPWidget *spw);

END_GNOME_DECLS

#endif
