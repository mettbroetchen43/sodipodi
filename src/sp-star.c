#define __SP_STAR_C__

/*
 * <sodipodi:star> implementation
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>
#include "svg/svg.h"
#include "attributes.h"
#include "sp-star.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "dialogs/object-attributes.h"
#include "helper/sp-intl.h"

#define noSTAR_VERBOSE

#define SP_EPSILON 1e-9

static void sp_star_class_init (SPStarClass *class);
static void sp_star_init (SPStar *star);

static void sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr);
static SPRepr *sp_star_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_star_set (SPObject *object, unsigned int key, const unsigned char *value);

static SPKnotHolder *sp_star_knot_holder (SPItem * item, SPDesktop *desktop);
static gchar * sp_star_description (SPItem * item);
static GSList * sp_star_snappoints (SPItem * item, GSList * points);
static void sp_star_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_star_set_shape (SPShape *shape);

static void sp_star_star_properties (GtkMenuItem *menuitem, SPAnchor *anchor);

static SPPolygonClass *parent_class;

GType
sp_star_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (SPStarClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_star_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPStar),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_star_init,
		};
		type = g_type_register_static (SP_TYPE_POLYGON, "SPStar", &info, 0);
	}
	return type;
}

static void
sp_star_class_init (SPStarClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;
	SPShapeClass * shape_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;
	path_class = (SPPathClass *) class;
	shape_class = (SPShapeClass *) class;

	parent_class = g_type_class_ref (SP_TYPE_POLYGON);

	sp_object_class->build = sp_star_build;
	sp_object_class->write = sp_star_write;
	sp_object_class->set = sp_star_set;

	item_class->knot_holder = sp_star_knot_holder;
	item_class->description = sp_star_description;
	item_class->snappoints = sp_star_snappoints;
	item_class->menu = sp_star_menu;

	shape_class->set_shape = sp_star_set_shape;
}

static void
sp_star_init (SPStar * star)
{
	SP_PATH (star)->independent = FALSE;

	star->sides = 5;
	star->cx = 0.0;
	star->cy = 0.0;
	star->r1 = 1.0;
	star->r2 = 0.001;
	star->arg1 = star->arg2 = 0.0;
}

static void
sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

	sp_object_read_attr (object, "sodipodi:cx");
	sp_object_read_attr (object, "sodipodi:cy");
	sp_object_read_attr (object, "sodipodi:sides");
	sp_object_read_attr (object, "sodipodi:r1");
	sp_object_read_attr (object, "sodipodi:r2");
	sp_object_read_attr (object, "sodipodi:arg1");
	sp_object_read_attr (object, "sodipodi:arg2");
}

static SPRepr *
sp_star_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPStar *star;

	star = SP_STAR (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("polygon");
	}

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "sodipodi:type", "star");
		sp_repr_set_int_attribute (repr, "sodipodi:sides", star->sides);
		sp_repr_set_double_attribute (repr, "sodipodi:cx", star->cx);
		sp_repr_set_double_attribute (repr, "sodipodi:cy", star->cy);
		sp_repr_set_double_attribute (repr, "sodipodi:r1", star->r1);
		sp_repr_set_double_attribute (repr, "sodipodi:r2", star->r2);
		sp_repr_set_double_attribute (repr, "sodipodi:arg1", star->arg1);
		sp_repr_set_double_attribute (repr, "sodipodi:arg2", star->arg2);
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags | SP_POLYGON_WRITE_POINTS);

	return repr;
}

static void
sp_star_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPShape *shape;
	SPStar *star;
	gulong unit;

	shape = SP_SHAPE (object);
	star = SP_STAR (object);

	/* fixme: we should really collect updates */
	switch (key) {
	case SP_ATTR_SODIPODI_SIDES:
		if (value) {
			star->sides = atoi (value);
			star->sides = CLAMP (star->sides, 3, 16);
		} else {
			star->sides = 5;
		}
		sp_shape_set_shape (shape);
		break;
	case SP_ATTR_SODIPODI_CX:
		if (!sp_svg_length_read_lff (value, &unit, NULL, &star->cx) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			star->cx = 0.0;
		}
		sp_shape_set_shape (shape);
		break;
	case SP_ATTR_SODIPODI_CY:
		if (!sp_svg_length_read_lff (value, &unit, NULL, &star->cy) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			star->cy = 0.0;
		}
		sp_shape_set_shape (shape);
		break;
	case SP_ATTR_SODIPODI_R1:
		if (!sp_svg_length_read_lff (value, &unit, NULL, &star->r1) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			star->r1 = 1.0;
		}
		/* fixme: Need CLAMP (Lauris) */
		sp_shape_set_shape (shape);
		break;
	case SP_ATTR_SODIPODI_R2:
		if (!sp_svg_length_read_lff (value, &unit, NULL, &star->r2) ||
		    (unit == SP_SVG_UNIT_EM) ||
		    (unit == SP_SVG_UNIT_EX) ||
		    (unit == SP_SVG_UNIT_PERCENT)) {
			star->r2 = 0.0;
		}
		sp_shape_set_shape (shape);
		return;
	case SP_ATTR_SODIPODI_ARG1:
		if (value) {
			star->arg1 = atof (value);
		} else {
			star->arg1 = 0.0;
		}
		sp_shape_set_shape (shape);
		break;
	case SP_ATTR_SODIPODI_ARG2:
		if (value) {
			star->arg2 = atof (value);
		} else {
			star->arg2 = 0.0;
		}
		sp_shape_set_shape (shape);
		break;
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static gchar *
sp_star_description (SPItem *item)
{
	SPStar *star;

	star = SP_STAR (item);

	return g_strdup_printf ("Star of %d sides", star->sides);
}

static void
sp_star_set_shape (SPShape *shape)
{
	SPStar *star;
	gint i;
	gint sides;
	SPCurve *c;
	ArtPoint p;
	
	star = SP_STAR (shape);

	sp_path_clear (SP_PATH (shape));
	
#if 0
	if (star->r1 < 1e-12) || (star->r2 < 1e-12)) return;
	if (star->sides < 3) return;
#endif
	
	c = sp_curve_new ();
	
	sides = star->sides;

	/* i = 0 */
	sp_star_get_xy (star, SP_STAR_POINT_KNOT1, 0, &p);
	sp_curve_moveto (c, p.x, p.y);
	sp_star_get_xy (star, SP_STAR_POINT_KNOT2, 0, &p);
	sp_curve_lineto (c, p.x, p.y);
	for (i = 1; i < sides; i++) {
		sp_star_get_xy (star, SP_STAR_POINT_KNOT1, i, &p);
		sp_curve_lineto (c, p.x, p.y);
		sp_star_get_xy (star, SP_STAR_POINT_KNOT2, i, &p);
		sp_curve_lineto (c, p.x, p.y);
	}
	
	sp_curve_closepath (c);
	sp_path_add_bpath (SP_PATH (star), c, TRUE, NULL);
	sp_curve_unref (c);

	sp_object_request_modified (SP_OBJECT (star), SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_star_knot1_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPStar *star;
	gdouble dx, dy, arg1, darg1;

	star = SP_STAR (item);

	dx = p->x - star->cx;
	dy = p->y - star->cy;

        arg1 = atan2 (dy, dx);
	darg1 = arg1 - star->arg1;
        
	if (state & GDK_CONTROL_MASK) {
		star->r1    = hypot(dx, dy);
	} else {		
		star->r1    = hypot(dx, dy);
		star->arg1  = arg1;
		star->arg2 += darg1;
	}
}

static void
sp_star_knot2_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPStar *star;
	gdouble dx, dy;

	star = SP_STAR (item);

	dx = p->x - star->cx;
	dy = p->y - star->cy;

	if (state & GDK_CONTROL_MASK) {
		star->r2   = hypot (dx, dy);
		star->arg2 = star->arg1 + M_PI/star->sides;
	} else {
		star->r2   = hypot (dx, dy);
		star->arg2 = atan2 (dy, dx);
	}
}

static void
sp_star_knot1_get (SPItem *item, ArtPoint *p)
{
	SPStar *star;

	g_return_if_fail (item != NULL);
	g_return_if_fail (p != NULL);

	star = SP_STAR(item);

	sp_star_get_xy (star, SP_STAR_POINT_KNOT1, 0, p);
}

static void
sp_star_knot2_get (SPItem *item, ArtPoint *p)
{
	SPStar *star;

	g_return_if_fail (item != NULL);
	g_return_if_fail (p != NULL);

	star = SP_STAR(item);

	sp_star_get_xy (star, SP_STAR_POINT_KNOT2, 0, p);
}

static SPKnotHolder *
sp_star_knot_holder (SPItem *item, SPDesktop *desktop)
{
	SPStar  *star;
	SPKnotHolder *knot_holder;
	
	star = SP_STAR(item);

#if 0
	/* if you want to get parent knot_holder, do it */
	if (SP_ITEM_CLASS(parent_class)->knot_holder)
		knot_holder = (* SP_ITEM_CLASS(parent_class)->knot_holder) (item);
#else
	/* we don't need to get parent knot_holder */
	knot_holder = sp_knot_holder_new (desktop, item, NULL);
#endif

	sp_knot_holder_add (knot_holder,
			    sp_star_knot1_set,
			    sp_star_knot1_get);
	sp_knot_holder_add (knot_holder,
			    sp_star_knot2_set,
			    sp_star_knot2_get);

	return knot_holder;
}

void
sp_star_position_set (SPStar *star, gint sides, gdouble cx, gdouble cy, gdouble r1, gdouble r2, gdouble arg1, gdouble arg2)
{
	g_return_if_fail (star != NULL);
	g_return_if_fail (SP_IS_STAR (star));
	
	star->sides = CLAMP (sides, 3, 16);
	star->cx = cx;
	star->cy = cy;
	star->r1 = MAX (r1, 0.001);
	star->r2 = CLAMP (r2, 0.0, star->r1);
	star->arg1 = arg1;
	star->arg2 = arg2;
	
	sp_shape_set_shape (SP_SHAPE(star));
}

static GSList * 
sp_star_snappoints (SPItem * item, GSList * points)
{
#if 0
	/* fixme: We should use all corners of star anyways (Lauris) */
	SPStar *star;
	ArtPoint * p, p1, p2, p3;
	gdouble affine[6];
	
	star = SP_STAR(item);
	
	/* we use two points of star */
	sp_star_get_xy (star, SP_STAR_POINT_KNOT1, 0, &p1);
	sp_star_get_xy (star, SP_STAR_POINT_KNOT2, 0, &p2);
	p3.x = star->cx; p3.y = star->cy;
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
#endif
	
	return points;
}

/**
 * sp_star_get_xy: Get X-Y value as item coordinate system
 * @star: star item
 * @point: point type to obtain X-Y value
 * @index: index of vertex
 * @p: pointer to store X-Y value
 *
 * Initial item coordinate system is same as document coordinate system.
 */

void
sp_star_get_xy (SPStar *star, SPStarPoint point, gint index, ArtPoint *p)
{
	gdouble arg, darg;

	darg = 2.0 * M_PI / (double) star->sides;

	switch (point) {
	case SP_STAR_POINT_KNOT1:
		arg = star->arg1 + index * darg;
		p->x = star->r1 * cos (arg) + star->cx;
		p->y = star->r1 * sin (arg) + star->cy;
		break;
	case SP_STAR_POINT_KNOT2:
		arg = star->arg2 + index * darg;
		p->x = star->r2 * cos (arg) + star->cx;
		p->y = star->r2 * sin (arg) + star->cy;
		break;
	}
}

/* Generate context menu item section */

static void
sp_star_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (((SPItemClass *) parent_class)->menu)
		((SPItemClass *) parent_class)->menu (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Star"));
	m = gtk_menu_new ();
	/* Link dialog */
	w = gtk_menu_item_new_with_label (_("Star Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_star_star_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

static void
sp_star_star_properties (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	sp_object_attributes_dialog (SP_OBJECT (anchor), "SPStar");
}

