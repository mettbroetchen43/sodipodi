#define SP_ARC_CONTEXT_C

#include <math.h>
#include "sodipodi.h"
#include "sp-ellipse.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-arc.xpm"
#include "arc-context.h"
#include "sp-metrics.h"

static void sp_arc_context_class_init (SPArcContextClass * klass);
static void sp_arc_context_init (SPArcContext * arc_context);
static void sp_arc_context_destroy (GtkObject * object);

#if 0
static void sp_arc_context_setup (SPEventContext *ec);
#endif
static gint sp_arc_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_arc_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_arc_drag (SPArcContext * ec, double x, double y, guint state);
static void sp_arc_finish (SPArcContext * ec);

static SPEventContextClass * parent_class;

GtkType
sp_arc_context_get_type (void)
{
	static GtkType arc_context_type = 0;

	if (!arc_context_type) {

		static const GtkTypeInfo arc_context_info = {
			"SPArcContext",
			sizeof (SPArcContext),
			sizeof (SPArcContextClass),
			(GtkClassInitFunc) sp_arc_context_class_init,
			(GtkObjectInitFunc) sp_arc_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		arc_context_type = gtk_type_unique (sp_event_context_get_type (), &arc_context_info);
	}

	return arc_context_type;
}

static void
sp_arc_context_class_init (SPArcContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_arc_context_destroy;

#if 0
	event_context_class->setup = sp_arc_context_setup;
#endif
	event_context_class->root_handler = sp_arc_context_root_handler;
	event_context_class->item_handler = sp_arc_context_item_handler;
}

static void
sp_arc_context_init (SPArcContext * arc_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (arc_context);

	event_context->cursor_shape = cursor_arc_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;

	arc_context->item = NULL;
}

static void
sp_arc_context_destroy (GtkObject * object)
{
	SPArcContext * ac;

	ac = SP_ARC_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (ac->item) sp_arc_finish (ac);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

#if 0
static void
sp_arc_context_setup (SPEventContext *ec)
{
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}
#endif

static gint
sp_arc_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_arc_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging;
	SPArcContext * ac;
	gint ret;
	SPDesktop * desktop;

	desktop = event_context->desktop;
	ac = SP_ARC_CONTEXT (event_context);
	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			/* Position center */
			sp_desktop_w2d_xy_point (event_context->desktop, &ac->center, event->button.x, event->button.y);
			/* Snap center to nearest magnetic point */
			sp_desktop_free_snap (event_context->desktop, &ac->center);
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
			sp_arc_drag (ac, p.x, p.y, event->motion.state);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			dragging = FALSE;
			sp_arc_finish (ac);
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
sp_arc_drag (SPArcContext * ac, double x, double y, guint state)
{
	SPDesktop * desktop;
	ArtPoint p0, p1;
	gdouble x0, y0, x1, y1;
	GString * xs, * ys;
	gchar status[80];

	desktop = SP_EVENT_CONTEXT (ac)->desktop;

	if (!ac->item) {
		SPRepr * repr, * style;
		SPCSSAttr * css;
		/* Create object */
		repr = sp_repr_new ("path");
		sp_repr_set_attr (repr, "sodipodi:type", "arc");
		/* Set style */
		style = sodipodi_get_repr (SODIPODI, "paint.arc");
		if (style) {
			css = sp_repr_css_attr_inherited (style, "style");
			sp_repr_css_set (repr, css, "style");
			sp_repr_css_attr_unref (css);
		}
		ac->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
	}

	/* This is bit ugly, but so we are */

	if (state & GDK_CONTROL_MASK) {
		gdouble dx, dy;
		/* fixme: Snapping */
		dx = x - ac->center.x;
		dy = y - ac->center.y;
		if ((fabs (dx) > fabs (dy)) && (dy != 0.0)) {
			dx = floor (dx/dy + 0.5) * dy;
		} else if (dx != 0.0) {
			dy = floor (dy/dx + 0.5) * dx;
		}
		p1.x = ac->center.x + dx;
		p1.y = ac->center.y + dy;
		if (state & GDK_SHIFT_MASK) {
			gdouble l0, l1;
			p0.x = ac->center.x - dx;
			p0.y = ac->center.y - dy;
			l0 = sp_desktop_vector_snap (desktop, &p0, p0.x - p1.x, p0.y - p1.y);
			l1 = sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
			if (l0 < l1) {
				p1.x = 2 * ac->center.x - p0.x;
				p1.y = 2 * ac->center.y - p0.y;
			} else {
				p0.x = 2 * ac->center.x - p1.x;
				p0.y = 2 * ac->center.y - p1.y;
			}
		} else {
			p0.x = ac->center.x;
			p0.y = ac->center.y;
			sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
		}
	} else if (state & GDK_SHIFT_MASK) {
		double p0h, p0v, p1h, p1v;
		/* Corner point movements are bound */
		p0.x = 2 * ac->center.x - x;
		p0.y = 2 * ac->center.y - y;
		p1.x = x;
		p1.y = y;
		p0h = sp_desktop_horizontal_snap (desktop, &p0);
		p0v = sp_desktop_vertical_snap (desktop, &p0);
		p1h = sp_desktop_horizontal_snap (desktop, &p1);
		p1v = sp_desktop_vertical_snap (desktop, &p1);
		if (p0h < p1h) {
			/* Use Point 0 horizontal position */
			p1.x = 2 * ac->center.x - p0.x;
		} else {
			p0.x = 2 * ac->center.x - p1.x;
		}
		if (p0v < p1v) {
			/* Use Point 0 vertical position */
			p1.y = 2 * ac->center.y - p0.y;
		} else {
			p0.y = 2 * ac->center.y - p1.y;
		}
	} else {
		/* Free movement for corner point */
		p0.x = ac->center.x;
		p0.y = ac->center.y;
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

	sp_arc_set (SP_ARC (ac->item), (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2);

	// status text
	xs = SP_PT_TO_METRIC_STRING (fabs(x1-x0), SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (fabs(y1-y0), SP_DEFAULT_METRIC);
	sprintf (status, "Draw arc  %s x %s", xs->str, ys->str);
	sp_view_set_status (SP_VIEW (desktop), status, FALSE);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_arc_finish (SPArcContext * ac)
{
	if (ac->item != NULL) {
		SPDesktop * desktop;
		SPGenericEllipse * ellipse;
		SPRepr * repr;

		desktop = SP_EVENT_CONTEXT (ac)->desktop;
		ellipse = SP_GENERICELLIPSE (ac->item);
		repr = SP_OBJECT (ac->item)->repr;

		sp_repr_set_double_attribute (repr, "cx", ellipse->cx.computed);
		sp_repr_set_double_attribute (repr, "cy", ellipse->cy.computed);
		sp_repr_set_double_attribute (repr, "rx", ellipse->rx.computed);
		sp_repr_set_double_attribute (repr, "ry", ellipse->ry.computed);

		sp_selection_set_item (SP_DT_SELECTION (desktop), ac->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		ac->item = NULL;
	}
}

