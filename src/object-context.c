#define OBJECT_CONTEXT_C

#include <math.h>
#include <libgnomeui/gnome-canvas.h>
#include "xml/repr.h"
#include "sp-rect.h"
#include "sp-ellipse.h"
#include "event-broker.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "sp-cursor.h"
#include "pixmaps/cursor-rect.xpm"
#include "pixmaps/cursor-ellipse.xpm"
#include "object-context.h"

SPObjectType object_type = SP_OBJECT_NONE;
const gchar * object_name = NULL;

static void sp_object_set (SPRepr * repr, double x0, double y0, double x1, double y1);

void sp_object_context_set (SPDesktop * desktop, SPObjectType type)
{
	GtkWidget * w;
	GdkBitmap * bitmap, * mask;
	GdkCursor * cursor;
	
	object_type = type;

	switch (type) {
		case SP_OBJECT_RECT:
		sp_cursor_bitmap_and_mask_from_xpm (&bitmap, &mask,
			cursor_rect_xpm);
		object_name = "rect";
		break;
		case SP_OBJECT_ELLIPSE:
		sp_cursor_bitmap_and_mask_from_xpm (&bitmap, &mask,
			cursor_ellipse_xpm);
		object_name="ellipse";
		break;
		default:
		break;
	}
	w = GTK_WIDGET (SP_DT_CANVAS (desktop));
	cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
		&w->style->black, &w->style->white, 4, 4);
	gdk_window_set_cursor (w->window, cursor);
	gdk_cursor_destroy (cursor);
	gdk_bitmap_unref (bitmap);
	gdk_bitmap_unref (mask);
}

void sp_object_release (SPDesktop * desktop)
{
	GtkWidget * w;

	w = GTK_WIDGET (SP_DT_CANVAS (desktop));
	object_type = SP_OBJECT_NONE;
	object_name = NULL;
	gdk_window_set_cursor (w->window, NULL);
	return;
}

gint
sp_object_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event) {

	return FALSE;
}

gint
sp_object_root_handler (SPDesktop * desktop, GdkEvent * event)
{
	static int dragging;
	static double xs, ys;
	ArtPoint s, c;
	static SPRepr * repr = NULL;
	gint ret;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			s.x = xs = event->button.x;
			s.y = ys = event->button.y;
			dragging = TRUE;
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			double x0, y0, x1, y1, dx, dy;
			x0 = xs;
			y0 = ys;
			c.x = x1 = event->button.x;
			c.y = y1 = event->button.y;
			dx = x1 - xs;
			dy = y1 - ys;
			if (event->button.state & GDK_CONTROL_MASK) {
				if (fabs (dx) > fabs (dy)) {
					dx = fabs (dy) * dx / fabs (dx);
					x1 = xs + dx;
				} else {
					dy = fabs (dx) * dy / fabs (dy);
					y1 = ys + dy;
				}
			}
			if (event->button.state & GDK_SHIFT_MASK) {
				x0 = xs - dx;
				y0 = ys - dy;
			}
			sp_desktop_w2doc_xy_point (desktop, &s, x0, y0);
			c.x = x1;
			c.y = y1;
			sp_desktop_w2doc_xy_point (desktop, &c, x1, y1);
			x0 = MIN (s.x, c.x);
			y0 = MIN (s.y, c.y);
			x1 = MAX (s.x, c.x);
			y1 = MAX (s.y, c.y);
			if (repr == NULL) {
				repr = sp_repr_new_with_name (object_name);
				sp_repr_append_child (SP_ITEM (SP_DT_DOCUMENT (desktop))->repr, repr);
				sp_repr_unref (repr);
#if 0
				sp_selection_set_repr (repr);
#endif
			}
			sp_object_set (repr, x0, y0, x1, y1);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			dragging = FALSE;
			repr = NULL;
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return ret;
}

static void
sp_object_set (SPRepr * repr, double x0, double y0, double x1, double y1)
{
	switch (object_type) {
	case SP_OBJECT_RECT:
		sp_repr_set_double_attribute (repr, "x", x0);
		sp_repr_set_double_attribute (repr, "y", y0);
		sp_repr_set_double_attribute (repr, "width", x1 - x0);
		sp_repr_set_double_attribute (repr, "height", y1 - y0);
		break;
	case SP_OBJECT_ELLIPSE:
		sp_repr_set_attr (repr, "closed", "1");
		sp_repr_set_double_attribute (repr, "x", (x0 + x1) / 2);
		sp_repr_set_double_attribute (repr, "y", (y0 + y1) / 2);
		sp_repr_set_double_attribute (repr, "rx", (x1 - x0) / 2);
		sp_repr_set_double_attribute (repr, "ry", (y1 - y0) / 2);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

