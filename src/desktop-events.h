#ifndef DESKTOP_EVENTS_H
#define DESKTOP_EVENTS_H

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>

/* Item handlers */

void sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);
void sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

/* Default handlers */

gint sp_desktop_enter_notify (GtkWidget * widget, GdkEventCrossing * event);

/* Rulers */

gint sp_dt_hruler_event (GtkWidget * widget, GdkEvent * event, gpointer data);
gint sp_dt_vruler_event (GtkWidget * widget, GdkEvent * event, gpointer data);

/* Guides */

void sp_dt_guide_event (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

#endif

