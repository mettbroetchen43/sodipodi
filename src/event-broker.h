#ifndef EVENT_BROKER_H
#define EVENT_BROKER_H

#include <libgnomeui/gnome-canvas.h>
#include "desktop.h"

typedef enum {
	SP_EVENT_CONTEXT_SELECT,
	SP_EVENT_CONTEXT_NODE_EDIT,
	SP_EVENT_CONTEXT_RECT,
	SP_EVENT_CONTEXT_ELLIPSE,
	SP_EVENT_CONTEXT_DRAW,
	SP_EVENT_CONTEXT_TEXT,
	SP_EVENT_CONTEXT_ZOOM
} SPEventContextType;

gint
sp_event_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

gint
sp_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);

void sp_event_context_set_select (GtkWidget * widget);
void sp_event_context_set_node_edit (GtkWidget * widget);
void sp_event_context_set_rect (GtkWidget * widget);
void sp_event_context_set_ellipse (GtkWidget * widget);
void sp_event_context_set_freehand (GtkWidget * widget);
void sp_event_context_set_text (GtkWidget * widget);
void sp_event_context_set_zoom (GtkWidget * widget);

void
sp_broker_selection_changed (SPDesktop * desktop);

#endif
