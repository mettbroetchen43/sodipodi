#define SP_SPIRAL_C

#include <gnome.h>
#include <math.h>
#include "svg/svg.h"
#include "sp-spiral.h"
#include "helper/bezier-utils.h"

#define noSPIRAL_VERBOSE

#define SPIRAL_TOLERANCE 2.0
#define SAMPLE_STEP      (1.0/4.0) /* step per 2PI */
#define SAMPLE_SIZE      8      /* sample size per one bezier */


static void sp_spiral_class_init (SPSpiralClass *class);
static void sp_spiral_init (SPSpiral *spiral);
static void sp_spiral_destroy (GtkObject *object);

static void sp_spiral_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_spiral_read_attr (SPObject * object, const gchar * attr);

static void sp_spiral_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_spiral_description (SPItem * item);
static GSList * sp_spiral_snappoints (SPItem * item, GSList * points);

static SPSpiralHand sp_spiral_read_type (const gchar *str);


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

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_spiral_destroy;

	sp_object_class->build = sp_spiral_build;
	sp_object_class->read_attr = sp_spiral_read_attr;

	item_class->bbox = sp_spiral_bbox;
	item_class->description = sp_spiral_description;
	item_class->snappoints = sp_spiral_snappoints;
}

static void
sp_spiral_init (SPSpiral * spiral)
{
	SP_PATH (spiral)->independent = FALSE;
	
	spiral->cx         = 0.0;
	spiral->cy         = 0.0;
	spiral->hand       = SP_SPIRAL_HAND_RIGHT;
	spiral->expansion  = 1.0;
	spiral->revolution = 3.0;
	spiral->radius     = 0.0;
	spiral->argument   = 0.0;
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
	sp_spiral_read_attr (object, "sodipodi:hand");
	sp_spiral_read_attr (object, "sodipodi:expansion");
	sp_spiral_read_attr (object, "sodipodi:revolution");
	sp_spiral_read_attr (object, "sodipodi:radius");
	sp_spiral_read_attr (object, "sodipodi:argument");
	sp_spiral_read_attr (object, "sodipodi:t0");
}

static void
sp_spiral_read_attr (SPObject * object, const gchar * attr)
{
	SPSpiral * spiral;
	const gchar * astr;
	SPSVGUnit unit;

	spiral = SP_SPIRAL (object);

#ifdef SPIRAL_VERBOSE
	g_print ("sp_spiral_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "sodipodi:cx") == 0) {
		spiral->cx = sp_svg_read_length (&unit, astr, 0.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:cy") == 0) {
		spiral->cy = sp_svg_read_length (&unit, astr, 0.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:hand") == 0) {
		spiral->hand = sp_spiral_read_type (astr);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:expansion") == 0) {
		spiral->expansion = sp_svg_read_length (&unit, astr, 1.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:revolution") == 0) {
		spiral->revolution = sp_svg_read_length (&unit, astr, 3.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:radius") == 0) {
		spiral->radius = sp_svg_read_length (&unit, astr, 0.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:argument") == 0) {
		spiral->argument = sp_svg_read_length (&unit, astr, 0.0);
		sp_spiral_set_shape (spiral);
		return;
	}
	if (strcmp (attr, "sodipodi:t0") == 0) {
		spiral->t0 = sp_svg_read_length (&unit, astr, 0.0);
		sp_spiral_set_shape (spiral);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);
}

static gchar *
sp_spiral_description (SPItem * item)
{
	return g_strdup ("Spiral");
}

void
sp_spiral_set_shape (SPSpiral * spiral)
{
	ArtPoint	darray[SAMPLE_SIZE + 1];
	ArtPoint	hat1, hat2;
	ArtPoint	bezier[4];
	gint		i;
	gdouble		tstep,  t;
	gdouble		dstep, d;
	SPCurve	       *c;

	
	sp_path_clear (SP_PATH (spiral));
	
	if (spiral->radius < 1e-12) return;
	
	c = sp_curve_new ();
	
#ifdef SPIRAL_VERBOSE
	g_print ("cx=%g, cy=%g, ex=%g, rev=%g, rad=%g, arg=%g, t0=%g\n",
		 spiral->cx,
		 spiral->cy,
		 spiral->expansion,
		 spiral->revolution,
		 spiral->radius,
		 spiral->argument,
		 spiral->t0);
#endif
	
	tstep = SAMPLE_STEP/spiral->revolution;
	dstep = tstep/(SAMPLE_SIZE - 1.0);

	for (d = spiral->t0 - dstep, i = 0; i < 2; d += dstep, i++)
		sp_spiral_get_xy (spiral, d, &darray[i].x, &darray[i].y);

	if (spiral->t0 - dstep >= 0.0) {
		sp_darray_center_tangent (darray, 1, &hat1);
		hat1.x = - hat1.x;
		hat1.y = - hat1.y;
	} else {
		sp_darray_left_tangent (darray, 1, &hat1);
	}

	sp_curve_moveto (c, darray[1].x, darray[1].y);

	/* Fixme: there is a rest spiral */
	for (t = spiral->t0; t < 1.0; t += tstep)
	{
		for (d = t - dstep, i = 0; i <= SAMPLE_SIZE; d += dstep, i++)
			sp_spiral_get_xy (spiral, d, &darray[i].x, &darray[i].y);
		
		sp_darray_center_tangent (darray, SAMPLE_SIZE - 1, &hat2);

		/* Fixme:
		   we shuld specify a maximum error using better algorithm.
		*/
		if (sp_bezier_fit_cubic_full (bezier, darray, 0, SAMPLE_SIZE - 1,
					      &hat1, &hat2,
					      SPIRAL_TOLERANCE*SPIRAL_TOLERANCE,
					      1) != -1) {
			sp_curve_curveto (c, bezier[1].x, bezier[1].y,
					  bezier[2].x, bezier[2].y,
					  bezier[3].x, bezier[3].y);
		} else {
#ifdef SPIRAL_VERBOSE
			g_print ("cant_fit_cubic: t=%g\n", t);
#endif
			for (i = 1; i<SAMPLE_SIZE; i++)
				sp_curve_lineto (c, darray[i].x, darray[i].y);
		}

		hat1.x = - hat2.x;
		hat1.y = - hat2.y;
	}
  
	sp_path_add_bpath (SP_PATH (spiral), c, TRUE, NULL);
	sp_curve_unref (c);
}

static void
sp_spiral_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}

void
sp_spiral_set       (SPSpiral          *spiral,
		     gdouble            cx,
		     gdouble            cy,
		     SPSpiralHand       hand,
		     gdouble            expansion,
		     gdouble            revolution,
		     gdouble            radius,
		     gdouble            argument,
		     gdouble            t0)
{
	g_return_if_fail (spiral != NULL);
	g_return_if_fail (SP_IS_SPIRAL (spiral));
	
	spiral->cx         = cx;
	spiral->cy         = cy;
	spiral->hand       = hand;
	spiral->expansion  = expansion;
	spiral->revolution = revolution;
	spiral->radius     = radius;
	spiral->argument   = argument;
	spiral->t0         = t0;
	
	sp_spiral_set_shape (spiral);
}

static GSList * 
sp_spiral_snappoints (SPItem * item, GSList * points)
{
	SPSpiral *spiral;
	ArtPoint * p, p1, p2, p3;
	gdouble affine[6];
	
	spiral = SP_SPIRAL(item);
	
	sp_spiral_get_xy (spiral, 0.0, &p1.x, &p1.y);
	sp_spiral_get_xy (spiral, spiral->t0, &p2.x, &p2.y);
	sp_spiral_get_xy (spiral, 1.0, &p3.x, &p3.y);
	
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

static SPSpiralHand
sp_spiral_read_type (const gchar *str)
{
	g_return_val_if_fail (str != NULL, SP_SPIRAL_HAND_RIGHT);
	
	if (strcmp (str, "left") == 0)
		return SP_SPIRAL_HAND_LEFT;
	
	return SP_SPIRAL_HAND_RIGHT;
}

void
sp_spiral_get_xy (SPSpiral *spiral,
		  gdouble   t,
		  gdouble  *x,
		  gdouble  *y)
{
	gdouble rad = spiral->radius * pow(t, spiral->expansion);
	gdouble arg = 2.0 * M_PI * spiral->revolution * t + spiral->argument;
	
	*x = spiral->cx + rad * cos (arg);
	*y = spiral->cy - rad * sin (arg)
		* (spiral->hand == SP_SPIRAL_HAND_LEFT ? -1.0 : 1.0);
}

void
sp_spiral_to_repr (SPSpiral *spiral,
		   SPRepr   *repr)
{
	g_return_if_fail (spiral != NULL);
	
	sp_repr_set_double_attribute (repr, "sodipodi:cx", spiral->cx);
	sp_repr_set_double_attribute (repr, "sodipodi:cy", spiral->cy);
	sp_repr_set_attr (repr, "sodipodi:hand",
			  (spiral->hand == SP_SPIRAL_HAND_LEFT ? "left" : "right"));
	sp_repr_set_double_attribute (repr, "sodipodi:expansion", spiral->expansion);
	sp_repr_set_double_attribute (repr, "sodipodi:revolution", spiral->revolution);
	sp_repr_set_double_attribute (repr, "sodipodi:radius", spiral->radius);
	sp_repr_set_double_attribute (repr, "sodipodi:argument", spiral->argument);
	sp_repr_set_double_attribute (repr, "sodipodi:t0", spiral->t0);
}
