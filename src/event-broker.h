#ifndef SP_EVENT_BROKER_H
#define SP_EVENT_BROKER_H

#include <gtk/gtk.h>

void sp_event_context_set_select (GtkWidget * widget);
void sp_event_context_set_node_edit (GtkWidget * widget);
void sp_event_context_set_rect (GtkWidget * widget);
void sp_event_context_set_ellipse (GtkWidget * widget);
void sp_event_context_set_star (GtkWidget * widget);
void sp_event_context_set_spiral (GtkWidget * widget);
void sp_event_context_set_dynahand (GtkWidget * widget);
void sp_event_context_set_freehand (GtkWidget * widget);
void sp_event_context_set_text (GtkWidget * widget);
void sp_event_context_set_zoom (GtkWidget * widget);

#endif
