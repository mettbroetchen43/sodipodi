#define __SP_WINDOW_C__

/*
 * Generic window implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <gtk/gtkwindow.h>

#include "api.h"
#include "shortcuts.h"

#include "window.h"

static int
sp_window_key_press (GtkWidget *widgt, GdkEventKey *event)
{
	unsigned int shortcut;
	shortcut = event->keyval;
	if (event->state & GDK_SHIFT_MASK) shortcut |= SP_SHORTCUT_SHIFT_MASK;
	if (event->state & GDK_CONTROL_MASK) shortcut |= SP_SHORTCUT_CONTROL_MASK;
	if (event->state & GDK_MOD1_MASK) shortcut |= SP_SHORTCUT_ALT_MASK;
	return sp_shortcut_run (shortcut);
}

static GSList *dialog_list = NULL;
static GSList *master_list = NULL;

static void
sp_window_destroy (GtkObject *object, gpointer data)
{
	if (g_slist_find (dialog_list, object)) {
		dialog_list = g_slist_remove (dialog_list, object);
	} else if (g_slist_find (master_list, object)) {
		master_list = g_slist_remove (master_list, object);
	} else {
		g_warning ("Destroyed window neither in dialog nor master list");
	}
}

#if 0
static void
sp_window_master_state (GtkWidget *widget, GdkEventWindowState *event, gpointer data)
{
	if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) {
		GtkWindow *window;
		GSList *l;
		if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
			/* Window got iconified */
			for (l = dialog_list; l; l = l->next) {
				window = (GtkWindow *) l->data;
				gtk_window_iconify (window);
			}
		} else {
			/* Window got deiconified */
			for (l = dialog_list; l; l = l->next) {
				window = (GtkWindow *) l->data;
				gtk_window_deiconify (window);
			}
		}
	}
}
#endif

GtkWidget *
sp_window_new (const unsigned char *title, unsigned int resizeable, unsigned int dialog)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title ((GtkWindow *) window, title);
	gtk_window_set_resizable ((GtkWindow *) window, resizeable);

	/* Test */
	if (dialog) {
		if (sp_config_value_get_boolean ("windows.dialogs.behaviour", "hintDialog", TRUE)) {
			gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
		}
		if (sp_config_value_get_boolean ("windows.dialogs.behaviour", "skipTaskbar", TRUE)) {
			gtk_window_set_skip_taskbar_hint ((GtkWindow *) window, TRUE);
		}
		dialog_list = g_slist_prepend (dialog_list, window);
	} else {
		master_list = g_slist_prepend (master_list, window);
		/* g_signal_connect ((GObject *) window, "window_state_event", (GCallback) sp_window_master_state, NULL); */
	}

	g_signal_connect ((GObject *) window, "destroy", (GCallback) sp_window_destroy, NULL);

	g_signal_connect_after ((GObject *) window, "key_press_event", (GCallback) sp_window_key_press, NULL);

	return window;
}

#if 0
static gboolean
sp_transformation_dialog_frame (GtkWindow *window, GdkEvent *event, gpointer data)
{
	g_print ("Gotcha\n");
	return 0;
}
	g_signal_connect ((GObject *) dlg, "window_state_event", (GCallback) sp_transformation_dialog_frame, NULL);
#endif



