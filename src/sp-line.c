#define __SP_LINE_C__

/*
 * SVG <line> implementation
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
#include "sp-line.h"

#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))

static void sp_line_class_init (SPLineClass *class);
static void sp_line_init (SPLine *line);
static void sp_line_destroy (GtkObject *object);

static void sp_line_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_line_read_attr (SPObject * object, const gchar * attr);
static SPRepr *sp_line_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_line_description (SPItem * item);

static void sp_line_set_shape (SPLine * line);

static SPShapeClass *parent_class;

GtkType
sp_line_get_type (void)
{
	static GtkType line_type = 0;

	if (!line_type) {
		GtkTypeInfo line_info = {
			"SPLine",
			sizeof (SPLine),
			sizeof (SPLineClass),
			(GtkClassInitFunc) sp_line_class_init,
			(GtkObjectInitFunc) sp_line_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		line_type = gtk_type_unique (sp_shape_get_type (), &line_info);
	}
	return line_type;
}

static void
sp_line_class_init (SPLineClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_line_destroy;

	sp_object_class->build = sp_line_build;
	sp_object_class->read_attr = sp_line_read_attr;
	sp_object_class->write = sp_line_write;

	item_class->description = sp_line_description;
}

static void
sp_line_init (SPLine * line)
{
	SP_PATH (line) -> independent = FALSE;
	line->x1 = line->y1 = line->x2 = line->y2 = 0.0;
}

static void
sp_line_destroy (GtkObject *object)
{
	SPLine *line;

	line = SP_LINE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_line_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_line_read_attr (object, "x1");
	sp_line_read_attr (object, "y1");
	sp_line_read_attr (object, "x2");
	sp_line_read_attr (object, "y2");
}

static void
sp_line_read_attr (SPObject * object, const gchar * attr)
{
	SPLine * line;
	double n;

	line = SP_LINE (object);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "x1") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, line->x1);
		line->x1 = n;
		sp_line_set_shape (line);
		return;
	}
	if (strcmp (attr, "y1") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, line->y1);
		line->y1 = n;
		sp_line_set_shape (line);
		return;
	}
	if (strcmp (attr, "x2") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, line->x2);
		line->x2 = n;
		sp_line_set_shape (line);
		return;
	}
	if (strcmp (attr, "y2") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, line->y2);
		line->y2 = n;
		sp_line_set_shape (line);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static SPRepr *
sp_line_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPLine *line;

	line = SP_LINE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("line");
	}

	if (repr != SP_OBJECT_REPR (object)) {
		sp_repr_merge (repr, SP_OBJECT_REPR (object), "id");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static gchar *
sp_line_description (SPItem * item)
{
	return g_strdup ("Line");
}

static void
sp_line_set_shape (SPLine * line)
{
	SPCurve * c;
	
	sp_path_clear (SP_PATH (line));

	if (hypot (line->x2 - line->x1, line->y2 - line->y1) < 1e-12) return;

	c = sp_curve_new ();

	sp_curve_moveto (c, line->x1, line->y1);
	sp_curve_lineto (c, line->x2, line->y2);

	sp_path_add_bpath (SP_PATH (line), c, TRUE, NULL);
	sp_curve_unref (c);
}

