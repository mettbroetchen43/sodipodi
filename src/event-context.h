#ifndef SP_EVENT_CONTEXT_H
#define SP_EVENT_CONTEXT_H

/*
 * SPEventContext
 *
 * This is per desktop object, which (its derivatives) implements
 * different actions bound to mouse events.
 *
 * These functions are meant to use only from children & desktop
 *
 */

#include <gtk/gtk.h>
#include "forward.h"

#define SP_TYPE_EVENT_CONTEXT            (sp_event_context_get_type ())
#define SP_EVENT_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_EVENT_CONTEXT, SPEventContext))
#define SP_EVENT_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_EVENT_CONTEXT, SPEventContextClass))
#define SP_IS_EVENT_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_EVENT_CONTEXT))
#define SP_IS_EVENT_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_EVENT_CONTEXT))

struct _SPEventContext {
	GtkObject object;
	SPDesktop * desktop;
	gchar ** cursor_shape;
	gint hot_x, hot_y;
	GdkCursor * cursor;
};

struct _SPEventContextClass {
	GtkObjectClass parent_class;
	void (* setup) (SPEventContext * event_context, SPDesktop * desktop);
	gint (* root_handler) (SPEventContext * event_context, GdkEvent * event);
	gint (* item_handler) (SPEventContext * event_context, SPItem * item, GdkEvent * event);
};

/* Standard Gtk function */

GtkType sp_event_context_get_type (void);

/* Constructor */

SPEventContext * sp_event_context_new (SPDesktop * desktop, GtkType type);

/* Method invokers */

gint sp_event_context_root_handler (SPEventContext * event_context, GdkEvent * event);
gint sp_event_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

void sp_event_root_menu_popup (GtkWidget * widget, SPItem * item, GdkEventButton * event);

#endif
