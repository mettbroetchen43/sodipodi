#ifndef DESKTOP_EVENTS_H
#define DESKTOP_EVENTS_H

#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>

void sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);
void sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

#endif
