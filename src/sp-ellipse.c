#define __SP_ELLIPSE_C__

/*
 * SVG <ellipse> and related implementations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <string.h>

#include "helper/art-utils.h"
#include "svg/svg.h"
#include "style.h"
/* For versioning */
#include "sp-root.h"

#include "sp-ellipse.h"

/* Common parent class */

#define noELLIPSE_VERBOSE

#define SP_2PI (2 * M_PI)

#if 1
/* Hmmm... shouldn't this also qualify */
/* Wheter it is faster or not, well, nobody knows */
#define sp_round(v,m) (((v) < 0.0) ? (ceil ((v) / (m) - 0.5)) * (m) : (floor ((v) / (m) + 0.5)))
#else
/* we does not use C99 round(3) function yet */
static double sp_round (double x, double y)
{
	double remain;

	g_assert (y > 0.0);

	/* return round(x/y) * y; */

	remain = fmod(x, y);

	if (remain >= 0.5*y)
		return x - remain + y;
	else
		return x - remain;
}
#endif

static void sp_genericellipse_class_init (SPGenericEllipseClass *klass);
static void sp_genericellipse_init (SPGenericEllipse *ellipse);

static void sp_genericellipse_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_genericellipse_read_attr (SPObject * object, const gchar * attr);

static GSList *sp_genericellipse_snappoints (SPItem *item, GSList *points);

static void sp_genericellipse_glue_set_shape (SPShape *shape);
static void sp_genericellipse_set_shape (SPGenericEllipse *ellipse);
static void sp_genericellipse_normalize (SPGenericEllipse *ellipse);

static SPShapeClass *ge_parent_class;

GType
sp_genericellipse_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPGenericEllipseClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_genericellipse_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPGenericEllipse),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_genericellipse_init,
		};
		type = g_type_register_static (SP_TYPE_SHAPE, "SPGenericEllipse", &info, 0);
	}
	return type;
}

static void
sp_genericellipse_class_init (SPGenericEllipseClass *klass)
{
	GObjectClass *gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;
	SPShapeClass *shape_class;

	gobject_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	shape_class = (SPShapeClass *) klass;

	ge_parent_class = g_type_class_ref (SP_TYPE_SHAPE);

	sp_object_class->build = sp_genericellipse_build;
	sp_object_class->read_attr = sp_genericellipse_read_attr;

	item_class->snappoints = sp_genericellipse_snappoints;

	shape_class->set_shape = sp_genericellipse_glue_set_shape;
}

static void
sp_genericellipse_init (SPGenericEllipse *ellipse)
{
	ellipse->shape.path.independent = FALSE;

	sp_svg_length_unset (&ellipse->cx, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&ellipse->cy, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&ellipse->rx, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&ellipse->ry, SP_SVG_UNIT_NONE, 0.0, 0.0);

	ellipse->start = 0.0;
	ellipse->end = SP_2PI;
	ellipse->closed = TRUE;
}

static void
sp_genericellipse_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (((SPObjectClass *) ge_parent_class)->build)
		(* ((SPObjectClass *) ge_parent_class)->build) (object, document, repr);
}

static void
sp_genericellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *str;

	ellipse = SP_GENERICELLIPSE (object);

#ifdef ELLIPSE_VERBOSE
	g_print ("sp_ellipse_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);

	if (((SPObjectClass *) ge_parent_class)->read_attr)
		(* ((SPObjectClass *) ge_parent_class)->read_attr) (object, attr);
}

static void sp_genericellipse_glue_set_shape (SPShape *shape)
{
	SPGenericEllipse *ge;

	ge = SP_GENERICELLIPSE (shape);

	sp_genericellipse_set_shape (ge);
}

static void
sp_genericellipse_update_length (SPSVGLength *length, gdouble em, gdouble ex, gdouble scale)
{
	if (length->unit == SP_SVG_UNIT_EM) {
		length->computed = length->value * em;
	} else if (length->unit == SP_SVG_UNIT_EX) {
		length->computed = length->value * ex;
	} else if (length->unit == SP_SVG_UNIT_PERCENT) {
		length->computed = length->value * scale;
	}
}

static void
sp_genericellipse_compute_values (SPGenericEllipse *ellipse)
{
	SPStyle *style;
	gdouble i2vp[6], vp2i[6];
	gdouble aw, ah;
	gdouble d;

	style = SP_OBJECT_STYLE (ellipse);

	/* fixme: It is somewhat dangerous, yes (Lauris) */
	/* fixme: And it is terribly slow too (Lauris) */
	/* fixme: In general we want to keep viewport scales around */
	sp_item_i2vp_affine (SP_ITEM (ellipse), i2vp);
	art_affine_invert (vp2i, i2vp);
	aw = sp_distance_d_matrix_d_transform (1.0, vp2i);
	ah = sp_distance_d_matrix_d_transform (1.0, vp2i);
	/* sqrt ((actual_width) ** 2 + (actual_height) ** 2)) / sqrt (2) */
	d = sqrt (aw * aw + ah * ah) * M_SQRT1_2;

	sp_genericellipse_update_length (&ellipse->cx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_genericellipse_update_length (&ellipse->cy, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_genericellipse_update_length (&ellipse->rx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_genericellipse_update_length (&ellipse->ry, style->font_size.computed, style->font_size.computed * 0.5, d);
}

#define C1 0.552

/* fixme: Think (Lauris) */

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

	sp_path_clear (SP_PATH (ellipse));

	/* fixme: Maybe track, whether we have em,ex,% (Lauris) */
	/* fixme: Alternately we can use ::modified to keep everything up-to-date (Lauris) */
	sp_genericellipse_compute_values (ellipse);

	if ((ellipse->rx.computed < 1e-18) || (ellipse->ry.computed < 1e-18)) return;
	if (fabs (ellipse->end - ellipse->start) < 1e-9) return;

	sp_genericellipse_normalize (ellipse);

	cx = 0.0;
	cy = 0.0;
	rx = ellipse->rx.computed;
	ry = ellipse->ry.computed;

	len = fmod (ellipse->end - ellipse->start, SP_2PI);
	if (len < 0.0) len += SP_2PI;
	if (fabs (len) < 1e-9) {
		slice = FALSE;
	} else {
		slice = TRUE;
	}

	art_affine_scale (affine, rx, ry);
	affine[4] = ellipse->cx.computed;
	affine[5] = ellipse->cy.computed;

	i = 0;
	if (ellipse->closed) {
		bpath[i].code = ART_MOVETO;
	} else {
		bpath[i].code = ART_MOVETO_OPEN;
	}
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
	sp_path_add_bpath (SP_PATH (ellipse), c, TRUE, NULL);
	sp_curve_unref (c);

	sp_object_request_modified (SP_OBJECT (ellipse), SP_OBJECT_MODIFIED_FLAG);
}

static GSList * 
sp_genericellipse_snappoints (SPItem *item, GSList *points)
{
	SPGenericEllipse *ge;
	ArtPoint *p;
	gdouble affine[6];

	ge = SP_GENERICELLIPSE (item);

	/* we use corners of item and center of ellipse */
	if (SP_ITEM_CLASS (ge_parent_class)->snappoints)
		points = (SP_ITEM_CLASS (ge_parent_class)->snappoints) (item, points);

	p = g_new (ArtPoint,1);
	p->x = ge->cx.computed;
	p->y = ge->cy.computed;
	sp_item_i2d_affine (item, affine);
	art_affine_point (p, p, affine);
	points = g_slist_append (points, p);
	return points;
}

static void
sp_genericellipse_normalize (SPGenericEllipse *ellipse)
{
	double diff;

	ellipse->start = fmod (ellipse->start, SP_2PI);
	ellipse->end = fmod (ellipse->end, SP_2PI);

	if (ellipse->start < 0.0)
		ellipse->start += SP_2PI;
	diff = ellipse->start - ellipse->end;
	if (diff >= 0.0)
		ellipse->end += diff - fmod(diff, SP_2PI) + SP_2PI;

	/* Now we keep: 0 <= start < end <= 2*PI */
}

/*
 * return values:
 *   1  : inside
 *   0  : on the curves
 *   -1 : outside
 */
static gint
sp_genericellipse_side (SPGenericEllipse *ellipse, const ArtPoint *p)
{
	gdouble dx, dy;
	gdouble s;

	dx = p->x - ellipse->cx.computed;
	dy = p->y - ellipse->cy.computed;

	s = dx * dx / (ellipse->rx.computed * ellipse->rx.computed) + dy * dy / (ellipse->ry.computed * ellipse->ry.computed);
	if (s < 1.0) return 1;
	if (s > 1.0) return -1;
	return 0;
}

/* SVG <ellipse> element */

static void sp_ellipse_class_init (SPEllipseClass *class);
static void sp_ellipse_init (SPEllipse *ellipse);

static void sp_ellipse_build (SPObject * object, SPDocument * document, SPRepr * repr);
static SPRepr *sp_ellipse_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_ellipse_read_attr (SPObject * object, const gchar * attr);
static gchar * sp_ellipse_description (SPItem * item);

static SPGenericEllipseClass *ellipse_parent_class;

GType
sp_ellipse_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPEllipseClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_ellipse_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPEllipse),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_ellipse_init,
		};
		type = g_type_register_static (SP_TYPE_GENERICELLIPSE, "SPEllipse", &info, 0);
	}
	return type;
}

static void
sp_ellipse_class_init (SPEllipseClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	ellipse_parent_class = g_type_class_ref (SP_TYPE_GENERICELLIPSE);

	sp_object_class->build = sp_ellipse_build;
	sp_object_class->write = sp_ellipse_write;
	sp_object_class->read_attr = sp_ellipse_read_attr;

	item_class->description = sp_ellipse_description;
}

static void
sp_ellipse_init (SPEllipse *ellipse)
{
	/* Nothing special */
}

static void
sp_ellipse_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (((SPObjectClass *) ellipse_parent_class)->build)
		(* ((SPObjectClass *) ellipse_parent_class)->build) (object, document, repr);

	sp_ellipse_read_attr (object, "cx");
	sp_ellipse_read_attr (object, "cy");
	sp_ellipse_read_attr (object, "rx");
	sp_ellipse_read_attr (object, "ry");
}

static SPRepr *
sp_ellipse_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPGenericEllipse *ellipse;

	ellipse = SP_GENERICELLIPSE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("ellipse");
	}

	sp_repr_set_double_attribute (repr, "cx", ellipse->cx.computed);
	sp_repr_set_double_attribute (repr, "cy", ellipse->cy.computed);
	sp_repr_set_double_attribute (repr, "rx", ellipse->rx.computed);
	sp_repr_set_double_attribute (repr, "ry", ellipse->ry.computed);
	
	if (((SPObjectClass *) ellipse_parent_class)->write)
		(* ((SPObjectClass *) ellipse_parent_class)->write) (object, repr, flags);

	return repr;
}

static void
sp_ellipse_read_attr (SPObject * object, const gchar * attr)
{
	SPGenericEllipse *ellipse;
	const gchar *str;
	gulong unit;

	ellipse = SP_GENERICELLIPSE (object);

#ifdef ELLIPSE_VERBOSE
	g_print ("sp_ellipse_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);

	if (!strcmp (attr, "cx")) {
		if (!sp_svg_length_read (str, &ellipse->cx)) {
			ellipse->cx.set = FALSE;
			ellipse->cx.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "cy")) {
		if (!sp_svg_length_read (str, &ellipse->cy)) {
			ellipse->cy.set = FALSE;
			ellipse->cy.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "rx")) {
		if (sp_svg_length_read_lff (str, &unit, &ellipse->rx.value, &ellipse->rx.computed) && (ellipse->rx.value > 0.0)) {
			ellipse->rx.set = TRUE;
			ellipse->rx.unit = unit;
		} else {
			ellipse->rx.set = FALSE;
			ellipse->rx.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "ry")) {
		if (sp_svg_length_read_lff (str, &unit, &ellipse->ry.value, &ellipse->ry.computed) && (ellipse->ry.value > 0.0)) {
			ellipse->ry.set = TRUE;
			ellipse->ry.unit = unit;
		} else {
			ellipse->ry.set = FALSE;
			ellipse->ry.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else {
		if (((SPObjectClass *) ellipse_parent_class)->read_attr)
			(* ((SPObjectClass *) ellipse_parent_class)->read_attr) (object, attr);
	}
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

	ge->cx.computed = x;
	ge->cy.computed = y;
	ge->rx.computed = rx;
	ge->ry.computed = ry;

	sp_genericellipse_set_shape (ge);
}

/* SVG <circle> element */

static void sp_circle_class_init (SPCircleClass *class);
static void sp_circle_init (SPCircle *circle);

static void sp_circle_build (SPObject * object, SPDocument * document, SPRepr * repr);
static SPRepr *sp_circle_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_circle_read_attr (SPObject * object, const gchar * attr);
static gchar * sp_circle_description (SPItem * item);

static SPGenericEllipseClass *circle_parent_class;

GType
sp_circle_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPCircleClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_circle_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPCircle),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_circle_init,
		};
		type = g_type_register_static (SP_TYPE_GENERICELLIPSE, "SPCircle", &info, 0);
	}
	return type;
}

static void
sp_circle_class_init (SPCircleClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	circle_parent_class = g_type_class_ref (SP_TYPE_GENERICELLIPSE);

	sp_object_class->build = sp_circle_build;
	sp_object_class->write = sp_circle_write;
	sp_object_class->read_attr = sp_circle_read_attr;

	item_class->description = sp_circle_description;
}

static void
sp_circle_init (SPCircle *circle)
{
	/* Nothing special */
}

static void
sp_circle_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	if (((SPObjectClass *) circle_parent_class)->build)
		(* ((SPObjectClass *) circle_parent_class)->build) (object, document, repr);

	sp_circle_read_attr (object, "cx");
	sp_circle_read_attr (object, "cy");
	sp_circle_read_attr (object, "r");
}

static SPRepr *
sp_circle_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPGenericEllipse *ellipse;

	ellipse = SP_GENERICELLIPSE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("circle");
	}

	sp_repr_set_double_attribute (repr, "cx", ellipse->cx.computed);
	sp_repr_set_double_attribute (repr, "cy", ellipse->cy.computed);
	sp_repr_set_double_attribute (repr, "r", ellipse->rx.computed);
	
	if (((SPObjectClass *) circle_parent_class)->write)
		((SPObjectClass *) circle_parent_class)->write (object, repr, flags);

	return repr;
}

static void
sp_circle_read_attr (SPObject *object, const gchar * attr)
{
	SPGenericEllipse *circle;
	const gchar *str;
	gulong unit;

	circle = SP_GENERICELLIPSE (object);

#ifdef CIRCLE_VERBOSE
	g_print ("sp_circle_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);

	if (!strcmp (attr, "cx")) {
		if (!sp_svg_length_read (str, &circle->cx)) {
			circle->cx.set = FALSE;
			circle->cx.computed = 0.0;
		}
		sp_genericellipse_set_shape (circle);
	} else if (!strcmp (attr, "cy")) {
		if (!sp_svg_length_read (str, &circle->cy)) {
			circle->cy.set = FALSE;
			circle->cy.computed = 0.0;
		}
		sp_genericellipse_set_shape (circle);
	} if (!strcmp (attr, "r")) {
		if (sp_svg_length_read_lff (str, &unit, &circle->rx.value, &circle->rx.computed) && (circle->rx.value > 0.0)) {
			circle->rx.set = TRUE;
			circle->rx.unit = unit;
		} else {
			circle->rx.set = FALSE;
			circle->rx.computed = 0.0;
		}
		circle->ry.computed = circle->rx.computed;
		sp_genericellipse_set_shape (circle);
	} else {
		if (((SPObjectClass *) circle_parent_class)->read_attr)
			(* ((SPObjectClass *) circle_parent_class)->read_attr) (object, attr);
	}
}

static gchar *
sp_circle_description (SPItem * item)
{
	return g_strdup ("Circle");
}

/* <path sodipodi:type="arc"> element */

static void sp_arc_class_init (SPArcClass *class);
static void sp_arc_init (SPArc *arc);

static void sp_arc_build (SPObject * object, SPDocument * document, SPRepr * repr);
static SPRepr *sp_arc_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_arc_read_attr (SPObject *object, const gchar *attr);
static void sp_arc_modified (SPObject *object, guint flags);

static gchar * sp_arc_description (SPItem * item);
static SPKnotHolder *sp_arc_knot_holder (SPItem * item, SPDesktop *desktop);

static SPGenericEllipseClass *arc_parent_class;

GType
sp_arc_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPArcClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_arc_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPArc),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_arc_init,
		};
		type = g_type_register_static (SP_TYPE_GENERICELLIPSE, "SPArc", &info, 0);
	}
	return type;
}

static void
sp_arc_class_init (SPArcClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	arc_parent_class = g_type_class_ref (SP_TYPE_GENERICELLIPSE);

	sp_object_class->build = sp_arc_build;
	sp_object_class->write = sp_arc_write;
	sp_object_class->read_attr = sp_arc_read_attr;
	sp_object_class->modified = sp_arc_modified;

	item_class->description = sp_arc_description;
	item_class->knot_holder = sp_arc_knot_holder;
}

static void
sp_arc_init (SPArc *arc)
{
	/* Nothing special */
}

/* fixme: Better place (Lauris) */

static guint
sp_arc_find_version (SPObject *object)
{

	while (object) {
		if (SP_IS_ROOT (object)) {
			return SP_ROOT (object)->sodipodi;
		}
		object = SP_OBJECT_PARENT (object);
	}

	return 0;
}

static void
sp_arc_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	guint version;

	if (((SPObjectClass *) arc_parent_class)->build)
		(* ((SPObjectClass *) arc_parent_class)->build) (object, document, repr);

	version = sp_arc_find_version (object);

	if (version < 25) {
		/* Old spec violating arc attributes */
		sp_arc_read_attr (object, "cx");
		sp_arc_read_attr (object, "cy");
		sp_arc_read_attr (object, "rx");
		sp_arc_read_attr (object, "ry");
	} else {
		/* New attributes */
		sp_arc_read_attr (object, "sodipodi:cx");
		sp_arc_read_attr (object, "sodipodi:cy");
		sp_arc_read_attr (object, "sodipodi:rx");
		sp_arc_read_attr (object, "sodipodi:ry");
	}

	sp_arc_read_attr (object, "sodipodi:start");
	sp_arc_read_attr (object, "sodipodi:end");
	sp_arc_read_attr (object, "sodipodi:open");

	if (version < 25) {
		/* fixme: I am 99.9% sure we can do this here safely, but check nevertheless (Lauris) */
		sp_arc_write (object, repr, SP_OBJECT_WRITE_SODIPODI);
		sp_repr_set_attr (repr, "cx", NULL);
		sp_repr_set_attr (repr, "cy", NULL);
		sp_repr_set_attr (repr, "rx", NULL);
		sp_repr_set_attr (repr, "ry", NULL);
	}
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
#define ARC_BUFSIZE 256
	SPGenericEllipse *ge;
	ArtPoint p1, p2;
	gint fa, fs;
	gdouble  dt;
	gchar c[ARC_BUFSIZE];

	ge = SP_GENERICELLIPSE (arc);

	sp_arc_get_xy (arc, ge->start, &p1);
	sp_arc_get_xy (arc, ge->end, &p2);

	dt = fmod (ge->end - ge->start, SP_2PI);
	if (fabs (dt) < 1e-6) {
		ArtPoint ph;
		sp_arc_get_xy (arc, (ge->start + ge->end) / 2.0, &ph);
		g_snprintf (c, ARC_BUFSIZE, "M %f %f A %f %f 0 %d %d %f,%f A %g %g 0 %d %d %g %g L %f %f z",
			    p1.x, p1.y,
			    ge->rx.computed, ge->ry.computed,
			    1, (dt > 0),
			    ph.x, ph.y,
			    ge->rx.computed, ge->ry.computed,
			    1, (dt > 0),
			    p2.x, p2.y,
			    ge->cx.computed, ge->cy.computed);
	} else {
		fa = (fabs (dt) > M_PI) ? 1 : 0;
		fs = (dt > 0) ? 1 : 0;
#ifdef ARC_VERBOSE
		g_print ("start:%g end:%g fa=%d fs=%d\n", ge->start, ge->end, fa, fs);
#endif
		if (ge->closed) {
			g_snprintf (c, ARC_BUFSIZE, "M %f,%f A %f,%f 0 %d %d %f,%f L %f,%f z",
				    p1.x, p1.y,
				    ge->rx.computed, ge->ry.computed,
				    fa, fs,
				    p2.x, p2.y,
				    ge->cx.computed, ge->cy.computed);
		} else {
			g_snprintf (c, ARC_BUFSIZE, "M %f,%f A %f,%f 0 %d %d %f,%f",
				    p1.x, p1.y,
				    ge->rx.computed, ge->ry.computed,
				    fa, fs, p2.x, p2.y);
		}
	}

	return sp_repr_set_attr (repr, "d", c);
}

static SPRepr *
sp_arc_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPGenericEllipse *ge;
	SPArc *arc;

	ge = SP_GENERICELLIPSE (object);
	arc = SP_ARC (object);

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
			repr = sp_repr_new ("path");
		}

		sp_repr_set_attr (repr, "sodipodi:type", "arc");
		sp_repr_set_double_attribute (repr, "sodipodi:cx", ge->cx.computed);
		sp_repr_set_double_attribute (repr, "sodipodi:cy", ge->cy.computed);
		sp_repr_set_double_attribute (repr, "sodipodi:rx", ge->rx.computed);
		sp_repr_set_double_attribute (repr, "sodipodi:ry", ge->ry.computed);
		sp_repr_set_double_attribute (repr, "sodipodi:start", ge->start);
		sp_repr_set_double_attribute (repr, "sodipodi:end", ge->end);
		sp_repr_set_attr (repr, "sodipodi:open", (!ge->closed) ? "true" : NULL);

		sp_arc_set_elliptical_path_attribute (arc, repr);
	} else {
		gdouble dt;
		dt = fmod (ge->end - ge->start, SP_2PI);
		if (fabs (dt) < 1e-6) {
			if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
				repr = sp_repr_new ("ellipse");
			}
			sp_repr_set_double_attribute (repr, "cx", ge->cx.computed);
			sp_repr_set_double_attribute (repr, "cy", ge->cy.computed);
			sp_repr_set_double_attribute (repr, "rx", ge->rx.computed);
			sp_repr_set_double_attribute (repr, "ry", ge->ry.computed);
		} else {
			if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
				repr = sp_repr_new ("path");
			}
			sp_arc_set_elliptical_path_attribute (arc, repr);
		}
	}

	if (((SPObjectClass *) arc_parent_class)->write)
		((SPObjectClass *) arc_parent_class)->write (object, repr, flags);

	return repr;
}

static void
sp_arc_read_attr (SPObject *object, const gchar *attr)
{
	SPGenericEllipse *ellipse;
	const gchar *str;
	guint version;
	gulong unit;
	double n;

	ellipse = SP_GENERICELLIPSE (object);
	
#ifdef ARC_VERBOSE
	g_print ("sp_arc_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);

	version = sp_arc_find_version (object);

	if (((version < 25) && !strcmp (attr, "cx")) || ((version >= 25) && !strcmp (attr, "sodipodi:cx"))) {
		if (!sp_svg_length_read (str, &ellipse->cx)) {
			ellipse->cx.set = FALSE;
			ellipse->cx.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (((version < 25) && !strcmp (attr, "cy")) || ((version >= 25) && !strcmp (attr, "sodipodi:cy"))) {
		if (!sp_svg_length_read (str, &ellipse->cy)) {
			ellipse->cy.set = FALSE;
			ellipse->cy.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (((version < 25) && !strcmp (attr, "rx")) || ((version >= 25) && !strcmp (attr, "sodipodi:rx"))) {
		if (sp_svg_length_read_lff (str, &unit, &ellipse->rx.value, &ellipse->rx.computed) && (ellipse->rx.value > 0.0)) {
			ellipse->rx.set = TRUE;
			ellipse->rx.unit = unit;
		} else {
			ellipse->rx.set = FALSE;
			ellipse->rx.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (((version < 25) && !strcmp (attr, "ry")) || ((version >= 25) && !strcmp (attr, "sodipodi:ry"))) {
		if (sp_svg_length_read_lff (str, &unit, &ellipse->ry.value, &ellipse->ry.computed) && (ellipse->ry.value > 0.0)) {
			ellipse->ry.set = TRUE;
			ellipse->ry.unit = unit;
		} else {
			ellipse->ry.set = FALSE;
			ellipse->ry.computed = 0.0;
		}
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "sodipodi:start")) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->start);
		ellipse->start = n;
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "sodipodi:end")) {
		n = sp_repr_get_double_attribute (object->repr, attr, ellipse->end);
		ellipse->end = n;
		sp_genericellipse_set_shape (ellipse);
	} else if (!strcmp (attr, "sodipodi:open")) {
		if (sp_repr_attr_is_set (object->repr, attr))
			ellipse->closed = FALSE;
		else
			ellipse->closed = TRUE;
		sp_genericellipse_set_shape (ellipse);
	} else {
		if (((SPObjectClass *) arc_parent_class)->read_attr)
			((SPObjectClass *) arc_parent_class)->read_attr (object, attr);
	}
}

static void
sp_arc_modified (SPObject *object, guint flags)
{
	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		sp_arc_set_elliptical_path_attribute (SP_ARC (object), SP_OBJECT_REPR (object));
	}

	if (((SPObjectClass *) arc_parent_class)->modified)
		((SPObjectClass *) arc_parent_class)->modified (object, flags);
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

	ge->cx.computed = x;
	ge->cy.computed = y;
	ge->rx.computed = rx;
	ge->ry.computed = ry;

	sp_genericellipse_set_shape (ge);
}

static void
sp_arc_start_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPGenericEllipse *ge;
	SPArc *arc;
	gdouble dx, dy;

	ge = SP_GENERICELLIPSE (item);
	arc = SP_ARC(item);

	ge->closed = (sp_genericellipse_side (ge, p) == -1) ? TRUE : FALSE;

	dx = p->x - ge->cx.computed;
	dy = p->y - ge->cy.computed;

	ge->start = atan2 (dy / ge->ry.computed, dx / ge->rx.computed);
	if (state & GDK_CONTROL_MASK) {
		ge->start = sp_round(ge->start, M_PI_4);
	}
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
	SPArc *arc;
	gdouble dx, dy;

	ge = SP_GENERICELLIPSE (item);
	arc = SP_ARC(item);

	ge->closed = (sp_genericellipse_side (ge, p) == -1) ? TRUE : FALSE;

	dx = p->x - ge->cx.computed;
	dy = p->y - ge->cy.computed;
	ge->end = atan2 (dy / ge->ry.computed, dx / ge->rx.computed);
	if (state & GDK_CONTROL_MASK) {
		ge->end = sp_round(ge->end, M_PI_4);
	}
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
	knot_holder = sp_knot_holder_new (desktop, item, NULL);
	
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

	p->x = ge->rx.computed * cos(arg) + ge->cx.computed;
	p->y = ge->ry.computed * sin(arg) + ge->cy.computed;
}

