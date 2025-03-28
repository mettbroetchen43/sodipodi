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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkcheckbutton.h>

#include "helper/sp-intl.h"
#include "helper/window.h"

#include "../sodipodi.h"
#include "display-settings.h"

static GtkWidget *dialog = NULL;

extern gint nr_arena_image_x_sample;
extern gint nr_arena_image_y_sample;
extern gdouble nr_arena_global_delta;

static GtkWidget *sp_display_dialog_new (void);

static void
sp_display_dialog_destroy (GtkObject *object, gpointer data)
{
	dialog = NULL;
}

void
sp_display_dialog (void)
{
	if (dialog == NULL) {
		dialog = sp_display_dialog_new ();
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (sp_display_dialog_destroy), NULL);
	}

	gtk_window_present ((GtkWindow *) dialog);
}

static void
sp_display_dialog_set_oversample (GtkMenuItem *item, gpointer data)
{
	gint os;

	os = GPOINTER_TO_INT (data);

	g_return_if_fail (os >= 0);
	g_return_if_fail (os <= 4);

	nr_arena_image_x_sample = os;
	nr_arena_image_y_sample = os;

	sodipodi_refresh_display (SODIPODI);
}

static void
sp_display_dialog_cursor_tolerance_changed (GtkAdjustment *adj, gpointer data)
{
	nr_arena_global_delta = adj->value;
}

static void
sp_display_dialog_decoration_toggled (GtkToggleButton *tb, gpointer data)
{
	sp_config_value_set_boolean ("windows.dialogs.behaviour", "hintDialog", tb->active, TRUE);
}

static void
sp_display_dialog_taskbar_toggled (GtkToggleButton *tb, gpointer data)
{
	sp_config_value_set_boolean ("windows.dialogs.behaviour", "skipTaskbar", tb->active, TRUE);
}

static GtkWidget *
sp_display_dialog_new (void)
{
	GtkWidget *dialog, *nb, *l, *vb, *hb, *om, *m, *i, *sb, *cb;
	GtkObject *a;

	dialog = sp_window_new (_("Display settings"), FALSE, TRUE);

	nb = gtk_notebook_new ();
	gtk_container_add (GTK_CONTAINER (dialog), nb);

	/* Rendering settings */
	/* Notebook tab */
	l = gtk_label_new (_("Rendering"));
	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

	/* Oversampling menu */
	hb = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	l = gtk_label_new (_("Oversample bitmaps:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hb), l, FALSE, FALSE, 0);
	om = gtk_option_menu_new ();
	gtk_box_pack_start (GTK_BOX (hb), om, TRUE, TRUE, 0);

	m = gtk_menu_new ();

	i = gtk_menu_item_new_with_label (_("None"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (0));
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("2x2"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (1));
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("4x4"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (2));
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("8x8"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (3));
	gtk_menu_append (GTK_MENU (m), i);
	i = gtk_menu_item_new_with_label (_("16x16"));
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_display_dialog_set_oversample), GINT_TO_POINTER (4));
	gtk_menu_append (GTK_MENU (m), i);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);

	gtk_option_menu_set_history (GTK_OPTION_MENU (om), nr_arena_image_x_sample);

	/* Input settings */
	/* Notebook tab */
	l = gtk_label_new (_("Input"));
	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

	hb = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	l = gtk_label_new (_("Default cursor tolerance:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hb), l, FALSE, FALSE, 0);

	a = gtk_adjustment_new (0.0, 0.0, 10.0, 0.1, 1.0, 0.0);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (a), nr_arena_global_delta);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 1);
	gtk_box_pack_start (GTK_BOX (hb), sb, TRUE, TRUE, 0);

	gtk_signal_connect (GTK_OBJECT (a), "value_changed",
			    GTK_SIGNAL_FUNC (sp_display_dialog_cursor_tolerance_changed), NULL);

	/* Windows settings */
	/* Notebook tab */
	l = gtk_label_new (_("Windows"));
	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

	cb = gtk_check_button_new_with_label (_("Set dialog hint for dialogs"));
	gtk_toggle_button_set_active ((GtkToggleButton *) cb, sp_config_value_get_boolean ("windows.dialogs.behaviour", "hintDialog", 1));
	gtk_box_pack_start (GTK_BOX (vb), cb, FALSE, FALSE, 0);
	g_signal_connect ((GObject *) cb, "toggled", (GCallback) sp_display_dialog_decoration_toggled, NULL);

	cb = gtk_check_button_new_with_label (_("Skip taskbar entry for dialogs"));
	gtk_toggle_button_set_active ((GtkToggleButton *) cb, sp_config_value_get_boolean ("windows.dialogs.behaviour", "skipTaskbar", 1));
	gtk_box_pack_start (GTK_BOX (vb), cb, FALSE, FALSE, 0);
	g_signal_connect ((GObject *) cb, "toggled", (GCallback) sp_display_dialog_taskbar_toggled, NULL);

	gtk_widget_show_all (nb);

	return dialog;
}

