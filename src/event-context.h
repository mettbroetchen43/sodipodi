#ifndef __SP_EVENT_CONTEXT_H__
#define __SP_EVENT_CONTEXT_H__

/*
 * Base class for event processors
 *
 * This is per desktop object, which (its derivatives) implements
 * different actions bound to mouse events.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdk.h>
#include <gtk/gtkobject.h>
#include "xml/repr.h"
#include "forward.h"

#define SP_TYPE_EVENT_CONTEXT (sp_event_context_get_type ())
#define SP_EVENT_CONTEXT(o) (GTK_CHECK_CAST ((o), SP_TYPE_EVENT_CONTEXT, SPEventContext))
#define SP_EVENT_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_EVENT_CONTEXT, SPEventContextClass))
#define SP_IS_EVENT_CONTEXT(o) (GTK_CHECK_TYPE ((o), SP_TYPE_EVENT_CONTEXT))
#define SP_IS_EVENT_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_EVENT_CONTEXT))

struct _SPEventContext {
	GtkObject object;
	SPDesktop *desktop;
	SPRepr *repr;
	gchar **cursor_shape;
	gint hot_x, hot_y;
	GdkCursor *cursor;
};

struct _SPEventContextClass {
	GtkObjectClass parent_class;
	void (* setup) (SPEventContext *ec);
	void (* set) (SPEventContext *ec, const guchar *key, const guchar *val);
	gint (* root_handler) (SPEventContext *ec, GdkEvent *event);
	gint (* item_handler) (SPEventContext *ec, SPItem *item, GdkEvent *event);
};

#define SP_EVENT_CONTEXT_DESKTOP(e) (SP_EVENT_CONTEXT (e)->desktop)
#define SP_EVENT_CONTEXT_REPR(e) (SP_EVENT_CONTEXT (e)->repr)

GtkType sp_event_context_get_type (void);

SPEventContext *sp_event_context_new (GtkType type, SPDesktop *desktop, SPRepr *repr);

void sp_event_context_read (SPEventContext *ec, const guchar *key);

gint sp_event_context_root_handler (SPEventContext *ec, GdkEvent *event);
gint sp_event_context_item_handler (SPEventContext *ec, SPItem *item, GdkEvent *event);

void sp_event_root_menu_popup (SPDesktop *desktop, SPItem *item, GdkEvent *event);

#endif
