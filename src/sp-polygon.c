#define __SP_POLYGON_C__

/*
 * SVG <polygon> and <polyline> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <libarikkei/arikkei-strlib.h>
#include <libnr/nr-macros.h>

#include "helper/sp-intl.h"
#include "svg/svg.h"
#include "attributes.h"
#include "sp-polygon.h"

static gchar *sp_svg_write_points (const ArtBpath * bpath);

/* SPPolygon */

static void sp_polygon_class_init (SPPolygonClass *class);
static void sp_polygon_init (SPPolygon *polygon);

static void sp_polygon_build (SPObject * object, SPDocument * document, SPRepr * repr);
static SPRepr *sp_polygon_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_polygon_set (SPObject *object, unsigned int key, const unsigned char *value);

static gchar *sp_polygon_description (SPItem *item);

static SPShapeClass *polygon_parent_class;

GType
sp_polygon_get_type (void)
{
	static GType polygon_type = 0;

	if (!polygon_type) {
		GTypeInfo polygon_info = {
			sizeof (SPPolygonClass),
			NULL, NULL,
			(GClassInitFunc) sp_polygon_class_init,
			NULL, NULL,
			sizeof (SPPolygon),
			16,
			(GInstanceInitFunc) sp_polygon_init,
		};
		polygon_type = g_type_register_static (SP_TYPE_SHAPE, "SPPolygon", &polygon_info, 0);
	}
	return polygon_type;
}

static void
sp_polygon_class_init (SPPolygonClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	polygon_parent_class = g_type_class_ref (SP_TYPE_SHAPE);

	sp_object_class->build = sp_polygon_build;
	sp_object_class->write = sp_polygon_write;
	sp_object_class->set = sp_polygon_set;

	item_class->description = sp_polygon_description;
}

static void
sp_polygon_init (SPPolygon * polygon)
{
	/* Nothing here */
}

static void
sp_polygon_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) polygon_parent_class)->build)
		((SPObjectClass *) polygon_parent_class)->build (object, document, repr);

	sp_object_read_attr (object, "points");
}

static void
sp_polygon_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPPolygon *polygon;
	unsigned int ncoords;

	polygon = SP_POLYGON (object);

	switch (key) {
	case SP_ATTR_POINTS:
		if (value && sp_svg_points_read (value, NULL, &ncoords) && (ncoords >= 6)) {
			SPCurve *curve;
			float *coords;
			int i;
			curve = sp_curve_new ();
			coords = nr_new (float, ncoords);
			sp_svg_points_read (value, coords, NULL);
			sp_curve_moveto (curve, coords[0], coords[1]);
			for (i = 2; i < ncoords; i += 2) {
				sp_curve_lineto (curve, coords[i], coords[i + 1]);
			}
			nr_free (coords);
			sp_curve_closepath (curve);
			sp_shape_set_curve ((SPShape *) polygon, curve, TRUE);
			sp_curve_unref (curve);
		} else {
			sp_shape_set_curve ((SPShape *) polygon, NULL, TRUE);
		}
		break;
	default:
		if (((SPObjectClass *) polygon_parent_class)->set)
			((SPObjectClass *) polygon_parent_class)->set (object, key, value);
		break;
	}
}

static SPRepr *
sp_polygon_write (SPObject *object, SPRepr *repr, guint flags)
{
        SPShape *shape;
        ArtBpath *abp;
        gchar *str;

	shape = SP_SHAPE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("polygon");
	}

	/* We can safely write points here, because all subclasses require it too (Lauris) */
	abp = sp_curve_first_bpath (shape->curve);
	str = sp_svg_write_points (abp);
	sp_repr_set_attr (repr, "points", str);
	g_free (str);

	if (((SPObjectClass *) (polygon_parent_class))->write)
		((SPObjectClass *) (polygon_parent_class))->write (object, repr, flags);

	return repr;
}

static gchar *
sp_polygon_description (SPItem * item)
{
	return g_strdup ("Polygon");
}

/* SPPolyLine */

static void sp_polyline_class_init (SPPolyLineClass *class);
static void sp_polyline_init (SPPolyLine *polyline);

static void sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_polyline_set (SPObject *object, unsigned int key, const unsigned char *value);
static SPRepr *sp_polyline_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_polyline_description (SPItem * item);

static SPShapeClass *polyline_parent_class;

GType
sp_polyline_get_type (void)
{
	static GType polyline_type = 0;
	if (!polyline_type) {
		GTypeInfo polyline_info = {
			sizeof (SPPolyLineClass),
			NULL, NULL,
			(GClassInitFunc) sp_polyline_class_init,
			NULL, NULL,
			sizeof (SPPolyLine),
			16,
			(GInstanceInitFunc) sp_polyline_init,
		};
		polyline_type = g_type_register_static (SP_TYPE_SHAPE, "SPPolyLine", &polyline_info, 0);
	}
	return polyline_type;
}

static void
sp_polyline_class_init (SPPolyLineClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	polyline_parent_class = g_type_class_ref (SP_TYPE_SHAPE);

	sp_object_class->build = sp_polyline_build;
	sp_object_class->set = sp_polyline_set;
	sp_object_class->write = sp_polyline_write;

	item_class->description = sp_polyline_description;
}

static void
sp_polyline_init (SPPolyLine * polyline)
{
	/* Nothing here */
}

static void
sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) polyline_parent_class)->build)
		((SPObjectClass *) polyline_parent_class)->build (object, document, repr);

	sp_object_read_attr (object, "points");
}

static void
sp_polyline_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPPolyLine *polyline;
	unsigned int ncoords;

	polyline = SP_POLYLINE (object);

	switch (key) {
	case SP_ATTR_POINTS:
		if (value && sp_svg_points_read (value, NULL, &ncoords) && (ncoords >= 6)) {
			SPCurve *curve;
			float *coords;
			int i;
			curve = sp_curve_new ();
			coords = nr_new (float, ncoords);
			sp_svg_points_read (value, coords, NULL);
			sp_curve_moveto (curve, coords[0], coords[1]);
			for (i = 2; i < ncoords; i += 2) {
				sp_curve_lineto (curve, coords[i], coords[i + 1]);
			}
			nr_free (coords);
			sp_shape_set_curve ((SPShape *) polyline, curve, TRUE);
			sp_curve_unref (curve);
		} else {
			sp_shape_set_curve ((SPShape *) polyline, NULL, TRUE);
		}
		break;
	default:
		if (((SPObjectClass *) polygon_parent_class)->set)
			((SPObjectClass *) polygon_parent_class)->set (object, key, value);
		break;
	}
}

static SPRepr *
sp_polyline_write (SPObject *object, SPRepr *repr, guint flags)
{
        SPShape *shape;
        ArtBpath *abp;
        gchar *str;

	shape = SP_SHAPE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("polyline");
	}

	/* We can safely write points here, because all subclasses require it too (Lauris) */
	abp = sp_curve_first_bpath (shape->curve);
	str = sp_svg_write_points (abp);
	sp_repr_set_attr (repr, "points", str);
	g_free (str);

	if (((SPObjectClass *) (polyline_parent_class))->write)
		((SPObjectClass *) (polyline_parent_class))->write (object, repr, flags);

	return repr;

}

static gchar *
sp_polyline_description (SPItem * item)
{
	return g_strdup ("PolyLine");
}

/*
 * sp_svg_write_polygon: Write points attribute for polygon tag.
 * @bpath: 
 *
 * Return value: points attribute string.
 */
static gchar *
sp_svg_write_points (const ArtBpath * bpath)
{
	GString *result;
	int i;
	char *res;
	
	g_return_val_if_fail (bpath != NULL, NULL);

	result = g_string_sized_new (40);

	for (i = 0; bpath[i].code != ART_END; i++) {
		unsigned char c0[32], c1[32];
		switch (bpath [i].code){
		case ART_LINETO:
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			sp_svg_number_write_d (c0, bpath[i].x3, 6, 1, FALSE);
			sp_svg_number_write_d (c1, bpath[i].y3, 6, 1, FALSE);
			g_string_sprintfa (result, "%s,%s ", c0, c1);
			break;

		case ART_CURVETO:
		default:
			g_assert_not_reached ();
		}
	}
	res = result->str;
	g_string_free (result, FALSE);

	return res;
}

