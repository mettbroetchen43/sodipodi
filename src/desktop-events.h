#ifndef DESKTOP_EVENTS_H
#define DESKTOP_EVENTS_H

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>

/* Item handlers */

void sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);
void sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

/* Default handlers */

gint sp_desktop_enter_notify (GtkWidget * widget, GdkEventCrossing * event);
gint sp_desktop_button_press (GtkWidget * widget, GdkEventButton * event);
gint sp_desktop_button_release (GtkWidget * widget, GdkEventButton * event);
gint sp_desktop_motion_notify (GtkWidget * widget, GdkEventMotion * event);

#endif
