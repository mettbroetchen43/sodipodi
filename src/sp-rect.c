#define __SP_RECT_C__

/*
 * SPRect
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <math.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "svg/svg.h"
#include "dialogs/object-attributes.h"
#include "sp-rect.h"

#define noRECT_VERBOSE

static void sp_rect_class_init (SPRectClass *class);
static void sp_rect_init (SPRect *rect);
static void sp_rect_destroy (GtkObject *object);

static void sp_rect_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_rect_read_attr (SPObject * object, const gchar * attr);

static gchar * sp_rect_description (SPItem * item);
static GSList * sp_rect_snappoints (SPItem * item, GSList * points);
static void sp_rect_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);
static void sp_rect_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_rect_rect_properties (GtkMenuItem *menuitem, SPAnchor *anchor);

static void sp_rect_set_shape (SPRect * rect);

static SPShapeClass *parent_class;

GtkType
sp_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		GtkTypeInfo rect_info = {
			"SPRect",
			sizeof (SPRect),
			sizeof (SPRectClass),
			(GtkClassInitFunc) sp_rect_class_init,
			(GtkObjectInitFunc) sp_rect_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		rect_type = gtk_type_unique (sp_shape_get_type (), &rect_info);
	}
	return rect_type;
}

static void
sp_rect_class_init (SPRectClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	gtk_object_class->destroy = sp_rect_destroy;

	sp_object_class->build = sp_rect_build;
	sp_object_class->read_attr = sp_rect_read_attr;

	item_class->description = sp_rect_description;
	item_class->snappoints = sp_rect_snappoints;
	item_class->write_transform = sp_rect_write_transform;
	item_class->menu = sp_rect_menu;
}

static void
sp_rect_init (SPRect * rect)
{
	SP_PATH (rect) -> independent = FALSE;
	rect->x = rect->y = 0.0;
	rect->width = rect->height = 0.0;
	rect->rx = rect->ry = 0.0;
}

static void
sp_rect_destroy (GtkObject *object)
{
	SPRect *rect;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_RECT (object));

	rect = SP_RECT (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_rect_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_rect_read_attr (object, "x");
	sp_rect_read_attr (object, "y");
	sp_rect_read_attr (object, "width");
	sp_rect_read_attr (object, "height");
	sp_rect_read_attr (object, "rx");
	sp_rect_read_attr (object, "ry");
}

static void
sp_rect_read_attr (SPObject * object, const gchar * attr)
{
	SPRect * rect;
	const gchar * astr;
	SPSVGUnit unit;
	double n;

	rect = SP_RECT (object);

#ifdef RECT_VERBOSE
	g_print ("sp_rect_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "x") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->x = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->y = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "width") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->width = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "height") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->height = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "rx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->rx = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "ry") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		rect->ry = n;
		sp_rect_set_shape (rect);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_rect_description (SPItem * item)
{
	return g_strdup ("Rectangle");
}

#define C1 0.554

static void
sp_rect_set_shape (SPRect * rect)
{
	double x, y, rx, ry;
	SPCurve * c;
	
	sp_path_clear (SP_PATH (rect));

	if ((rect->height < 1e-12) || (rect->width < 1e-12)) return;

	c = sp_curve_new ();

	x = rect->x;
	y = rect->y;

	if ((rect->rx != 0.0) && (rect->ry != 0.0)) {
		rx = rect->rx;
		ry = rect->ry;
		if (rx > rect->width / 2) rx = rect->width / 2;
		if (ry > rect->height / 2) ry = rect->height / 2;

		sp_curve_moveto (c, x + rx, y + 0.0);
		sp_curve_curveto (c, x + rx * (1 - C1), y + 0.0, x + 0.0, y + ry * (1 - C1), x + 0.0, y + ry);
		if (ry < rect->height / 2)
			sp_curve_lineto (c, x + 0.0, y + rect->height - ry);
		sp_curve_curveto (c, x + 0.0, y + rect->height - ry * (1 - C1), x + rx * (1 - C1), y + rect->height, x + rx, y + rect->height);
		if (rx < rect->width / 2)
			sp_curve_lineto (c, x + rect->width - rx, y + rect->height);
		sp_curve_curveto (c, x + rect->width - rx * (1 - C1), y + rect->height, x + rect->width, y + rect->height - ry * (1 - C1), x + rect->width, y + rect->height - ry);
		if (ry < rect->height / 2)
			sp_curve_lineto (c, x + rect->width, y + ry);
		sp_curve_curveto (c, x + rect->width, y + ry * (1 - C1), x + rect->width - rx * (1 - C1), y + 0.0, x + rect->width - rx, y + 0.0);
		if (rx < rect->width / 2)
			sp_curve_lineto (c, x + rx, y + 0.0);
	} else {
		sp_curve_moveto (c, x + 0.0, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + rect->height);
		sp_curve_lineto (c, x + rect->width, y + rect->height);
		sp_curve_lineto (c, x + rect->width, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + 0.0);
	}

	sp_curve_closepath_current (c);
	sp_path_add_bpath (SP_PATH (rect), c, TRUE, NULL);
	sp_curve_unref (c);
}

void
sp_rect_set (SPRect * rect, gdouble x, gdouble y, gdouble width, gdouble height)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->x = x;
	rect->y = y;
	rect->width = width;
	rect->height = height;

	sp_rect_set_shape (rect);
}

static GSList * 
sp_rect_snappoints (SPItem * item, GSList * points)
{
  ArtPoint * p, p1, p2, p3, p4;
  gdouble affine[6];

  /* we use corners of rect only */
  p1.x = SP_RECT(item)->x;
  p1.y = SP_RECT(item)->y;
  p2.x = p1.x + SP_RECT(item)->width;
  p2.y = p1.y;
  p3.x = p1.x;
  p3.y = p1.y + SP_RECT(item)->height;
  p4.x = p1.x + SP_RECT(item)->width;
  p4.y = p1.y + SP_RECT(item)->height;
  sp_item_i2d_affine (item, affine);

  p = g_new (ArtPoint,1);
  art_affine_point (p, &p1, affine);
  points = g_slist_append (points, p);
  p = g_new (ArtPoint,1);
  art_affine_point (p, &p2, affine);
  points = g_slist_append (points, p);
  p = g_new (ArtPoint,1);
  art_affine_point (p, &p3, affine);
  points = g_slist_append (points, p);
  p = g_new (ArtPoint,1);
  art_affine_point (p, &p4, affine);
  points = g_slist_append (points, p);

  return points;
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static void
sp_rect_write_transform (SPItem *item, SPRepr *repr, gdouble *transform)
{
	SPRect *rect;
	gdouble rev[6];
	gdouble px, py, sw, sh;

	rect = SP_RECT (item);

	/* Calculate text start in parent coords */
	px = transform[0] * rect->x + transform[2] * rect->y + transform[4];
	py = transform[1] * rect->x + transform[3] * rect->y + transform[5];

	/* Clear translation */
	transform[4] = 0.0;
	transform[5] = 0.0;

	/* Scalers */
	sw = sqrt (transform[0] * transform[0] + transform[1] * transform[1]);
	sh = sqrt (transform[2] * transform[2] + transform[3] * transform[3]);
	if (sw > 1e-9) {
		transform[0] = transform[0] / sw;
		transform[1] = transform[1] / sw;
	} else {
		transform[0] = 1.0;
		transform[1] = 0.0;
	}
	if (sh > 1e-9) {
		transform[2] = transform[2] / sh;
		transform[3] = transform[3] / sh;
	} else {
		transform[2] = 0.0;
		transform[3] = 1.0;
	}
	sp_repr_set_double_attribute (repr, "width", rect->width * sw);
	sp_repr_set_double_attribute (repr, "height", rect->height * sh);
	sp_repr_set_double_attribute (repr, "rx", rect->rx * sw);
	sp_repr_set_double_attribute (repr, "ry", rect->ry * sh);

	/* Find start in item coords */
	art_affine_invert (rev, transform);
	sp_repr_set_double_attribute (repr, "x", px * rev[0] + py * rev[2]);
	sp_repr_set_double_attribute (repr, "y", px * rev[1] + py * rev[3]);

	if ((fabs (transform[0] - 1.0) > 1e-9) ||
	    (fabs (transform[3] - 1.0) > 1e-9) ||
	    (fabs (transform[1]) > 1e-9) ||
	    (fabs (transform[2]) > 1e-9)) {
		guchar t[80];
		sp_svg_write_affine (t, 80, transform);
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
	} else {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	}

}

/* Generate context menu item section */

static void
sp_rect_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Rect"));
	m = gtk_menu_new ();
	/* Link dialog */
	w = gtk_menu_item_new_with_label (_("Rect Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_rect_rect_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

static void
sp_rect_rect_properties (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	sp_object_attributes_dialog (SP_OBJECT (anchor), "SPRect");
}


