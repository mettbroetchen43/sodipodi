#define __SP_ARC_CONTEXT_C__

/*
 * Ellipse drawing context
 *
 * Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Mitsuru Oka
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include "helper/sp-intl.h"
#include "helper/sp-canvas.h"
#include "xml/repr-private.h"
#include "verbs.h"
#include "sodipodi.h"
#include "sp-ellipse.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-arc.xpm"
#include "arc-context.h"
#include "sp-metrics.h"

static void sp_arc_context_class_init (SPArcContextClass *klass);
static void sp_arc_context_init (SPArcContext *arc_context);
static void sp_arc_context_dispose (GObject *object);

static gint sp_arc_context_root_handler (SPEventContext * event_context, GdkEvent * event);

static void sp_arc_drag (SPArcContext * ec, double x, double y, guint state);
static void sp_arc_finish (SPArcContext * ec);

static SPEventContextClass * parent_class;

GtkType
sp_arc_context_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPArcContextClass),
			NULL, NULL,
			(GClassInitFunc) sp_arc_context_class_init,
			NULL, NULL,
			sizeof (SPArcContext),
			4,
			(GInstanceInitFunc) sp_arc_context_init,
		};
		type = g_type_register_static (SP_TYPE_EVENT_CONTEXT, "SPArcContext", &info, 0);
	}
	return type;
}

static void
sp_arc_context_class_init (SPArcContextClass *klass)
{
	GObjectClass *object_class;
	SPEventContextClass *event_context_class;

	object_class = (GObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = sp_arc_context_dispose;

	event_context_class->root_handler = sp_arc_context_root_handler;
}

static void
sp_arc_context_init (SPArcContext *arc_context)
{
	SPEventContext *ec;
	ec = (SPEventContext *) arc_context;
	ec->verb = SP_VERB_CONTEXT_ARC;
	ec->cursor_shape = cursor_arc_xpm;
	ec->hot_x = 4;
	ec->hot_y = 4;
}

static void
sp_arc_context_dispose (GObject *object)
{
	SPArcContext * ac;

	ac = SP_ARC_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (ac->item) sp_arc_finish (ac);

	G_OBJECT_CLASS (parent_class)->dispose (object);
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
			NRPointF fp;
			dragging = TRUE;
			/* Position center */
			sp_desktop_w2d_xy_point (event_context->desktop, &fp,
									 (float) event->button.x, (float) event->button.y);
			ac->center.x = fp.x;
			ac->center.y = fp.y;
			/* Snap center to nearest magnetic point */
			sp_desktop_free_snap (event_context->desktop, &ac->center);
			sp_canvas_item_grab (SP_CANVAS_ITEM (desktop->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && event->motion.state && GDK_BUTTON1_MASK) {
			NRPointF p;
			sp_desktop_w2d_xy_point (event_context->desktop, &p,
					(float) event->motion.x, (float) event->motion.y);
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
		sp_canvas_item_ungrab (SP_CANVAS_ITEM (desktop->acetate), event->button.time);
		break;
	default:
		break;
	}

	if (!ret) {
		if (((SPEventContextClass *) parent_class)->root_handler)
			ret = ((SPEventContextClass *) parent_class)->root_handler (event_context, event);
	}

	return ret;
}

static void
sp_arc_drag (SPArcContext * ac, double x, double y, guint state)
{
	SPDesktop * desktop;
	NRPointF p0, p1;
	gdouble x0, y0, x1, y1;
	unsigned char c0[32], c1[32], c[256];

	desktop = SP_EVENT_CONTEXT (ac)->desktop;

	if (!ac->item) {
		ac->item = sp_event_context_create_item ((SPEventContext *) ac, "path", "arc", "tools.shapes.arc");
	}

	/* This is bit ugly, but so we are */

	if (state & GDK_CONTROL_MASK) {
		float dx, dy;
		/* fixme: Snapping */
		dx = x - ac->center.x;
		dy = y - ac->center.y;
		if ((fabs (dx) > fabs (dy)) && (dy != 0.0)) {
			dx = floor (dx / dy + 0.5) * dy;
		} else if (dx != 0.0) {
			dy = floor (dy / dx + 0.5) * dx;
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

	sp_desktop_dt2root_xy_point (desktop, &p0, p0.x, p0.y);
	sp_desktop_dt2root_xy_point (desktop, &p1, p1.x, p1.y);

	x0 = MIN (p0.x, p1.x);
	y0 = MIN (p0.y, p1.y);
	x1 = MAX (p0.x, p1.x);
	y1 = MAX (p0.y, p1.y);

	sp_arc_position_set (SP_ARC (ac->item), (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2);

	sp_desktop_write_length (desktop, c0, 32, fabs (x1 - x0));
	sp_desktop_write_length (desktop, c1, 32, fabs (y1 - y0));
	g_snprintf (c, 256, _("Drawing arc %s x %s"), c0, c1);
	sp_view_set_status (SP_VIEW (desktop), c, FALSE);
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

		sp_repr_set_double (repr, "sodipodi:cx", ellipse->cx.computed);
		sp_repr_set_double (repr, "sodipodi:cy", ellipse->cy.computed);
		sp_repr_set_double (repr, "sodipodi:rx", ellipse->rx.computed);
		sp_repr_set_double (repr, "sodipodi:ry", ellipse->ry.computed);

		sp_selection_set_item (SP_DT_SELECTION (desktop), ac->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		ac->item = NULL;
	}
}

