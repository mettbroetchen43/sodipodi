#define __SP_EVENT_CONTEXT_C__

/*
 * Base class for event processors
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

#include <config.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include "helper/sp-canvas.h"
#include "xml/repr-private.h"
#include "sp-cursor.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "event-context.h"
#include "event-broker.h"
#include "sp-item.h"
#include "zoom-context.h"
#include "file.h"
#include "interface.h"
#include "helper/sp-intl.h"
#include "selection-chemistry.h"
#include "dialogs/desktop-properties.h"

static void sp_event_context_class_init (SPEventContextClass *klass);
static void sp_event_context_init (SPEventContext *event_context);
static void sp_event_context_dispose (GObject *object);

static void sp_event_context_private_setup (SPEventContext *ec);
static gint sp_event_context_private_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_event_context_private_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void set_event_location (SPDesktop * desktop, GdkEvent * event);

static GObjectClass *parent_class;

unsigned int
sp_event_context_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPEventContextClass),
			NULL, NULL,
			(GClassInitFunc) sp_event_context_class_init,
			NULL, NULL,
			sizeof (SPEventContext),
			4,
			(GInstanceInitFunc) sp_event_context_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "SPEventContext", &info, 0);
	}
	return type;
}

static void
sp_event_context_class_init (SPEventContextClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = sp_event_context_dispose;

	klass->setup = sp_event_context_private_setup;
	klass->root_handler = sp_event_context_private_root_handler;
	klass->item_handler = sp_event_context_private_item_handler;
}

static void
sp_event_context_init (SPEventContext *event_context)
{
	event_context->desktop = NULL;
	event_context->cursor = NULL;
}

static void
sp_event_context_dispose (GObject *object)
{
	SPEventContext *ec;

	ec = SP_EVENT_CONTEXT (object);

	if (ec->cursor != NULL) {
		gdk_cursor_unref (ec->cursor);
		ec->cursor = NULL;
	}

	if (ec->desktop) {
		ec->desktop = NULL;
	}

	if (ec->repr) {
		sp_repr_remove_listener_by_data (ec->repr, ec);
		sp_repr_unref (ec->repr);
		ec->repr = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sp_event_context_private_setup (SPEventContext *ec)
{
	GtkWidget *w;
	GdkBitmap *bitmap, * mask;

	w = GTK_WIDGET (SP_DT_CANVAS (ec->desktop));
	if (w->window) {
		/* fixme: */
		if (ec->cursor_shape) {
			bitmap = NULL;
			mask = NULL;
			sp_cursor_bitmap_and_mask_from_xpm (&bitmap, &mask, ec->cursor_shape);
			if ((bitmap != NULL) && (mask != NULL)) {
				ec->cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
					&w->style->black, &w->style->white,
					ec->hot_x, ec->hot_y);
			}
		}
		gdk_window_set_cursor (w->window, ec->cursor);
	}
}

static gint
sp_event_context_private_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static ArtPoint s;
	static gboolean panning = FALSE;
	gint ret;
	SPDesktop * desktop;
	ret = FALSE;

       	desktop = event_context->desktop;

	switch (event->type) {
	case GDK_2BUTTON_PRESS:
		sp_desktop_dialog ();
		break;
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 2:
			s.x = event->button.x;
			s.y = event->button.y;
			panning = TRUE;
			sp_canvas_item_grab (SP_CANVAS_ITEM (desktop->acetate),
					     GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
					     NULL, event->button.time);

			ret = TRUE;
			break;
		case 3:
			/* fixme: */
			sp_event_root_menu_popup (desktop, NULL, event);
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (panning && (event->motion.state & GDK_BUTTON2_MASK)) {
			sp_desktop_scroll_world (event_context->desktop, event->motion.x - s.x, event->motion.y - s.y);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 2) {
			panning = FALSE;
			sp_canvas_item_ungrab (SP_CANVAS_ITEM (desktop->acetate), event->button.time);
			ret = TRUE;
		}
		break;
        case GDK_KEY_PRESS:
		switch (event->key.keyval) {  
		case GDK_Tab: // disable tab/shift-tab which cycle widget focus
		case GDK_ISO_Left_Tab: // they will get different functions
			ret = TRUE;
			break;
		case GDK_F1: // F1 - select context 
			sp_event_context_set_select(NULL);
			ret = TRUE;
			break;
		case GDK_F2: // F2 - node edit context 
			sp_event_context_set_node_edit(NULL);	    
			ret = TRUE;
			break;
		case GDK_F3: // F3 - zoom context
			sp_event_context_set_zoom(NULL);	    
			ret = TRUE;
			break;
		case GDK_F4: // F4 - rect context
			sp_event_context_set_rect(NULL);	    
			ret = TRUE;
			break;
		case GDK_F5: // F5 - arc context
			sp_event_context_set_arc(NULL);	    
			ret = TRUE;
			break;
		case GDK_F6: // F6 - frehand line context
			sp_event_context_set_freehand(NULL);	    
			ret = TRUE;
			break;
		case GDK_F7: // F7 - text context
			sp_event_context_set_text(NULL);	    
			ret = TRUE;
			break;
		case GDK_equal: // = 
		case GDK_plus: // + - zoom in
			sp_zoom_in(NULL);
			ret = TRUE;
			break;
		case GDK_minus: // - - zoom out
			sp_zoom_out(NULL);
			ret = TRUE;
			break;
		case GDK_0: // 0 - zoom entire page
			sp_zoom_page(NULL);
			ret = TRUE;
			break;
		case GDK_1: // 1 - zoom 1 to 1
			sp_zoom_1_to_1(NULL);
			ret = TRUE;
			break;
		case GDK_z: // Ctrl z - undo
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_undo (NULL);
				ret = TRUE;
			}
			break;
		case GDK_r: // Ctrl r - redo
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_redo (NULL);
				ret = TRUE;
			}
			break;
		case GDK_w: // Crtl w - close view
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_ui_close_view (NULL);
				ret = TRUE;
			}
			break;
		case GDK_n: // Crtl n - new document
			if (event->key.state & GDK_CONTROL_MASK) {
				ret = TRUE;
				sp_file_new ();
			}
			break;
		case GDK_N: // Ctrl N - new view
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_ui_new_view (NULL);
				ret = TRUE;
			}
			break;
		case GDK_o: // Ctrl o - open file
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_file_open_dialog (NULL, NULL);
				ret = TRUE;
			}
			break;
#if 0
		case GDK_e: // Ctrl e - export file
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_file_export (NULL);
				ret = TRUE;
			}
			break;
#endif
		case GDK_i: // Ctrl i - import file
			if (event->key.state & GDK_CONTROL_MASK) {
				sp_file_import (NULL);
				ret = TRUE;
			}
			break;
		case GDK_p: // Crtl p - print document
			if (event->key.state & GDK_CONTROL_MASK) {
				ret = TRUE;
				sp_file_print (NULL, NULL);
			}
			break;
		case GDK_P: // Crtl P - print preview
			if (event->key.state & GDK_CONTROL_MASK) {
				ret = TRUE;
				sp_file_print_preview (NULL, NULL);
			}
			break;
	  case GDK_s: // Crtl s - save file
	    if (event->key.state & GDK_CONTROL_MASK) {
	      ret = TRUE;
	      sp_file_save (NULL);
	    }
	    break;
	  case GDK_S: // Crtl S - save file as
	    if (event->key.state & GDK_CONTROL_MASK) {
	      ret = TRUE;
	      sp_file_save_as (NULL);
	    }
	    break;
	  case GDK_q: // Ctrl q - quit
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_file_exit ();
	      ret = TRUE;
	    }
	    break;
	  case GDK_Left: // Ctrl Left 
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_desktop_scroll_world (event_context->desktop, 10, 0);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Up: // Ctrl Up
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_desktop_scroll_world (event_context->desktop, 0, +10);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Right: // Ctrl Right
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_desktop_scroll_world (event_context->desktop, -10, 0);
	      ret = TRUE;
	    }
	    break;
	  case GDK_Down: // Ctrl Down
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_desktop_scroll_world (event_context->desktop, 0, -10);
	      ret = TRUE;
	    }
	    break;
	  case GDK_space: // Space - root menu
	    sp_event_root_menu_popup (desktop, NULL, event);
	    ret= TRUE;
	    break;
          }
          g_print ("What a funny key: %d \n", event->key.keyval);
          break;
	default:
		break;
	}

	return ret;
}

/* fixme: do context sensitive popup menu on items */

static gint
sp_event_context_private_item_handler (SPEventContext *ctx, SPItem *item, GdkEvent *event)
{
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 3:
			/* fixme: */
			sp_event_root_menu_popup (ctx->desktop, item, event);
			return TRUE;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return FALSE;
}

static void
sp_ec_repr_destroy (SPRepr *repr, gpointer data)
{
	g_warning ("Oops! Repr destroyed while event context still present");
}

static gboolean
sp_ec_repr_change_attr (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data)
{
	SPEventContext *ec;

	ec = SP_EVENT_CONTEXT (data);

	/* In theory we could verify values here */

	return TRUE;
}

static void
sp_ec_repr_attr_changed (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data)
{
	SPEventContext *ec;

	ec = SP_EVENT_CONTEXT (data);

	if (((SPEventContextClass *) G_OBJECT_GET_CLASS (ec))->set)
		((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set (ec, key, newval);
}

SPReprEventVector sp_ec_event_vector = {
	sp_ec_repr_destroy,
	NULL, /* Add child */
	NULL, /* Child added */
	NULL, /* Remove child */
	NULL, /* Child removed */
	sp_ec_repr_change_attr,
	sp_ec_repr_attr_changed,
	NULL, /* Change content */
	NULL, /* Content changed */
	NULL, /* Change_order */
	NULL /* Order changed */
};

SPEventContext *
sp_event_context_new (GType type, SPDesktop *desktop, SPRepr *repr)
{
	SPEventContext *ec;

	g_return_val_if_fail (g_type_is_a (type, SP_TYPE_EVENT_CONTEXT), NULL);
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	ec = g_object_new (type, NULL);

	ec->desktop = desktop;

	ec->repr = repr;
	if (ec->repr) {
		sp_repr_ref (ec->repr);
		sp_repr_add_listener (ec->repr, &sp_ec_event_vector, ec);
	}

	if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->setup)
		((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->setup (ec);

	return ec;
}

void
sp_event_context_finish (SPEventContext *ec)
{
	g_return_if_fail (ec != NULL);
	g_return_if_fail (SP_IS_EVENT_CONTEXT (ec));

	if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->finish)
		((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->finish (ec);
}

void
sp_event_context_read (SPEventContext *ec, const guchar *key)
{
	g_return_if_fail (ec != NULL);
	g_return_if_fail (SP_IS_EVENT_CONTEXT (ec));
	g_return_if_fail (key != NULL);

	if (ec->repr) {
		const guchar *val;
		val = sp_repr_attr (ec->repr, key);
		if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set)
			((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set (ec, key, val);
	}
}

gint
sp_event_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	gint ret;

	ret = ((SPEventContextClass *) G_OBJECT_GET_CLASS (event_context))->root_handler (event_context, event);

	set_event_location (event_context->desktop, event);

	return ret;
}

gint
sp_event_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = ((SPEventContextClass *) G_OBJECT_GET_CLASS(event_context))->item_handler (event_context, item, event);

	if (! ret) {
		ret = sp_event_context_root_handler (event_context, event);
	} else {
		set_event_location (event_context->desktop, event);
	}

	return ret;
}

GtkWidget *
sp_event_context_config_widget (SPEventContext *ec)
{
	g_return_val_if_fail (ec != NULL, NULL);
	g_return_val_if_fail (SP_IS_EVENT_CONTEXT (ec), NULL);

	if (((SPEventContextClass *) G_OBJECT_GET_CLASS (ec))->config_widget)
		((SPEventContextClass *) G_OBJECT_GET_CLASS (ec))->config_widget (ec);

	return NULL;
}

static void
set_event_location (SPDesktop * desktop, GdkEvent * event)
{
	NRPointF p;

	if (event->type != GDK_MOTION_NOTIFY)
		return;

	sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
	sp_view_set_position (SP_VIEW (desktop), p.x, p.y);
	sp_desktop_set_coordinate_status (desktop, p.x, p.y, 0);
}

void
sp_event_root_menu_popup (SPDesktop *desktop, SPItem *item, GdkEvent *event)
{
	GtkWidget *menu;

	menu = sp_ui_generic_menu (SP_VIEW (desktop), item);
	gtk_widget_show (menu);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, NULL, event->button.button, event->button.time);
		break;
	case GDK_KEY_PRESS:
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, NULL, 0, event->key.time);
		break;
	default:
		break;
	}
}

#if 0
static void
sp_event_grab_item_destroy (GtkObject * object, gpointer data)
{
	gtk_menu_item_remove_submenu (GTK_MENU_ITEM (data));
	gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);
}
#endif
