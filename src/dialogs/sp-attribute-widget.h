#ifndef __SP_ATTRIBUTE_ATTRIBUTE_WIDGET_H__
#define __SP_ATTRIBUTE_ATTRIBUTE_WIDGET_H__

/*
 * SPAttributeWidget
 *
 * Widget, that listens and modifies repr attributes
 *
 * Authors:
 *  Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_ATTRIBUTE_WIDGET (sp_attribute_widget_get_type ())
#define SP_ATTRIBUTE_WIDGET(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ATTRIBUTE_WIDGET, SPAttributeWidget))
#define SP_ATTRIBUTE_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ATTRIBUTE_WIDGET, SPAttributeWidgetClass))
#define SP_IS_ATTRIBUTE_WIDGET(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ATTRIBUTE_WIDGET))
#define SP_IS_ATTRIBUTE_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ATTRIBUTE_WIDGET))

#define SP_TYPE_ATTRIBUTE_TABLE (sp_attribute_table_get_type ())
#define SP_ATTRIBUTE_TABLE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ATTRIBUTE_TABLE, SPAttributeTable))
#define SP_ATTRIBUTE_TABLE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ATTRIBUTE_TABLE, SPAttributeTableClass))
#define SP_IS_ATTRIBUTE_TABLE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ATTRIBUTE_TABLE))
#define SP_IS_ATTRIBUTE_TABLE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ATTRIBUTE_TABLE))

typedef struct _SPAttributeWidget SPAttributeWidget;
typedef struct _SPAttributeWidgetClass SPAttributeWidgetClass;

typedef struct _SPAttributeTable SPAttributeTable;
typedef struct _SPAttributeTableClass SPAttributeTableClass;

#include <gtk/gtkentry.h>
#include <gtk/gtkvbox.h>
#include "../forward.h"

struct _SPAttributeWidget {
	GtkEntry entry;
	guint blocked : 1;
	SPObject *object;
	guchar *attribute;
};

struct _SPAttributeWidgetClass {
	GtkEntryClass entry_class;
};

GtkType sp_attribute_widget_get_type (void);

GtkWidget *sp_attribute_widget_new (SPObject *object, const guchar *attribute);
void sp_attribute_widget_set_object (SPAttributeWidget *spw, SPObject *object, const guchar *attribute);

/* SPAttributeTable */

struct _SPAttributeTable {
	GtkVBox vbox;
	guint blocked : 1;
	GtkWidget *table;
	SPObject *object;
	gint num_attr;
	guchar **attributes;
	GtkWidget **entries;
};

struct _SPAttributeTableClass {
	GtkEntryClass entry_class;
};

GtkType sp_attribute_table_get_type (void);

GtkWidget *sp_attribute_table_new (SPObject *object, gint num_attr, const guchar **labels, const guchar **attributes);
void sp_attribute_table_set_object (SPAttributeTable *spw, SPObject *object, gint num_attr, const guchar **labels, const guchar **attrs);

END_GNOME_DECLS

#endif
