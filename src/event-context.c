#define SP_EVENT_CONTEXT_C

#include <glade/glade.h>
#include "sp-cursor.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "event-context.h"

static void sp_event_context_class_init (SPEventContextClass * klass);
static void sp_event_context_init (SPEventContext * event_context);
static void sp_event_context_destroy (GtkObject * object);

static void sp_event_context_private_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_event_context_private_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_event_context_private_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void set_event_location (SPDesktop * desktop, GdkEvent * event);

static void sp_event_root_menu_popup (GdkEventButton * event);

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

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 2:
			s.x = event->button.x;
			s.y = event->button.y;
			panning = TRUE;
			ret = TRUE;
			break;
		case 3:
			sp_event_root_menu_popup (&event->button);
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
			ret = TRUE;
		}
		break;
	default:
		break;
	}

	return ret;
}

/* fixme: do context sensitive popup menu on items */

static gint
sp_event_context_private_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
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
}

static void
sp_event_root_menu_popup (GdkEventButton * event)
{
	static GtkMenu * menu = NULL;
	GladeXML * xml;

	if (menu == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "popup_main");
		g_return_if_fail (xml != NULL);

		glade_xml_signal_autoconnect (xml);

		menu = GTK_MENU (glade_xml_get_widget (xml, "popup_main"));
		g_return_if_fail (menu != NULL);
	}

	gtk_menu_popup (menu, NULL, NULL, 0, NULL, event->button, event->time);
}
