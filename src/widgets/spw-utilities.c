#define __SPW_UTILITIES_C__

/* 
 * Sodipodi Widget Utilities
 * 
 * Authors:
 *   Bryce W. Harrington <brycehar@bryceharrington.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 * 
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <libnr/nr-macros.h>

#include <gtk/gtksignal.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtklabel.h>

#include <glib.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhscale.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkframe.h>
 
#include "xml/repr-private.h"

#include "helper/sp-intl.h"
#include "helper/window.h"
#include "widgets/button.h"
#include "sodipodi.h"
#include "document.h"
#include "desktop-handles.h"
#include "sp-item-transform.h"
#include "selection.h"

#include "helper/unit-menu.h"
#include "spw-utilities.h"

void
spw_checkbutton (GtkWidget *dialog, GtkWidget *table, const unsigned char *label, const unsigned char *key,
		 int col, int row, int sensitive, GCallback cb)
{
	GtkWidget *b;
	b = gtk_check_button_new_with_label (label);
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (table), b, col, col+1, row, row+1,
			  GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", (gpointer) key);
	gtk_object_set_data (GTK_OBJECT (dialog), key, b);
	g_signal_connect (G_OBJECT (b), "toggled",
			  cb, dialog);
	if (sensitive) {
		gtk_widget_set_sensitive (b, FALSE);
	}
}

void
spw_dropdown(GtkWidget * dialog, GtkWidget * t,
	     const guchar * label, guchar * key, int row,
	     GtkWidget * selector
	     )
{
  GtkWidget * l;
  l = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
  gtk_widget_show (l);
  gtk_table_attach (GTK_TABLE (t), l, 0, 1, row, row+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (selector);
  gtk_table_attach (GTK_TABLE (t), selector, 1, 2, row, row+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_object_set_data (GTK_OBJECT (dialog), key, selector);
}

void
spw_unit_selector(GtkWidget * dialog, GtkWidget * t,
		  const guchar * label, guchar * key, int row,
		  GtkWidget * us, GCallback cb)
{
  GtkWidget * l, * sb;
  GtkObject * a;
  l = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
  gtk_widget_show (l);
  gtk_table_attach (GTK_TABLE (t), l, 0, 1, row, row+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
  gtk_object_set_data (GTK_OBJECT (a), "key", key);
  gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
  gtk_object_set_data (GTK_OBJECT (dialog), key, a);
  sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
  sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
  gtk_widget_show (sb);
  gtk_table_attach (GTK_TABLE (t), sb, 1, 2, row, row+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  g_signal_connect (G_OBJECT (a), "value_changed", cb, dialog);
}


/* Config widgets */

static void
sp_config_check_button_destroy (GObject *object, SPRepr *repr)
{
	sp_repr_remove_listener_by_data (repr, object);
	sp_repr_unref (repr);
}

static void
sp_config_check_button_repr_attr_changed (SPRepr *repr,
					  const unsigned char *key,
					  const unsigned char *oldval, const unsigned char *newval,
					  void *data)
{
	const unsigned char *ownkey, *trueval;
	unsigned int active;
	ownkey = g_object_get_data ((GObject *) data, "key");
	if (strcmp (key, ownkey)) return;
	trueval = g_object_get_data ((GObject *) data, "trueval");
	active = (!trueval && !newval) || (trueval && newval && !strcmp (newval, trueval));
	gtk_toggle_button_set_active ((GtkToggleButton *) data, active);
}

SPReprEventVector sp_config_check_button_event_vector = {
	NULL, /* Destroy */
	NULL, /* Add child */
	NULL, /* Child added */
	NULL, /* Remove child */
	NULL, /* Child removed */
	NULL, /* Change attribute */
	sp_config_check_button_repr_attr_changed,
	NULL, /* Change content */
	NULL, /* Content changed */
	NULL, /* Change_order */
	NULL /* Order changed */
};

void
sp_config_check_button_toggled (GtkToggleButton *button, SPRepr *repr)
{
	const unsigned char *key, *val;
	key = g_object_get_data ((GObject *) button, "key");
	if (gtk_toggle_button_get_active (button)) {
		val = g_object_get_data ((GObject *) button, "trueval");
	} else {
		val = g_object_get_data ((GObject *) button, "falseval");
	}
	printf ("Setting '%s' to '%s'\n", key, val);
	sp_repr_set_attr (repr, key, val);
}

GtkWidget *
sp_config_check_button_new (const unsigned char *text,
			    const unsigned char *path, const unsigned char *key,
			    const unsigned char *trueval, const unsigned char *falseval)
{
	GtkWidget *w;
	SPRepr *repr;
	w = gtk_check_button_new_with_label (text);
	repr = sodipodi_get_repr (SODIPODI, path);
	if (repr) {
		sp_repr_ref (repr);
		g_object_set_data ((GObject *) w, "repr", (gpointer) repr);
		g_object_set_data ((GObject *) w, "key", (gpointer) key);
		g_object_set_data ((GObject *) w, "trueval", (gpointer) trueval);
		g_object_set_data ((GObject *) w, "falseval", (gpointer) falseval);
		/* Connect destroy signal */
		g_signal_connect ((GObject *) w, "destroy", (GCallback) sp_config_check_button_destroy, repr);
		/* Connect repr change_attr event */
		sp_repr_add_listener (repr, &sp_config_check_button_event_vector, w);
		/* Connect toggled signal */
		g_signal_connect ((GObject *) w, "toggled", (GCallback) sp_config_check_button_toggled, repr);
	}
	return w;
}

