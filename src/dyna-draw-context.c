#define SP_DYNA_DRAW_CONTEXT_C

/*
 * drawing-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make freehand generating curves instead of lines
 *
 * Copyright (C) Mitsuru Oka (oka326@parkcity.ne.jp), 2001
 * Copyright (C) Lauris Kaplinski (lauris@kaplinski.com), 1999-2000
 *
 * Contains pieces of code published in:
 * "Graphics Gems", Academic Press, 1990
 *  by Philip J. Schneider
 *
 */

#define noDYNA_DRAW_VERBOSE

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"

#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "dyna-draw-context.h"

#define TOLERANCE 1.0
#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))
static void sp_dyna_draw_context_class_init (SPDynaDrawContextClass * klass);
static void sp_dyna_draw_context_init (SPDynaDrawContext * dyna_draw_context);
static void sp_dyna_draw_context_destroy (GtkObject * object);

static void sp_dyna_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_dyna_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_dyna_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void clear_current (SPDynaDrawContext * dc);
static void set_to_accumulated (SPDynaDrawContext * dc);
static void concat_current (SPDynaDrawContext * dc);
#if 0
static void repr_destroyed (SPRepr * repr, gpointer data);
#endif

static void test_inside (SPDynaDrawContext * dc, double x, double y);
static void move_ctrl (SPDynaDrawContext * dc, double x, double y);
static void remove_ctrl (SPDynaDrawContext * dc);

static void fit_and_split (SPDynaDrawContext * dc);

static SPEventContextClass * parent_class;

GtkType
sp_dyna_draw_context_get_type (void)
{
	static GtkType dyna_draw_context_type = 0;

	if (!dyna_draw_context_type) {

		static const GtkTypeInfo dyna_draw_context_info = {
			"SPDynaDrawContext",
			sizeof (SPDynaDrawContext),
			sizeof (SPDynaDrawContextClass),
			(GtkClassInitFunc) sp_dyna_draw_context_class_init,
			(GtkObjectInitFunc) sp_dyna_draw_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		dyna_draw_context_type = gtk_type_unique (sp_event_context_get_type (), &dyna_draw_context_info);
	}

	return dyna_draw_context_type;
}

static void
sp_dyna_draw_context_class_init (SPDynaDrawContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_dyna_draw_context_destroy;

	event_context_class->setup = sp_dyna_draw_context_setup;
	event_context_class->root_handler = sp_dyna_draw_context_root_handler;
	event_context_class->item_handler = sp_dyna_draw_context_item_handler;
}

static void
sp_dyna_draw_context_init (SPDynaDrawContext * dc)
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
sp_dyna_draw_context_destroy (GtkObject * object)
{
	SPDynaDrawContext * dc;

	dc = SP_DYNA_DRAW_CONTEXT (object);

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
sp_dyna_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPDynaDrawContext * dc;
	SPStyle *style;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	dc = SP_DYNA_DRAW_CONTEXT (event_context);

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
sp_dyna_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

/*
 *  DynaDraw utility supoorts
 */
#define DYNA_EPSILON 1.0e-6

typedef struct _DynaFilter DynaFilter;

struct _DynaFilter {
  double curx, cury;
  double velx, vely, vel;
  double accx, accy, acc;
  double angx, angy;
  double lastx, lasty;
  int fixed_angle;
  /* attributes, it should be editable via sodipodi GUI */
  double mass, drag;
  double angle;
  double width;
};


static double
flerp (double f0,
       double f1,
       double p)
{
  return f0 + (f1 - f0) * p;
}

static void
dyna_filter_init (DynaFilter *f,
                  double  x,
                  double  y)
{
  f->curx = x;
  f->cury = y;
  f->lastx = x;
  f->lasty = y;
  f->velx = 0.0;
  f->vely = 0.0;
  f->accx = 0.0;
  f->accy = 0.0;
  /* Fixme: default attribute value */
  f->mass = 0.2;
  f->drag = 0.5;
  f->angle = 30.0;
  f->width = 0.3;
}

static int
dyna_filter_apply (DynaFilter *f,
                   double  x,
                   double  y)
{
  double mass, drag;
  double fx, fy;
  
  /* Calculate mass and drag */  
  mass = flerp (1.0, 160.0, f->mass);
  drag = flerp (0.0, 0.5, f->drag * f->drag);
  
  /* Calculate force and acceleration */
  fx = x - f->curx;
  fy = y - f->cury;
  f->acc = sqrt (fx * fx + fy * fy);
  if (f->acc < DYNA_EPSILON)
    return FALSE;

  f->accx = fx / mass;
  f->accy = fy / mass;
  
  /* Calculate new velocity */
  f->velx += f->accx;
  f->vely += f->accy;
  f->vel = sqrt (f->velx * f->velx + f->vely * f->vely);
  f->angx = -f->vely;
  f->angy = f->velx;
  if (f->vel < DYNA_EPSILON)
    return FALSE;
  
  /* Calculate angle of drawing tool */
  f->angx /= f->vel;
  f->angy /= f->vel;
  if (f->fixed_angle) {
    f->angx = cos (f->angle * M_PI / 180.0);
    f->angy = -sin (f->angle * M_PI / 180.0);
  }
  
  /* Apply drag */
  f->velx *= 1.0 - drag;
  f->vely *= 1.0 - drag;
  
  /* Update position */
  f->lastx = f->curx;
  f->lasty = f->cury;
  f->curx += f->velx;
  f->cury += f->vely;
  
  return TRUE;
}


gint
sp_dyna_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging = FALSE;
	static gboolean freehand = FALSE;
        static DynaFilter dyna_mouse;
	SPDynaDrawContext * dc;
	SPDesktop * desktop;
	ArtPoint p;
	gint ret;

	dc = SP_DYNA_DRAW_CONTEXT (event_context);
	desktop = event_context->desktop;

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {

			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
                        dyna_filter_init (&dyna_mouse, p.x, p.y);
                        dyna_filter_apply (&dyna_mouse, p.x, p.y);
                        p.x = dyna_mouse.curx;
                        p.y = dyna_mouse.cury;

			test_inside (dc, p.x, p.y);
			if (!dc->cinside) {
				sp_desktop_free_snap (desktop, &p);
				sp_curve_reset (dc->accumulated);
				if (dc->repr) {
#if 0
					gtk_signal_disconnect (GTK_OBJECT (dc->repr), dc->destroyid);
#endif
					dc->repr = NULL;
				}
				move_ctrl (dc, p.x, p.y);
			} else if (dc->accumulated->end > 1) {
				ArtBpath * bp;
				bp = sp_curve_last_bpath (dc->accumulated);
				p.x = bp->x3;
				p.y = bp->y3;
				move_ctrl (dc, dc->accumulated->bpath->x3, dc->accumulated->bpath->y3);
			} else {
				ret = TRUE;
				break;
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
                
                dyna_filter_apply (&dyna_mouse, p.x, p.y);
                p.x = dyna_mouse.curx;
                p.y = dyna_mouse.cury;

		test_inside (dc, p.x, p.y);
		if (event->motion.state & GDK_BUTTON1_MASK) {
			dragging = TRUE;
			freehand = TRUE;
			g_assert (dc->npoints > 0);
			if (!dc->cinside) {
				sp_desktop_free_snap (desktop, &p);
			}

			if ((p.x != dc->p[dc->npoints - 1].x) || (p.y != dc->p[dc->npoints - 1].y)) {
				dc->p[dc->npoints++] = p;
				fit_and_split (dc);
			}
			ret = TRUE;
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
clear_current (SPDynaDrawContext * dc)
{
	sp_canvas_shape_clear (dc->currentshape);
	/* reset curve */
	sp_curve_reset (dc->currentcurve);
	/* reset points */
	dc->npoints = 0;
}

static void
set_to_accumulated (SPDynaDrawContext * dc)
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
concat_current (SPDynaDrawContext * dc)
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
	SPDynaDrawContext * dc;

	dc = SP_DYNA_DRAW_CONTEXT (data);

	dc->repr = NULL;
	sp_curve_reset (dc->accumulated);

	remove_ctrl (dc);
}
#endif

static void
test_inside (SPDynaDrawContext * dc, double x, double y)
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
move_ctrl (SPDynaDrawContext * dc, double x, double y)
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
remove_ctrl (SPDynaDrawContext * dc)
{
	if (dc->citem) {
		gtk_object_destroy (GTK_OBJECT (dc->citem));
		dc->citem = NULL;
	}
}

/*
 * An Algorithm for Automatically Fitting Digitized Curves
 * by Philip J. Schneider
 * from "Graphics Gems", Academic Press, 1990
 */

typedef ArtPoint * BezierCurve;

/* Forward declarations */
static gboolean FitCubic(ArtPoint * b, ArtPoint * d, gint first, gint last, ArtPoint tHat1, ArtPoint tHat2, gdouble error);
static void GenerateBezier (ArtPoint * b, ArtPoint * d, gint first, gint last, double * uPrime, ArtPoint tHat1, ArtPoint tHat2);
static gdouble * Reparameterize(ArtPoint * d, gint first, gint last, gdouble * u, BezierCurve bezCurve);
static gdouble NewtonRaphsonRootFind(BezierCurve Q, ArtPoint P, gdouble u);
static ArtPoint BezierII (gint degree, ArtPoint * V, gdouble t);
static gdouble B0 (gdouble u);
static gdouble B1 (gdouble u);
static gdouble B2 (gdouble u);
static gdouble B3 (gdouble u);
static ArtPoint ComputeLeftTangent (ArtPoint * d, gint end);
static ArtPoint ComputeRightTangent (ArtPoint * d, gint end);
#if 0
static ArtPoint ComputeCenterTangent (ArtPoint * d, gint center);
#endif
static gdouble * ChordLengthParameterize(ArtPoint * d, gint first, gint last);
static gdouble ComputeMaxError(ArtPoint * d, gint first, gint last, BezierCurve bezCurve, gdouble * u, gint * splitPoint);
static ArtPoint V2AddII (ArtPoint a, ArtPoint b);
static ArtPoint V2ScaleIII (ArtPoint v, gdouble s);
static ArtPoint V2SubII (ArtPoint a, ArtPoint b);
static void V2Normalize (ArtPoint * v);

#define V2Dot(a,b) ((a)->x * (b)->x + (a)->y * (b)->y)

#define MAXPOINTS	1000		/* The most points you can have */

static void
fit_and_split (SPDynaDrawContext * dc)
{
	ArtPoint t0, t1;
	ArtPoint b[4];
	gdouble tolerance;

	g_assert (dc->npoints > 1);

	tolerance = SP_EVENT_CONTEXT (dc)->desktop->w2d[0] * TOLERANCE;
	tolerance = tolerance * tolerance;

	t0 = ComputeLeftTangent(dc->p, 0);
	t1 = ComputeRightTangent(dc->p, dc->npoints - 1);

	if (FitCubic (b, dc->p, 0, dc->npoints - 1, t0, t1, tolerance) && dc->npoints < 16) {
		/* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
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
#ifdef DYNA_DRAW_VERBOSE
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

/*
 *  FitCubic :
 *  	Fit a Bezier curve to a (sub)set of digitized points
 */

static gboolean
FitCubic(ArtPoint * b, ArtPoint * d, gint first, gint last, ArtPoint tHat1, ArtPoint tHat2, gdouble error)
{
#if 0
    Point2	*d;			/*  Array of digitized points */
    int		first, last;	/* Indices of first and last pts in region */
    ArtPoint	tHat1, tHat2;	/* Unit tangent vectors at endpoints */
    double	error;		/*  User-defined error squared	   */
    BezierCurve	bezCurve; /*Control points of fitted Bezier curve*/
#endif
    double	*u;		/*  Parameter values for point  */
    double	*uPrime;	/*  Improved parameter values */
    double	maxError;	/*  Maximum fitting error	 */
    int		splitPoint;	/*  Point to split point set at	 */
    int		nPts;		/*  Number of points in subset  */
    double	iterationError; /*Error below which you try iterating  */
    int		maxIterations = 4; /*  Max times to try iterating  */
#if 0
    ArtPoint	tHatCenter;   	/* Unit tangent vector at splitPoint */
#endif
    int		i;		

    iterationError = error * error;
    nPts = last - first + 1;

    /*  Use heuristic if region only has two points in it */
    if (nPts == 2) {
	    double dist = hypot (d[last].x - d[first].x, d[last].y - d[first].y) / 3.0;

		b[0] = d[first];
		b[3] = d[last];
#if 0
		V2Add(&bezCurve[0], V2Scale(&tHat1, dist), &bezCurve[1]);
		V2Add(&bezCurve[3], V2Scale(&tHat2, dist), &bezCurve[2]);
#else
		b[1].x = tHat1.x * dist + b[0].x;
		b[1].y = tHat1.y * dist + b[0].y;
		b[2].x = tHat2.x * dist + b[3].x;
		b[2].y = tHat2.y * dist + b[3].y;
#endif
#if 0
		DrawBezierCurve(3, bezCurve);
#endif
		return TRUE;
    }

    /*  Parameterize points, and attempt to fit curve */
    u = ChordLengthParameterize(d, first, last);
    GenerateBezier(b, d, first, last, u, tHat1, tHat2);

    /*  Find max deviation of points to fitted curve */
    maxError = ComputeMaxError(d, first, last, b, u, &splitPoint);

    if (maxError < error) {
#if 0
		DrawBezierCurve(3, bezCurve);
#endif
		g_free (u);
		return TRUE;
    }


    /*  If error not too large, try some reparameterization  */
    /*  and iteration */
    if (maxError < iterationError) {
		for (i = 0; i < maxIterations; i++) {
	    	uPrime = Reparameterize(d, first, last, u, b);
	    	GenerateBezier(b, d, first, last, uPrime, tHat1, tHat2);
	    	maxError = ComputeMaxError(d, first, last, b, uPrime, &splitPoint);
	    	if (maxError < error) {
#if 0
			DrawBezierCurve(3, bezCurve);
#endif
			g_free (u);
			g_free (uPrime);
			return TRUE;
	    }
	    g_free (u);
	    u = uPrime;
	}
    }

    g_free (u);
    return FALSE;
#if 0
    /* Fitting failed -- split at max error point and fit recursively */
    g_free (u);
    g_free (bezCurve);
    tHatCenter = ComputeCenterTangent(d, splitPoint);
    FitCubic(d, first, splitPoint, tHat1, tHatCenter, error);
    V2Negate(&tHatCenter);
    FitCubic(d, splitPoint, last, tHatCenter, tHat2, error);
#endif
}

/*
 *  GenerateBezier :
 *  Use least-squares method to find Bezier control points for region.
 *
 */
static void
GenerateBezier (ArtPoint * b, ArtPoint * d, gint first, gint last, double * uPrime, ArtPoint tHat1, ArtPoint tHat2)
{
#if 0
    Point2	*d;			/*  Array of digitized points	*/
    int		first, last;		/*  Indices defining region	*/
    double	*uPrime;		/*  Parameter values for region */
    Vector2	tHat1, tHat2;	/*  Unit tangents at endpoints	*/
#endif
    int 	i;
    ArtPoint 	A[MAXPOINTS][2];	/* Precomputed rhs for eqn	*/
    int 	nPts;			/* Number of pts in sub-curve */
    double 	C[2][2];			/* Matrix C		*/
    double 	X[2];			/* Matrix X			*/
    double 	det_C0_C1,		/* Determinants of matrices	*/
    	   	det_C0_X,
	   		det_X_C1;
    double 	alpha_l,		/* Alpha values, left and right	*/
    	   	alpha_r;
    ArtPoint 	tmp;			/* Utility variable		*/
#if 0
    BezierCurve	bezCurve;	/* RETURN bezier curve ctl pts	*/
#endif

    nPts = last - first + 1;
 
    /* Compute the A's	*/
    for (i = 0; i < nPts; i++) {
		ArtPoint       	v1, v2;
		gdouble b1, b2;

		b1 = B1(uPrime[i]);
		b2 = B2(uPrime[i]);
		v1.x = tHat1.x * b1;
		v1.y = tHat1.y * b1;
		v2.x = tHat2.x * b2;
		v2.y = tHat2.y * b2;
#if 0
		V2Scale(&v1, B1(uPrime[i]));
		V2Scale(&v2, B2(uPrime[i]));
#endif
		A[i][0] = v1;
		A[i][1] = v2;
    }

    /* Create the C and X matrices	*/
    C[0][0] = 0.0;
    C[0][1] = 0.0;
    C[1][0] = 0.0;
    C[1][1] = 0.0;
    X[0]    = 0.0;
    X[1]    = 0.0;

    for (i = 0; i < nPts; i++) {
        C[0][0] += V2Dot(&A[i][0], &A[i][0]);
		C[0][1] += V2Dot(&A[i][0], &A[i][1]);
/*					C[1][0] += V2Dot(&A[i][0], &A[i][1]);*/	
		C[1][0] = C[0][1];
		C[1][1] += V2Dot(&A[i][1], &A[i][1]);

		tmp = V2SubII(d[first + i],
	        V2AddII(
	          V2ScaleIII(d[first], B0(uPrime[i])),
		    	V2AddII(
		      		V2ScaleIII(d[first], B1(uPrime[i])),
		        			V2AddII(
	                  		V2ScaleIII(d[last], B2(uPrime[i])),
	                    		V2ScaleIII(d[last], B3(uPrime[i]))))));
	

	X[0] += V2Dot(&A[i][0], &tmp);
	X[1] += V2Dot(&A[i][1], &tmp);
    }

    /* Compute the determinants of C and X	*/
    det_C0_C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
    det_C0_X  = C[0][0] * X[1]    - C[0][1] * X[0];
    det_X_C1  = X[0]    * C[1][1] - X[1]    * C[0][1];

    /* Finally, derive alpha values	*/
    if (det_C0_C1 == 0.0) {
		det_C0_C1 = (C[0][0] * C[1][1]) * 10e-12;
    }
    alpha_l = det_X_C1 / det_C0_C1;
    alpha_r = det_C0_X / det_C0_C1;


    /*  If alpha negative, use the Wu/Barsky heuristic (see text) */
	/* (if alpha is 0, you get coincident control points that lead to
	 * divide by zero in any subsequent NewtonRaphsonRootFind() call. */
    if (alpha_l < 1.0e-6 || alpha_r < 1.0e-6) {
		double	dist;

		dist = hypot (d[last].x - d[first].x, d[last].y - d[first].y) / 3.0;

		b[0] = d[first];
		b[3] = d[last];
#if 0
		V2Add(&bezCurve[0], V2Scale(&tHat1, dist), &bezCurve[1]);
		V2Add(&bezCurve[3], V2Scale(&tHat2, dist), &bezCurve[2]);
#else
		b[1].x = tHat1.x * dist + b[0].x;
		b[1].y = tHat1.y * dist + b[0].y;
		b[2].x = tHat2.x * dist + b[3].x;
		b[2].y = tHat2.y * dist + b[3].y;
#endif
		return;
    }

    /*  First and last control points of the Bezier curve are */
    /*  positioned exactly at the first and last data points */
    /*  Control points 1 and 2 are positioned an alpha distance out */
    /*  on the tangent vectors, left and right, respectively */
    b[0] = d[first];
    b[3] = d[last];
#if 0
    V2Add(&bezCurve[0], V2Scale(&tHat1, alpha_l), &bezCurve[1]);
    V2Add(&bezCurve[3], V2Scale(&tHat2, alpha_r), &bezCurve[2]);
#else
    b[1].x = tHat1.x * alpha_l + b[0].x;
    b[1].y = tHat1.y * alpha_l + b[0].y;
    b[2].x = tHat2.x * alpha_r + b[3].x;
    b[2].y = tHat2.y * alpha_r + b[3].y;
#endif
    return;
}

/*
 *  Reparameterize:
 *	Given set of points and their parameterization, try to find
 *   a better parameterization.
 *
 */
static gdouble *
Reparameterize(ArtPoint * d, gint first, gint last, gdouble * u, BezierCurve bezCurve)
{
#if 0
    Point2	*d;			/*  Array of digitized points	*/
    int		first, last;		/*  Indices defining region	*/
    double	*u;			/*  Current parameter values	*/
    BezierCurve	bezCurve;	/*  Current fitted curve	*/
#endif
    int 	nPts = last-first+1;	
    int 	i;
    gdouble	*uPrime;		/*  New parameter values	*/

    uPrime = g_new (gdouble, nPts);

    for (i = first; i <= last; i++) {
		uPrime[i-first] = NewtonRaphsonRootFind(bezCurve, d[i], u[i-
					first]);
    }
    return (uPrime);
}

/*
 *  NewtonRaphsonRootFind :
 *	Use Newton-Raphson iteration to find better root.
 */
static gdouble
NewtonRaphsonRootFind(BezierCurve Q, ArtPoint P, gdouble u)
{
#if 0
    BezierCurve	Q;			/*  Current fitted curve	*/
    Point2 		P;		/*  Digitized point		*/
    double 		u;		/*  Parameter value for "P"	*/
#endif
    double 		numerator, denominator;
    ArtPoint 		Q1[3], Q2[2];	/*  Q' and Q''			*/
    ArtPoint		Q_u, Q1_u, Q2_u; /*u evaluated at Q, Q', & Q''	*/
    double 		uPrime;		/*  Improved u			*/
    int 		i;
    
    /* Compute Q(u)	*/
    Q_u = BezierII(3, Q, u);
    
    /* Generate control vertices for Q'	*/
    for (i = 0; i <= 2; i++) {
		Q1[i].x = (Q[i+1].x - Q[i].x) * 3.0;
		Q1[i].y = (Q[i+1].y - Q[i].y) * 3.0;
    }
    
    /* Generate control vertices for Q'' */
    for (i = 0; i <= 1; i++) {
		Q2[i].x = (Q1[i+1].x - Q1[i].x) * 2.0;
		Q2[i].y = (Q1[i+1].y - Q1[i].y) * 2.0;
    }
    
    /* Compute Q'(u) and Q''(u)	*/
    Q1_u = BezierII(2, Q1, u);
    Q2_u = BezierII(1, Q2, u);
    
    /* Compute f(u)/f'(u) */
    numerator = (Q_u.x - P.x) * (Q1_u.x) + (Q_u.y - P.y) * (Q1_u.y);
    denominator = (Q1_u.x) * (Q1_u.x) + (Q1_u.y) * (Q1_u.y) +
		      	  (Q_u.x - P.x) * (Q2_u.x) + (Q_u.y - P.y) * (Q2_u.y);
    
    /* u = u - f(u)/f'(u) */
    uPrime = u - (numerator/denominator);
    return (uPrime);
}

/*
 *  Bezier :
 *  	Evaluate a Bezier curve at a particular parameter value
 * 
 */
static ArtPoint
BezierII (gint degree, ArtPoint * V, gdouble t)
{
#if 0
    int		degree;		/* The degree of the bezier curve	*/
    Point2 	*V;		/* Array of control points		*/
    double 	t;		/* Parametric value to find point for	*/
#endif
    int 	i, j;		
    ArtPoint 	Q;	        /* Point on curve at parameter t	*/
    ArtPoint 	*Vtemp;		/* Local copy of control points		*/

    /* Copy array	*/
    Vtemp = g_new (ArtPoint, degree + 1);

    for (i = 0; i <= degree; i++) {
		Vtemp[i] = V[i];
    }

    /* Triangle computation	*/
    for (i = 1; i <= degree; i++) {	
		for (j = 0; j <= degree-i; j++) {
	    	Vtemp[j].x = (1.0 - t) * Vtemp[j].x + t * Vtemp[j+1].x;
	    	Vtemp[j].y = (1.0 - t) * Vtemp[j].y + t * Vtemp[j+1].y;
		}
    }

    Q = Vtemp[0];
    g_free (Vtemp);
    return Q;
}

/*
 *  B0, B1, B2, B3 :
 *	Bezier multipliers
 */
static gdouble B0 (gdouble u)
{
    double tmp = 1.0 - u;
    return (tmp * tmp * tmp);
}


static gdouble B1 (gdouble u)
{
    double tmp = 1.0 - u;
    return (3 * u * (tmp * tmp));
}

static gdouble B2 (gdouble u)
{
    double tmp = 1.0 - u;
    return (3 * u * u * tmp);
}

static gdouble B3(gdouble u)
{
    return (u * u * u);
}



/*
 * ComputeLeftTangent, ComputeRightTangent, ComputeCenterTangent :
 *Approximate unit tangents at endpoints and "center" of digitized curve
 */
static ArtPoint
ComputeLeftTangent (ArtPoint * d, gint end)
{
    ArtPoint	tHat1;
    tHat1 = V2SubII(d[end+1], d[end]);
    V2Normalize(&tHat1);
    return tHat1;
}

static ArtPoint
ComputeRightTangent (ArtPoint * d, gint end)
{
    ArtPoint	tHat2;
    tHat2 = V2SubII(d[end-1], d[end]);
    V2Normalize(&tHat2);
    return tHat2;
}

#if 0
static ArtPoint
ComputeCenterTangent(ArtPoint * d, gint center)
{
    ArtPoint	V1, V2, tHatCenter;

    V1 = V2SubII(d[center-1], d[center]);
    V2 = V2SubII(d[center], d[center+1]);
    tHatCenter.x = (V1.x + V2.x)/2.0;
    tHatCenter.y = (V1.y + V2.y)/2.0;
    V2Normalize(&tHatCenter);
    return tHatCenter;
}
#endif

/*
 *  ChordLengthParameterize :
 *	Assign parameter values to digitized points 
 *	using relative distances between points.
 */
static gdouble *
ChordLengthParameterize(ArtPoint * d, gint first, gint last)
{
    int		i;	
    gdouble	*u;			/*  Parameterization		*/

    u = g_new (gdouble, last - first + 1);

    u[0] = 0.0;
    for (i = first+1; i <= last; i++) {
		u[i-first] = u[i-first-1] +
			hypot (d[i].x - d[i-1].x, d[i].y - d[i-1].y);
    }

    for (i = first + 1; i <= last; i++) {
		u[i-first] = u[i-first] / u[last-first];
    }

    return(u);
}




/*
 *  ComputeMaxError :
 *	Find the maximum squared distance of digitized points
 *	to fitted curve.
*/
static gdouble
ComputeMaxError(ArtPoint * d, gint first, gint last, BezierCurve bezCurve, gdouble * u, gint * splitPoint)
{
#if 0
    Point2	*d;			/*  Array of digitized points	*/
    int		first, last;		/*  Indices defining region	*/
    BezierCurve	bezCurve;		/*  Fitted Bezier curve		*/
    double	*u;			/*  Parameterization of points	*/
    int		*splitPoint;		/*  Point of maximum error	*/
#endif
    int		i;
    double	maxDist;		/*  Maximum error		*/
    double	dist;		/*  Current error		*/
    ArtPoint	P;			/*  Point on curve		*/
    ArtPoint	v;			/*  Vector from point to curve	*/

    *splitPoint = (last - first + 1)/2;
    maxDist = 0.0;
    for (i = first + 1; i < last; i++) {
		P = BezierII(3, bezCurve, u[i-first]);
		v = V2SubII(P, d[i]);
		dist = v.x * v.x + v.y * v.y;
		if (dist >= maxDist) {
	    	maxDist = dist;
	    	*splitPoint = i;
		}
    }
    return (maxDist);
}

static ArtPoint V2AddII (ArtPoint a, ArtPoint b)
{
    ArtPoint	c;
    c.x = a.x + b.x;  c.y = a.y + b.y;
    return (c);
}

static ArtPoint V2ScaleIII (ArtPoint v, gdouble s)
{
    ArtPoint result;
    result.x = v.x * s; result.y = v.y * s;
    return (result);
}

static ArtPoint V2SubII (ArtPoint a, ArtPoint b)
{
    ArtPoint	c;
    c.x = a.x - b.x; c.y = a.y - b.y;
    return (c);
}

static void
V2Normalize (ArtPoint * v)
{
	gdouble len;

	len = hypot (v->x, v->y);
	v->x /= len;
	v->y /= len;
}

