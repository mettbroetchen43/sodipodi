#define __SP_DRAW_CONTEXT_C__

/*
 * Generic drawing context
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

#define DRAW_VERBOSE

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/bezier-utils.h"
#include "helper/sodipodi-ctrl.h"
#include "helper/sp-ctrlline.h"
#include "helper/canvas-bpath.h"

#include "sodipodi.h"
#include "document.h"
#include "sp-path.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "draw-context.h"

#define TOLERANCE 1.0

/* Drawing anchors */

struct _SPDrawAnchor {
	SPDrawContext *dc;
	SPCurve *curve;
	guint start : 1;
	guint active : 1;
	ArtPoint dp, wp;
	GnomeCanvasItem *ctrl;
};

static void sp_draw_context_class_init (SPDrawContextClass *klass);
static void sp_draw_context_init (SPDrawContext *dc);
static void sp_draw_context_finalize (GtkObject *object);

static void sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);

static void spdc_selection_changed (SPSelection *sel, SPDrawContext *dc);
static void spdc_selection_modified (SPSelection *sel, guint flags, SPDrawContext *dc);
static void spdc_read_selection (SPDrawContext *dc, SPSelection *sel);

static void spdc_concat_colors_and_flush (SPDrawContext *dc);
static void spdc_flush_white (SPDrawContext *dc, SPCurve *gc);

static void spdc_reset_colors (SPDrawContext *dc);
static void spdc_free_colors (SPDrawContext *dc);

static SPDrawAnchor *test_inside (SPDrawContext * dc, gdouble wx, gdouble wy);
static SPDrawAnchor *sp_draw_anchor_test (SPDrawAnchor *anchor, gdouble wx, gdouble wy, gboolean activate);

static void fit_and_split (SPDrawContext * dc);

static SPDrawAnchor *sp_draw_anchor_new (SPDrawContext *dc, SPCurve *curve, gboolean start, gdouble x, gdouble y);
static SPDrawAnchor *sp_draw_anchor_destroy (SPDrawAnchor *anchor);

static SPEventContextClass *draw_parent_class;

GtkType
sp_draw_context_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPDrawContext",
			sizeof (SPDrawContext),
			sizeof (SPDrawContextClass),
			(GtkClassInitFunc) sp_draw_context_class_init,
			(GtkObjectInitFunc) sp_draw_context_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_EVENT_CONTEXT, &info);
	}

	return type;
}

static void
sp_draw_context_class_init (SPDrawContextClass *klass)
{
	GtkObjectClass *object_class;
	SPEventContextClass *event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	draw_parent_class = gtk_type_class (SP_TYPE_EVENT_CONTEXT);

	object_class->finalize = sp_draw_context_finalize;

	event_context_class->setup = sp_draw_context_setup;
}

static void
sp_draw_context_init (SPDrawContext *dc)
{
	dc->npoints = 0;
}

static void
sp_draw_context_finalize (GtkObject *object)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (object);

	gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_SELECTION (SP_EVENT_CONTEXT_DESKTOP (dc))), dc);

	spdc_free_colors (dc);

	GTK_OBJECT_CLASS (draw_parent_class)->finalize (object);
}

static void
sp_draw_context_setup (SPEventContext *ec, SPDesktop *dt)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (draw_parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (draw_parent_class)->setup (ec, dt);

	/* Connect signals to track selection changes */
	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (dt)), "changed", GTK_SIGNAL_FUNC (spdc_selection_changed), dc);
	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (dt)), "modified", GTK_SIGNAL_FUNC (spdc_selection_modified), dc);

	/* Create red bpath */
	dc->red_bpath = sp_canvas_bpath_new (SP_DT_SKETCH (ec->desktop), NULL);
	sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (dc->red_bpath), 0xff00007f, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);
	/* Create red curve */
	dc->red_curve = sp_curve_new_sized (4);

	/* Create blue bpath */
	dc->blue_bpath = sp_canvas_bpath_new (SP_DT_SKETCH (ec->desktop), NULL);
	sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (dc->blue_bpath), 0x0000ff7f, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);
	/* Create blue curve */
	dc->blue_curve = sp_curve_new_sized (8);

	/* Create green curve */
	dc->green_curve = sp_curve_new_sized (64);
	/* No green anchor by default */
	dc->green_anchor = NULL;

	spdc_read_selection (dc, SP_DT_SELECTION (dt));
}

/*
 * Selection handlers
 */

static void
spdc_selection_changed (SPSelection *sel, SPDrawContext *dc)
{
	g_print ("Selection changed in draw context\n");
	spdc_read_selection (dc, sel);
}

/* fixme: We have to ensure this is not delayed (Lauris) */

static void
spdc_selection_modified (SPSelection *sel, guint flags, SPDrawContext *dc)
{
	g_print ("Selection modified in draw context\n");
	spdc_read_selection (dc, sel);
}

static void
spdc_read_selection (SPDrawContext *dc, SPSelection *sel)
{
	SPItem *item;

	spdc_reset_colors (dc);

	/* fixme: Think (Lauris) */
	dc->sa = NULL;
	dc->ea = NULL;

	item = sp_selection_item (SP_DT_SELECTION (SP_EVENT_CONTEXT_DESKTOP (dc)));

	if (item && SP_IS_PATH (item) && SP_PATH (item)->independent) {
		SPCurve *norm;
		gdouble i2dt[6];
		GSList *l;
		/* Create new white data */
		/* Item */
		dc->white_item = item;
		/* Curve list */
		/* We keep it in desktop coordinates to eliminate calculation errors */
		norm = sp_path_normalized_bpath (SP_PATH (item));
		sp_item_i2d_affine (dc->white_item, i2dt);
		norm = sp_curve_transform (norm, i2dt);
		g_return_if_fail (norm != NULL);
		dc->white_curves = sp_curve_split (norm);
		sp_curve_unref (norm);
		/* Anchor list */
		for (l = dc->white_curves; l != NULL; l = l->next) {
			SPCurve *c;
			c = l->data;
			g_return_if_fail (c->end > 1);
			if (c->bpath->code == ART_MOVETO_OPEN) {
				ArtBpath *s, *e;
				SPDrawAnchor *a;
				s = sp_curve_first_bpath (c);
				e = sp_curve_last_bpath (c);
				a = sp_draw_anchor_new (dc, c, TRUE, s->x3, s->y3);
				dc->white_anchors = g_slist_prepend (dc->white_anchors, a);
				a = sp_draw_anchor_new (dc, c, FALSE, e->x3, e->y3);
				dc->white_anchors = g_slist_prepend (dc->white_anchors, a);
			}
		}
		/* fixme: recalculate active anchor, reset red (DONE), green (DONE) and blue (DONE) , release drags */
		/* The latter is not needed, if we can ensure that 'modified' is not delayed */
	}
}

static void
spdc_endpoint_snap (SPDrawContext *dc, ArtPoint *p, guint state)
{
	if (state & GDK_CONTROL_MASK) {
		/* Constrained motion */
		if (fabs (p->x - dc->p[0].x) > fabs (p->y - dc->p[0].y)) {
			/* Horizontal */
			p->y = dc->p[0].y;
			sp_desktop_horizontal_snap (SP_EVENT_CONTEXT_DESKTOP (dc), p);
		} else {
			/* Vertical */
			p->x = dc->p[0].x;
			sp_desktop_vertical_snap (SP_EVENT_CONTEXT_DESKTOP (dc), p);
		}
	} else {
		/* Free */
		sp_desktop_free_snap (SP_EVENT_CONTEXT_DESKTOP (dc), p);
	}
}

/*
 * Concats red, blue and green
 * If any anchors are defined, process these, optionally removing curves from white list
 * Invoke _flush_white to write result back to object
 *
 */

static void
spdc_concat_colors_and_flush (SPDrawContext *dc)
{
	SPCurve *c;

	/* Concat RBG */
	c = dc->green_curve;

	/* Green */
	dc->green_curve = sp_curve_new_sized (64);
	while (dc->green_bpaths) {
		gtk_object_destroy (GTK_OBJECT (dc->green_bpaths->data));
		dc->green_bpaths = g_slist_remove (dc->green_bpaths, dc->green_bpaths->data);
	}
	/* Blue */
	sp_curve_append_continuous (c, dc->blue_curve, 1e-9);
	sp_curve_reset (dc->blue_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->blue_bpath), NULL);
	/* Red */
	sp_curve_append_continuous (c, dc->red_curve, 1e-9);
	sp_curve_reset (dc->red_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);

	/* Step A - test, whether we ended on green anchor */
	if (dc->green_anchor && dc->green_anchor->active) {
		g_print ("We hit green anchor, closing Green-Blue-Red\n");
		sp_curve_closepath_current (c);
		/* Closed path, just flush */
		spdc_flush_white (dc, c);
		sp_curve_unref (c);
		return;
	}

	/* Step B - both start and end anchored to same curve */
	if (dc->sa && dc->ea && (dc->sa->curve == dc->ea->curve)) {
		g_print ("We hit bot start and end of single curve, closing paths\n");
		if (dc->sa->start) {
			SPCurve *r;
			g_print ("Reversing curve\n");
			r = sp_curve_reverse (c);
			sp_curve_unref (c);
			c = r;
		}
		sp_curve_append_continuous (dc->sa->curve, c, 1e-9);
		sp_curve_unref (c);
		sp_curve_closepath_current (dc->sa->curve);
		spdc_flush_white (dc, NULL);
		return;
	}

	/* Step C - test start */
	if (dc->sa) {
		SPCurve *s;
		g_print ("Curve start hit anchor\n");
		s = dc->sa->curve;
		dc->white_curves = g_slist_remove (dc->white_curves, s);
		if (dc->sa->start) {
			SPCurve *r;
			g_print ("Reversing curve\n");
			r = sp_curve_reverse (s);
			sp_curve_unref (s);
			s = r;
		}
		sp_curve_append_continuous (s, c, 1e-9);
		sp_curve_unref (c);
		c = s;
	}

	/* Step D - test end */
	if (dc->ea) {
		SPCurve *e;
		g_print ("Curve end hit anchor\n");
		e = dc->ea->curve;
		dc->white_curves = g_slist_remove (dc->white_curves, e);
		if (!dc->ea->start) {
			SPCurve *r;
			g_print ("Reversing curve\n");
			r = sp_curve_reverse (e);
			sp_curve_unref (e);
			e = r;
		}
		sp_curve_append_continuous (c, e, 1e-9);
		sp_curve_unref (e);
	}

	spdc_flush_white (dc, c);

	sp_curve_unref (c);
}

/*
 * Flushes white curve(s) and additional curve into object
 *
 * No cleaning of colored curves - this has to be done by caller
 * No rereading of white data, so if you cannot rely on ::modified, do it in caller
 *
 */

static void
spdc_flush_white (SPDrawContext *dc, SPCurve *gc)
{
	SPCurve *c;

	if (dc->white_curves) {
		g_assert (dc->white_item);
		c = sp_curve_concat (dc->white_curves);
		g_slist_free (dc->white_curves);
		dc->white_curves = NULL;
		if (gc) {
			sp_curve_append (c, gc, FALSE);
		}
	} else if (gc) {
		c = gc;
		sp_curve_ref (c);
	} else {
		return;
	}

	/* Now we have to go back to item coordinates at last */
	if (dc->white_item) {
		gdouble d2item[6];
		sp_item_dt2i_affine (dc->white_item, SP_EVENT_CONTEXT_DESKTOP (dc), d2item);
		c = sp_curve_transform (c, d2item);
	} else {
		gdouble d2item[6];
		sp_desktop_d2doc_affine (SP_EVENT_CONTEXT_DESKTOP (dc), d2item);
		c = sp_curve_transform (c, d2item);
	}

	if (c && !sp_curve_empty (c)) {
		SPDesktop *dt;
		SPDocument *doc;
		SPRepr *repr;
		gchar *str;

		/* We actually have something to write */

		dt = SP_EVENT_CONTEXT_DESKTOP (dc);
		doc = SP_DT_DOCUMENT (dt);

		if (dc->white_item) {
			repr = SP_OBJECT_REPR (dc->white_item);
		} else {
			SPRepr *style;
			repr = sp_repr_new ("path");
			style = sodipodi_get_repr (SODIPODI, "paint.freehand");
			if (style) {
				SPCSSAttr *css;
				css = sp_repr_css_attr_inherited (style, "style");
				sp_repr_css_set (repr, css, "style");
				sp_repr_css_attr_unref (css);
			}
		}

		str = sp_svg_write_path (SP_CURVE_BPATH (c));
		g_assert (str != NULL);
		sp_repr_set_attr (repr, "d", str);
		g_free (str);

		if (!dc->white_item) {
			/* Attach repr */
			sp_document_add_repr (SP_DT_DOCUMENT (dt), repr);
			sp_selection_set_repr (SP_DT_SELECTION (dt), repr);
			sp_repr_unref (repr);
		}

		sp_document_done (doc);
	}

	sp_curve_unref (c);

	/* Flush pending updates */
	sp_document_ensure_up_to_date (SP_DT_DOCUMENT (SP_EVENT_CONTEXT_DESKTOP (dc)));
}

/*
 * Returns FIRST active anchor (the activated one)
 */

static SPDrawAnchor *
test_inside (SPDrawContext *dc, gdouble wx, gdouble wy)
{
	GSList *l;
	SPDrawAnchor *active;

	active = NULL;

	/* Test green anchor */
	if (dc->green_anchor) {
		active = sp_draw_anchor_test (dc->green_anchor, wx, wy, TRUE);
	}

	for (l = dc->white_anchors; l != NULL; l = l->next) {
		SPDrawAnchor *na;
		na = sp_draw_anchor_test ((SPDrawAnchor *) l->data, wx, wy, !active);
		if (!active && na) active = na;
	}

	return active;
}

static void
fit_and_split (SPDrawContext * dc)
{
	ArtPoint b[4];
	gdouble tolerance;

	g_assert (dc->npoints > 1);

	tolerance = SP_EVENT_CONTEXT (dc)->desktop->w2d[0] * TOLERANCE;
	tolerance = tolerance * tolerance;

	if (sp_bezier_fit_cubic (b, dc->p, dc->npoints, tolerance) > 0 && dc->npoints < SP_DRAW_POINTS_MAX) {
		/* Fit and draw and reset state */
		sp_curve_reset (dc->red_curve);
		sp_curve_moveto (dc->red_curve, b[0].x, b[0].y);
		sp_curve_curveto (dc->red_curve, b[1].x, b[1].y, b[2].x, b[2].y, b[3].x, b[3].y);
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), dc->red_curve);
	} else {
		SPCurve *curve;
		GnomeCanvasItem *cshape;
		/* Fit and draw and copy last point */
		g_assert (!sp_curve_empty (dc->red_curve));
		sp_curve_append_continuous (dc->green_curve, dc->red_curve, 1e-9);
		curve = sp_curve_copy (dc->red_curve);

		/* fixme: */
		cshape = sp_canvas_bpath_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop), curve);
		sp_curve_unref (curve);
		sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (cshape), 0x00bf00ff, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);

		dc->green_bpaths = g_slist_prepend (dc->green_bpaths, cshape);

		dc->p[0] = dc->p[dc->npoints - 2];
		dc->p[1] = dc->p[dc->npoints - 1];
		dc->npoints = 2;
	}
}

static void
spdc_reset_colors (SPDrawContext *dc)
{
	/* Red */
	sp_curve_reset (dc->red_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
	/* Blue */
	sp_curve_reset (dc->blue_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->blue_bpath), NULL);
	/* Green */
	while (dc->green_bpaths) {
		gtk_object_destroy (GTK_OBJECT (dc->green_bpaths->data));
		dc->green_bpaths = g_slist_remove (dc->green_bpaths, dc->green_bpaths->data);
	}
	sp_curve_reset (dc->green_curve);
	if (dc->green_anchor) {
		dc->green_anchor = sp_draw_anchor_destroy (dc->green_anchor);
	}
	/* White */
	if (dc->white_item) {
		/* We do not hold refcount */
		dc->white_item = NULL;
	}
	while (dc->white_curves) {
		sp_curve_unref ((SPCurve *) dc->white_curves->data);
		dc->white_curves = g_slist_remove (dc->white_curves, dc->white_curves->data);
	}
	while (dc->white_anchors) {
		sp_draw_anchor_destroy ((SPDrawAnchor *) dc->white_anchors->data);
		dc->white_anchors = g_slist_remove (dc->white_anchors, dc->white_anchors->data);
	}
}

static void
spdc_free_colors (SPDrawContext *dc)
{
	/* Red */
	if (dc->red_bpath) {
		gtk_object_destroy (GTK_OBJECT (dc->red_bpath));
		dc->red_bpath = NULL;
	}
	if (dc->red_curve) {
		dc->red_curve = sp_curve_unref (dc->red_curve);
	}
	/* Blue */
	if (dc->blue_bpath) {
		gtk_object_destroy (GTK_OBJECT (dc->blue_bpath));
		dc->blue_bpath = NULL;
	}
	if (dc->blue_curve) {
		dc->blue_curve = sp_curve_unref (dc->blue_curve);
	}
	/* Green */
	while (dc->green_bpaths) {
		gtk_object_destroy (GTK_OBJECT (dc->green_bpaths->data));
		dc->green_bpaths = g_slist_remove (dc->green_bpaths, dc->green_bpaths->data);
	}
	if (dc->green_curve) {
		dc->green_curve = sp_curve_unref (dc->green_curve);
	}
	if (dc->green_anchor) {
		dc->green_anchor = sp_draw_anchor_destroy (dc->green_anchor);
	}
	/* White */
	if (dc->white_item) {
		/* We do not hold refcount */
		dc->white_item = NULL;
	}
	while (dc->white_curves) {
		sp_curve_unref ((SPCurve *) dc->white_curves->data);
		dc->white_curves = g_slist_remove (dc->white_curves, dc->white_curves->data);
	}
	while (dc->white_anchors) {
		sp_draw_anchor_destroy ((SPDrawAnchor *) dc->white_anchors->data);
		dc->white_anchors = g_slist_remove (dc->white_anchors, dc->white_anchors->data);
	}
}

/*
 * Anchors
 */

static SPDrawAnchor *
sp_draw_anchor_new (SPDrawContext *dc, SPCurve *curve, gboolean start, gdouble dx, gdouble dy)
{
	SPDrawAnchor *a;

	a = g_new (SPDrawAnchor, 1);

	a->dc = dc;
	a->curve = curve;
	a->start = start;
	a->active = FALSE;
	a->dp.x = dx;
	a->dp.y = dy;
	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT_DESKTOP (dc), &a->wp, dx, dy);
	a->ctrl = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT_DESKTOP (dc)), SP_TYPE_CTRL,
					 "size", 4.0,
					 "filled", 0,
					 "fill_color", 0xff00007f,
					 "stroked", 1,
					 "stroke_color", 0x000000ff,
					 NULL);

	sp_ctrl_moveto (SP_CTRL (a->ctrl), dx, dy);

	return a;
}

static SPDrawAnchor *
sp_draw_anchor_destroy (SPDrawAnchor *anchor)
{
	if (anchor->ctrl) {
		gtk_object_destroy (GTK_OBJECT (anchor->ctrl));
	}
	g_free (anchor);
	return NULL;
}

#define A_SNAP 4.0

static SPDrawAnchor *
sp_draw_anchor_test (SPDrawAnchor *anchor, gdouble wx, gdouble wy, gboolean activate)
{
	if (activate && (fabs (wx - anchor->wp.x) <= A_SNAP) && (fabs (wy - anchor->wp.y) <= A_SNAP)) {
		if (!anchor->active) {
			gnome_canvas_item_set (anchor->ctrl, "filled", TRUE, NULL);
			anchor->active = TRUE;
		}
		return anchor;
	}

	if (anchor->active) {
		gnome_canvas_item_set (anchor->ctrl, "filled", FALSE, NULL);
		anchor->active = FALSE;
	}
	return NULL;
}

/* Pencil context */

static void sp_pencil_context_class_init (SPPencilContextClass *klass);
static void sp_pencil_context_init (SPPencilContext *dc);
static void sp_pencil_context_finalize (GtkObject *object);

static void sp_pencil_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_pencil_context_root_handler (SPEventContext * event_context, GdkEvent * event);

static void spdc_set_startpoint (SPPencilContext *dc, ArtPoint *p, guint state);
static void spdc_set_endpoint (SPPencilContext *dc, ArtPoint *p, guint state);
static void spdc_finish_endpoint (SPPencilContext *dc, ArtPoint *p, gboolean snap, guint state);
static void spdc_add_freehand_point (SPPencilContext *dc, ArtPoint *p, guint state);

static SPDrawContextClass *pencil_parent_class;

GtkType
sp_pencil_context_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPPencilContext",
			sizeof (SPPencilContext),
			sizeof (SPPencilContextClass),
			(GtkClassInitFunc) sp_pencil_context_class_init,
			(GtkObjectInitFunc) sp_pencil_context_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_DRAW_CONTEXT, &info);
	}

	return type;
}

static void
sp_pencil_context_class_init (SPPencilContextClass *klass)
{
	GtkObjectClass *object_class;
	SPEventContextClass *event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	pencil_parent_class = gtk_type_class (SP_TYPE_DRAW_CONTEXT);

	object_class->finalize = sp_pencil_context_finalize;

	event_context_class->setup = sp_pencil_context_setup;
	event_context_class->root_handler = sp_pencil_context_root_handler;
}

static void
sp_pencil_context_init (SPPencilContext *pc)
{
	pc->state = SP_PENCIL_CONTEXT_IDLE;
}

static void
sp_pencil_context_finalize (GtkObject *object)
{
	SPPencilContext *pc;

	pc = SP_PENCIL_CONTEXT (object);

	GTK_OBJECT_CLASS (pencil_parent_class)->finalize (object);
}

static void
sp_pencil_context_setup (SPEventContext *ec, SPDesktop *dt)
{
	SPPencilContext *pc;

	pc = SP_PENCIL_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (pencil_parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (pencil_parent_class)->setup (ec, dt);
}

gint
sp_pencil_context_root_handler (SPEventContext *ec, GdkEvent *event)
{
	SPDrawContext *dc;
	SPPencilContext *pc;
	SPDesktop *dt;
	ArtPoint p;
	gint ret;
	SPDrawAnchor *anchor;

	dc = SP_DRAW_CONTEXT (ec);
	pc = SP_PENCIL_CONTEXT (ec);
	dt = ec->desktop;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			/* Grab mouse, so release will not pass unnoticed */
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (dt->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			/* Find desktop coordinates */
			sp_desktop_w2d_xy_point (dt, &p, event->button.x, event->button.y);
			/* Test, whether we hit any anchor */
			anchor = test_inside (dc, event->button.x, event->button.y);

			switch (pc->state) {
			case SP_PENCIL_CONTEXT_ADDLINE:
				/* Current segment will be finished with release */
				ret = TRUE;
				break;
			default:
				/* Set first point of sequence */
				if (anchor) {
					p = anchor->dp;
				}
				dc->sa = anchor;
				spdc_set_startpoint (pc, &p, event->button.state);
				ret = TRUE;
				break;
			}
		}
		break;
	case GDK_MOTION_NOTIFY:
		/* Find desktop coordinates */
		sp_desktop_w2d_xy_point (dt, &p, event->motion.x, event->motion.y);
		/* Test, whether we hit any anchor */
		anchor = test_inside (dc, event->button.x, event->button.y);

		switch (pc->state) {
		case SP_PENCIL_CONTEXT_ADDLINE:
			/* Set red endpoint */
			if (anchor) {
				p = anchor->dp;
			}
			spdc_set_endpoint (pc, &p, event->motion.state);
			ret = TRUE;
			break;
		default:
			/* We may be idle or already freehand */
			if (event->motion.state & GDK_BUTTON1_MASK) {
				pc->state = SP_PENCIL_CONTEXT_FREEHAND;
				if (!dc->sa && !dc->green_anchor) {
					/* Create green anchor */
					dc->green_anchor = sp_draw_anchor_new (dc, dc->green_curve, TRUE, dc->p[0].x, dc->p[0].y);
				}
				/* fixme: I am not sure, whether we want to snap to anchors in middle of freehand (Lauris) */
				if (anchor) {
					p = anchor->dp;
				} else {
					sp_desktop_free_snap (dt, &p);
				}
				spdc_add_freehand_point (pc, &p, event->motion.state);
				ret = TRUE;
			}
			break;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			/* Find desktop coordinates */
			sp_desktop_w2d_xy_point (dt, &p, event->motion.x, event->motion.y);
			/* Test, whether we hit any anchor */
			anchor = test_inside (dc, event->button.x, event->button.y);

			switch (pc->state) {
			case SP_PENCIL_CONTEXT_IDLE:
				/* Releasing button in idle mode means single click */
				/* We have already set up start point/anchor in button_press */
				pc->state = SP_PENCIL_CONTEXT_ADDLINE;
				ret = TRUE;
				break;
			case SP_PENCIL_CONTEXT_ADDLINE:
				/* Finish segment now */
				if (anchor) {
					p = anchor->dp;
				}
				dc->ea = anchor;
				spdc_finish_endpoint (pc, &p, !anchor, event->button.state);
				pc->state = SP_PENCIL_CONTEXT_IDLE;
				ret = TRUE;
				break;
			case SP_PENCIL_CONTEXT_FREEHAND:
				/* Finish segment now */
				/* fixme: Clean up what follows (Lauris) */
				if (anchor) {
					p = anchor->dp;
				}
				dc->ea = anchor;
				/* Write curves to object */
				g_print ("Finishing freehand\n");
				spdc_concat_colors_and_flush (dc);
				pc->state = SP_PENCIL_CONTEXT_IDLE;
				ret = TRUE;
				break;
			default:
				break;
			}
			/* Release grab now */
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (dt->acetate), event->button.time);
			ret = TRUE;
		}
		break;
#if 0
	case GDK_KEY_PRESS:
		/* fixme: */
		switch (event->key.keyval) {
		case GDK_p:
			if (dc->state == SP_PENCIL_CONTEXT_IDLE) {
				dc->state = SP_PENCIL_CONTEXT_PEN_POINT;
				dc->npoints = 0;
				gnome_canvas_item_show (dc->c1);
				gnome_canvas_item_show (dc->cl1);
				ret = TRUE;
			}
			break;
		case GDK_f:
			g_print ("Finishing pen\n");
			spdc_concat_colors_and_flush (dc);
			dc->state = SP_PENCIL_CONTEXT_IDLE;
			gnome_canvas_item_hide (dc->c0);
			gnome_canvas_item_hide (dc->c1);
			gnome_canvas_item_hide (dc->cl0);
			gnome_canvas_item_hide (dc->cl1);
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
#endif
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (pencil_parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (pencil_parent_class)->root_handler (ec, event);
	}

	return ret;
}

/*
 * Reset points and set new starting point
 */

static void
spdc_set_startpoint (SPPencilContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	dc->npoints = 0;
	dc->p[dc->npoints++] = *p;
}

/*
 * Change moving enpoint position
 *
 * - Ctrl constraints moving to H/V direction, snapping in given direction
 * - otherwise we snap freely to whatever attractors are available
 *
 * Number of points is (re)set to 2 always, 2nd point is modified
 * We change RED curve
 */

static void
spdc_set_endpoint (SPPencilContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	g_assert (dc->npoints > 0);

	spdc_endpoint_snap (dc, p, state);

	dc->p[1] = *p;
	dc->npoints = 2;

	sp_curve_reset (dc->red_curve);
	sp_curve_moveto (dc->red_curve, dc->p[0].x, dc->p[0].y);
	sp_curve_lineto (dc->red_curve, dc->p[1].x, dc->p[1].y);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), dc->red_curve);
}

/*
 * Set endpoint final position and end addline mode
 * fixme: I'd like remove red reset from concat colors (lauris)
 * fixme: Still not sure, how it will make most sense
 */

static void
spdc_finish_endpoint (SPPencilContext *pc, ArtPoint *p, gboolean snap, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	if (SP_CURVE_LENGTH (dc->red_curve) < 2) {
		/* Just a click, reset red curve and continue */
		g_print ("Finishing empty red curve\n");
		sp_curve_reset (dc->red_curve);
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
	} else if (SP_CURVE_LENGTH (dc->red_curve) > 2) {
		g_warning ("Red curve length is %d", SP_CURVE_LENGTH (dc->red_curve));
		sp_curve_reset (dc->red_curve);
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
	} else {
		ArtBpath *s, *e;
		/* We have actual line */
		if (snap) {
			/* Do (bogus?) snap */
			spdc_endpoint_snap (dc, p, state);
		}
		/* fixme: We really should test start and end anchors instead */
		s = SP_CURVE_SEGMENT (dc->red_curve, 0);
		e = SP_CURVE_SEGMENT (dc->red_curve, 1);
		if ((e->x3 == s->x3) && (e->y3 == s->y3)) {
			/* Empty line, reset red curve and continue */
			g_print ("Finishing zero length red curve\n");
			sp_curve_reset (dc->red_curve);
			sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
		} else {
			/* Write curves to object */
			g_print ("Finishing real red curve\n");
			spdc_concat_colors_and_flush (dc);
		}
	}
}

static void
spdc_add_freehand_point (SPPencilContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	/* fixme: Cleanup following (Lauris) */
	g_assert (dc->npoints > 0);
	if ((p->x != dc->p[dc->npoints - 1].x) || (p->y != dc->p[dc->npoints - 1].y)) {
		dc->p[dc->npoints++] = *p;
		fit_and_split (dc);
	}
}

/* Pen context */

static void sp_pen_context_class_init (SPPenContextClass *klass);
static void sp_pen_context_init (SPPenContext *pc);
static void sp_pen_context_finalize (GtkObject *object);

static void sp_pen_context_setup (SPEventContext *ec, SPDesktop *dt);
static gint sp_pen_context_root_handler (SPEventContext *ec, GdkEvent *event);

static void spdc_pen_set_point (SPPenContext *pc, ArtPoint *p, guint state);
static void spdc_pen_set_ctrl (SPPenContext *pc, ArtPoint *p, guint state);
static void spdc_pen_finish_segment (SPPenContext *pc, ArtPoint *p, guint state);

static SPDrawContextClass *pen_parent_class;

GtkType
sp_pen_context_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPPenContext",
			sizeof (SPPenContext),
			sizeof (SPPenContextClass),
			(GtkClassInitFunc) sp_pen_context_class_init,
			(GtkObjectInitFunc) sp_pen_context_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_DRAW_CONTEXT, &info);
	}

	return type;
}

static void
sp_pen_context_class_init (SPPenContextClass *klass)
{
	GtkObjectClass *object_class;
	SPEventContextClass *event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	pen_parent_class = gtk_type_class (SP_TYPE_DRAW_CONTEXT);

	object_class->finalize = sp_pen_context_finalize;

	event_context_class->setup = sp_pen_context_setup;
	event_context_class->root_handler = sp_pen_context_root_handler;
}

static void
sp_pen_context_init (SPPenContext *pc)
{
	pc->state = SP_PEN_CONTEXT_POINT;

	pc->c0 = NULL;
	pc->c1 = NULL;
	pc->cl0 = NULL;
	pc->cl1 = NULL;
}

static void
sp_pen_context_finalize (GtkObject *object)
{
	SPPenContext *pc;

	pc = SP_PEN_CONTEXT (object);

	if (pc->c0) gtk_object_destroy (GTK_OBJECT (pc->c0));
	if (pc->c1) gtk_object_destroy (GTK_OBJECT (pc->c1));
	if (pc->cl0) gtk_object_destroy (GTK_OBJECT (pc->cl0));
	if (pc->cl1) gtk_object_destroy (GTK_OBJECT (pc->cl1));

	GTK_OBJECT_CLASS (pen_parent_class)->finalize (object);
}

static void
sp_pen_context_setup (SPEventContext *ec, SPDesktop *dt)
{
	SPPenContext *pc;

	pc = SP_PEN_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (pen_parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (pen_parent_class)->setup (ec, dt);

	/* Pen indicators */
	pc->c0 = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT_DESKTOP (ec)), SP_TYPE_CTRL,
					"shape", SP_CTRL_SHAPE_CIRCLE,
					"size", 4.0,
					"filled", 0,
					"fill_color", 0xff00007f,
					"stroked", 1,
					"stroke_color", 0x0000ff7f,
					NULL);
	pc->c1 = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT_DESKTOP (ec)), SP_TYPE_CTRL,
					"shape", SP_CTRL_SHAPE_CIRCLE,
					"size", 4.0,
					"filled", 0,
					"fill_color", 0xff00007f,
					"stroked", 1,
					"stroke_color", 0x0000ff7f,
					NULL);
	pc->cl0 = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT_DESKTOP (ec)), SP_TYPE_CTRLLINE, NULL);
	sp_ctrlline_set_rgba32 (SP_CTRLLINE (pc->cl0), 0x0000007f);
	pc->cl1 = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT_DESKTOP (ec)), SP_TYPE_CTRLLINE, NULL);
	sp_ctrlline_set_rgba32 (SP_CTRLLINE (pc->cl1), 0x0000007f);

	gnome_canvas_item_hide (pc->c0);
	gnome_canvas_item_hide (pc->c1);
	gnome_canvas_item_hide (pc->cl0);
	gnome_canvas_item_hide (pc->cl1);
}

gint
sp_pen_context_root_handler (SPEventContext *ec, GdkEvent *event)
{
	SPDrawContext *dc;
	SPPenContext *pc;
	SPDesktop *dt;
	ArtPoint p;
	gint ret;
	SPDrawAnchor *anchor;

	dc = SP_DRAW_CONTEXT (ec);
	pc = SP_PEN_CONTEXT (ec);
	dt = ec->desktop;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			/* Grab mouse, so release will not pass unnoticed */
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (dt->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			/* Find desktop coordinates */
			sp_desktop_w2d_xy_point (dt, &p, event->button.x, event->button.y);
			/* Test, whether we hit any anchor */
			anchor = test_inside (dc, event->button.x, event->button.y);

			switch (pc->state) {
			case SP_PEN_CONTEXT_POINT:
			case SP_PEN_CONTEXT_CONTROL:
				break;
			default:
				break;
			}
		}
		break;
	case GDK_MOTION_NOTIFY:
		/* Find desktop coordinates */
		sp_desktop_w2d_xy_point (dt, &p, event->motion.x, event->motion.y);
		/* Test, whether we hit any anchor */
		anchor = test_inside (dc, event->button.x, event->button.y);

		switch (pc->state) {
		case SP_PEN_CONTEXT_POINT:
			if (dc->npoints != 0) {
				/* Only set point, if we are already appending */
				/* fixme: Snapping */
				spdc_pen_set_point (pc, &p, event->motion.state);
				ret = TRUE;
			}
			break;
		case SP_PEN_CONTEXT_CONTROL:
			/* fixme: Snapping */
			spdc_pen_set_ctrl (pc, &p, event->motion.state);
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			/* Find desktop coordinates */
			sp_desktop_w2d_xy_point (dt, &p, event->motion.x, event->motion.y);
			/* Test, whether we hit any anchor */
			anchor = test_inside (dc, event->button.x, event->button.y);

			switch (pc->state) {
			case SP_PEN_CONTEXT_POINT:
				if (dc->npoints == 0) {
					/* Start new thread only with button release */
					if (anchor) {
						p = anchor->dp;
					}
					dc->sa = anchor;
					spdc_pen_set_point (pc, &p, event->motion.state);
				} else {
					/* Set end anchor here */
					dc->ea = anchor;
				}
				pc->state = SP_PEN_CONTEXT_CONTROL;
				ret = TRUE;
				break;
			case SP_PEN_CONTEXT_CONTROL:
				/* End current segment */
				spdc_pen_finish_segment (pc, &p, event->button.state);
				pc->state = SP_PEN_CONTEXT_POINT;
				ret = TRUE;
				break;
			default:
				break;
			}
			/* Release grab now */
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (dt->acetate), event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_KEY_PRESS:
		/* fixme: */
		switch (event->key.keyval) {
		case GDK_Return:
			g_print ("Finishing pen\n");
			spdc_concat_colors_and_flush (dc);
			dc->npoints = 0;
			pc->state = SP_PEN_CONTEXT_POINT;
			gnome_canvas_item_hide (pc->c0);
			gnome_canvas_item_hide (pc->c1);
			gnome_canvas_item_hide (pc->cl0);
			gnome_canvas_item_hide (pc->cl1);
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (pen_parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (pen_parent_class)->root_handler (ec, event);
	}

	return ret;
}

static void
spdc_pen_set_point (SPPenContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	if (dc->npoints == 0) {
		/* Just set initial point */
		dc->p[0] = *p;
		dc->p[1] = *p;
		dc->npoints = 2;
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
	} else {
		dc->p[2] = *p;
		dc->p[3] = *p;
		dc->p[4] = *p;
		dc->npoints = 5;
		sp_curve_reset (dc->red_curve);
		sp_curve_moveto (dc->red_curve, dc->p[0].x, dc->p[0].y);
		sp_curve_curveto (dc->red_curve, dc->p[1].x, dc->p[1].y, dc->p[2].x, dc->p[2].y, dc->p[3].x, dc->p[3].y);
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), dc->red_curve);
	}
}

static void
spdc_pen_set_ctrl (SPPenContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	gnome_canvas_item_show (pc->c1);
	gnome_canvas_item_show (pc->cl1);

	if (dc->npoints == 2) {
		dc->p[1] = *p;
		gnome_canvas_item_hide (pc->c0);
		gnome_canvas_item_hide (pc->cl0);
		sp_ctrl_moveto (SP_CTRL (pc->c1), dc->p[1].x, dc->p[1].y);
		sp_ctrlline_set_coords (SP_CTRLLINE (pc->cl1), dc->p[0].x, dc->p[0].y, dc->p[1].x, dc->p[1].y);
	} else if (dc->npoints == 5) {
		dc->p[4] = *p;
		gnome_canvas_item_show (pc->c0);
		gnome_canvas_item_show (pc->cl0);
		if (state & GDK_CONTROL_MASK) {
			gdouble dx, dy;
			dx = p->x - dc->p[3].x;
			dy = p->y - dc->p[3].y;
			dc->p[2].x = dc->p[3].x - dx;
			dc->p[2].y = dc->p[3].y - dy;
			sp_curve_reset (dc->red_curve);
			sp_curve_moveto (dc->red_curve, dc->p[0].x, dc->p[0].y);
			sp_curve_curveto (dc->red_curve, dc->p[1].x, dc->p[1].y, dc->p[2].x, dc->p[2].y, dc->p[3].x, dc->p[3].y);
			sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), dc->red_curve);
		}
		sp_ctrl_moveto (SP_CTRL (pc->c0), dc->p[2].x, dc->p[2].y);
		sp_ctrlline_set_coords (SP_CTRLLINE (pc->cl0), dc->p[3].x, dc->p[3].y, dc->p[2].x, dc->p[2].y);
		sp_ctrl_moveto (SP_CTRL (pc->c1), dc->p[4].x, dc->p[4].y);
		sp_ctrlline_set_coords (SP_CTRLLINE (pc->cl1), dc->p[3].x, dc->p[3].y, dc->p[4].x, dc->p[4].y);
	} else {
		g_warning ("Something bad happened - npoints is %d", dc->npoints);
	}
}

static void
spdc_pen_finish_segment (SPPenContext *pc, ArtPoint *p, guint state)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (pc);

	if (!sp_curve_empty (dc->red_curve)) {
		SPCurve *curve;
		GnomeCanvasItem *cshape;

		sp_curve_append_continuous (dc->green_curve, dc->red_curve, 1e-9);
		curve = sp_curve_copy (dc->red_curve);
		/* fixme: */
		cshape = sp_canvas_bpath_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop), curve);
		sp_curve_unref (curve);
		sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (cshape), 0x00bf00ff, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);

		dc->green_bpaths = g_slist_prepend (dc->green_bpaths, cshape);

		dc->p[0] = dc->p[3];
		dc->p[1] = dc->p[4];
		dc->npoints = 2;
	}
}

