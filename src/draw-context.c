#define SP_DRAW_CONTEXT_C

/*
 * drawing-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make freehand generating curves instead of lines
 *
 * Copyright (C) Lauris Kaplinski (lauris@kaplinski.com), 1999-2000
 *
 * Contains pieces of code published in:
 * "Graphics Gems", Academic Press, 1990
 *  by Philip J. Schneider
 *
 */

#define noDRAW_VERBOSE

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/bezier-utils.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"

#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "draw-context.h"

#define TOLERANCE 1.0
#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))
static void sp_draw_context_class_init (SPDrawContextClass * klass);
static void sp_draw_context_init (SPDrawContext * draw_context);
static void sp_draw_context_destroy (GtkObject * object);

static void sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void clear_current (SPDrawContext * dc);
static void set_to_accumulated (SPDrawContext * dc);
static void concat_current (SPDrawContext * dc);
#if 0
static void repr_destroyed (SPRepr * repr, gpointer data);
#endif

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
	dc->accumulated = NULL;
	dc->npoints = 0;
	dc->segments = NULL;
	dc->currentcurve = NULL;
	dc->currentshape = NULL;
	dc->repr = NULL;
	dc->citem = NULL;
	dc->ccolor = 0xff0000ff;
	dc->cinside = -1;
}

static void
sp_draw_context_destroy (GtkObject * object)
{
	SPDrawContext * dc;

	dc = SP_DRAW_CONTEXT (object);

#if 0
	/* fixme: */
	if (dc->repr) gtk_signal_disconnect (GTK_OBJECT (dc->repr), dc->destroyid);
#endif

	if (dc->accumulated) sp_curve_unref (dc->accumulated);

	while (dc->segments) {
		gtk_object_destroy (GTK_OBJECT (dc->segments->data));
		dc->segments = g_slist_remove (dc->segments, dc->segments->data);
	}

	if (dc->currentcurve) sp_curve_unref (dc->currentcurve);

	if (dc->currentshape) gtk_object_destroy (GTK_OBJECT (dc->currentshape));

	if (dc->citem) gtk_object_destroy (GTK_OBJECT (dc->citem));

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPDrawContext * dc;
	SPStyle *style;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	dc = SP_DRAW_CONTEXT (event_context);

	dc->accumulated = sp_curve_new_sized (32);
	dc->currentcurve = sp_curve_new_sized (4);

	/* fixme: */
	style = sp_style_new (NULL);
	style->fill.type = SP_PAINT_TYPE_NONE;
	style->stroke.type = SP_PAINT_TYPE_COLOR;
	style->stroke_width.unit = SP_UNIT_ABSOLUTE;
	style->stroke_width.distance = 1.0;
	style->absolute_stroke_width = 1.0;
	style->user_stroke_width = 1.0;
	dc->currentshape = (SPCanvasShape *) gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
								    SP_TYPE_CANVAS_SHAPE, NULL);
	sp_canvas_shape_set_style (dc->currentshape, style);
	sp_style_unref (style);
	gtk_signal_connect (GTK_OBJECT (dc->currentshape), "event",
			    GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);
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
	static gboolean addline = FALSE;
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
			if (!dc->cinside) {
				sp_desktop_free_snap (desktop, &p);
			} else if (dc->accumulated->end > 1) {
				ArtBpath * bp;
				bp = sp_curve_last_bpath (dc->accumulated);
				p.x = bp->x3;
				p.y = bp->y3;
			} else {
				ret = TRUE;
				break;
			}

			if (addline) {
				ArtBpath * bp;
				addline = FALSE;
				if (dc->currentcurve->end > 1) {
					bp = sp_curve_last_bpath (dc->currentcurve);
					p.x = bp->x3;
					p.y = bp->y3;
					/* We were in straight-line mode - draw it now */
					concat_current (dc);
					if (dc->cinside) {
						if (dc->accumulated->end > 3) {
							sp_curve_closepath_current (dc->accumulated);
						}
					}
					set_to_accumulated (dc);
					clear_current (dc);
					if (dc->accumulated->closed) {
						/* reset accumulated curve */
						sp_curve_reset (dc->accumulated);
						if (dc->repr) {
#if 0
							gtk_signal_disconnect (GTK_OBJECT (dc->repr), dc->destroyid);
#endif
							dc->repr = NULL;
						}
						remove_ctrl (dc);
					} else if (dc->currentcurve->end > 1) {
						move_ctrl (dc, dc->accumulated->bpath->x3, dc->accumulated->bpath->y3);
					}
				}
			} else {
				addline = TRUE;
				if (!dc->cinside) {
					sp_curve_reset (dc->accumulated);
					if (dc->repr) {
#if 0
						gtk_signal_disconnect (GTK_OBJECT (dc->repr), dc->destroyid);
#endif
						dc->repr = NULL;
					}
					move_ctrl (dc, p.x, p.y);
				} else {
					move_ctrl (dc, dc->accumulated->bpath->x3, dc->accumulated->bpath->y3);
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
			addline = FALSE;
			g_assert (dc->npoints > 0);
			if (!dc->cinside) {
				sp_desktop_free_snap (desktop, &p);
			}

			if ((p.x != dc->p[dc->npoints - 1].x) || (p.y != dc->p[dc->npoints - 1].y)) {
				dc->p[dc->npoints++] = p;
				fit_and_split (dc);
			}
			ret = TRUE;
		} else if (addline) {
			g_assert (dc->npoints > 0);
			if (event->motion.state & GDK_CONTROL_MASK) {
				if (fabs (p.x - dc->p[0].x) > fabs (p.y - dc->p[0].y)) {
					p.y = dc->p[0].y;
				} else {
					p.x = dc->p[0].x;
				}
			}
			dc->p[1] = p;
			dc->npoints = 2;

			sp_curve_reset (dc->currentcurve);
			sp_curve_moveto (dc->currentcurve, dc->p[dc->npoints - 2].x, dc->p[dc->npoints - 2].y);
			sp_curve_lineto (dc->currentcurve, p.x, p.y);
			sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging && event->button.button == 1) {
			dragging = FALSE;

			if (freehand) {
				freehand = FALSE;
				/* Remove all temporary line segments */
				while (dc->segments) {
					gtk_object_destroy (GTK_OBJECT (dc->segments->data));
					dc->segments = g_slist_remove (dc->segments, dc->segments->data);
				}
				/* Create object */
				concat_current (dc);
				if (dc->cinside) {
					if (dc->accumulated->end > 3) {
						sp_curve_closepath_current (dc->accumulated);
					}
				}
				set_to_accumulated (dc);
				if (dc->cinside) {
					/* reset accumulated curve */
					sp_curve_reset (dc->accumulated);
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

			if (!addline) {
				if (dc->accumulated->end > 0) {
					ArtBpath * bp;
					bp = sp_curve_last_bpath (dc->accumulated);
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
clear_current (SPDrawContext * dc)
{
	sp_canvas_shape_clear (dc->currentshape);
	/* reset curve */
	sp_curve_reset (dc->currentcurve);
	/* reset points */
	dc->npoints = 0;
}

static void
set_to_accumulated (SPDrawContext * dc)
{
	SPDesktop * desktop;

	desktop = SP_EVENT_CONTEXT (dc)->desktop;

	if (!sp_curve_empty (dc->accumulated)) {
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
#if 0
			dc->destroyid = gtk_signal_connect (GTK_OBJECT (dc->repr), "destroy",
							    GTK_SIGNAL_FUNC (repr_destroyed), dc);
#endif
			sp_document_add_repr (SP_DT_DOCUMENT (desktop), dc->repr);
			sp_repr_unref (dc->repr);
			sp_selection_set_repr (SP_DT_SELECTION (desktop), dc->repr);
		}
		sp_desktop_d2doc_affine (desktop, d2doc);
		abp = art_bpath_affine_transform (sp_curve_first_bpath (dc->accumulated), d2doc);
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
	if (!sp_curve_empty (dc->currentcurve)) {
		ArtBpath * bpath;
		if (sp_curve_empty (dc->accumulated)) {
			bpath = sp_curve_first_bpath (dc->currentcurve);
			g_assert (bpath->code == ART_MOVETO_OPEN);
			sp_curve_moveto (dc->accumulated, bpath->x3, bpath->y3);
		}
		bpath = sp_curve_last_bpath (dc->currentcurve);
		if (bpath->code == ART_CURVETO) {
			sp_curve_curveto (dc->accumulated, bpath->x1, bpath->y1, bpath->x2, bpath->y2, bpath->x3, bpath->y3);
		} else if (bpath->code == ART_LINETO) {
			sp_curve_lineto (dc->accumulated, bpath->x3, bpath->y3);
		} else {
			g_assert_not_reached ();
		}
	}
}

#if 0
static void
repr_destroyed (SPRepr * repr, gpointer data)
{
	SPDrawContext * dc;

	dc = SP_DRAW_CONTEXT (data);

	dc->repr = NULL;
	sp_curve_reset (dc->accumulated);

	remove_ctrl (dc);
}
#endif

static void
test_inside (SPDrawContext * dc, double x, double y)
{
	ArtPoint p;

	if (dc->citem == NULL) {
		dc->cinside = FALSE;
		return;
	}

	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &p, x, y);

	if ((fabs (p.x - dc->cpos.x) > 4.0) || (fabs (p.y - dc->cpos.y) > 4.0)) {
		if (dc->cinside != 0) {
			gnome_canvas_item_set (dc->citem,
				"filled", FALSE, NULL);
			dc->cinside = 0;
		}
	} else {
		if (dc->cinside != 1) {
			gnome_canvas_item_set (dc->citem,
				"filled", TRUE, NULL);
				dc->cinside = 1;
		}
	}
}


static void
move_ctrl (SPDrawContext * dc, double x, double y)
{
	if (dc->citem == NULL) {
		dc->citem = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT (dc)->desktop),
			SP_TYPE_CTRL,
			"size", 4.0,
			"filled", 0,
			"fill_color", dc->ccolor,
			"stroked", 1,
			"stroke_color", 0x000000ff,
			NULL);
		gtk_signal_connect (GTK_OBJECT (dc->citem), "event",
			GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);
	}
	sp_ctrl_moveto (SP_CTRL (dc->citem), x, y);
	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &dc->cpos, x, y);
	dc->cinside = -1;
}

static void
remove_ctrl (SPDrawContext * dc)
{
	if (dc->citem) {
		gtk_object_destroy (GTK_OBJECT (dc->citem));
		dc->citem = NULL;
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

	if (sp_bezier_fit_cubic (b, dc->p, 0, dc->npoints - 1, tolerance) && dc->npoints < 16) {
		/* Fit and draw and reset state */
#ifdef DRAW_VERBOSE
		g_print ("%d", dc->npoints);
#endif
		g_assert ((b[0].x > -8000.0) && (b[0].x < 8000.0));
		g_assert ((b[0].y > -8000.0) && (b[0].y < 8000.0));
		g_assert ((b[1].x > -8000.0) && (b[1].x < 8000.0));
		g_assert ((b[1].y > -8000.0) && (b[1].y < 8000.0));
		g_assert ((b[2].x > -8000.0) && (b[2].x < 8000.0));
		g_assert ((b[2].y > -8000.0) && (b[2].y < 8000.0));
		g_assert ((b[3].x > -8000.0) && (b[3].x < 8000.0));
		g_assert ((b[3].y > -8000.0) && (b[3].y < 8000.0));
		sp_curve_reset (dc->currentcurve);
		sp_curve_moveto (dc->currentcurve, b[0].x, b[0].y);
		sp_curve_curveto (dc->currentcurve, b[1].x, b[1].y, b[2].x, b[2].y, b[3].x, b[3].y);
		sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
	} else {
		SPCurve * curve;
		SPStyle *style;
		SPCanvasShape * cshape;
		/* Fit and draw and copy last point */
#ifdef DRAW_VERBOSE
		g_print("[%d]Yup\n", dc->npoints);
#endif
		g_assert (!sp_curve_empty (dc->currentcurve));
		concat_current (dc);
		curve = sp_curve_copy (dc->currentcurve);
		/* fixme: */
		style = sp_style_new (NULL);
		style->fill.type = SP_PAINT_TYPE_NONE;
		style->stroke.type = SP_PAINT_TYPE_COLOR;
		style->stroke_width.unit = SP_UNIT_ABSOLUTE;
		style->stroke_width.distance = 1.0;
		style->absolute_stroke_width = 1.0;
		style->user_stroke_width = 1.0;
		cshape = (SPCanvasShape *) gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
								  SP_TYPE_CANVAS_SHAPE, NULL);
		sp_canvas_shape_set_style (cshape, style);
		sp_style_unref (style);
		gtk_signal_connect (GTK_OBJECT (cshape), "event",
				    GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);

		sp_canvas_shape_add_component (cshape, curve, TRUE, NULL);
		sp_curve_unref (curve);

		dc->segments = g_slist_prepend (dc->segments, cshape);

		dc->p[0] = dc->p[dc->npoints - 2];
		dc->p[1] = dc->p[dc->npoints - 1];
		dc->npoints = 1;
	}
}
