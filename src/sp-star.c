#define SP_STAR_C

#include <gnome.h>
#include <math.h>
#include "svg/svg.h"
#include "sp-star.h"

#define noSTAR_VERBOSE

static void sp_star_class_init (SPStarClass *class);
static void sp_star_init (SPStar *star);
static void sp_star_destroy (GtkObject *object);

static void sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_star_read_attr (SPObject * object, const gchar * attr);

static void sp_star_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_star_description (SPItem * item);
static GSList * sp_star_snappoints (SPItem * item, GSList * points);

/*static void sp_star_set_shape (SPStar * star);*/

static SPShapeClass *parent_class;

GtkType
sp_star_get_type (void)
{
	static GtkType star_type = 0;

	if (!star_type) {
		GtkTypeInfo star_info = {
			"SPStar",
			sizeof (SPStar),
			sizeof (SPStarClass),
			(GtkClassInitFunc) sp_star_class_init,
			(GtkObjectInitFunc) sp_star_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		star_type = gtk_type_unique (sp_shape_get_type (), &star_info);
	}
	return star_type;
}

static void
sp_star_class_init (SPStarClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_star_destroy;

	sp_object_class->build = sp_star_build;
	sp_object_class->read_attr = sp_star_read_attr;

	item_class->bbox = sp_star_bbox;
	item_class->description = sp_star_description;
	item_class->snappoints = sp_star_snappoints;
}

static void
sp_star_init (SPStar * star)
{
  SP_PATH (star)->independent = FALSE;
  star->sides = 3;
  star->cx    = star-> cy = 0.0;
  star->r1    = star->r2 = 0.0;
  star->arg1  = star->arg2 = 0.0;
}

static void
sp_star_destroy (GtkObject *object)
{
	SPStar *star;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_STAR (object));

	star = SP_STAR (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);
	sp_star_read_attr (object, "sodipodi:sides");
	sp_star_read_attr (object, "sodipodi:cx");
	sp_star_read_attr (object, "sodipodi:cy");
	sp_star_read_attr (object, "sodipodi:r1");
	sp_star_read_attr (object, "sodipodi:r2");
	sp_star_read_attr (object, "sodipodi:arg1");
	sp_star_read_attr (object, "sodipodi:arg2");
}

static void
sp_star_read_attr (SPObject * object, const gchar * attr)
{
	SPStar * star;
	const gchar * astr;
	SPSVGUnit unit;
	double n;

	star = SP_STAR (object);

#ifdef STAR_VERBOSE
	g_print ("sp_star_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "sodipodi:sides") == 0) {
	  if (astr)
	    star->sides = (gint)strtol (astr, NULL, 10);
	  else
	    star->sides = 3;
	  sp_star_set_shape (star);
	  return;
	}
	if (strcmp (attr, "sodipodi:cx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->cx = n;
		sp_star_set_shape (star);
		return;
	}
	if (strcmp (attr, "sodipodi:cy") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->cy = n;
		sp_star_set_shape (star);
		return;
	}
	if (strcmp (attr, "sodipodi:r1") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->r1 = n;
		sp_star_set_shape (star);
		return;
	}
	if (strcmp (attr, "sodipodi:r2") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->r2 = n;
		sp_star_set_shape (star);
		return;
	}
	if (strcmp (attr, "sodipodi:arg1") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->arg1 = n;
		sp_star_set_shape (star);
		return;
	}
	if (strcmp (attr, "sodipodi:arg2") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->arg2 = n;
		sp_star_set_shape (star);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_star_description (SPItem * item)
{
	return g_strdup ("Star");
}

/*static*/ 
void
sp_star_set_shape (SPStar * star)
{
  gint   i;
  gint   sides;
  gdouble cx, cy, r1, r2, arg1, arg2, darg;
  SPCurve * c;
	
  sp_path_clear (SP_PATH (star));

  if ((star->r1 < 1e-12) || (star->r2 < 1e-12)) return;

  c = sp_curve_new ();
  
  sides = star->sides;
  g_assert (sides > 2);
  cx = star->cx;
  cy = star->cy;
  r1 = star->r1;
  r2 = star->r2;
  arg1 = star->arg1;
  arg2 = star->arg2;
  darg = 2.0*M_PI/(double)sides;
  
  /* i = 0 */
  sp_curve_moveto (c, cx + r1*cos(arg1), cy + r1*sin(arg1));
  sp_curve_lineto (c, cx + r2*cos(arg2), cy + r2*sin(arg2));
  for (i = 1; i<sides; i++) {
    arg1 += darg;
    arg2 += darg;
    sp_curve_lineto (c, cx + r1*cos(arg1), cy + r1*sin(arg1));
    sp_curve_lineto (c, cx + r2*cos(arg2), cy + r2*sin(arg2));
  }
  
  sp_curve_closepath (c);
  sp_path_add_bpath (SP_PATH (star), c, TRUE, NULL);
  sp_curve_unref (c);
}

static void
sp_star_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}

void
sp_star_set (SPStar * star,
	     gint sides,
	     gdouble cx, gdouble cy,
	     gdouble r1, gdouble r2,
	     gdouble arg1, gdouble arg2)
{
  g_return_if_fail (star != NULL);
  g_return_if_fail (SP_IS_STAR (star));

  star->sides = sides;
  star->cx = cx;
  star->cy = cy;
  star->r1 = r1;
  star->r2 = r2;
  star->arg1 = arg1;
  star->arg2 = arg2;
  
  sp_star_set_shape (star);
}

static GSList * 
sp_star_snappoints (SPItem * item, GSList * points)
{
  SPStar *star;
  ArtPoint * p, p1, p2;
  gdouble affine[6];

  star = SP_STAR(item);
  
  /* we use two points of star */
  p1.x = star->r1 * cos(star->arg1);
  p1.y = star->r1 * sin(star->arg1);
  p2.x = star->r2 * cos(star->arg2);
  p2.y = star->r2 * sin(star->arg2);

  sp_item_i2d_affine (item, affine);

  p = g_new (ArtPoint,1);
  art_affine_point (p, &p1, affine);
  points = g_slist_append (points, p);
  p = g_new (ArtPoint,1);
  art_affine_point (p, &p2, affine);
  points = g_slist_append (points, p);

  return points;
}

void
sp_star_to_repr  (SPStar *star,
                  SPRepr *repr)
{
  g_return_if_fail (star != NULL);

  sp_repr_set_int_attribute (repr, "sodipodi:sides", star->sides);
  sp_repr_set_double_attribute (repr, "sodipodi:cx", star->cx);
  sp_repr_set_double_attribute (repr, "sodipodi:cy", star->cy);
  sp_repr_set_double_attribute (repr, "sodipodi:r1", star->r1);
  sp_repr_set_double_attribute (repr, "sodipodi:r2", star->r2);
  sp_repr_set_double_attribute (repr, "sodipodi:arg1", star->arg1);
  sp_repr_set_double_attribute (repr, "sodipodi:arg2", star->arg2);
}
