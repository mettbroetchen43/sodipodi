#define SP_DESKTOP_EVENTS_C

#include "helper/sp-guide.h"
#include "desktop.h"
#include "mdi-desktop.h"
#include "desktop-affine.h"
#include "desktop-events.h"

static gboolean sp_inside_widget (GtkWidget * widget, gint x, gint y);

/* Root item handler */

void
sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	sp_event_context_root_handler (SP_DESKTOP (data)->event_context, event);
}

/*
 * fixme: this conatins a hack, to deal with deleting a view, which is
 * completely on another view, in which case active_desktop will not be updated
 *
 */

void
sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	gpointer ddata;
	SPDesktop * desktop;

	ddata = gtk_object_get_data (GTK_OBJECT (item->canvas), "SPDesktop");
	g_return_if_fail (ddata != NULL);

	desktop = SP_DESKTOP (ddata);

	sp_event_context_item_handler (desktop->event_context, SP_ITEM (data), event);
}

gint
sp_desktop_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
	sp_active_desktop_set (SP_DESKTOP (widget));

	return FALSE;
}

typedef enum {
	SP_DT_NONE,
	SP_DT_HGUIDE,
	SP_DT_VGUIDE
} SPDTStateType;

static SPDTStateType state = SP_DT_NONE;
static SPGuide * hguide = NULL;
static SPGuide * vguide = NULL;

gint
sp_desktop_button_press (GtkWidget * widget, GdkEventButton * event)
{
	SPDesktop * desktop;

	if (event->button == 1) {
		desktop = SP_DESKTOP (widget);
		if (sp_inside_widget (GTK_WIDGET (desktop->hruler), event->x, event->y)) {
			gdk_pointer_grab (widget->window, FALSE,
				GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, NULL,
				event->time);
			state = SP_DT_HGUIDE;
			if (hguide == NULL) {
				hguide = (SPGuide *) gnome_canvas_item_new (desktop->guides,
					SP_TYPE_GUIDE,
					"orientation", SP_GUIDE_ORIENTATION_HORIZONTAL,
					NULL);
			}
			gnome_canvas_item_show (GNOME_CANVAS_ITEM (hguide));
			return FALSE;
		}
		if (sp_inside_widget (GTK_WIDGET (desktop->vruler), event->x, event->y)) {
			gdk_pointer_grab (widget->window, FALSE,
				GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, NULL,
				event->time);
			state = SP_DT_VGUIDE;
			if (vguide == NULL) {
				vguide = (SPGuide *) gnome_canvas_item_new (desktop->guides,
					SP_TYPE_GUIDE,
					"orientation", SP_GUIDE_ORIENTATION_VERTICAL,
					NULL);
			}
			gnome_canvas_item_show (GNOME_CANVAS_ITEM (vguide));
			return FALSE;
		}
	}
	return FALSE;
}

gint
sp_desktop_button_release (GtkWidget * widget, GdkEventButton * event)
{
	SPDesktop * desktop;

	if (event->button == 1) {
		desktop = SP_DESKTOP (widget);
		switch (state) {
		case SP_DT_HGUIDE:
			gdk_pointer_ungrab (event->time);
			gnome_canvas_item_hide (GNOME_CANVAS_ITEM (hguide));
			break;
		case SP_DT_VGUIDE:
			gdk_pointer_ungrab (event->time);
			gnome_canvas_item_hide (GNOME_CANVAS_ITEM (vguide));
			break;
		default:
			break;
		}
		state = SP_DT_NONE;
	}
	return FALSE;
}

gint
sp_desktop_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
	SPDesktop * desktop;
	ArtPoint p;

	desktop = SP_DESKTOP (widget);

	switch (state) {
	case SP_DT_HGUIDE:
		gnome_canvas_window_to_world (desktop->canvas,
			0, event->y - GTK_WIDGET (desktop->canvas)->allocation.y,
			&p.x, &p.y);
		sp_desktop_w2d_xy_point (desktop, &p, p.x, p.y);
		sp_guide_moveto (hguide, p.x, p.y);
		break;
	case SP_DT_VGUIDE:
		gnome_canvas_window_to_world (desktop->canvas,
			event->x - GTK_WIDGET (desktop->canvas)->allocation.x, 0,
			&p.x, &p.y);
		sp_desktop_w2d_xy_point (desktop, &p, p.x, p.y);
		sp_guide_moveto (vguide, p.x, p.y);
		break;
	default:
		break;
	}

	return FALSE;
}

static gboolean
sp_inside_widget (GtkWidget * widget, gint x, gint y)
{
	if (!GTK_WIDGET_VISIBLE (widget)) return FALSE;
	x -= widget->allocation.x;
	y -= widget->allocation.y;
	if (x < 0) return FALSE;
	if (y < 0) return FALSE;
	if (x >= widget->allocation.width) return FALSE;
	if (y >= widget->allocation.height) return FALSE;
	return TRUE;
}

