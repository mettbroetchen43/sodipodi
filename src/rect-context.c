#define __SP_RECT_CONTEXT_C__

/*
 * Rectangle drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <math.h>
#include "sp-rect.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-rect.xpm"
#include "rect-context.h"
#include "sp-metrics.h"

static void sp_rect_context_class_init (SPRectContextClass * klass);
static void sp_rect_context_init (SPRectContext * rect_context);
static void sp_rect_context_destroy (GtkObject * object);

static void sp_rect_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_rect_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_rect_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_rect_drag (SPRectContext * rc, double x, double y, guint state);
static void sp_rect_finish (SPRectContext * rc);

static SPEventContextClass * parent_class;

GtkType
sp_rect_context_get_type (void)
{
	static GtkType rect_context_type = 0;

	if (!rect_context_type) {

		static const GtkTypeInfo rect_context_info = {
			"SPRectContext",
			sizeof (SPRectContext),
			sizeof (SPRectContextClass),
			(GtkClassInitFunc) sp_rect_context_class_init,
			(GtkObjectInitFunc) sp_rect_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		rect_context_type = gtk_type_unique (sp_event_context_get_type (), &rect_context_info);
	}

	return rect_context_type;
}

static void
sp_rect_context_class_init (SPRectContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_rect_context_destroy;

	event_context_class->setup = sp_rect_context_setup;
	event_context_class->root_handler = sp_rect_context_root_handler;
	event_context_class->item_handler = sp_rect_context_item_handler;
}

static void
sp_rect_context_init (SPRectContext * rect_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (rect_context);

	event_context->cursor_shape = cursor_rect_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;

	rect_context->item = NULL;
}

static void
sp_rect_context_destroy (GtkObject * object)
{
	SPRectContext * rc;

	rc = SP_RECT_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (rc->item) sp_rect_finish (rc);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_rect_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPRectContext * rc;

	rc = SP_RECT_CONTEXT (event_context);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_rect_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_rect_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging;
	SPRectContext * rc;
	gint ret;
	SPDesktop * desktop;

	desktop = event_context->desktop;
	rc = SP_RECT_CONTEXT (event_context);
	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			/* Position center */
			sp_desktop_w2d_xy_point (event_context->desktop, &rc->center, event->button.x, event->button.y);
			/* Snap center to nearest magnetic point */
			sp_desktop_free_snap (event_context->desktop, &rc->center);
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
			sp_rect_drag (rc, p.x, p.y, event->motion.state);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			dragging = FALSE;
			sp_rect_finish (rc);
			ret = TRUE;
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
		}
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
sp_rect_drag (SPRectContext * rc, double x, double y, guint state)
{
	SPDesktop * desktop;
	ArtPoint p0, p1;
	gdouble x0, y0, x1, y1;
	GString * xs, * ys;
	gchar status[80];

	desktop = SP_EVENT_CONTEXT (rc)->desktop;

	if (!rc->item) {
		SPRepr * repr, * style;
		SPCSSAttr * css;
		/* Create object */
		repr = sp_repr_new ("rect");
		/* Set style */
		style = sodipodi_get_repr (SODIPODI, "paint.shape.rect");
		if (style) {
			css = sp_repr_css_attr_inherited (style, "style");
			sp_repr_css_set (repr, css, "style");
			sp_repr_css_attr_unref (css);
		}
		rc->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
	}

	/* This is bit ugly, but so we are */

	if (state & GDK_CONTROL_MASK) {
		gdouble dx, dy;
		/* fixme: Snapping */
		dx = x - rc->center.x;
		dy = y - rc->center.y;
		if ((fabs (dx) > fabs (dy)) && (dy != 0.0)) {
			dx = floor (dx/dy + 0.5) * dy;
		} else if (dx != 0.0) {
			dy = floor (dy/dx + 0.5) * dx;
		}
		p1.x = rc->center.x + dx;
		p1.y = rc->center.y + dy;
		if (state & GDK_SHIFT_MASK) {
			gdouble l0, l1;
			p0.x = rc->center.x - dx;
			p0.y = rc->center.y - dy;
			l0 = sp_desktop_vector_snap (desktop, &p0, p0.x - p1.x, p0.y - p1.y);
			l1 = sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
			if (l0 < l1) {
				p1.x = 2 * rc->center.x - p0.x;
				p1.y = 2 * rc->center.y - p0.y;
			} else {
				p0.x = 2 * rc->center.x - p1.x;
				p0.y = 2 * rc->center.y - p1.y;
			}
		} else {
			p0.x = rc->center.x;
			p0.y = rc->center.y;
			sp_desktop_vector_snap (desktop, &p1, p1.x - p0.x, p1.y - p0.y);
		}
	} else if (state & GDK_SHIFT_MASK) {
		double p0h, p0v, p1h, p1v;
		/* Corner point movements are bound */
		p0.x = 2 * rc->center.x - x;
		p0.y = 2 * rc->center.y - y;
		p1.x = x;
		p1.y = y;
		p0h = sp_desktop_horizontal_snap (desktop, &p0);
		p0v = sp_desktop_vertical_snap (desktop, &p0);
		p1h = sp_desktop_horizontal_snap (desktop, &p1);
		p1v = sp_desktop_vertical_snap (desktop, &p1);
		if (p0h < p1h) {
			/* Use Point 0 horizontal position */
			p1.x = 2 * rc->center.x - p0.x;
		} else {
			p0.x = 2 * rc->center.x - p1.x;
		}
		if (p0v < p1v) {
			/* Use Point 0 vertical position */
			p1.y = 2 * rc->center.y - p0.y;
		} else {
			p0.y = 2 * rc->center.y - p1.y;
		}
	} else {
		/* Free movement for corner point */
		p0.x = rc->center.x;
		p0.y = rc->center.y;
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

	sp_rect_set (SP_RECT (rc->item), x0, y0, x1 - x0, y1 - y0);

	// status text
	xs = SP_PT_TO_METRIC_STRING (fabs(x1-x0), SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (fabs(y1-y0), SP_DEFAULT_METRIC);
	sprintf (status, "Draw rectangle  %s x %s", xs->str, ys->str);
	sp_desktop_set_status (desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_rect_finish (SPRectContext * rc)
{
	if (rc->item != NULL) {
		SPDesktop * desktop;
		SPRect * rect;
		SPRepr * repr;

		desktop = SP_EVENT_CONTEXT (rc)->desktop;
		rect = SP_RECT (rc->item);
		repr = SP_OBJECT (rc->item)->repr;

		sp_repr_set_double_attribute (repr, "x", rect->x);
		sp_repr_set_double_attribute (repr, "y", rect->y);
		sp_repr_set_double_attribute (repr, "width", rect->width);
		sp_repr_set_double_attribute (repr, "height", rect->height);

		sp_selection_set_item (SP_DT_SELECTION (desktop), rc->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		rc->item = NULL;
	}
}

