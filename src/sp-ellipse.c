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

#define SP_2PI (2 * M_PI)

static void sp_genericellipse_class_init (SPGenericEllipseClass *klass);
static void sp_genericellipse_init (SPGenericEllipse *ellipse);
static void sp_genericellipse_destroy (GtkObject *object);
static void sp_genericellipse_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_genericellipse_write_repr (SPObject *object, SPRepr *repr);
static void sp_genericellipse_read_attr (SPObject * object, const gchar * attr);

static GSList *sp_genericellipse_snappoints (SPItem *item, GSList *points);

static void sp_genericellipse_glue_set_shape (SPShape *shape);
static void sp_genericellipse_set_shape (SPGenericEllipse *ellipse);
static void sp_genericellipse_normalize (SPGenericEllipse *ellipse);

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
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;
	SPShapeClass *shape_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	shape_class = (SPShapeClass *) klass;

	ge_parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_genericellipse_destroy;

	sp_object_class->build = sp_genericellipse_build;
	sp_object_class->write_repr = sp_genericellipse_write_repr;
	sp_object_class->read_attr = sp_genericellipse_read_attr;

	item_class->snappoints = sp_genericellipse_snappoints;

	shape_class->set_shape = sp_genericellipse_glue_set_shape;
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

static void
sp_genericellipse_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (SP_OBJECT_CLASS (ge_parent_class)->build)
		(* SP_OBJECT_CLASS (ge_parent_class)->build) (object, document, repr);

	sp_genericellipse_read_attr (object, "cx");
	sp_genericellipse_read_attr (object, "cy");
}

static void
sp_genericellipse_write_repr (SPObject *object, SPRepr *repr)
{
	SPGenericEllipse *ellipse;

	ellipse = SP_GENERICELLIPSE (object);

	sp_repr_set_double_attribute (repr, "cx", ellipse->x);
	sp_repr_set_double_attribute (repr, "cy", ellipse->y);
	
	if (SP_OBJECT_CLASS (ge_parent_class)->write_repr)
		(* SP_OBJECT_CLASS (ge_parent_class)->write_repr) (object, repr);
}

static void
sp_genericellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *astr;
	const SPUnit *unit;
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

	if (SP_OBJECT_CLASS(ge_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (ge_parent_class)->read_attr) (object, attr);
}

static void sp_genericellipse_glue_set_shape (SPShape *shape)
{
	SPGenericEllipse *ge;

	ge = SP_GENERICELLIPSE (shape);

	sp_genericellipse_set_shape (ge);
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

/*
	if (ellipse->start > ellipse->end) {
		e = ellipse->end;
		ellipse->end = ellipse->start;
		ellipse->start = e;
	}
*/
	sp_genericellipse_normalize (ellipse);

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

static void
sp_genericellipse_normalize (SPGenericEllipse *ellipse)
{
	ellipse->start = fmod(ellipse->start, SP_2PI);
	ellipse->end = fmod(ellipse->end, SP_2PI) + SP_2PI;
}

/* SVG <ellipse> element */

static void sp_ellipse_class_init (SPEllipseClass *class);
static void sp_ellipse_init (SPEllipse *ellipse);
static void sp_ellipse_destroy (GtkObject *object);

static void sp_ellipse_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_ellipse_write_repr (SPObject *object, SPRepr *repr);
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
	sp_object_class->write_repr = sp_ellipse_write_repr;
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

	sp_ellipse_read_attr (object, "rx");
	sp_ellipse_read_attr (object, "ry");
}

static void
sp_ellipse_write_repr (SPObject *object, SPRepr *repr)
{
	SPGenericEllipse *ellipse;

	ellipse = SP_GENERICELLIPSE (object);

	sp_repr_set_double_attribute (repr, "rx", ellipse->rx);
	sp_repr_set_double_attribute (repr, "ry", ellipse->ry);
	
	if (SP_OBJECT_CLASS (ellipse_parent_class)->write_repr)
		(* SP_OBJECT_CLASS (ellipse_parent_class)->write_repr) (object, repr);
/*
	sp_ellipse_read_attr (object, "sodipodi:start");
	sp_ellipse_read_attr (object, "sodipodi:end");
	sp_ellipse_read_attr (object, "sodipodi:open");
*/
}

static void
sp_ellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *astr;
	const SPUnit *unit;
	double n;

	ellipse = SP_GENERICELLIPSE (object);

#ifdef ELLIPSE_VERBOSE
	g_print ("sp_ellipse_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

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
static void sp_circle_write_repr (SPObject *object, SPRepr *repr);
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
	sp_object_class->write_repr = sp_circle_write_repr;
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

	sp_circle_read_attr (object, "r");
}

static void
sp_circle_write_repr (SPObject *object, SPRepr *repr)
{
	SPGenericEllipse *ellipse;

	ellipse = SP_GENERICELLIPSE (object);

	g_assert (ellipse->rx == ellipse->ry);
	sp_repr_set_double_attribute (repr, "r", ellipse->rx);
	
	if (SP_OBJECT_CLASS (circle_parent_class)->write_repr)
		(* SP_OBJECT_CLASS (circle_parent_class)->write_repr) (object, repr);
}

static void
sp_circle_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *circle;
	const gchar *astr;
	const SPUnit *unit;
	double n;

	circle = SP_GENERICELLIPSE (object);

#ifdef CIRCLE_VERBOSE
	g_print ("sp_circle_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	if (strcmp (attr, "r") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		circle->rx = n;
		circle->ry = n;
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

/* <path sodipodi:type="arc"> element */

static void sp_arc_class_init (SPArcClass *class);
static void sp_arc_init (SPArc *arc);
static void sp_arc_destroy (GtkObject *object);

static void sp_arc_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_arc_write_repr (SPObject *object, SPRepr *repr);
static void sp_arc_read_attr (SPObject * object, const gchar * attr);
static gchar * sp_arc_description (SPItem * item);
static SPKnotHolder *sp_arc_knot_holder (SPItem * item, SPDesktop *desktop);

static SPGenericEllipseClass *arc_parent_class;

GtkType
sp_arc_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPArc",
			sizeof (SPArc),
			sizeof (SPArcClass),
			(GtkClassInitFunc) sp_arc_class_init,
			(GtkObjectInitFunc) sp_arc_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_GENERICELLIPSE, &info);
	}
	return type;
}

static void
sp_arc_class_init (SPArcClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	arc_parent_class = gtk_type_class (SP_TYPE_GENERICELLIPSE);

	gtk_object_class->destroy = sp_arc_destroy;

	sp_object_class->build = sp_arc_build;
	sp_object_class->write_repr = sp_arc_write_repr;
	sp_object_class->read_attr = sp_arc_read_attr;

	item_class->description = sp_arc_description;
	item_class->knot_holder = sp_arc_knot_holder;
}

static void
sp_arc_init (SPArc *arc)
{
}

static void
sp_arc_destroy (GtkObject *object)
{
	SPArc *arc;

	arc = SP_ARC (object);

	if (GTK_OBJECT_CLASS (arc_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (arc_parent_class)->destroy) (object);
}

static void
sp_arc_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (SP_OBJECT_CLASS (arc_parent_class)->build)
		(* SP_OBJECT_CLASS (arc_parent_class)->build) (object, document, repr);

	sp_arc_read_attr (object, "rx");
	sp_arc_read_attr (object, "ry");
	sp_arc_read_attr (object, "sodipodi:start");
	sp_arc_read_attr (object, "sodipodi:end");
	sp_arc_read_attr (object, "sodipodi:open");
}

/*
 * sp_arc_set_elliptical_path_attribute:
 *
 * Convert center to endpoint parameterization and set it to repr.
 *
 * See SVG 1.0 Specification W3C Recommendation
 * ``F.6 Ellptical arc implementation notes'' for more detail.
 */
static gboolean
sp_arc_set_elliptical_path_attribute (SPArc *arc, SPRepr *repr)
{
#define ARC_BUFSIZE 128
	SPGenericEllipse *ge;
	ArtPoint p1, p2;
	gdouble  dt;
	gint fa, fs;
	gchar c[ARC_BUFSIZE];

	ge = SP_GENERICELLIPSE (arc);

	sp_arc_get_xy (arc, ge->start, &p1);
	sp_arc_get_xy (arc, ge->end, &p2);

	dt = fmod(ge->end - ge->start, SP_2PI);
	fa = (fabs(dt) > M_PI) ? 1 : 0;
	fs = (dt > 0) ? 1 : 0;
#ifdef ARC_VERBOSE
	g_print ("start:%g end:%g fa=%d fs=%d\n", ge->start, ge->end, fa, fs);
#endif
	g_snprintf (c, ARC_BUFSIZE, "M %f,%f A %f,%f 0 %d %d %f,%f L %f,%f z",
		    p1.x, p1.y, ge->rx, ge->ry, fa, fs, p2.x, p2.y, ge->x, ge->y);

	return sp_repr_set_attr (repr, "d", c);
}

static void
sp_arc_write_repr (SPObject *object, SPRepr *repr)
{
	SPGenericEllipse *ellipse;
	SPArc *arc;

	ellipse = SP_GENERICELLIPSE (object);
	arc = SP_ARC (object);

	sp_repr_set_double_attribute (repr, "cx", ellipse->x);
	sp_repr_set_double_attribute (repr, "cy", ellipse->y);

	sp_repr_set_double_attribute (repr, "rx", ellipse->rx);
	sp_repr_set_double_attribute (repr, "ry", ellipse->ry);
	sp_repr_set_double_attribute (repr, "sodipodi:start", ellipse->start);
	sp_repr_set_double_attribute (repr, "sodipodi:end", ellipse->end);
	if (! ellipse->closed)
		sp_repr_set_attr (repr, "sodipodi:open", "true");

	sp_arc_set_elliptical_path_attribute (arc, repr);
	
/*  	if (SP_OBJECT_CLASS (arc_parent_class)->write_repr) */
/*  		(* SP_OBJECT_CLASS (arc_parent_class)->write_repr) (object, repr); */
}

static void
sp_arc_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *astr;
	const SPUnit *unit;
	double n;

	ellipse = SP_GENERICELLIPSE (object);
	
#ifdef ARC_VERBOSE
	g_print ("sp_arc_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

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

	if (SP_OBJECT_CLASS(arc_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (arc_parent_class)->read_attr) (object, attr);
}

static gchar *
sp_arc_description (SPItem * item)
{
	return g_strdup ("Arc");
}

void
sp_arc_set (SPArc *arc, gdouble x, gdouble y, gdouble rx, gdouble ry)
{
	SPGenericEllipse *ge;

	g_return_if_fail (arc != NULL);
	g_return_if_fail (SP_IS_ARC (arc));

	ge = SP_GENERICELLIPSE (arc);

	ge->x = x;
	ge->y = y;
	ge->rx = rx;
	ge->ry = ry;

	sp_genericellipse_set_shape (ge);
}

static void
sp_arc_start_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPGenericEllipse *ge;
	gdouble dx, dy;

	ge = SP_GENERICELLIPSE (item);

	dx = p->x - ge->x;
	dy = p->y - ge->y;
	ge->start = atan2(dy/ge->ry, dx/ge->rx);
	sp_genericellipse_normalize (ge);
}

static void
sp_arc_start_get (SPItem *item, ArtPoint *p)
{
	SPGenericEllipse *ge;
	SPArc *arc;

	ge = SP_GENERICELLIPSE (item);
	arc = SP_ARC (item);

	sp_arc_get_xy (arc, ge->start, p);
}

static void
sp_arc_end_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPGenericEllipse *ge;
	gdouble dx, dy;

	ge = SP_GENERICELLIPSE (item);

	dx = p->x - ge->x;
	dy = p->y - ge->y;
	ge->end = atan2(dy/ge->ry, dx/ge->rx);
	sp_genericellipse_normalize (ge);
}

static void
sp_arc_end_get (SPItem *item, ArtPoint *p)
{
	SPGenericEllipse *ge;
	SPArc *arc;

	ge = SP_GENERICELLIPSE (item);
	arc = SP_ARC (item);

	sp_arc_get_xy (arc, ge->end, p);
}

static SPKnotHolder *
sp_arc_knot_holder (SPItem *item, SPDesktop *desktop)
{
	SPArc *arc;
	SPKnotHolder *knot_holder;

	arc = SP_ARC (item);
	knot_holder = sp_knot_holder_new (desktop, item);
	
	sp_knot_holder_add (knot_holder,
			    sp_arc_start_set,
			    sp_arc_start_get);
	sp_knot_holder_add (knot_holder,
			    sp_arc_end_set,
			    sp_arc_end_get);
	
	return knot_holder;
}

void
sp_arc_get_xy (SPArc *arc, gdouble arg, ArtPoint *p)
{
	SPGenericEllipse *ge;

	ge = SP_GENERICELLIPSE (arc);

	p->x = ge->rx * cos(arg) + ge->x;
	p->y = ge->ry * sin(arg) + ge->y;
}

