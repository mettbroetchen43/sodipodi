#ifndef __SP_ICON_H__
#define __SP_ICON_H__

/*
 * Generic icon widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _SPIcon SPIcon;
typedef struct _SPIconClass SPIconClass;

#define SP_TYPE_ICON (sp_icon_get_type ())
#define SP_ICON(o) (GTK_CHECK_CAST ((o), SP_TYPE_ICON, SPIcon))
#define SP_IS_ICON(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ICON))

enum {
	SP_ICON_INVALID,
	SP_ICON_BUTTON,
	SP_ICON_MENU,
	SP_ICON_TITLEBAR,
	SP_ICON_NOTEBOOK
};

#include <gtk/gtkwidget.h>

struct _SPIcon {
	GtkWidget widget;

	unsigned int size;

	unsigned char *px;
};

struct _SPIconClass {
	GtkWidgetClass parent_class;
};

GType sp_icon_get_type (void);

GtkWidget *sp_icon_new (unsigned int type, const unsigned char *name);

#endif
