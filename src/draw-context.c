#define SP_DRAW_CONTEXT_C

/*
 * drawing-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make freehand generating curves instead of lines
 *
 */

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"

#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "draw-context.h"

#define TOLERANCE 1.0

static void sp_draw_context_class_init (SPDrawContextClass * klass);
static void sp_draw_context_init (SPDrawContext * draw_context);
static void sp_draw_context_destroy (GtkObject * object);

static void sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

#if 0
static void sp_draw_finish_item (SPDrawContext * draw_context);

static void sp_draw_ctrl_test_inside (SPDrawContext * draw_context, double x, double y);
static void sp_draw_move_ctrl (SPDrawContext * draw_context, double x, double y);
static void sp_draw_remove_ctrl (SPDrawContext * draw_context);
static SPCanvasShape * sp_draw_create_item (SPDrawContext * draw_context);

static void sp_draw_path_startline (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_freehand (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_endfreehand (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_lineendpoint (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_endline (SPDrawContext * draw_context, double x, double y);
#endif

static void concat_current (SPDrawContext * dc);
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
}

static void
sp_draw_context_destroy (GtkObject * object)
{
	SPDrawContext * dc;

	dc = SP_DRAW_CONTEXT (object);

	if (dc->accumulated) sp_curve_unref (dc->accumulated);

	while (dc->segments) {
		gtk_object_destroy (GTK_OBJECT (dc->segments->data));
		dc->segments = g_slist_remove_link (dc->segments, dc->segments);
	}

	if (dc->currentcurve) sp_curve_unref (dc->currentcurve);

	if (dc->currentshape) gtk_object_destroy (GTK_OBJECT (dc->currentshape));

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPDrawContext * dc;
	SPStroke * stroke;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	dc = SP_DRAW_CONTEXT (event_context);

	dc->accumulated = sp_curve_new_sized (32);
	dc->currentcurve = sp_curve_new_sized (4);

	stroke = sp_stroke_new_colored (0x000000ff, 2.0, FALSE);
	dc->currentshape = (SPCanvasShape *) gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
								    SP_TYPE_CANVAS_SHAPE, NULL);
	sp_canvas_shape_set_stroke (dc->currentshape, stroke);
	sp_stroke_unref (stroke);
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
			/* initialize first point */
			g_assert (dc->npoints == 0);
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_desktop_free_snap (desktop, &p);
			dc->p[dc->npoints++] = p;
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			g_assert (dc->npoints > 0);
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			if (event->motion.state & GDK_CONTROL_MASK) {
				gdouble dx, dy;
				dx = p.x - dc->p[dc->npoints - 1].x;
				dy = p.y - dc->p[dc->npoints - 1].y;
				if (fabs (dy) > fabs (dx)) {
					p.x = dc->p[dc->npoints - 1].x;
					sp_desktop_vertical_snap (desktop, &p);
				} else {
					p.y = dc->p[dc->npoints - 1].y;
					sp_desktop_horizontal_snap (desktop, &p);
				}
			} else {
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
			/* Remove all temporary line segments */
			while (dc->segments) {
				gtk_object_destroy (GTK_OBJECT (dc->segments->data));
				dc->segments = g_slist_remove_link (dc->segments, dc->segments);
			}
			/* clear shape */
			sp_canvas_shape_clear (dc->currentshape);
			/* reset curve */
			sp_curve_reset (dc->currentcurve);
			/* reset points */
			dc->npoints = 0;

			/* Create object */
			concat_current (dc);
			if (!sp_curve_empty (dc->accumulated)) {
				SPRepr * repr;
				SPItem * item;
				gchar * str;
				gdouble d2doc[6];
				ArtBpath * abp;

				sp_desktop_d2doc_affine (desktop, d2doc);
				abp = art_bpath_affine_transform (sp_curve_first_bpath (dc->accumulated), d2doc);
				g_assert (abp != NULL);
				str = sp_svg_write_path (abp);
				g_assert (str != NULL);
				art_free (abp);
				repr = sp_repr_new ("path");
				sp_repr_set_attr (repr, "d", str);
				g_free (str);
				sp_repr_set_attr (repr, "style", "stroke:#000;stroke-width:1");
				item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
				sp_repr_unref (repr);
				sp_selection_set_item (SP_DT_SELECTION (desktop), item);
				sp_document_done (SP_DT_DOCUMENT (desktop));
			}

			/* reset accumulated curve */
			sp_curve_reset (dc->accumulated);

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
		g_assert (bpath->code == ART_CURVETO);
		sp_curve_curveto (dc->accumulated, bpath->x1, bpath->y1, bpath->x2, bpath->y2, bpath->x3, bpath->y3);
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
static ArtPoint ComputeCenterTangent (ArtPoint * d, gint center);
static gdouble * ChordLengthParameterize(ArtPoint * d, gint first, gint last);
static gdouble ComputeMaxError(ArtPoint * d, gint first, gint last, BezierCurve bezCurve, gdouble * u, gint * splitPoint);
static ArtPoint V2AddII (ArtPoint a, ArtPoint b);
static ArtPoint V2ScaleIII (ArtPoint v, gdouble s);
static ArtPoint V2SubII (ArtPoint a, ArtPoint b);
static void V2Normalize (ArtPoint * v);

#define V2Dot(a,b) ((a)->x * (b)->x + (a)->y * (b)->y)

#define MAXPOINTS	1000		/* The most points you can have */

static void
fit_and_split (SPDrawContext * dc)
{
	ArtPoint t0, t1;
	ArtPoint b[4];

	g_assert (dc->npoints > 1);

	t0 = ComputeLeftTangent(dc->p, 0);
	t1 = ComputeRightTangent(dc->p, dc->npoints - 1);

	if (FitCubic (b, dc->p, 0, dc->npoints - 1, t0, t1, TOLERANCE * TOLERANCE) && dc->npoints < 16) {
		/* Fit and draw and reset state */
		g_print ("%d", dc->npoints);
		sp_curve_reset (dc->currentcurve);
		sp_curve_moveto (dc->currentcurve, b[0].x, b[0].y);
		sp_curve_curveto (dc->currentcurve, b[1].x, b[1].y, b[2].x, b[2].y, b[3].x, b[3].y);
		sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
	} else {
		SPCurve * curve;
		SPStroke * stroke;
		SPCanvasShape * cshape;
		/* Fit and draw and copy last point */
		g_print("[%d]Yup\n", dc->npoints);
		g_assert (!sp_curve_empty (dc->currentcurve));
		concat_current (dc);
		curve = sp_curve_copy (dc->currentcurve);
		stroke = sp_stroke_new_colored (0x000000ff, 2.0, FALSE);
		cshape = (SPCanvasShape *) gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
								  SP_TYPE_CANVAS_SHAPE, NULL);
		sp_canvas_shape_set_stroke (cshape, stroke);
		sp_stroke_unref (stroke);
		gtk_signal_connect (GTK_OBJECT (cshape), "event",
				    GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (dc)->desktop);

		sp_canvas_shape_add_component (cshape, curve, TRUE, NULL);

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
			return TRUE;
	    }
	    g_free (u);
	    u = uPrime;
	}
    }

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



