#define SP_RECT_C

#include <gnome.h>
#include "svg/svg.h"
#include "sp-rect.h"

#define noRECT_VERBOSE

enum {ARG_0, ARG_X, ARG_Y, ARG_WIDTH, ARG_HEIGHT, ARG_RX, ARG_RY};

static void sp_rect_class_init (SPRectClass *class);
static void sp_rect_init (SPRect *rect);
static void sp_rect_destroy (GtkObject *object);
static void sp_rect_set_arg (GtkObject * object, GtkArg * arg, guint arg_id);

static void sp_rect_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_rect_read_attr (SPObject * object, const gchar * attr);

static void sp_rect_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_rect_description (SPItem * item);
static GSList * sp_rect_snappoints (SPItem * item, GSList * points);

static void sp_rect_set_shape (SPRect * rect);

static SPShapeClass *parent_class;

GtkType
sp_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		GtkTypeInfo rect_info = {
			"SPRect",
			sizeof (SPRect),
			sizeof (SPRectClass),
			(GtkClassInitFunc) sp_rect_class_init,
			(GtkObjectInitFunc) sp_rect_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		rect_type = gtk_type_unique (sp_shape_get_type (), &rect_info);
	}
	return rect_type;
}

static void
sp_rect_class_init (SPRectClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_add_arg_type ("SPRect::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X);
	gtk_object_add_arg_type ("SPRect::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y);
	gtk_object_add_arg_type ("SPRect::width", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH);
	gtk_object_add_arg_type ("SPRect::height", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_HEIGHT);
	gtk_object_add_arg_type ("SPRect::rx", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_RX);
	gtk_object_add_arg_type ("SPRect::ry", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_RY);

	gtk_object_class->destroy = sp_rect_destroy;
	gtk_object_class->set_arg = sp_rect_set_arg;

	sp_object_class->build = sp_rect_build;
	sp_object_class->read_attr = sp_rect_read_attr;

	item_class->bbox = sp_rect_bbox;
	item_class->description = sp_rect_description;
	item_class->snappoints = sp_rect_snappoints;
}

static void
sp_rect_init (SPRect * rect)
{
	SP_PATH (rect) -> independent = FALSE;
	rect->x = rect->y = 0.0;
	rect->width = rect->height = 0.0;
	rect->rx = rect->ry = 0.0;
}

static void
sp_rect_destroy (GtkObject *object)
{
	SPRect *rect;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_RECT (object));

	rect = SP_RECT (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_rect_set_arg (GtkObject * object, GtkArg * arg, guint arg_id)
{
	SPRect * rect;

	rect = SP_RECT (object);

	switch (arg_id) {
	case ARG_X:
		rect->x = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	case ARG_Y:
		rect->y = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	case ARG_WIDTH:
		rect->width = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	case ARG_HEIGHT:
		rect->height = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	case ARG_RX:
		rect->rx = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	case ARG_RY:
		rect->ry = GTK_VALUE_DOUBLE (*arg);
		sp_rect_set_shape (rect);
		break;
	}
}

static void
sp_rect_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_rect_read_attr (object, "x");
	sp_rect_read_attr (object, "y");
	sp_rect_read_attr (object, "width");
	sp_rect_read_attr (object, "height");
	sp_rect_read_attr (object, "rx");
	sp_rect_read_attr (object, "ry");
}

static void
sp_rect_read_attr (SPObject * object, const gchar * attr)
{
	SPRect * rect;
	const gchar * astr;
	SPSVGUnit unit;
	double n;

	rect = SP_RECT (object);

#ifdef RECT_VERBOSE
	g_print ("sp_rect_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "x") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->x = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->y = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "width") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->width = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "height") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->height = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "rx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->rx = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "ry") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->ry = n;
		sp_rect_set_shape (rect);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_rect_description (SPItem * item)
{
	return g_strdup ("Rectangle");
}

#define C1 0.554

static void
sp_rect_set_shape (SPRect * rect)
{
	double x, y, rx, ry;
	SPCurve * c;
	
	sp_path_clear (SP_PATH (rect));

	if ((rect->height < 1e-12) || (rect->width < 1e-12)) return;

	c = sp_curve_new ();

	x = rect->x;
	y = rect->y;

	if ((rect->rx != 0.0) && (rect->ry != 0.0)) {
		rx = rect->rx;
		ry = rect->ry;
		if (rx > rect->width / 2) rx = rect->width / 2;
		if (ry > rect->height / 2) ry = rect->height / 2;

		sp_curve_moveto (c, x + rx, y + 0.0);
		sp_curve_curveto (c, x + rx * (1 - C1), y + 0.0, x + 0.0, y + ry * (1 - C1), x + 0.0, y + ry);
		if (ry < rect->height / 2)
			sp_curve_lineto (c, x + 0.0, y + rect->height - ry);
		sp_curve_curveto (c, x + 0.0, y + rect->height - ry * (1 - C1), x + rx * (1 - C1), y + rect->height, x + rx, y + rect->height);
		if (rx < rect->width / 2)
			sp_curve_lineto (c, x + rect->width - rx, y + rect->height);
		sp_curve_curveto (c, x + rect->width - rx * (1 - C1), y + rect->height, x + rect->width, y + rect->height - ry * (1 - C1), x + rect->width, y + rect->height - ry);
		if (ry < rect->height / 2)
			sp_curve_lineto (c, x + rect->width, y + ry);
		sp_curve_curveto (c, x + rect->width, y + ry * (1 - C1), x + rect->width - rx * (1 - C1), y + 0.0, x + rect->width - rx, y + 0.0);
		if (rx < rect->width / 2)
			sp_curve_lineto (c, x + rx, y + 0.0);
	} else {
		sp_curve_moveto (c, x + 0.0, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + rect->height);
		sp_curve_lineto (c, x + rect->width, y + rect->height);
		sp_curve_lineto (c, x + rect->width, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + 0.0);
	}

	sp_curve_closepath_current (c);
	sp_path_add_bpath (SP_PATH (rect), c, TRUE, NULL);
	sp_curve_unref (c);
}

static void
sp_rect_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}

void
sp_rect_set (SPRect * rect, gdouble x, gdouble y, gdouble width, gdouble height)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->x = x;
	rect->y = y;
	rect->width = width;
	rect->height = height;

	sp_rect_set_shape (rect);
}

static GSList * 
sp_rect_snappoints (SPItem * item, GSList * points)
{
  ArtPoint * p, p1, p2, p3, p4;
  gdouble affine[6];

  /* we use corners of rect only */
  p1.x = SP_RECT(item)->x;
  p1.y = SP_RECT(item)->y;
  p2.x = p1.x + SP_RECT(item)->width;
  p2.y = p1.y;
  p3.x = p1.x;
  p3.y = p1.y + SP_RECT(item)->height;
  p4.x = p1.x + SP_RECT(item)->width;
  p4.y = p1.y + SP_RECT(item)->height;
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
  p = g_new (ArtPoint,1);
  art_affine_point (p, &p4, affine);
  points = g_slist_append (points, p);

  return points;
}
