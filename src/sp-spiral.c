#define __SP_SPIRAL_C__

/*
 * <sodipodi:spiral> implementation
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "svg/svg.h"
#include "knotholder.h"
#include "helper/bezier-utils.h"
#include "dialogs/object-attributes.h"

#include "sp-spiral.h"

#define noSPIRAL_VERBOSE

#define SP_EPSILON       1e-5
#define SP_EPSILON_2     (SP_EPSILON * SP_EPSILON)
#define SP_HUGE          1e5

#define SPIRAL_TOLERANCE 3.0
#define SAMPLE_STEP      (1.0/4.0) /* step per 2PI */
#define SAMPLE_SIZE      8      /* sample size per one bezier */


static void sp_spiral_class_init (SPSpiralClass *class);
static void sp_spiral_init (SPSpiral *spiral);
static void sp_spiral_destroy (GtkObject *object);

static void sp_spiral_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_spiral_write_repr (SPObject * object, SPRepr * repr);
static void sp_spiral_read_attr (SPObject * object, const gchar * attr);

static SPKnotHolder *sp_spiral_knot_holder (SPItem * item, SPDesktop *desktop);
static gchar * sp_spiral_description (SPItem * item);
static GSList * sp_spiral_snappoints (SPItem * item, GSList * points);
static void sp_spiral_set_shape (SPShape *shape);

static void sp_spiral_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);
static void sp_spiral_spiral_properties (GtkMenuItem *menuitem, SPAnchor *anchor);

static SPShapeClass *parent_class;

GtkType
sp_spiral_get_type (void)
{
	static GtkType spiral_type = 0;

	if (!spiral_type) {
		GtkTypeInfo spiral_info = {
			"SPSpiral",
			sizeof (SPSpiral),
			sizeof (SPSpiralClass),
			(GtkClassInitFunc) sp_spiral_class_init,
			(GtkObjectInitFunc) sp_spiral_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		spiral_type = gtk_type_unique (sp_shape_get_type (), &spiral_info);
	}
	return spiral_type;
}

static void
sp_spiral_class_init (SPSpiralClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPShapeClass *shape_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;
	shape_class = (SPShapeClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_spiral_destroy;

	sp_object_class->build = sp_spiral_build;
	sp_object_class->write_repr = sp_spiral_write_repr;
	sp_object_class->read_attr = sp_spiral_read_attr;

	item_class->knot_holder = sp_spiral_knot_holder;
	item_class->description = sp_spiral_description;
	item_class->snappoints = sp_spiral_snappoints;
	item_class->menu = sp_spiral_menu;

	shape_class->set_shape = sp_spiral_set_shape;
}

static void
sp_spiral_init (SPSpiral * spiral)
{
	SP_PATH (spiral)->independent = FALSE;
	
	spiral->cx         = 0.0;
	spiral->cy         = 0.0;
	spiral->exp        = 1.0;
	spiral->revo       = 3.0;
	spiral->rad        = 0.0;
	spiral->arg        = 0.0;
	spiral->t0         = 0.0;
}

static void
sp_spiral_destroy (GtkObject *object)
{
	SPSpiral *spiral;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_SPIRAL (object));

	spiral = SP_SPIRAL (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_spiral_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);
	sp_spiral_read_attr (object, "sodipodi:cx");
	sp_spiral_read_attr (object, "sodipodi:cy");
	sp_spiral_read_attr (object, "sodipodi:expansion");
	sp_spiral_read_attr (object, "sodipodi:revolution");
	sp_spiral_read_attr (object, "sodipodi:radius");
	sp_spiral_read_attr (object, "sodipodi:argument");
	sp_spiral_read_attr (object, "sodipodi:t0");
}

static void
sp_spiral_write_repr (SPObject * object, SPRepr * repr)
{
	SPSpiral *spiral;

	spiral = SP_SPIRAL (object);

	/* Fixme: we may replace these attributes by
	 * sodipodi:spiral="cx cy exp revo rad arg t0"
	 */
	if ((spiral->cx > SP_EPSILON) || (spiral->cx < -SP_EPSILON))
		sp_repr_set_double_attribute (repr, "sodipodi:cx", spiral->cx);
	if ((spiral->cy > SP_EPSILON) || (spiral->cy < -SP_EPSILON))
		sp_repr_set_double_attribute (repr, "sodipodi:cy", spiral->cy);
	sp_repr_set_double_attribute (repr, "sodipodi:expansion", spiral->exp);
	sp_repr_set_double_attribute (repr, "sodipodi:revolution", spiral->revo);
	sp_repr_set_double_attribute (repr, "sodipodi:radius", spiral->rad);
	sp_repr_set_double_attribute (repr, "sodipodi:argument", spiral->arg);
	sp_repr_set_double_attribute (repr, "sodipodi:t0", spiral->t0);

	if (((SPObjectClass *) (parent_class))->write_repr)
		(*((SPObjectClass *) (parent_class))->write_repr) (object, repr);
}

#if 0
/* Yeah, this is nice, but values are of different type */
/* How would you interpet sodipodi:expansion="10mm" ;-() (Lauris) */
	/* I use dirty macro */
#define sp_spiral_test_new_dvalue(dst,defval) { 				\
		gdouble dvalue = dst;					\
		dst = sp_svg_read_length (&unit, astr, defval);		\
		if (sp_spiral_is_invalid (spiral)) {			\
			g_warning ("new " #dst " value is invalid, so ignored: %g\n", dst);	\
			dst = dvalue;					\
			return;						\
		}							\
	}
#endif

static void
sp_spiral_read_attr (SPObject * object, const gchar * attr)
{
	SPSpiral *spiral;
	SPShape  *shape;
	const gchar *str;
	gulong unit;

	spiral = SP_SPIRAL (object);
	shape  = SP_SHAPE (object);

#ifdef SPIRAL_VERBOSE
	g_print ("sp_spiral_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);


	/* fixme: we should really collect updates */
	if (!strcmp (attr, "sodipodi:cx")) {
		if (!sp_svg_length_read_lff (str, &unit, NULL, &spiral->cx) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			spiral->cx = 0.0;
		}
		sp_shape_set_shape (shape);
	} else if (!strcmp (attr, "sodipodi:cy")) {
		if (!sp_svg_length_read_lff (str, &unit, NULL, &spiral->cy) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			spiral->cy = 0.0;
		}
		sp_shape_set_shape (shape);
	} else if (!strcmp (attr, "sodipodi:expansion")) {
		if (str) {
			spiral->exp = atof (str);
			spiral->exp = MAX (spiral->exp, 0.0);
		} else {
			spiral->exp = 1.0;
		}
		sp_shape_set_shape (shape);
	} else if (!strcmp (attr, "sodipodi:revolution")) {
		if (str) {
			spiral->revo = atof (str);
			spiral->revo = CLAMP (spiral->revo, 0.05, 20.0);
		} else {
			spiral->revo = 3.0;
		}
		sp_shape_set_shape (shape);
	} else if (!strcmp (attr, "sodipodi:radius")) {
		if (!sp_svg_length_read_lff (str, &unit, NULL, &spiral->rad) ||
		    (unit != SP_SVG_UNIT_EM) ||
		    (unit != SP_SVG_UNIT_EX) ||
		    (unit != SP_SVG_UNIT_PERCENT)) {
			spiral->rad = MAX (spiral->rad, 0.0);
		}
		sp_shape_set_shape (shape);
	} else if (strcmp (attr, "sodipodi:argument") == 0) {
		if (str) {
			spiral->arg = atof (str);
		} else {
			spiral->arg = 0.0;
		}
		sp_shape_set_shape (shape);
	} else if (strcmp (attr, "sodipodi:t0") == 0) {
		if (str) {
			spiral->t0 = atof (str);
			spiral->t0 = CLAMP (spiral->t0, -1.0, 1.0);
		} else {
			spiral->t0 = 0.0;
		}
		sp_shape_set_shape (shape);
	} else {
		if (SP_OBJECT_CLASS (parent_class)->read_attr) {
			SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);
		}
	}
}

static gchar *
sp_spiral_description (SPItem * item)
{
	return g_strdup ("Spiral");
}

static void
sp_spiral_fit_and_draw (SPSpiral *spiral,
			SPCurve	 *c,
			double dstep,
			ArtPoint *darray,
			ArtPoint *hat1,
			ArtPoint *hat2,
			double    t)
{
#define BEZIER_SIZE   4
#define FITTING_DEPTH 3
#define BEZIER_LENGTH (BEZIER_SIZE * (2 << (FITTING_DEPTH - 1)))

	ArtPoint	bezier[BEZIER_LENGTH];
	gdouble		d;
	gint depth, i;
	
	for (d = t, i = 0; i <= SAMPLE_SIZE; d += dstep, i++) {
		sp_spiral_get_xy (spiral, d, &darray[i]);
	}
	
	sp_darray_center_tangent (darray, SAMPLE_SIZE - 1, hat2);
	
	/* Fixme:
	   we should use better algorithm to specify maximum error.
	*/
	depth = sp_bezier_fit_cubic_full (bezier, darray, SAMPLE_SIZE,
					  hat1, hat2,
					  SPIRAL_TOLERANCE*SPIRAL_TOLERANCE,
					  FITTING_DEPTH);
#ifdef SPIRAL_DEBUG
	if (t==spiral->t0 || t==1.0)
		g_print ("[%s] depth=%d, dstep=%g, t0=%g, t=%g, arg=%g\n",
			 debug_state, depth, dstep, spiral->t0, t, spiral->arg);
#endif
	if (depth != -1) {
		for (i = 0; i < 4*depth; i += 4) {
			sp_curve_curveto (c, bezier[i + 1].x, bezier[i + 1].y,
					  bezier[i + 2].x, bezier[i + 2].y,
					  bezier[i + 3].x, bezier[i + 3].y);
#ifdef SPIRAL_DEBUG
			if (debug_fit_and_draw)
				g_print("[(%g,%g)-(%g,%g)-(%g,%g)]\n", bezier[i+1].x, bezier[i+1].y, bezier[i+2].x, bezier[i+2].y, bezier[i+3].x, bezier[i+3].y);
#endif
		}
	} else {
#ifdef SPIRAL_VERBOSE
		g_print ("cant_fit_cubic: t=%g\n", t);
#endif
		for (i = 1; i < SAMPLE_SIZE; i++)
			sp_curve_lineto (c, darray[i].x, darray[i].y);
	}
}

static void
sp_spiral_set_shape (SPShape *shape)
{
	SPSpiral       *spiral;
	ArtPoint	darray[SAMPLE_SIZE + 1];
	ArtPoint	hat1, hat2;
	gint		i;
	gdouble		tstep, t;
	gdouble		dstep, d;
	SPCurve	       *c;

	spiral = SP_SPIRAL(shape);

	sp_object_request_modified (SP_OBJECT (spiral), SP_OBJECT_MODIFIED_FLAG);

	sp_path_clear (SP_PATH (shape));
	
	if (spiral->rad < SP_EPSILON) return;
	
	c = sp_curve_new ();
	
#ifdef SPIRAL_VERBOSE
	g_print ("ex=%g, revo=%g, rad=%g, arg=%g, t0=%g\n",
		 spiral->cx,
		 spiral->cy,
		 spiral->exp,
		 spiral->revo,
		 spiral->rad,
		 spiral->arg,
		 spiral->t0);
#endif
	
	tstep = SAMPLE_STEP/spiral->revo;
	dstep = tstep/(SAMPLE_SIZE - 1.0);

	if (spiral->t0 - dstep >= 0.0) {
		for (d = spiral->t0 - dstep, i = 0; i <= 2; d += dstep, i++)
			sp_spiral_get_xy (spiral, d, &darray[i]);

		sp_darray_center_tangent (darray, 1, &hat1);
		hat1.x = - hat1.x;
		hat1.y = - hat1.y;
	} else {
		for (d = spiral->t0, i = 1; i <= 2; d += dstep, i++)
			sp_spiral_get_xy (spiral, d, &darray[i]);

		sp_darray_left_tangent (darray, 1, &hat1);
	}

	sp_curve_moveto (c, darray[1].x, darray[1].y);

	for (t = spiral->t0; t < (1.0-tstep); t += tstep)
	{
		sp_spiral_fit_and_draw (spiral, c, dstep, darray, &hat1, &hat2, t);

		hat1.x = - hat2.x;
		hat1.y = - hat2.y;
	}
	if ((1.0 - t) > SP_EPSILON)
		sp_spiral_fit_and_draw (spiral, c, (1.0 - t)/(SAMPLE_SIZE - 1.0),
					darray, &hat1, &hat2, t);
  
	sp_path_add_bpath (SP_PATH (spiral), c, TRUE, NULL);
	sp_curve_unref (c);
}

/*
 * set attributes via inner (t=t0) knot point:
 *   [default] increase/decrease inner point
 *   [shift]   increase/decrease inner and outer arg synchronizely
 *   [control] constrain inner arg to round per PI/4
 */
static void
sp_spiral_inner_set (SPItem   *item,
		     const ArtPoint *p,
		     guint state)
{
	SPSpiral *spiral;
	gdouble   dx, dy;
	gdouble   arg_tmp;
	gdouble   arg_t0;
	gdouble   arg_t0_new;

	spiral = SP_SPIRAL (item);

	dx = p->x - spiral->cx;
	dy = p->y - spiral->cy;
	sp_spiral_get_polar (spiral, spiral->t0, NULL, &arg_t0);
/*  	arg_t0 = 2.0*M_PI*spiral->revo * spiral->t0 + spiral->arg; */
	arg_tmp = atan2(dy, dx) - arg_t0;
	arg_t0_new = arg_tmp - floor((arg_tmp+M_PI)/(2.0*M_PI))*2.0*M_PI + arg_t0;
	spiral->t0 = (arg_t0_new - spiral->arg) / (2.0*M_PI*spiral->revo);
#if 0				/* we need round function */
	/* round inner arg per PI/4, if CTRL is pressed */
	if ((state & GDK_CONTROL_MASK) &&
	    (fabs(spiral->revo) > SP_EPSILON_2)) {
		gdouble arg = 2.0*M_PI*spiral->revo*spiral->t0 + spiral->arg;
		t0 = (round(arg/(0.25*M_PI))*0.25*M_PI
		      - spiral->arg)/(2.0*M_PI*spiral->revo);
	}
#endif
	spiral->t0 = CLAMP (spiral->t0, 0.0, 0.999);

#if 0
	/* outer point synchronize with inner point, if SHIFT is pressed */
	if (state & GDK_SHIFT_MASK) {
		spiral->revo += spiral->revo * (t0 - spiral->t0);
	}
	spiral->t0 = t0;
#endif

}

/*
 * set attributes via outer (t=1) knot point:
 *   [default] increase/decrease revolution factor
 *   [control] constrain inner arg to round per PI/4
 */
static void
sp_spiral_outer_set (SPItem   *item,
		     const ArtPoint *p,
		     guint state)
{
	SPSpiral *spiral;
	gdouble   dx, dy;
/*  	gdouble arg; */
	
	spiral = SP_SPIRAL (item);

	dx = p->x - spiral->cx;
	dy = p->y - spiral->cy;
	spiral->arg = atan2(dy, dx) - 2.0*M_PI*spiral->revo;
	spiral->rad = hypot(dx, dy);
#if 0
 /* we need round function */
/*  	arg  = -atan2(p->y, p->x) - spiral->arg; */
	if (state & GDK_CONTROL_MASK) {
		spiral->revo = (round(arg/(0.25*M_PI))*0.25*M_PI)/(2.0*M_PI);
	} else {
		spiral->revo = arg/(2.0*M_PI);
	}
	spiral->revo = arg/(2.0*M_PI);
#endif
}

static void
sp_spiral_inner_get (SPItem *item,
		     ArtPoint *p)
{
	SPSpiral *spiral;

	spiral = SP_SPIRAL (item);

	sp_spiral_get_xy (spiral, spiral->t0, p);
}

static void
sp_spiral_outer_get (SPItem *item,
		     ArtPoint *p)
{
	SPSpiral *spiral;

	spiral = SP_SPIRAL (item);

	sp_spiral_get_xy (spiral, 1.0, p);
}

static SPKnotHolder *
sp_spiral_knot_holder (SPItem * item, SPDesktop *desktop)
{
	SPSpiral *spiral;
	SPKnotHolder *knot_holder;

	spiral = SP_SPIRAL (item);
	knot_holder = sp_knot_holder_new (desktop, item);

	sp_knot_holder_add (knot_holder,
			    sp_spiral_inner_set,
			    sp_spiral_inner_get);
	sp_knot_holder_add (knot_holder,
			    sp_spiral_outer_set,
			    sp_spiral_outer_get);

	return knot_holder;
}

void
sp_spiral_set       (SPSpiral          *spiral,
		     gdouble            cx,
		     gdouble            cy,
		     gdouble            exp,
		     gdouble            revo,
		     gdouble            rad,
		     gdouble            arg,
		     gdouble            t0)
{
	g_return_if_fail (spiral != NULL);
	g_return_if_fail (SP_IS_SPIRAL (spiral));
	
	spiral->cx         = cx;
	spiral->cy         = cy;
	spiral->exp        = exp;
	spiral->revo       = revo;
	spiral->rad        = rad;
	spiral->arg        = arg;
	spiral->t0         = t0;
	
	sp_shape_set_shape (SP_SHAPE(spiral));
}

static GSList * 
sp_spiral_snappoints (SPItem * item, GSList * points)
{
	SPSpiral *spiral;
	ArtPoint * p, p1, p2, p3;
	gdouble affine[6];
	
	spiral = SP_SPIRAL(item);
	
	sp_spiral_get_xy (spiral, 0.0, &p1);
	sp_spiral_get_xy (spiral, spiral->t0, &p2);
	sp_spiral_get_xy (spiral, 1.0, &p3);
	
	sp_item_i2d_affine (item, affine);
	
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p1, affine);
	points = g_slist_append (points, p);
	
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p2, affine);
	points = g_slist_append (points, p);
	
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p3, affine);
	points = g_slist_append (points, p);
	
	return points;
}

void
sp_spiral_get_xy (SPSpiral *spiral,
		  gdouble   t,
		  ArtPoint *p)
{
	gdouble rad, arg;

	g_return_if_fail (spiral != NULL);
	g_return_if_fail (SP_IS_SPIRAL(spiral));
	g_return_if_fail (p != NULL);

	rad = spiral->rad * pow(t, spiral->exp);
	arg = 2.0 * M_PI * spiral->revo * t + spiral->arg;
	
	p->x = rad * cos (arg) + spiral->cx;
	p->y = rad * sin (arg) + spiral->cy;
}

void
sp_spiral_get_polar (SPSpiral *spiral, gdouble t, gdouble *rad, gdouble *arg)
{
	g_return_if_fail (spiral != NULL);
	g_return_if_fail (SP_IS_SPIRAL(spiral));

	if (rad)
		*rad = spiral->rad * pow(t, spiral->exp);
	if (arg)
		*arg = 2.0 * M_PI * spiral->revo * t + spiral->arg;
}

gboolean
sp_spiral_is_invalid (SPSpiral *spiral)
{
	gdouble rad;

	sp_spiral_get_polar (spiral, 0.0, &rad, NULL);
	if (rad < 0.0 || rad > SP_HUGE) {
		g_print ("rad(t=0)=%g\n", rad);
		return TRUE;
	}
	sp_spiral_get_polar (spiral, 1.0, &rad, NULL);
	if (rad < 0.0 || rad > SP_HUGE) {
		g_print ("rad(t=1)=%g\n", rad);
		return TRUE;
	}
	return FALSE;
}

/* Generate context menu item section */

static void
sp_spiral_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Spiral"));
	m = gtk_menu_new ();
	/* Link dialog */
	w = gtk_menu_item_new_with_label (_("Spiral Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_spiral_spiral_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

static void
sp_spiral_spiral_properties (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	sp_object_attributes_dialog (SP_OBJECT (anchor), "SPSpiral");
}

