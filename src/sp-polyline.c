#define SP_POLYLINE_C

#include <math.h>
#include <string.h>
#include "sp-polyline.h"

enum {ARG_0, ARG_POINTS};

static void sp_polyline_class_init (SPPolyLineClass *class);
static void sp_polyline_init (SPPolyLine *polyline);
static void sp_polyline_destroy (GtkObject *object);
static void sp_polyline_set_arg (GtkObject * object, GtkArg * arg, guint arg_id);

static void sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_polyline_read_attr (SPObject * object, const gchar * attr);

static void sp_polyline_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_polyline_description (SPItem * item);

static SPShapeClass *parent_class;

GtkType
sp_polyline_get_type (void)
{
	static GtkType polyline_type = 0;

	if (!polyline_type) {
		GtkTypeInfo polyline_info = {
			"SPPolyLine",
			sizeof (SPPolyLine),
			sizeof (SPPolyLineClass),
			(GtkClassInitFunc) sp_polyline_class_init,
			(GtkObjectInitFunc) sp_polyline_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		polyline_type = gtk_type_unique (sp_shape_get_type (), &polyline_info);
	}
	return polyline_type;
}

static void
sp_polyline_class_init (SPPolyLineClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_add_arg_type ("SPPolyLine::points", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_POINTS);

	gtk_object_class->destroy = sp_polyline_destroy;
	gtk_object_class->set_arg = sp_polyline_set_arg;

	sp_object_class->build = sp_polyline_build;
	sp_object_class->read_attr = sp_polyline_read_attr;

	item_class->bbox = sp_polyline_bbox;
	item_class->description = sp_polyline_description;
}

static void
sp_polyline_init (SPPolyLine * polyline)
{
	SP_PATH (polyline)->independent = FALSE;
}

static void
sp_polyline_destroy (GtkObject *object)
{
	SPPolyLine *polyline;

	polyline = SP_POLYLINE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_polyline_set_arg (GtkObject * object, GtkArg * arg, guint arg_id)
{
	SPPolyLine * polyline;

	polyline = SP_POLYLINE (object);

	switch (arg_id) {
	case ARG_POINTS:
		g_warning ("::points not implemented");
		break;
	}
}

static void
sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_polyline_read_attr (object, "points");
}

static void
sp_polyline_read_attr (SPObject * object, const gchar * attr)
{
	SPPolyLine * polyline;
	const gchar * astr;

	polyline = SP_POLYLINE (object);

	astr = sp_repr_attr (object->repr, attr);

	if (strcmp (attr, "points") == 0) {
		SPCurve * curve;
		const gchar * cptr;
		char * eptr;
		gboolean hascpt;

		sp_path_clear (SP_PATH (polyline));
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
		
		sp_path_add_bpath (SP_PATH (polyline), curve, TRUE, NULL);
		sp_curve_unref (curve);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_polyline_description (SPItem * item)
{
	return g_strdup ("PolyLine");
}

static void
sp_polyline_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}


