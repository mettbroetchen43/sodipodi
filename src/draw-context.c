#define __SP_DRAW_CONTEXT_C__

/*
 * Freehand drawing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#define DRAW_VERBOSE

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/bezier-utils.h"
#include "helper/sodipodi-ctrl.h"
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
#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))
#define DC_DESKTOP(dc) (((SPEventContext *)(dc))->desktop)


static void sp_draw_context_class_init (SPDrawContextClass * klass);
static void sp_draw_context_init (SPDrawContext * draw_context);
static void sp_draw_context_destroy (GtkObject * object);

static void sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_draw_context_selection_changed (SPSelection *sel, SPDrawContext *dc);
static void sp_draw_context_selection_modified (SPSelection *sel, guint flags, SPDrawContext *dc);

static void spdc_set_endpoint (SPDrawContext *dc, ArtPoint *p, guint state);
static void spdc_finish_endpoint (SPDrawContext *dc, ArtPoint *p, guint state);
static void spdc_concat_colors_and_flush (SPDrawContext *dc);
static void spdc_flush_white (SPDrawContext *dc, SPCurve *gc);

static void spdc_clear_white_data (SPDrawContext *dc);
static void spdc_clear_red_data (SPDrawContext *dc);

SPDrawAnchor *sp_draw_anchor_test (SPDrawAnchor *anchor, gdouble wx, gdouble wy, gboolean activate);

static void clear_current (SPDrawContext * dc);
static void set_to_accumulated (SPDrawContext * dc);
static void concat_current (SPDrawContext * dc);

static void test_inside (SPDrawContext * dc, double x, double y);
static void move_ctrl (SPDrawContext * dc, double x, double y);
static void remove_ctrl (SPDrawContext * dc);

static void fit_and_split (SPDrawContext * dc);

static SPEventContextClass * parent_class;

GtkType
sp_draw_context_get_type (void)
{
	static GtkType draw_context_type = 0;

	if (!draw_context_type) {

		static const GtkTypeInfo draw_context_info = {
			"SPDrawContext",
			sizeof (SPDrawContext),
			sizeof (SPDrawContextClass),
			(GtkClassInitFunc) sp_draw_context_class_init,
			(GtkObjectInitFunc) sp_draw_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		draw_context_type = gtk_type_unique (sp_event_context_get_type (), &draw_context_info);
	}

	return draw_context_type;
}

static void
sp_draw_context_class_init (SPDrawContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_draw_context_destroy;

	event_context_class->setup = sp_draw_context_setup;
	event_context_class->root_handler = sp_draw_context_root_handler;
	event_context_class->item_handler = sp_draw_context_item_handler;
}

static void
sp_draw_context_init (SPDrawContext * dc)
{
	dc->white_item = NULL;
	dc->white_cl = NULL;
	dc->white_al = NULL;

	dc->red_bpath = NULL;
	dc->red_curve = NULL;

	dc->bbp = NULL;
	dc->bc = NULL;

	dc->gl = NULL;
	dc->gc = NULL;

	dc->npoints = 0;
}

static void
sp_draw_context_destroy (GtkObject *object)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (object);

	gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_SELECTION (DC_DESKTOP (dc))), dc);

	spdc_clear_white_data (dc);
	spdc_clear_red_data (dc);

	if (dc->bbp) {
		/* Destroy blue bpath */
		gtk_object_destroy (GTK_OBJECT (dc->bbp));
		dc->bbp = NULL;
	}

	if (dc->bc) {
		/* Destroy blue curve */
		dc->bc = sp_curve_unref (dc->bc);
	}

	while (dc->gl) {
		/* Destroy green list */
		gtk_object_destroy (GTK_OBJECT (dc->gl->data));
		dc->gl = g_slist_remove (dc->gl, dc->gl->data);
	}

	if (dc->gc) {
		/* Destroy green curve */
		dc->gc = sp_curve_unref (dc->gc);
	}

	if (dc->ga) {
		/* Destroy green anchor */
		sp_draw_anchor_destroy (dc->ga);
		dc->ga = NULL;
	}

	if (dc->sa) {
		/* Destroy start anchor */
		sp_draw_anchor_destroy (dc->sa);
		dc->sa = NULL;
	}

	if (dc->ea) {
		/* Destrou end anchor */
		sp_draw_anchor_destroy (dc->ea);
		dc->ea = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_draw_context_setup (SPEventContext *ec, SPDesktop *dt)
{
	SPDrawContext *dc;

	dc = SP_DRAW_CONTEXT (ec);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (ec, dt);

	/* Connect signals to track selection changes */
	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (dt)), "changed",
			    GTK_SIGNAL_FUNC (sp_draw_context_selection_changed), dc);
	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (dt)), "modified",
			    GTK_SIGNAL_FUNC (sp_draw_context_selection_modified), dc);

	/* Create red bpath */
	dc->red_bpath = sp_canvas_bpath_new (SP_DT_SKETCH (ec->desktop), NULL);
	sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (dc->red_bpath), 0xff00007f, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);
	/* Create red curve */
	dc->red_curve = sp_curve_new_sized (4);

	/* Create blue bpath */
	dc->bbp = sp_canvas_bpath_new (SP_DT_SKETCH (ec->desktop), NULL);
	sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (dc->bbp), 0x0000ff7f, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);
	/* Create blue curve */
	dc->bc = sp_curve_new_sized (8);

	/* Create green curve */
	dc->gc = sp_curve_new_sized (64);
	/* Create hidden green anchor */
	dc->ga = sp_draw_anchor_new (dc, dc->gc, FALSE, 0, 0);
	gnome_canvas_item_hide (dc->ga->ctrl);

	/* Attach existing selection */
	/* fixme: fixme: fixme: (lauris) */
}

static void
sp_draw_context_selection_changed (SPSelection *sel, SPDrawContext *dc)
{
	g_print ("Selection changed in draw context\n");
}

/* fixme: We have to ensure this is not delayed (Lauris) */

static void
sp_draw_context_selection_modified (SPSelection *sel, guint flags, SPDrawContext *dc)
{
	SPItem *item;

	g_print ("Selection modified in draw context\n");

	/* Clear white data */
	spdc_clear_white_data (dc);
	/* Reset red data */
	sp_curve_reset (dc->red_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);

	item = sp_selection_item (SP_DT_SELECTION (DC_DESKTOP (dc)));

	if (item && SP_IS_PATH (item) && SP_PATH (item)->independent) {
		SPCurve *norm;
		GSList *l;
		/* Create new white data */
		/* Item */
		dc->white_item = item;
		/* Curve list */
		norm = sp_path_normalized_bpath (SP_PATH (item));
		g_return_if_fail (norm != NULL);
		dc->white_cl = sp_curve_split (norm);
		sp_curve_unref (norm);
		/* Anchor list */
		for (l = dc->white_cl; l != NULL; l = l->next) {
			SPCurve *c;
			c = l->data;
			g_return_if_fail (c->end > 1);
			if (c->bpath->code == ART_MOVETO_OPEN) {
				SPDrawAnchor *a;
				ArtPoint p;
				sp_desktop_doc2d_xy_point (DC_DESKTOP (dc), &p, c->bpath[0].x3, c->bpath[0].y3);
				a = sp_draw_anchor_new (dc, c, TRUE, p.x, p.y);
				dc->white_al = g_slist_prepend (dc->white_al, a);
				sp_desktop_doc2d_xy_point (DC_DESKTOP (dc), &p, c->bpath[c->end - 1].x3, c->bpath[c->end - 1].y3);
				a = sp_draw_anchor_new (dc, c, FALSE, p.x, p.y);
				dc->white_al = g_slist_prepend (dc->white_al, a);
			}
		}
		/* fixme: recalculate active anchor, reset red (DONE), green and blue, release drags */
		/* The latter is not needed, if we can ensure that 'modified' is not delayed */
	}
}

static gint
sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

gint
sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging = FALSE;
	static gboolean freehand = FALSE;
	SPDrawContext * dc;
	SPDesktop * desktop;
	ArtPoint p;
	gint ret;

	dc = SP_DRAW_CONTEXT (event_context);
	desktop = event_context->desktop;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;

			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			test_inside (dc, p.x, p.y);
			if (!dc->ga || !dc->ga->active) {
				sp_desktop_free_snap (desktop, &p);
			} else if (dc->gc->end > 1) {
				ArtBpath * bp;
				bp = sp_curve_last_bpath (dc->gc);
				p.x = bp->x3;
				p.y = bp->y3;
			} else {
				ret = TRUE;
				break;
			}

			if (dc->addline) {
				/* End addline mode */
				spdc_finish_endpoint (dc, &p, event->button.state);
			} else {
				dc->addline = TRUE;
				if (!dc->ga || !dc->ga->active) {
					sp_curve_reset (dc->gc);
					if (dc->repr) {
						dc->repr = NULL;
					}
					move_ctrl (dc, p.x, p.y);
				} else {
					move_ctrl (dc, dc->gc->bpath->x3, dc->gc->bpath->y3);
				}
			}
			/* initialize first point */
			dc->npoints = 0;
			dc->p[dc->npoints++] = p;
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
		test_inside (dc, p.x, p.y);
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			freehand = TRUE;
			dc->addline = FALSE;
			g_assert (dc->npoints > 0);
			if (!dc->ga || !dc->ga->active) {
				sp_desktop_free_snap (desktop, &p);
			}

			if ((p.x != dc->p[dc->npoints - 1].x) || (p.y != dc->p[dc->npoints - 1].y)) {
				dc->p[dc->npoints++] = p;
				fit_and_split (dc);
			}
			ret = TRUE;
		} else if (dc->addline) {
			/* Free moving in addline mode - set red endpoint */
			spdc_set_endpoint (dc, &p, event->motion.state);
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
			dragging = FALSE;

			if (freehand) {
				freehand = FALSE;
				/* Remove all temporary line segments */
				while (dc->gl) {
					gtk_object_destroy (GTK_OBJECT (dc->gl->data));
					dc->gl = g_slist_remove (dc->gl, dc->gl->data);
				}
				/* Create object */
				concat_current (dc);
				if (dc->ga && dc->ga->active) {
					if (dc->gc->end > 3) {
						sp_curve_closepath_current (dc->gc);
					}
				}
				set_to_accumulated (dc);
				if (dc->ga && dc->ga->active) {
					/* reset accumulated curve */
					sp_curve_reset (dc->gc);
					clear_current (dc);
					if (dc->repr) {
#if 0
						gtk_signal_disconnect (GTK_OBJECT (dc->repr), dc->destroyid);
#endif
						dc->repr = NULL;
					}
					remove_ctrl (dc);
				}
			}

			if (!dc->addline) {
				if (dc->gc->end > 0) {
					ArtBpath * bp;
					bp = sp_curve_last_bpath (dc->gc);
					move_ctrl (dc, bp->x3, bp->y3);
					test_inside (dc, bp->x3, bp->y3);
				}
			}

			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
			ret = TRUE;
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
spdc_endpoint_snap (SPDrawContext *dc, ArtPoint *p, guint state)
{
	if (state & GDK_CONTROL_MASK) {
		/* Constrained motion */
		if (fabs (p->x - dc->p[0].x) > fabs (p->y - dc->p[0].y)) {
			/* Horizontal */
			p->y = dc->p[0].y;
			sp_desktop_horizontal_snap (DC_DESKTOP (dc), p);
		} else {
			/* Vertical */
			p->x = dc->p[0].x;
			sp_desktop_vertical_snap (DC_DESKTOP (dc), p);
		}
	} else {
		/* Free */
		sp_desktop_free_snap (DC_DESKTOP (dc), p);
	}
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
spdc_set_endpoint (SPDrawContext *dc, ArtPoint *p, guint state)
{
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
spdc_finish_endpoint (SPDrawContext *dc, ArtPoint *p, guint state)
{
	dc->addline = FALSE;

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
		/* Do (bogus?) snap */
		spdc_endpoint_snap (dc, p, state);
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
			g_print ("Finishing real red curve curve\n");
			spdc_concat_colors_and_flush (dc);
		}
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

	c = dc->gc;

	/* Green */
	dc->gc = sp_curve_new_sized (64);
	while (dc->gl) {
		gtk_object_destroy (GTK_OBJECT (dc->gl->data));
		dc->gl = g_slist_remove (dc->gl, dc->gl->data);
	}
	/* Blue */
	sp_curve_append (c, dc->bc, TRUE);
	sp_curve_reset (dc->bc);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->bbp), NULL);
	/* Red */
	sp_curve_append (c, dc->red_curve, TRUE);
	sp_curve_reset (dc->red_curve);
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);

	/* Step A - test, whether we ended on green anchor */
	if (dc->ga->active) {
		sp_curve_closepath_current (c);
		/* Closed path, just flush */
		spdc_flush_white (dc, c);
		sp_curve_unref (c);
		return;
	}

	/* Step B - both start and end anchored to same curve */
	if (dc->sa && dc->ea && (dc->sa->curve == dc->ea->curve)) {
		if (dc->sa->start) {
			SPCurve *r;
			r = sp_curve_reverse (c);
			sp_curve_unref (c);
			c = r;
		}
		sp_curve_append (dc->sa->curve, c, TRUE);
		sp_curve_unref (c);
		sp_curve_closepath_current (dc->sa->curve);
		spdc_flush_white (dc, NULL);
		return;
	}

	/* Step C - test start */
	if (dc->sa) {
		SPCurve *s;
		s = dc->sa->curve;
		dc->white_cl = g_slist_remove (dc->white_cl, s);
		if (dc->sa->start) {
			SPCurve *r;
			r = sp_curve_reverse (s);
			sp_curve_unref (s);
			s = r;
		}
		sp_curve_append (s, c, TRUE);
		sp_curve_unref (c);
		c = s;
	}

	/* Step D - test end */
	if (dc->ea) {
		SPCurve *e;
		e = dc->ea->curve;
		dc->white_cl = g_slist_remove (dc->white_cl, e);
		if (!dc->ea->start) {
			SPCurve *r;
			r = sp_curve_reverse (e);
			sp_curve_unref (e);
			e = r;
		}
		sp_curve_append (c, e, TRUE);
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

	if (dc->white_cl) {
		g_assert (dc->white_item);
		c = sp_curve_concat (dc->white_cl);
		g_slist_free (dc->white_cl);
		dc->white_cl = NULL;
		if (gc) {
			gdouble d2item[6];
			ArtBpath *abp;
			SPCurve *new;
			sp_item_dt2i_affine (dc->white_item, SP_EVENT_CONTEXT_DESKTOP (dc), d2item);
			abp = art_bpath_affine_transform (SP_CURVE_BPATH (gc), d2item);
			new = sp_curve_new_from_static_bpath (abp);
			sp_curve_append (c, new, FALSE);
			art_free (abp);
		}
	} else if (gc) {
		gdouble d2item[6];
		ArtBpath *abp;
		SPCurve *new;
		sp_desktop_d2doc_affine (SP_EVENT_CONTEXT_DESKTOP (dc), d2item);
		abp = art_bpath_affine_transform (SP_CURVE_BPATH (gc), d2item);
		new = sp_curve_new_from_foreign_bpath (abp);
		art_free (abp);
		c = new;
	} else {
		return;
	}

	if (c && !sp_curve_empty (c)) {
		SPRepr *repr;
		gchar *str;

		/* We actually have something to write */

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
			SPDesktop *dt;
			dt = SP_EVENT_CONTEXT_DESKTOP (dc);
			/* Attach repr */
			sp_document_add_repr (SP_DT_DOCUMENT (dt), repr);
			sp_selection_set_repr (SP_DT_SELECTION (dt), repr);
			sp_repr_unref (repr);
		}
	}

	sp_curve_unref (c);
}

static void
clear_current (SPDrawContext * dc)
{
	sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), NULL);
	/* reset curve */
	sp_curve_reset (dc->red_curve);
	/* reset points */
	dc->npoints = 0;
}

static void
set_to_accumulated (SPDrawContext * dc)
{
	SPDesktop * desktop;

	desktop = SP_EVENT_CONTEXT (dc)->desktop;

	if (!sp_curve_empty (dc->gc)) {
		ArtBpath *abp;
		gdouble d2doc[6];
		gchar * str;

		if (!dc->repr) {
			SPRepr * repr, * style;
			SPCSSAttr * css;
			/* Create object */
			repr = sp_repr_new ("path");
			/* Set style */
			style = sodipodi_get_repr (SODIPODI, "paint.freehand");
			if (style) {
				css = sp_repr_css_attr_inherited (style, "style");
				sp_repr_css_set (repr, css, "style");
				sp_repr_css_attr_unref (css);
			}
			dc->repr = repr;
			sp_document_add_repr (SP_DT_DOCUMENT (desktop), dc->repr);
			sp_repr_unref (dc->repr);
			sp_selection_set_repr (SP_DT_SELECTION (desktop), dc->repr);
		}
		sp_desktop_d2doc_affine (desktop, d2doc);
		abp = art_bpath_affine_transform (sp_curve_first_bpath (dc->gc), d2doc);
		str = sp_svg_write_path (abp);
		g_assert (str != NULL);
		art_free (abp);
		sp_repr_set_attr (dc->repr, "d", str);
		g_free (str);
	} else {
		if (dc->repr) sp_repr_unparent (dc->repr);
		dc->repr = NULL;
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

static void
concat_current (SPDrawContext * dc)
{
	if (!sp_curve_empty (dc->red_curve)) {
		ArtBpath * bpath;
		if (sp_curve_empty (dc->gc)) {
			bpath = sp_curve_first_bpath (dc->red_curve);
			g_assert (bpath->code == ART_MOVETO_OPEN);
			sp_curve_moveto (dc->gc, bpath->x3, bpath->y3);
		}
		bpath = sp_curve_last_bpath (dc->red_curve);
		if (bpath->code == ART_CURVETO) {
			sp_curve_curveto (dc->gc, bpath->x1, bpath->y1, bpath->x2, bpath->y2, bpath->x3, bpath->y3);
		} else if (bpath->code == ART_LINETO) {
			sp_curve_lineto (dc->gc, bpath->x3, bpath->y3);
		} else {
			g_assert_not_reached ();
		}
	}
}

static void
test_inside (SPDrawContext *dc, double x, double y)
{
	ArtPoint wp;
	GSList *l;
	SPDrawAnchor *active;

	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &wp, x, y);

	active = NULL;
	/* Test green anchor */
	if (dc->ga) {
		active = sp_draw_anchor_test (dc->ga, wp.x, wp.y, TRUE);
	}

	for (l = dc->white_al; l != NULL; l = l->next) {
		SPDrawAnchor *na;
		na = sp_draw_anchor_test ((SPDrawAnchor *) l->data, wp.x, wp.y, !active);
		if (!active && na) active = na;
	}
}


static void
move_ctrl (SPDrawContext *dc, gdouble x, gdouble y)
{
	gnome_canvas_item_show (dc->ga->ctrl);

	sp_ctrl_moveto (SP_CTRL (dc->ga->ctrl), x, y);
	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &dc->ga->wp, x, y);
#if 0
	dc->cinside = -1;
#endif
}

static void
remove_ctrl (SPDrawContext * dc)
{
	if (dc->ga) {
		sp_draw_anchor_destroy (dc->ga);
		dc->ga = NULL;
	}
}

static void
fit_and_split (SPDrawContext * dc)
{
	ArtPoint b[4];
	gdouble tolerance;

	g_assert (dc->npoints > 1);

	tolerance = SP_EVENT_CONTEXT (dc)->desktop->w2d[0] * TOLERANCE;
	tolerance = tolerance * tolerance;

	if (sp_bezier_fit_cubic (b, dc->p, dc->npoints, tolerance) > 0 && dc->npoints < 16) {
		/* Fit and draw and reset state */
#ifdef DRAW_VERBOSE
		g_print ("%d", dc->npoints);
#endif
#if 0
		g_assert ((b[0].x > -8000.0) && (b[0].x < 8000.0));
		g_assert ((b[0].y > -8000.0) && (b[0].y < 8000.0));
		g_assert ((b[1].x > -8000.0) && (b[1].x < 8000.0));
		g_assert ((b[1].y > -8000.0) && (b[1].y < 8000.0));
		g_assert ((b[2].x > -8000.0) && (b[2].x < 8000.0));
		g_assert ((b[2].y > -8000.0) && (b[2].y < 8000.0));
		g_assert ((b[3].x > -8000.0) && (b[3].x < 8000.0));
		g_assert ((b[3].y > -8000.0) && (b[3].y < 8000.0));
#endif
		sp_curve_reset (dc->red_curve);
		sp_curve_moveto (dc->red_curve, b[0].x, b[0].y);
		sp_curve_curveto (dc->red_curve, b[1].x, b[1].y, b[2].x, b[2].y, b[3].x, b[3].y);
		sp_canvas_bpath_set_bpath (SP_CANVAS_BPATH (dc->red_bpath), dc->red_curve);
	} else {
		SPCurve *curve;
		GnomeCanvasItem *cshape;
		/* Fit and draw and copy last point */
#ifdef DRAW_VERBOSE
		g_print("[%d]Yup\n", dc->npoints);
#endif
		g_assert (!sp_curve_empty (dc->red_curve));
		concat_current (dc);
		curve = sp_curve_copy (dc->red_curve);

		/* fixme: */
		cshape = sp_canvas_bpath_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop), curve);
		sp_curve_unref (curve);
		sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH (cshape), 0x00bf00ff, 1.0, ART_PATH_STROKE_JOIN_MITER, ART_PATH_STROKE_CAP_BUTT);

#if 0
		gtk_signal_connect (GTK_OBJECT (cshape), "event",
				    GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);
#endif

		dc->gl = g_slist_prepend (dc->gl, cshape);

		dc->p[0] = dc->p[dc->npoints - 2];
		dc->p[1] = dc->p[dc->npoints - 1];
		dc->npoints = 1;
	}
}

/*
 * - White item
 * - White curve list
 * - White anchor list
 */

static void
spdc_clear_white_data (SPDrawContext *dc)
{
	if (dc->white_item) {
		/* We do not reference item */
		dc->white_item = NULL;
	}

	while (dc->white_cl) {
		/* Clear white curve list */
		sp_curve_unref ((SPCurve *) dc->white_cl->data);
		dc->white_cl = g_slist_remove (dc->white_cl, dc->white_cl->data);
	}

	while (dc->white_al) {
		sp_draw_anchor_destroy ((SPDrawAnchor *) dc->white_al->data);
		dc->white_al = g_slist_remove (dc->white_al, dc->white_al->data);
	}
}

/*
 * - Red bpath
 * - Red curve
 */

static void
spdc_clear_red_data (SPDrawContext *dc)
{
	if (dc->red_bpath) {
		/* Destroy red bpath */
		gtk_object_destroy (GTK_OBJECT (dc->red_bpath));
		dc->red_bpath = NULL;
	}

	if (dc->red_curve) {
		/* Destroy red curve */
		dc->red_curve = sp_curve_unref (dc->red_curve);
	}
}

/*
 * Anchors
 */

SPDrawAnchor *
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
	sp_desktop_d2w_xy_point (DC_DESKTOP (dc), &a->wp, dx, dy);
	a->ctrl = gnome_canvas_item_new (SP_DT_CONTROLS (DC_DESKTOP (dc)), SP_TYPE_CTRL,
					 "size", 4.0,
					 "filled", 0,
					 "fill_color", 0xff00007f,
					 "stroked", 1,
					 "stroke_color", 0x000000ff,
					 NULL);
#if 0
	gtk_signal_connect (GTK_OBJECT (dc->citem), "event",
			    GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);
#endif
	sp_ctrl_moveto (SP_CTRL (a->ctrl), dx, dy);

	return a;
}

void
sp_draw_anchor_destroy (SPDrawAnchor *anchor)
{
	if (anchor->ctrl) {
		gtk_object_destroy (GTK_OBJECT (anchor->ctrl));
	}
	g_free (anchor);
}

#define A_SNAP 4.0

SPDrawAnchor *
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

