#define __SP_DISPLAY_SETTINGS_C__

/*
 * Display settings dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include "display-settings.h"

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;
static gint oversample = 0;

extern gint sp_canvas_image_x_sample;
extern gint sp_canvas_image_y_sample;

static void sp_display_dialog_setup (void);

static gint sp_display_dialog_delete (GtkWidget *widget, GdkEvent *event);

static void sp_display_dialog_set_oversample (GtkMenuItem *item, gpointer data);

void
sp_display_dialog (void)
{
	if (dialog == NULL) {
		g_assert (xml == NULL);
		xml = glade_xml_new (SODIPODI_GLADEDIR "/display.glade", "display_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "display_dialog");
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
				    GTK_SIGNAL_FUNC (sp_display_dialog_delete), NULL);
	} else {
		if (!GTK_WIDGET_VISIBLE (dialog)) gtk_widget_show (dialog);
	}

	sp_display_dialog_setup ();
}

static void
sp_display_dialog_setup (void)
{
	GtkWidget *w, *m, *i;

	g_assert (dialog != NULL);

	/* Show grid */
	w = glade_xml_get_widget (xml, "bitmap_oversample");
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (w));

	m = gtk_menu_new ();
	gtk_widget_show (m);

	i = gtk_menu_item_new_with_label (_("None"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (0));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("2x2"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (1));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("4x4"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (2));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("8x8"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (3));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("16x16"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (4));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (m), i);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);

	gtk_option_menu_set_history (GTK_OPTION_MENU (w), sp_canvas_image_x_sample);
}

void
sp_display_dialog_close (GtkWidget * widget)
{
	g_assert (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog)) gtk_widget_hide (dialog);
}

static gint
sp_display_dialog_delete (GtkWidget *widget, GdkEvent *event)
{
	sp_display_dialog_close (widget);

	return TRUE;
}

void
sp_display_dialog_apply (GtkWidget * widget)
{
	sp_canvas_image_x_sample = oversample;
	sp_canvas_image_y_sample = oversample;

	/* fixme: update display */
}

static void
sp_display_dialog_set_oversample (GtkMenuItem *item, gpointer data)
{
	gint os;

	os = GPOINTER_TO_INT (data);

	g_return_if_fail (os >= 0);
	g_return_if_fail (os <= 4);

	oversample = os;
}

