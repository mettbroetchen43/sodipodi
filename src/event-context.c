#define SP_EVENT_CONTEXT_C

#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
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
#include "selection-chemistry.h"
#include "dialogs/desktop-properties.h"

static void sp_event_context_class_init (SPEventContextClass * klass);
static void sp_event_context_init (SPEventContext * event_context);
static void sp_event_context_destroy (GtkObject * object);

static void sp_event_context_private_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_event_context_private_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_event_context_private_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void set_event_location (SPDesktop * desktop, GdkEvent * event);

static void sp_event_grab_item_destroy (GtkObject * object, gpointer data);

static GtkObjectClass * parent_class;

GtkType
sp_event_context_get_type (void)
{
	static GtkType event_context_type = 0;

	if (!event_context_type) {

		static const GtkTypeInfo event_context_info = {
			"SPEventContext",
			sizeof (SPEventContext),
			sizeof (SPEventContextClass),
			(GtkClassInitFunc) sp_event_context_class_init,
			(GtkObjectInitFunc) sp_event_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		event_context_type = gtk_type_unique (gtk_object_get_type (), &event_context_info);
	}

	return event_context_type;
}

static void
sp_event_context_class_init (SPEventContextClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = sp_event_context_destroy;

	klass->setup = sp_event_context_private_setup;
	klass->root_handler = sp_event_context_private_root_handler;
	klass->item_handler = sp_event_context_private_item_handler;
}

static void
sp_event_context_init (SPEventContext * event_context)
{
	event_context->desktop = NULL;
	event_context->cursor = NULL;
}

static void
sp_event_context_destroy (GtkObject * object)
{
	SPEventContext * event_context;

	event_context = SP_EVENT_CONTEXT (object);

	if (event_context->cursor != NULL)
		gdk_cursor_destroy (event_context->cursor);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_event_context_private_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	GtkWidget * w;
	GdkBitmap * bitmap, * mask;

	event_context->desktop = desktop;

	w = GTK_WIDGET (SP_DT_CANVAS (desktop));
	if (w->window) {
		/* fixme: */
		if (event_context->cursor_shape) {
			bitmap = NULL;
			mask = NULL;
			sp_cursor_bitmap_and_mask_from_xpm (&bitmap, &mask, event_context->cursor_shape);
			if ((bitmap != NULL) && (mask != NULL)) {
				event_context->cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
					&w->style->black, &w->style->white,
					event_context->hot_x, event_context->hot_y);
			}
		}
		gdk_window_set_cursor (w->window, event_context->cursor);
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
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
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
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
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
	  case GDK_F5: // F5 - ellipse context
	    sp_event_context_set_ellipse(NULL);	    
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
	      sp_file_open ();
	      ret = TRUE;
	    }
	    break;
	  case GDK_e: // Ctrl e - export file
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_file_export (NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_i: // Ctrl i - import file
	    if (event->key.state & GDK_CONTROL_MASK) {
	      sp_file_import (NULL);
	      ret = TRUE;
	    }
	    break;
	  case GDK_p: // Crtl p - print document
	    if (event->key.state & GDK_CONTROL_MASK) {
	      ret = TRUE;
	      sp_file_print (NULL);
	    }
	    break;
	  case GDK_P: // Crtl P - print preview
	    if (event->key.state & GDK_CONTROL_MASK) {
	      ret = TRUE;
	      sp_file_print_preview (NULL);
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

SPEventContext *
sp_event_context_new (SPDesktop * desktop, GtkType type)
{
	SPEventContext * event_context;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	event_context = gtk_type_new (type);

	(* SP_EVENT_CONTEXT_CLASS (event_context->object.klass)->setup) (event_context, desktop);

	return event_context;
}

gint
sp_event_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	gint ret;

	ret = (* SP_EVENT_CONTEXT_CLASS (event_context->object.klass)->root_handler) (event_context, event);

	set_event_location (event_context->desktop, event);

	return ret;
}

gint
sp_event_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = (* SP_EVENT_CONTEXT_CLASS (event_context->object.klass)->item_handler) (event_context, item, event);

	if (! ret) {
		ret = sp_event_context_root_handler (event_context, event);
	} else {
		set_event_location (event_context->desktop, event);
	}

	return ret;
}

static void
set_event_location (SPDesktop * desktop, GdkEvent * event)
{
	ArtPoint p;

	if (event->type != GDK_MOTION_NOTIFY)
		return;

	sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
	sp_desktop_set_position (desktop, p.x, p.y);
	sp_desktop_coordinate_status (desktop, p.x, p.y, 0);
}

void
sp_event_root_menu_popup (SPDesktop *desktop, SPItem *item, GdkEvent *event)
{
	static GtkMenu * menu = NULL;
	static GtkWidget * objitem = NULL;
	GladeXML * xml;

	if (menu == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "popup_main");
		g_return_if_fail (xml != NULL);

		glade_xml_signal_autoconnect (xml);

		menu = GTK_MENU (glade_xml_get_widget (xml, "popup_main"));
		g_return_if_fail (menu != NULL);

		objitem = gtk_menu_item_new_with_label (_("Object"));
		gtk_menu_append (menu, objitem);
		gtk_widget_show (objitem);
	}

	if (item != NULL) {
		GtkWidget * m;
		m = gtk_menu_new ();
		gtk_signal_connect_while_alive (GTK_OBJECT (item), "destroy",
						GTK_SIGNAL_FUNC (sp_event_grab_item_destroy), objitem, GTK_OBJECT (m));
		sp_item_menu (item, desktop, GTK_MENU (m));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (objitem), m);
		gtk_widget_show (m);
		gtk_widget_set_sensitive (objitem, TRUE);
	} else {
		gtk_menu_item_remove_submenu (GTK_MENU_ITEM (objitem));
		gtk_widget_set_sensitive (objitem, FALSE);
	}

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		gtk_menu_popup (menu, NULL, NULL, 0, NULL, event->button.button, event->button.time);
		break;
	case GDK_KEY_PRESS:
		gtk_menu_popup (menu, NULL, NULL, 0, NULL, 0, event->key.time);
		break;
	default:
	}
}

static void
sp_event_grab_item_destroy (GtkObject * object, gpointer data)
{
	gtk_menu_item_remove_submenu (GTK_MENU_ITEM (data));
	gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);
}
