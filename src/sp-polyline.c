#define __SP_POLYLINE_C__

/*
 * SVG <polyline> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <string.h>
#include "sp-polyline.h"

static void sp_polyline_class_init (SPPolyLineClass *class);
static void sp_polyline_init (SPPolyLine *polyline);

static void sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_polyline_read_attr (SPObject * object, const gchar * attr);
static SPRepr *sp_polyline_write (SPObject *object, SPRepr *repr, guint flags);

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

	sp_object_class->build = sp_polyline_build;
	sp_object_class->read_attr = sp_polyline_read_attr;
	sp_object_class->write = sp_polyline_write;

	item_class->description = sp_polyline_description;
}

static void
sp_polyline_init (SPPolyLine * polyline)
{
	SP_PATH (polyline)->independent = FALSE;
}

static void
sp_polyline_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

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

	if (((SPObjectClass *) parent_class)->read_attr)
		((SPObjectClass *) parent_class)->read_attr (object, attr);

}

static SPRepr *
sp_polyline_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPPolyLine *polyline;

	polyline = SP_POLYLINE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("polyline");
	}

	if (repr != SP_OBJECT_REPR (object)) {
		sp_repr_merge (repr, SP_OBJECT_REPR (object), "id");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static gchar *
sp_polyline_description (SPItem * item)
{
	return g_strdup ("PolyLine");
}

