#define SP_ELLIPSE_CONTEXT_C

#include <math.h>
#include "sodipodi.h"
#include "sp-ellipse.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-ellipse.xpm"
#include "ellipse-context.h"
#include "sp-metrics.h"

static void sp_ellipse_context_class_init (SPEllipseContextClass * klass);
static void sp_ellipse_context_init (SPEllipseContext * ellipse_context);
static void sp_ellipse_context_destroy (GtkObject * object);

static void sp_ellipse_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_ellipse_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_ellipse_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_ellipse_drag (SPEllipseContext * ec, double x, double y, guint state);
static void sp_ellipse_finish (SPEllipseContext * ec);

static SPEventContextClass * parent_class;

GtkType
sp_ellipse_context_get_type (void)
{
	static GtkType ellipse_context_type = 0;

	if (!ellipse_context_type) {

		static const GtkTypeInfo ellipse_context_info = {
			"SPEllipseContext",
			sizeof (SPEllipseContext),
			sizeof (SPEllipseContextClass),
			(GtkClassInitFunc) sp_ellipse_context_class_init,
			(GtkObjectInitFunc) sp_ellipse_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		ellipse_context_type = gtk_type_unique (sp_event_context_get_type (), &ellipse_context_info);
	}

	return ellipse_context_type;
}

static void
sp_ellipse_context_class_init (SPEllipseContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_ellipse_context_destroy;

	event_context_class->setup = sp_ellipse_context_setup;
	event_context_class->root_handler = sp_ellipse_context_root_handler;
	event_context_class->item_handler = sp_ellipse_context_item_handler;
}

static void
sp_ellipse_context_init (SPEllipseContext * ellipse_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (ellipse_context);

	event_context->cursor_shape = cursor_ellipse_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;

	ellipse_context->item = NULL;
}

static void
sp_ellipse_context_destroy (GtkObject * object)
{
	SPEllipseContext * ec;

	ec = SP_ELLIPSE_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (ec->item) sp_ellipse_finish (ec);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ellipse_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_ellipse_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_ellipse_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging;
	SPEllipseContext * ec;
	gint ret;
	SPDesktop * desktop;

	desktop = event_context->desktop;
	ec = SP_ELLIPSE_CONTEXT (event_context);
	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			/* Position center */
			sp_desktop_w2d_xy_point (event_context->desktop, &ec->center, event->button.x, event->button.y);
			/* Snap center to nearest magnetic point */
			sp_desktop_free_snap (event_context->desktop, &ec->center);
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && event->motion.state && GDK_BUTTON1_MASK) {
			ArtPoint p;
			sp_desktop_w2d_xy_point (event_context->desktop, &p, event->motion.x, event->motion.y);
			sp_ellipse_drag (ec, p.x, p.y, event->motion.state);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			dragging = FALSE;
			sp_ellipse_finish (ec);
			ret = TRUE;
		}
		gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
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
sp_ellipse_drag (SPEllipseContext * ec, double x, double y, guint state)
{
	SPDesktop * desktop;
	ArtPoint p0, p1;
	gdouble x0, y0, x1, y1;
	GString * xs, * ys;
	gchar status[80];

	desktop = SP_EVENT_CONTEXT (ec)->desktop;

	if (!ec->item) {
		SPRepr * repr, * style;
		SPCSSAttr * css;
		/* Create object */
		repr = sp_repr_new ("ellipse");
		/* Set style */
		style = sodipodi_get_repr (SODIPODI, "paint.shape.ellipse");
		if (style) {
			css = sp_repr_css_attr_inherited (style, "style");
			sp_repr_css_set (repr, css, "style");
			sp_repr_css_attr_unref (css);
		}
		ec->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
	}

	/* This is bit ugly, but so we are */

	if (state & GDK_CONTROL_MASK) {
		gdouble dx, dy;
		/* fixme: Snapping */
		dx = x - ec->center.x;
		dy = y - ec->center.y;
		if ((fabs (dx) > fabs (dy)) && (dy != 0.0)) {
			dx = floor (dx/dy + 0.5) * dy;
		} else if (dx != 0.0) {
			dy = floor (dy/dx + 0.5) * dx;
		}
		p1.x = ec->center.x + dx;
		p1.y = ec->center.y + dy;
		if (state & GDK_SHIFT_MASK) {
			gdouble l0, l1;
			p0.x = ec->center.x - dx;
			p0.y = ec->center.y - dy;
			l0 = sp_desktop_vector_snap (desktop, &p0, p0.x - p1.x, p0.y - p1.y);
			l1 = sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
			if (l0 < l1) {
				p1.x = 2 * ec->center.x - p0.x;
				p1.y = 2 * ec->center.y - p0.y;
			} else {
				p0.x = 2 * ec->center.x - p1.x;
				p0.y = 2 * ec->center.y - p1.y;
			}
		} else {
			p0.x = ec->center.x;
			p0.y = ec->center.y;
			sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
		}
	} else if (state & GDK_SHIFT_MASK) {
		double p0h, p0v, p1h, p1v;
		/* Corner point movements are bound */
		p0.x = 2 * ec->center.x - x;
		p0.y = 2 * ec->center.y - y;
		p1.x = x;
		p1.y = y;
		p0h = sp_desktop_horizontal_snap (desktop, &p0);
		p0v = sp_desktop_vertical_snap (desktop, &p0);
		p1h = sp_desktop_horizontal_snap (desktop, &p1);
		p1v = sp_desktop_vertical_snap (desktop, &p1);
		if (p0h < p1h) {
			/* Use Point 0 horizontal position */
			p1.x = 2 * ec->center.x - p0.x;
		} else {
			p0.x = 2 * ec->center.x - p1.x;
		}
		if (p0v < p1v) {
			/* Use Point 0 vertical position */
			p1.y = 2 * ec->center.y - p0.y;
		} else {
			p0.y = 2 * ec->center.y - p1.y;
		}
	} else {
		/* Free movement for corner point */
		p0.x = ec->center.x;
		p0.y = ec->center.y;
		p1.x = x;
		p1.y = y;
		sp_desktop_free_snap (desktop, &p1);
	}

	sp_desktop_d2doc_xy_point (desktop, &p0, p0.x, p0.y);
	sp_desktop_d2doc_xy_point (desktop, &p1, p1.x, p1.y);

	x0 = MIN (p0.x, p1.x);
	y0 = MIN (p0.y, p1.y);
	x1 = MAX (p0.x, p1.x);
	y1 = MAX (p0.y, p1.y);

	sp_ellipse_set (SP_ELLIPSE (ec->item), (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2);

	// status text
	xs = SP_PT_TO_METRIC_STRING (fabs(x1-x0), SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (fabs(y1-y0), SP_DEFAULT_METRIC);
	sprintf (status, "Draw ellipse  %s x %s", xs->str, ys->str);
	sp_desktop_set_status (desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_ellipse_finish (SPEllipseContext * ec)
{
	if (ec->item != NULL) {
		SPDesktop * desktop;
		SPGenericEllipse * ellipse;
		SPRepr * repr;

		desktop = SP_EVENT_CONTEXT (ec)->desktop;
		ellipse = SP_GENERICELLIPSE (ec->item);
		repr = SP_OBJECT (ec->item)->repr;

		sp_repr_set_double_attribute (repr, "cx", ellipse->x);
		sp_repr_set_double_attribute (repr, "cy", ellipse->y);
		sp_repr_set_double_attribute (repr, "rx", ellipse->rx);
		sp_repr_set_double_attribute (repr, "ry", ellipse->ry);

		sp_selection_set_item (SP_DT_SELECTION (desktop), ec->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		ec->item = NULL;
	}
}

