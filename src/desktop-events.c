#define SP_DESKTOP_EVENTS_C

#include "helper/sp-guide.h"
#include "sodipodi-private.h"
#include "desktop.h"
#include "sp-guide.h"
#include "desktop-affine.h"
#include "event-context.h"
#include "sp-metrics.h"
#include "sp-item.h"
#include "desktop-events.h"

static gchar coord_str[80];

/* Root item handler */


gboolean
sp_canvas_root_handler (GnomeCanvasItem * item, GdkEvent * event, SPDesktop * desktop)
{
  ArtPoint p0;

  // realize canvas enter, motion & leave for mouse coordinate statusbar
 
  switch (event->type) {
  case GDK_MOTION_NOTIFY:
  case GDK_ENTER_NOTIFY:
    p0.x = event->motion.x;
    p0.y = event->motion.y;
    art_affine_point (&p0, &p0, desktop->w2d);
    //g_print ("%f %f\n",p0.x,p0.y);
    sprintf (coord_str, " %s, %s",
	     SP_PT_TO_STRING (p0.x,SP_DEFAULT_METRIC)->str,
	     SP_PT_TO_STRING (p0.y,SP_DEFAULT_METRIC)->str
	     );
    gnome_appbar_set_status (desktop->coord_status, coord_str);
    break;
  case GDK_LEAVE_NOTIFY:
    gnome_appbar_clear_stack (desktop->coord_status);
    break;
  default:
    break;
  }
  return FALSE;
}

void
sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, SPDesktop * desktop)
{
  sp_event_context_root_handler (desktop->event_context, event);

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


// not needed 
gint
sp_desktop_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  	sodipodi_activate_desktop (SP_DESKTOP (widget));
	return FALSE;
}

static gint
sp_dt_ruler_event (GtkWidget * widget, GdkEvent * event, gpointer data, gboolean horiz)
{
	static gboolean dragging = FALSE;
	static GnomeCanvasItem * guide = NULL;
	static ArtPoint p;
	SPDesktop * desktop;

	desktop = SP_DESKTOP (data);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			guide = gnome_canvas_item_new (desktop->guides, SP_TYPE_GUIDELINE,
						       "orientation",
						       horiz ? SP_GUIDELINE_ORIENTATION_HORIZONTAL : SP_GUIDELINE_ORIENTATION_VERTICAL,
						       "color", desktop->namedview->guidehicolor,
						       NULL);
			gdk_pointer_grab (widget->window, FALSE,
					  GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
					  NULL, NULL,
					  event->button.time);
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging) {
			gnome_canvas_window_to_world (desktop->canvas,
						      horiz ? 0 : event->motion.x - widget->allocation.width,
						      horiz ? event->motion.y - widget->allocation.height : 0,
						      &p.x, &p.y);
			sp_desktop_w2d_xy_point (desktop, &p, p.x, p.y);
			sp_guideline_moveto ((SPGuideLine *) guide, p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
			gdk_pointer_ungrab (event->button.time);
			dragging = FALSE;
			gtk_object_destroy (GTK_OBJECT (guide));
			guide = NULL;
			if ((horiz && (event->button.y > widget->allocation.height)) ||
				(!horiz && (event->button.x > widget->allocation.width))) {
				SPRepr * repr;
				repr = sp_repr_new ("sodipodi:guide");
				sp_repr_set_attr (repr, "orientation", horiz ? "horizontal" : "vertical");
				sp_repr_set_double_attribute (repr, "position", horiz ? p.y : p.x);
				sp_repr_append_child (SP_OBJECT (desktop->namedview)->repr, repr);
				sp_repr_unref (repr);
			}
		}
	default:
		break;
	}
	return FALSE;
}

gint
sp_dt_hruler_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	return sp_dt_ruler_event (widget, event, data, TRUE);
}

gint
sp_dt_vruler_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	return sp_dt_ruler_event (widget, event, data, FALSE);
}

/* Guides */

void
sp_dt_guide_event (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	static gboolean dragging = FALSE;
	SPGuide * guide;
	SPDesktop * desktop;
	ArtPoint p;

	guide = SP_GUIDE (data);
	desktop = SP_DESKTOP (gtk_object_get_data (GTK_OBJECT (item->canvas), "SPDesktop"));

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			gnome_canvas_item_grab (item, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
					  NULL, event->button.time);
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_guide_moveto (guide, p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
			GtkWidget * w;
			double winx, winy;
			gnome_canvas_item_ungrab (item, event->button.time);
			dragging = FALSE;
			w = GTK_WIDGET (item->canvas);
			gnome_canvas_world_to_window (item->canvas, event->button.x, event->button.y, &winx, &winy);
			if ((winx >= 0) && (winy >= 0) &&
			    (winx < w->allocation.width) &&
			    (winy < w->allocation.height)) {
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_repr_set_double_attribute (SP_OBJECT (data)->repr,
							      "position", (guide->orientation == SP_GUIDE_HORIZONTAL) ? p.y : p.x);
			} else {
				sp_repr_unparent (SP_OBJECT (guide)->repr);
			}
		}
	case GDK_ENTER_NOTIFY:
		gnome_canvas_item_set (item, "color", guide->hicolor, NULL);
		break;
	case GDK_LEAVE_NOTIFY:
		gnome_canvas_item_set (item, "color", guide->color, NULL);
		break;
	default:
		break;
	}
}





