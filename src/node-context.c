#define __SP_NODE_CONTEXT_C__

/*
 * Node editing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "sp-path.h"
#include "rubberband.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "selection.h"
#include "nodepath.h"
#include "knotholder.h"
#include "pixmaps/cursor-node.xpm"
#include "node-context.h"
#include "sp-cursor.h"
#include "pixmaps/cursor-node-m.xpm"
#include "pixmaps/cursor-node-d.xpm"

static void sp_node_context_class_init (SPNodeContextClass * klass);
static void sp_node_context_init (SPNodeContext * node_context);
static void sp_node_context_destroy (GtkObject * object);

static void sp_node_context_setup (SPEventContext *ec);
static gint sp_node_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_node_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_node_context_selection_changed (SPSelection * selection, gpointer data);

static SPEventContextClass * parent_class;
GdkCursor * CursorNodeMouseover = NULL, * CursorNodeDragging = NULL;

GtkType
sp_node_context_get_type (void)
{
	static GtkType node_context_type = 0;

	if (!node_context_type) {

		static const GtkTypeInfo node_context_info = {
			"SPNodeContext",
			sizeof (SPNodeContext),
			sizeof (SPNodeContextClass),
			(GtkClassInitFunc) sp_node_context_class_init,
			(GtkObjectInitFunc) sp_node_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		node_context_type = gtk_type_unique (sp_event_context_get_type (), &node_context_info);
	}

	return node_context_type;
}

static void
sp_node_context_class_init (SPNodeContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_node_context_destroy;

	event_context_class->setup = sp_node_context_setup;
	event_context_class->root_handler = sp_node_context_root_handler;
	event_context_class->item_handler = sp_node_context_item_handler;

	// cursors in node context
	CursorNodeMouseover = sp_cursor_new_from_xpm (cursor_node_m_xpm , 1, 1); 
	CursorNodeDragging = sp_cursor_new_from_xpm (cursor_node_d_xpm , 1, 1); 
}

static void
sp_node_context_init (SPNodeContext * node_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (node_context);

	event_context->cursor_shape = cursor_node_xpm;
	event_context->hot_x = 1;
	event_context->hot_y = 1;
}

static void
sp_node_context_destroy (GtkObject * object)
{
	SPNodeContext * nc;

	nc = SP_NODE_CONTEXT (object);

	if (nc->nodepath) sp_nodepath_destroy (nc->nodepath);
	if (nc->knot_holder) sp_knot_holder_destroy (nc->knot_holder);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_node_context_setup (SPEventContext *ec)
{
	SPNodeContext *nc;
	SPItem *item;

	nc = SP_NODE_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (ec);

	gtk_signal_connect_while_alive (GTK_OBJECT (SP_DT_SELECTION (ec->desktop)), "changed",
					GTK_SIGNAL_FUNC (sp_node_context_selection_changed), nc,
					GTK_OBJECT (nc));

	item = sp_selection_item (SP_DT_SELECTION (ec->desktop));

	nc->nodepath = NULL;
	nc->knot_holder = NULL;
	if (item) {
		nc->nodepath = sp_nodepath_new (ec->desktop, item);
		if (! nc->nodepath) {
			nc->knot_holder = sp_item_knot_holder (item, ec->desktop);
		}
	}
}

static gint
sp_node_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	SPNodeContext *nc;
	SPDesktop * desktop;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;

	nc = SP_NODE_CONTEXT (event_context);

	switch (event->type) {
		case GDK_BUTTON_RELEASE:
			if (event->button.button == 1) {
				if (!nc->drag) {
					sp_selection_set_item (SP_DT_SELECTION (desktop), item);
					ret = TRUE;
				}
				break;
			}
			break;
		default:
			break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);
	}

	return ret;
}

static gint
sp_node_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	SPNodeContext * nc;
	NRPointF p;
	ArtDRect b;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;
	nc = SP_NODE_CONTEXT (event_context);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_rubberband_start (desktop, p.x, p.y);
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
			nc->drag = TRUE;
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				if (nc->nodepath) {
					sp_nodepath_select_rect (nc->nodepath, &b, event->button.state & GDK_SHIFT_MASK);
				}
				ret = TRUE;
			}
			nc->drag = FALSE;
			break;
		}
		break;
	case GDK_KEY_PRESS:
		if ((event->key.state & GDK_SHIFT_MASK) && !(event->key.state & GDK_CONTROL_MASK)) {
			switch (event->key.keyval) {
			case GDK_Insert:
			case GDK_KP_Insert:
				sp_node_selected_add_node ();
				ret = TRUE;
				break;
			case GDK_Delete:
			case GDK_KP_Delete:
				sp_node_selected_delete ();
				ret = TRUE;
				break;
			case GDK_C:
				sp_node_selected_set_type (SP_PATHNODE_CUSP);
				ret = TRUE;
				break;
			case GDK_S:
				sp_node_selected_set_type (SP_PATHNODE_SMOOTH);
				ret = TRUE;
				break;
			case GDK_Y:
				sp_node_selected_set_type (SP_PATHNODE_SYMM);
				ret = TRUE;
				break;
			case GDK_B:
				sp_node_selected_break ();
				ret = TRUE;
				break;
			case GDK_J:
				sp_node_selected_join ();
				ret = TRUE;
				break;
			case GDK_L:
				sp_node_selected_set_line_type (ART_LINETO);
				ret = TRUE;
				break;
			case GDK_K:
				sp_node_selected_set_line_type (ART_CURVETO);
				ret = TRUE;
				break;
			default:
				break;
			}
		}
		if (!(event->key.state & GDK_SHIFT_MASK) && !(event->key.state & GDK_CONTROL_MASK)) 
			ret = node_key (event);


		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (event_context, event);
	}

	return ret;
}

static void
sp_node_context_selection_changed (SPSelection * selection, gpointer data)
{
	SPNodeContext * nc;
	SPDesktop *desktop;
	SPItem * item;

	nc = SP_NODE_CONTEXT (data);

	if (nc->nodepath) sp_nodepath_destroy (nc->nodepath);
	if (nc->knot_holder) sp_knot_holder_destroy (nc->knot_holder);

	item = sp_selection_item (selection);
	
	desktop = selection->desktop;
	nc->nodepath = NULL;
	nc->knot_holder = NULL;
	if (item) {
		nc->nodepath = sp_nodepath_new (desktop, item);
		if (! nc->nodepath)
			nc->knot_holder = sp_item_knot_holder (item, desktop);
	}
}
