#define __SP_ELLIPSE_C__

/*
 * SPGenericEllipse, SPEllipse, SPCircle
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Ximian, Inc. and author
 *
 * You can distribute and modify that code under terms
 * of GNU GPL. See file COPYING for more information.
 *
 */

#include <math.h>
#include <gnome.h>
#include "svg/svg.h"
#include "sp-ellipse.h"

/* Common parent class */

#define noELLIPSE_VERBOSE

#define SP_2PI 2 * M_PI

static void sp_genericellipse_class_init (SPGenericEllipseClass *klass);
static void sp_genericellipse_init (SPGenericEllipse *ellipse);
static void sp_genericellipse_destroy (GtkObject *object);

static GSList *sp_genericellipse_snappoints (SPItem *item, GSList *points);

static void sp_genericellipse_set_shape (SPGenericEllipse *ellipse);

static SPShapeClass *ge_parent_class;

GtkType
sp_genericellipse_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPGenericEllipse",
			sizeof (SPGenericEllipse),
			sizeof (SPGenericEllipseClass),
			(GtkClassInitFunc) sp_genericellipse_class_init,
			(GtkObjectInitFunc) sp_genericellipse_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (sp_shape_get_type (), &info);
	}
	return type;
}

static void
sp_genericellipse_class_init (SPGenericEllipseClass *klass)
{
	GtkObjectClass *gtk_object_class;
	SPItemClass *item_class;

	gtk_object_class = (GtkObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	ge_parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_genericellipse_destroy;

	item_class->snappoints = sp_genericellipse_snappoints;
}

static void
sp_genericellipse_init (SPGenericEllipse *ellipse)
{
	ellipse->shape.path.independent = FALSE;
	ellipse->x = ellipse->y = 0.0;
	ellipse->rx = ellipse->ry = 0.0;
	ellipse->start = 0.0;
	ellipse->end = SP_2PI;
	ellipse->closed = TRUE;
}

static void
sp_genericellipse_destroy (GtkObject *object)
{
	if (GTK_OBJECT_CLASS (ge_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (ge_parent_class)->destroy) (object);
}

#define C1 0.552

static void sp_genericellipse_set_shape (SPGenericEllipse *ellipse)
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

static GSList * 
sp_genericellipse_snappoints (SPItem *item, GSList *points)
{
	ArtPoint * p;
	gdouble affine[6];

	/* we use corners of item and center of ellipse */
	if (SP_ITEM_CLASS (ge_parent_class)->snappoints)
		points = (SP_ITEM_CLASS (ge_parent_class)->snappoints) (item, points);

	p = g_new (ArtPoint,1);
	p->x = SP_GENERICELLIPSE (item)->x;
	p->y = SP_GENERICELLIPSE (item)->y;
	sp_item_i2d_affine (item, affine);
	art_affine_point (p, p, affine);
	points = g_slist_append (points, p);
	return points;
}

/* SVG <ellipse> element */

static void sp_ellipse_class_init (SPEllipseClass *class);
static void sp_ellipse_init (SPEllipse *ellipse);
static void sp_ellipse_destroy (GtkObject *object);

static void sp_ellipse_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_ellipse_read_attr (SPObject * object, const gchar * attr);
static gchar * sp_ellipse_description (SPItem * item);

static SPGenericEllipseClass *ellipse_parent_class;

GtkType
sp_ellipse_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPEllipse",
			sizeof (SPEllipse),
			sizeof (SPEllipseClass),
			(GtkClassInitFunc) sp_ellipse_class_init,
			(GtkObjectInitFunc) sp_ellipse_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_GENERICELLIPSE, &info);
	}
	return type;
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

	ellipse_parent_class = gtk_type_class (SP_TYPE_GENERICELLIPSE);

	gtk_object_class->destroy = sp_ellipse_destroy;

	sp_object_class->build = sp_ellipse_build;
	sp_object_class->read_attr = sp_ellipse_read_attr;

	item_class->description = sp_ellipse_description;
}

static void
sp_ellipse_init (SPEllipse *ellipse)
{
}

static void
sp_ellipse_destroy (GtkObject *object)
{
	SPEllipse *ellipse;

	ellipse = SP_ELLIPSE (object);

	if (GTK_OBJECT_CLASS (ellipse_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (ellipse_parent_class)->destroy) (object);
}

static void
sp_ellipse_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (SP_OBJECT_CLASS (ellipse_parent_class)->build)
		(* SP_OBJECT_CLASS (ellipse_parent_class)->build) (object, document, repr);

	sp_ellipse_read_attr (object, "cx");
	sp_ellipse_read_attr (object, "cy");
	sp_ellipse_read_attr (object, "rx");
	sp_ellipse_read_attr (object, "ry");
	sp_ellipse_read_attr (object, "sodipodi:start");
	sp_ellipse_read_attr (object, "sodipodi:end");
	sp_ellipse_read_attr (object, "sodipodi:open");
}

static void
sp_ellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *astr;
	SPSVGUnit unit;
	double n;

	ellipse = SP_GENERICELLIPSE (object);

#ifdef ELLIPSE_VERBOSE
	g_print ("sp_ellipse_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	if (strcmp (attr, "cx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		ellipse->x = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "cy") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		ellipse->y = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "rx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		ellipse->rx = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "ry") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		ellipse->ry = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "sodipodi:start") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->start);
		ellipse->start = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "sodipodi:end") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->end);
		ellipse->end = n;
		sp_genericellipse_set_shape (ellipse);
		return;
	}
	if (strcmp (attr, "sodipodi:open") == 0) {
		if (sp_repr_attr_is_set (object->repr, attr))
			ellipse->closed = FALSE;
		else
			ellipse->closed = TRUE;
		sp_genericellipse_set_shape (ellipse);
		return;
	}

	if (SP_OBJECT_CLASS(ellipse_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (ellipse_parent_class)->read_attr) (object, attr);
}

static gchar *
sp_ellipse_description (SPItem * item)
{
	return g_strdup ("Ellipse");
}


void
sp_ellipse_set (SPEllipse *ellipse, gdouble x, gdouble y, gdouble rx, gdouble ry)
{
	SPGenericEllipse *ge;

	g_return_if_fail (ellipse != NULL);
	g_return_if_fail (SP_IS_ELLIPSE (ellipse));

	ge = SP_GENERICELLIPSE (ellipse);

	ge->x = x;
	ge->y = y;
	ge->rx = rx;
	ge->ry = ry;

	sp_genericellipse_set_shape (ge);
}

/* SVG <circle> element */

static void sp_circle_class_init (SPCircleClass *class);
static void sp_circle_init (SPCircle *circle);
static void sp_circle_destroy (GtkObject *object);

static void sp_circle_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_circle_read_attr (SPObject * object, const gchar * attr);
static gchar * sp_circle_description (SPItem * item);

static SPGenericEllipseClass *circle_parent_class;

GtkType
sp_circle_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPCircle",
			sizeof (SPCircle),
			sizeof (SPCircleClass),
			(GtkClassInitFunc) sp_circle_class_init,
			(GtkObjectInitFunc) sp_circle_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_GENERICELLIPSE, &info);
	}
	return type;
}

static void
sp_circle_class_init (SPCircleClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	circle_parent_class = gtk_type_class (SP_TYPE_GENERICELLIPSE);

	gtk_object_class->destroy = sp_circle_destroy;

	sp_object_class->build = sp_circle_build;
	sp_object_class->read_attr = sp_circle_read_attr;

	item_class->description = sp_circle_description;
}

static void
sp_circle_init (SPCircle *circle)
{
}

static void
sp_circle_destroy (GtkObject *object)
{
	SPCircle *circle;

	circle = SP_CIRCLE (object);

	if (GTK_OBJECT_CLASS (circle_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (circle_parent_class)->destroy) (object);
}

static void
sp_circle_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (SP_OBJECT_CLASS (circle_parent_class)->build)
		(* SP_OBJECT_CLASS (circle_parent_class)->build) (object, document, repr);

	sp_circle_read_attr (object, "cx");
	sp_circle_read_attr (object, "cy");
	sp_circle_read_attr (object, "r");
	sp_circle_read_attr (object, "sodipodi:start");
	sp_circle_read_attr (object, "sodipodi:end");
	sp_circle_read_attr (object, "sodipodi:open");
}

static void
sp_circle_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *circle;
	const gchar *astr;
	SPSVGUnit unit;
	double n;

	circle = SP_GENERICELLIPSE (object);

#ifdef CIRCLE_VERBOSE
	g_print ("sp_circle_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	if (strcmp (attr, "cx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		circle->x = n;
		sp_genericellipse_set_shape (circle);
		return;
	}
	if (strcmp (attr, "cy") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		circle->y = n;
		sp_genericellipse_set_shape (circle);
		return;
	}
	if (strcmp (attr, "r") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		circle->rx = n;
		circle->ry = n;
		sp_genericellipse_set_shape (circle);
		return;
	}
	if (strcmp (attr, "sodipodi:start") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, circle->start);
		circle->start = n;
		sp_genericellipse_set_shape (circle);
		return;
	}
	if (strcmp (attr, "sodipodi:end") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, circle->end);
		circle->end = n;
		sp_genericellipse_set_shape (circle);
		return;
	}
	if (strcmp (attr, "sodipodi:open") == 0) {
		if (sp_repr_attr_is_set (object->repr, attr))
			circle->closed = FALSE;
		else
			circle->closed = TRUE;
		sp_genericellipse_set_shape (circle);
		return;
	}

	if (SP_OBJECT_CLASS(circle_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (circle_parent_class)->read_attr) (object, attr);
}

static gchar *
sp_circle_description (SPItem * item)
{
	return g_strdup ("Circle");
}

