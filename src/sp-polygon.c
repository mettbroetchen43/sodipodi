#define SP_POLYGON_C

#include <math.h>
#include <string.h>
#include "sp-polygon.h"

enum {ARG_0, ARG_POINTS};

static void sp_polygon_class_init (SPPolygonClass *class);
static void sp_polygon_init (SPPolygon *polygon);
static void sp_polygon_destroy (GtkObject *object);
static void sp_polygon_set_arg (GtkObject * object, GtkArg * arg, guint arg_id);

static void sp_polygon_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_polygon_write_repr (SPObject * object, SPRepr * repr);
static void sp_polygon_read_attr (SPObject * object, const gchar * attr);

static void sp_polygon_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_polygon_description (SPItem * item);

static SPShapeClass *parent_class;

GtkType
sp_polygon_get_type (void)
{
	static GtkType polygon_type = 0;

	if (!polygon_type) {
		GtkTypeInfo polygon_info = {
			"SPPolygon",
			sizeof (SPPolygon),
			sizeof (SPPolygonClass),
			(GtkClassInitFunc) sp_polygon_class_init,
			(GtkObjectInitFunc) sp_polygon_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		polygon_type = gtk_type_unique (sp_shape_get_type (), &polygon_info);
	}
	return polygon_type;
}

static void
sp_polygon_class_init (SPPolygonClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_add_arg_type ("SPPolygon::points", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_POINTS);

	gtk_object_class->destroy = sp_polygon_destroy;
	gtk_object_class->set_arg = sp_polygon_set_arg;

	sp_object_class->build = sp_polygon_build;
	sp_object_class->write_repr = sp_polygon_write_repr;
	sp_object_class->read_attr = sp_polygon_read_attr;

	item_class->bbox = sp_polygon_bbox;
	item_class->description = sp_polygon_description;
}

static void
sp_polygon_init (SPPolygon * polygon)
{
	SP_PATH (polygon)->independent = FALSE;
}

static void
sp_polygon_destroy (GtkObject *object)
{
	SPPolygon *polygon;

	polygon = SP_POLYGON (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_polygon_set_arg (GtkObject * object, GtkArg * arg, guint arg_id)
{
	SPPolygon * polygon;

	polygon = SP_POLYGON (object);

	switch (arg_id) {
	case ARG_POINTS:
		g_warning ("::points not implemented");
		break;
	}
}

static void
sp_polygon_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_polygon_read_attr (object, "points");
}

/*
 * sp_svg_write_polygon: Write points attribute for polygon tag.
 * @bpath: 
 *
 * Return value: points attribute string.
 */
static gchar *
sp_svg_write_polygon (const ArtBpath * bpath)
{
	GString *result;
	int i;
	int closed = 0;
	char *res;
	
	g_return_val_if_fail (bpath != NULL, NULL);

	result = g_string_sized_new (40);

	for (i = 0; bpath[i].code != ART_END; i++){
		switch (bpath [i].code){
		case ART_LINETO:
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			g_string_sprintfa (result, "%g,%g ", bpath [i].x3, bpath [i].y3);
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

static void
sp_polygon_write_repr (SPObject * object, SPRepr * repr)
{
        SPPath     *path;
        SPPathComp *pathcomp;
        ArtBpath   *abp;
        gchar      *str;

        path = SP_PATH(object);
        g_assert (path->comp);
        g_assert (path->comp->data);
        pathcomp = path->comp->data;
        g_assert (pathcomp);
        abp = sp_curve_first_bpath (pathcomp->curve);
	str = sp_svg_write_polygon (abp);
	sp_repr_set_attr (repr, "points", str);
	g_free (str);

#if 0
	/* stop to propagete to parent class.
	 * we don't need to generate d="" attribute.
	 */
	if (((SPObjectClass *) (parent_class))->write_repr)
		(*((SPObjectClass *) (parent_class))->write_repr) (object, repr);
#endif
}

static void
sp_polygon_read_attr (SPObject * object, const gchar * attr)
{
	SPPolygon * polygon;
	const gchar * astr;

	polygon = SP_POLYGON (object);

	astr = sp_repr_attr (object->repr, attr);

	if (strcmp (attr, "points") == 0) {
		SPCurve * curve;
		const gchar * cptr;
		char * eptr;
		gboolean hascpt;

		sp_path_clear (SP_PATH (polygon));
		if (!astr) return;
		curve = sp_curve_new ();
		hascpt = FALSE;

		cptr = astr;
		eptr = NULL;

		while (TRUE) {
			gdouble x, y;

			x = strtod (cptr, &eptr);
			if (eptr == cptr) break;
			cptr = strchr (eptr, ',');
			if (!cptr) break;
			cptr++;
			y = strtod (cptr, &eptr);
			if (eptr == cptr) break;
			cptr = eptr;
			if (hascpt) {
				sp_curve_lineto (curve, x, y);
			} else {
				sp_curve_moveto (curve, x, y);
				hascpt = TRUE;
			}
		}
		
		sp_curve_closepath (curve);
		sp_path_add_bpath (SP_PATH (polygon), curve, TRUE, NULL);
		sp_curve_unref (curve);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_polygon_description (SPItem * item)
{
	return g_strdup ("Polygon");
}

static void
sp_polygon_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}


