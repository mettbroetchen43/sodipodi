#define EVENT_BROKER_C

#include "helper/sp-canvas-util.h"
#include "dialogs/object-fill.h"
#include "dialogs/object-stroke.h"
#include "sp-select-context.h"
#include "sp-node-context.h"
#include "object-context.h"
#include "sp-draw-context.h"
#include "text-context.h"
#include "zoom-context.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "event-broker.h"

SPEventContextType event_context = SP_EVENT_CONTEXT_SELECT;

static void set_event_location (SPDesktop * desktop, GdkEvent * event);

static void
sp_event_context_release (SPDesktop * desktop) {

	switch (event_context) {
		case SP_EVENT_CONTEXT_SELECT:
			sp_select_context_release (desktop);
			break;
		case SP_EVENT_CONTEXT_NODE_EDIT:
			sp_node_context_release (desktop);
			break;
		case SP_EVENT_CONTEXT_RECT:
		case SP_EVENT_CONTEXT_ELLIPSE:
			sp_object_release (desktop);
			break;
		case SP_EVENT_CONTEXT_DRAW:
			sp_draw_context_release (desktop);
			break;
		case SP_EVENT_CONTEXT_TEXT:
			sp_text_release (desktop);
			break;
		case SP_EVENT_CONTEXT_ZOOM:
			sp_zoom_context_release (desktop);
			break;
		default:
			g_assert_not_reached ();
			break;
	}
}


gint
sp_event_handler (GnomeCanvasItem * ci, GdkEvent * event, gpointer data)
{
	SPDesktop * desktop;
	SPItem * item;
	gint ret;

	ret = FALSE;

	desktop = SP_CANVAS_ITEM_DESKTOP (ci);
	item = SP_ITEM (data);

	switch (event_context) {
		case SP_EVENT_CONTEXT_SELECT:
			ret = sp_select_handler (desktop, item, event);
			break;
		case SP_EVENT_CONTEXT_NODE_EDIT:
			ret = sp_node_handler (desktop, item, event);
			break;
		case SP_EVENT_CONTEXT_ZOOM:
			ret = sp_zoom_handler (desktop, item, event);
			break;
		case SP_EVENT_CONTEXT_RECT:
		case SP_EVENT_CONTEXT_ELLIPSE:
			ret = sp_object_handler (desktop, item, event);
			break;
		case SP_EVENT_CONTEXT_DRAW:
			ret = sp_draw_handler (desktop, item, event);
			break;
		case SP_EVENT_CONTEXT_TEXT:
			ret = sp_text_handler (desktop, item, event);
			break;
		default:
			g_assert_not_reached ();
	}

	if (!ret) {
		ret = sp_root_handler (SP_DT_ACETATE (desktop), event, desktop);
	} else {
		set_event_location (desktop, event);
	}

	return ret;
}

gint
sp_root_handler (GnomeCanvasItem * ci, GdkEvent * event, gpointer data)
{
	SPDesktop * desktop;
	gint ret = FALSE;
	static gint panning = FALSE;
	static ArtPoint s;
	double x, y;

	desktop = SP_DESKTOP (data);

	switch (event_context) {
		case SP_EVENT_CONTEXT_SELECT:
			ret = sp_select_root_handler (desktop, event);
			break;
		case SP_EVENT_CONTEXT_NODE_EDIT:
			ret = sp_node_root_handler (desktop, event);
			break;
		case SP_EVENT_CONTEXT_DRAW:
			ret = sp_draw_root_handler (desktop, event);
			break;
		case SP_EVENT_CONTEXT_ZOOM:
			ret = sp_zoom_root_handler (desktop, event);
			break;
		case SP_EVENT_CONTEXT_RECT:
		case SP_EVENT_CONTEXT_ELLIPSE:
			ret = sp_object_root_handler (desktop, event);
			break;
		case SP_EVENT_CONTEXT_TEXT:
			ret = sp_text_root_handler (desktop, event);
			break;
		default:
			g_assert_not_reached ();
	}
	if (!ret) {
	/* Nobody cares of event :-( */
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 2) {
			s.x = event->button.x;
			s.y = event->button.y;
			panning = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 2) {
			panning = FALSE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (panning && (event->motion.state & GDK_BUTTON2_MASK)) {
			x = event->motion.x;
			y = event->motion.y;
			sp_desktop_scroll_world (desktop, x - s.x, y - s.y);
		}
		break;
#if 0
	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_DELETE:
			sp_selection_delete ();
			break;
		default:
			break;
		}
		break;
#endif
	default:
		break;
	}
	}

	set_event_location (desktop, event);

	return TRUE;
}

void
sp_broker_selection_changed (SPDesktop * desktop)
{
	switch (event_context) {
		case SP_EVENT_CONTEXT_SELECT:
			sp_select_context_selection_changed (desktop);
			break;
		case SP_EVENT_CONTEXT_NODE_EDIT:
			sp_node_context_selection_changed (desktop);
			break;
		default:
			break;
	}

	sp_object_fill_selection_changed ();
	sp_object_stroke_selection_changed ();
}

void
sp_event_context_set_select (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_SELECT;
	sp_select_context_set (desktop);
}

void
sp_event_context_set_node_edit (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_NODE_EDIT;
	sp_node_context_set (desktop);
}

void
sp_event_context_set_rect (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_RECT;
	sp_object_context_set (desktop, SP_OBJECT_RECT);
}

void
sp_event_context_set_ellipse (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_ELLIPSE;
	sp_object_context_set (desktop, SP_OBJECT_ELLIPSE);
}

void
sp_event_context_set_freehand (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_DRAW;
	sp_draw_context_set (desktop);
}

void
sp_event_context_set_text (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_TEXT;
	sp_text_context_set (desktop);
}

void
sp_event_context_set_zoom (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_WIDGET_DESKTOP (widget);

	sp_event_context_release (desktop);
	event_context = SP_EVENT_CONTEXT_ZOOM;
	sp_zoom_context_set (desktop);
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

