#ifndef ZOOM_CONTEXT_H
#define ZOOM_CONTEXT_H

#include <libgnomeui/gnome-canvas.h>
#include "desktop.h"

gint sp_zoom_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);
gint sp_zoom_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_zoom_context_set (SPDesktop * desktop);

void sp_zoom_context_release (SPDesktop * desktop);

/* Menu handlers */

void sp_zoom_selection (GtkWidget * widget);
void sp_zoom_drawing (GtkWidget * widget);
void sp_zoom_page (GtkWidget * widget);

#endif
