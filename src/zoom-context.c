#define ZOOM_CONTEXT_C

#include <math.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include "rubberband.h"
#include "zoom-context.h"
#include "event-broker.h"
#include "desktop.h"
#include "sp-cursor.h"
#include "pixmaps/cursor-zoom.xpm"

void sp_zoom_context_set (SPDesktop * desktop)
{
#if 0
	GtkWidget * w;
	GdkBitmap * bitmap, * mask;
	GdkCursor * cursor;
	
	sp_cursor_bitmap_and_mask_from_xpm (&bitmap, &mask,
		cursor_zoom_xpm);

	w = GTK_WIDGET (GNOME_CANVAS_ITEM (sp_desktop ())->canvas);
	cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
		&w->style->black, &w->style->white, 4, 4);
	gdk_window_set_cursor (w->window, cursor);
	gdk_cursor_destroy (cursor);
	gdk_bitmap_unref (bitmap);
	gdk_bitmap_unref (mask);
#endif
}

void sp_zoom_context_release (SPDesktop * desktop)
{
#if 0
	GtkWidget * w;

	w = GTK_WIDGET (GNOME_CANVAS_ITEM (sp_desktop ())->canvas);

	gdk_window_set_cursor (w->window, NULL);
#endif
}

gint
sp_zoom_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event) {

	return FALSE;
}

gint
sp_zoom_root_handler (SPDesktop * desktop, GdkEvent * event)
{
#if 0
	ArtPoint p;
	ArtDRect b;
	double x, y;
	double affine[6];
	gint ret;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_w2d_xy_point (&p, event->button.x, event->button.y);
			sp_rubberband_start (p.x, p.y);
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_w2d_xy_point (&p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				sp_desktop_show_region (b.x0, b.y0, b.x1, b.y1);
			} else {
				p.x = event->button.x;
				p.y = event->button.y;
				sp_w2d (&p, &p);
				sp_desktop_zoom (2, p.x, p.y);
				ret = TRUE;
			}
			break;
		case 3:
			p.x = event->button.x;
			p.y = event->button.y;
			sp_w2d (&p, &p);
			sp_desktop_zoom (0.5, p.x, p.y);
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
#else
	return FALSE;
#endif
}


/* Menu handlers */

void
sp_zoom_selection (GtkWidget * widget)
{
#if 0
	double d2w[6], w2d[6];
	ArtDRect d;

	if (sp_selection_list () == NULL)
		return;

	gnome_canvas_item_i2w_affine ((GnomeCanvasItem *) sp_desktop (), d2w);
	art_affine_invert (w2d, d2w);

	sp_selection_bbox (&d);
	art_drect_affine_transform (&d, &d, w2d);

	sp_desktop_show_region (d.x0, d.y0, d.x1, d.y1);
#endif
}

void
sp_zoom_drawing (GtkWidget * widget)
{
#if 0
	SPEditable * r;
	ArtDRect d;
	ArtPoint p0, p1;

	r = sp_repr_editable (sp_drawing_root ());
	if (r == NULL) return;
	sp_editable_bbox (r, &d);
	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;
	sp_w2d_xy_point (&p0, d.x0, d.y0);
	sp_w2d_xy_point (&p1, d.x1, d.y1);
	sp_desktop_show_region (p0.x, p0.y, p1.x, p1.y);
#endif
}

void
sp_zoom_page (GtkWidget * widget)
{
#if 0
	ArtDRect d;

	sp_drawing_dimensions (&d);
	sp_desktop_show_region (d.x0, d.y0, d.x1, d.y1);
#endif
}


