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
#include "style.h"
#include "sp-line.h"
#include "helper/sp-intl.h"

#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))

static void sp_line_class_init (SPLineClass *class);
static void sp_line_init (SPLine *line);

static void sp_line_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_line_read_attr (SPObject * object, const gchar * attr);
static SPRepr *sp_line_write (SPObject *object, SPRepr *repr, guint flags);

static gchar *sp_line_description (SPItem * item);
static void sp_line_write_transform (SPItem *item, SPRepr *repr, gdouble *t);

static void sp_line_set_shape (SPLine * line);

static SPShapeClass *parent_class;

GType
sp_line_get_type (void)
{
	static GType line_type = 0;

	if (!line_type) {
		GTypeInfo line_info = {
			sizeof (SPLineClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_line_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPLine),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_line_init,
		};
		line_type = g_type_register_static (SP_TYPE_SHAPE, "SPLine", &line_info, 0);
	}
	return line_type;
}

static void
sp_line_class_init (SPLineClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = g_type_class_ref (SP_TYPE_SHAPE);

	sp_object_class->build = sp_line_build;
	sp_object_class->read_attr = sp_line_read_attr;
	sp_object_class->write = sp_line_write;

	item_class->description = sp_line_description;
	item_class->write_transform = sp_line_write_transform;
}

static void
sp_line_init (SPLine * line)
{
	SP_PATH (line) -> independent = FALSE;
	line->x1 = line->y1 = line->x2 = line->y2 = 0.0;
}


static void
sp_line_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

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

	if (((SPObjectClass *) parent_class)->read_attr)
		((SPObjectClass *) parent_class)->read_attr (object, attr);

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
sp_line_write_transform (SPItem *item, SPRepr *repr, gdouble *t)
{
	double sw, sh;
	SPLine *line;

	line = SP_LINE (item);

	/* fixme: Would be nice to preserve units here */
	sp_repr_set_double (repr, "x1", t[0] * line->x1 + t[2] * line->y1 + t[4]);
	sp_repr_set_double (repr, "y1", t[1] * line->x1 + t[3] * line->y1 + t[5]);
	sp_repr_set_double (repr, "x2", t[0] * line->x2 + t[2] * line->y2 + t[4]);
	sp_repr_set_double (repr, "y2", t[1] * line->x2 + t[3] * line->y2 + t[5]);

	/* Scalers */
	sw = sqrt (t[0] * t[0] + t[1] * t[1]);
	sh = sqrt (t[2] * t[2] + t[3] * t[3]);

	/* And last but not least */
	if ((fabs (sw - 1.0) > 1e-9) || (fabs (sh - 1.0) > 1e-9)) {
		SPStyle *style;
		guchar *str;
		/* Scale changed, so we have to adjust stroke width */
		style = SP_OBJECT_STYLE (item);
		style->stroke_width.computed *= sqrt (fabs (sw * sh));
		str = sp_style_write_difference (style, SP_OBJECT_STYLE (SP_OBJECT_PARENT (item)));
		sp_repr_set_attr (SP_OBJECT_REPR (item), "style", str);
		g_free (str);
	}
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

