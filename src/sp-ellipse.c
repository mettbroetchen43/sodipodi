#define SP_ELLIPSE_C

#include <math.h>
#include <gnome.h>
#include "sp-ellipse.h"

#define noELLIPSE_VERBOSE

#define SP_2PI 2 * M_PI

static void sp_ellipse_class_init (SPEllipseClass *class);
static void sp_ellipse_init (SPEllipse *ellipse);
static void sp_ellipse_destroy (GtkObject *object);

static void sp_ellipse_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_ellipse_read_attr (SPObject * object, const gchar * attr);

static void sp_ellipse_set_shape (SPEllipse * ellipse);

static SPShapeClass *parent_class;

GtkType
sp_ellipse_get_type (void)
{
	static GtkType ellipse_type = 0;

	if (!ellipse_type) {
		GtkTypeInfo ellipse_info = {
			"SPEllipse",
			sizeof (SPEllipse),
			sizeof (SPEllipseClass),
			(GtkClassInitFunc) sp_ellipse_class_init,
			(GtkObjectInitFunc) sp_ellipse_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		ellipse_type = gtk_type_unique (sp_shape_get_type (), &ellipse_info);
	}
	return ellipse_type;
}

static void
sp_ellipse_class_init (SPEllipseClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_ellipse_destroy;

	sp_object_class->build = sp_ellipse_build;
	sp_object_class->read_attr = sp_ellipse_read_attr;
}

static void
sp_ellipse_init (SPEllipse *ellipse)
{
	ellipse->shape.path.independent = FALSE;
	ellipse->x = ellipse->y = 0.0;
	ellipse->rx = ellipse->ry = 0.0;
	ellipse->start = 0.0;
	ellipse->end = SP_2PI;
	ellipse->closed = TRUE;
}

static void
sp_ellipse_destroy (GtkObject *object)
{
	SPEllipse *ellipse;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ELLIPSE (object));

	ellipse = SP_ELLIPSE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ellipse_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS (parent_class)->build) (object, document, repr);

	sp_ellipse_read_attr (object, "x");
	sp_ellipse_read_attr (object, "y");
	sp_ellipse_read_attr (object, "rx");
	sp_ellipse_read_attr (object, "ry");
	sp_ellipse_read_attr (object, "start");
	sp_ellipse_read_attr (object, "end");
	sp_ellipse_read_attr (object, "closed");
}

static void
sp_ellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPEllipse * ellipse;
	double n;

	ellipse = SP_ELLIPSE (object);

#ifdef ELLIPSE_VERBOSE
g_print ("sp_ellipse_read_attr: attr %s\n", attr);
#endif

	if (strcmp (attr, "x") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->x);
		ellipse->x = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->y);
		ellipse->y = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "rx") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->rx);
		ellipse->rx = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "ry") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->ry);
		ellipse->ry = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "start") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->start);
		ellipse->start = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "end") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->end);
		ellipse->end = n;
		sp_ellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "closed") == 0) {
		if (sp_repr_attr_is_set (object->repr, attr))
			ellipse->closed = TRUE;
		else
			ellipse->closed = FALSE;
		sp_ellipse_set_shape (ellipse);
		return;
	}

	if (SP_OBJECT_CLASS(parent_class)->read_attr)
		(* SP_OBJECT_CLASS (parent_class)->read_attr) (object, attr);
}

#define C1 0.552

static void sp_ellipse_set_shape (SPEllipse * ellipse)
{
	SPPath * path;
	SPCurve * c;
	ArtBpath bpath[16], * abp;

	double cx, cy, rx, ry, s, e;
	double x0, y0, x1, y1, x2, y2, x3, y3;
	double len;
	double affine[6];
	gint slice = FALSE;
	gint i;

	path = SP_PATH (ellipse);

	if (ellipse->start > ellipse->end) {
		e = ellipse->end;
		ellipse->end = ellipse->start;
		ellipse->start = e;
	}

	cx = 0.0;
	cy = 0.0;
	rx = ellipse->rx;
	ry = ellipse->ry;
	if (rx < 1e-8) rx = 1e-8;
	if (ry < 1e-8) ry = 1e-8;

	len = fmod (ellipse->end - ellipse->start, SP_2PI);
	if (len < 0.0) len += SP_2PI;
	if (fabs (len) < 0.01) {
		slice = FALSE;
	} else {
		slice = TRUE;
	}

	art_affine_scale (affine, rx, ry);
	affine[4] = ellipse->x;
	affine[5] = ellipse->y;

	i = 0;
	if (ellipse->closed)
		bpath[i].code = ART_MOVETO;
	else
		bpath[i].code = ART_MOVETO_OPEN;
	bpath[i].x3 = cos (ellipse->start);
	bpath[i].y3 = sin (ellipse->start);
	i++;

	for (s = ellipse->start; s < ellipse->end; s += M_PI_2) {
		e = s + M_PI_2;
		if (e > ellipse->end)
			e = ellipse->end;
		len = C1 * (e - s) / M_PI_2;
		x0 = cos (s);
		y0 = sin (s);
		x1 = x0 + len * cos (s + M_PI_2);
		y1 = y0 + len * sin (s + M_PI_2);
		x3 = cos (e);
		y3 = sin (e);
		x2 = x3 + len * cos (e - M_PI_2);
		y2 = y3 + len * sin (e - M_PI_2);
#ifdef ELLIPSE_VERBOSE
g_print ("step %d s %f e %f coords %f %f %f %f %f %f\n",
	i, s, e, x1, y1, x2, y2, x3, y3);
#endif
		bpath[i].code = ART_CURVETO;
		bpath[i].x1 = x1;
		bpath[i].y1 = y1;
		bpath[i].x2 = x2;
		bpath[i].y2 = y2;
		bpath[i].x3 = x3;
		bpath[i].y3 = y3;
		i++;
	}

	if (slice && ellipse->closed) {
		bpath[i].code = ART_LINETO;
		bpath[i].x3 = 0.0;
		bpath[i].y3 = 0.0;
		i++;
		bpath[i].code = ART_LINETO;
		bpath[i].x3 = bpath[0].x3;
		bpath[i].y3 = bpath[0].y3;
		i++;
	} else if (ellipse->closed) {
		bpath[i-1].x3 = bpath[0].x3;
		bpath[i-1].y3 = bpath[0].y3;
	}

	bpath[i].code = ART_END;

	abp = art_bpath_affine_transform (bpath, affine);

	c = sp_curve_new_from_bpath (abp);
	sp_path_clear (SP_PATH (ellipse));
	sp_path_add_bpath (SP_PATH (ellipse), c, TRUE, NULL);
	sp_curve_unref (c);
}

#if 0
static void
sp_ellipse_bbox (SPItem * item, ArtDRect * bbox)
{
	SPEllipse * ellipse;
	double affine[6];
	ArtPoint c, p;
	double cx, cy, rx, ry, t;

	ellipse = SP_ELLIPSE (item);

	gnome_canvas_item_i2w_affine (GNOME_CANVAS_ITEM (item), affine);
	
	cx = ellipse->x;
	cy = ellipse->y;
	rx = ellipse->rx;
	ry = ellipse->ry;

	c.x = cx;
	c.y = cy;
	art_affine_point (&c, &c, affine);

	bbox->x0 = bbox->x1 = c.x;
	bbox->y0 = bbox->y1 = c.y;

	for (t = 0.0; t < SP_2PI; t+= (M_PI_2 / 10)) {
		p.x = cx + rx * cos (t);
		p.y = cy + ry * sin (t);
		art_affine_point (&p, &p, affine);
		if (p.x < bbox->x0) bbox->x0 = p.x;
		if (p.y < bbox->y0) bbox->y0 = p.y;
		if (p.x > bbox->x1) bbox->x1 = p.x;
		if (p.y > bbox->y1) bbox->y1 = p.y;
	}

	return;
}
#endif
