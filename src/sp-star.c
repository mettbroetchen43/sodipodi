#define __SP_STAR_C__

/*
 * SPStar
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Mitsuru Oka
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <math.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "svg/svg.h"
#include "sp-star.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "dialogs/object-attributes.h"

#define noSTAR_VERBOSE

static void sp_star_class_init (SPStarClass *class);
static void sp_star_init (SPStar *star);
static void sp_star_destroy (GtkObject *object);

static void sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_star_write_repr (SPObject * object, SPRepr * repr);
static void sp_star_read_attr (SPObject * object, const gchar * attr);

static void sp_star_bbox (SPItem * item, ArtDRect * bbox);
static SPKnotHolder *sp_star_knot_holder (SPItem * item, SPDesktop *desktop);
static gchar * sp_star_description (SPItem * item);
static GSList * sp_star_snappoints (SPItem * item, GSList * points);
static void sp_star_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_star_set_shape (SPShape *shape);

static void sp_star_star_properties (GtkMenuItem *menuitem, SPAnchor *anchor);

static SPPolygonClass *parent_class;

GtkType
sp_star_get_type (void)
{
	static GtkType star_type = 0;

	if (!star_type) {
		GtkTypeInfo star_info = {
			"SPStar",
			sizeof (SPStar),
			sizeof (SPStarClass),
			(GtkClassInitFunc) sp_star_class_init,
			(GtkObjectInitFunc) sp_star_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		star_type = gtk_type_unique (sp_polygon_get_type (), &star_info);
	}
	return star_type;
}

static void
sp_star_class_init (SPStarClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;
	SPShapeClass * shape_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;
	path_class = (SPPathClass *) class;
	shape_class = (SPShapeClass *) class;

	parent_class = gtk_type_class (sp_polygon_get_type ());

	gtk_object_class->destroy = sp_star_destroy;

	sp_object_class->build = sp_star_build;
	sp_object_class->write_repr = sp_star_write_repr;
	sp_object_class->read_attr = sp_star_read_attr;

	item_class->bbox = sp_star_bbox;
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
	star->sides = 3;
	star->center.x = 0.0;
	star->center.y = 0.0;
	star->r1    = star->r2 = 0.0;
	star->arg1  = star->arg2 = 0.0;
}

static void
sp_star_destroy (GtkObject *object)
{
	SPStar *star;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_STAR (object));

	star = SP_STAR (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_star_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);
	sp_star_read_attr (object, "sodipodi:sides");
	sp_star_read_attr (object, "sodipodi:cx");
	sp_star_read_attr (object, "sodipodi:cy");
	sp_star_read_attr (object, "sodipodi:r1");
	sp_star_read_attr (object, "sodipodi:r2");
	sp_star_read_attr (object, "sodipodi:arg1");
	sp_star_read_attr (object, "sodipodi:arg2");
}

static void
sp_star_write_repr (SPObject * object, SPRepr * repr)
{
	SPStar *star;

	star = SP_STAR (object);

	sp_repr_set_int_attribute (repr, "sodipodi:sides", star->sides);
	sp_repr_set_double_attribute (repr, "sodipodi:cx", star->center.x);
	sp_repr_set_double_attribute (repr, "sodipodi:cy", star->center.y);
	sp_repr_set_double_attribute (repr, "sodipodi:r1", star->r1);
	sp_repr_set_double_attribute (repr, "sodipodi:r2", star->r2);
	sp_repr_set_double_attribute (repr, "sodipodi:arg1", star->arg1);
	sp_repr_set_double_attribute (repr, "sodipodi:arg2", star->arg2);
	
	if (((SPObjectClass *) (parent_class))->write_repr)
		(*((SPObjectClass *) (parent_class))->write_repr) (object, repr);
}

static void
sp_star_read_attr (SPObject * object, const gchar * attr)
{
	SPShape *shape;
	SPStar * star;
	const gchar * astr;
	SPSVGUnit unit;
	double n;

	shape = SP_SHAPE (object);
	star = SP_STAR (object);

#ifdef STAR_VERBOSE
	g_print ("sp_star_read_attr: attr %s\n", attr);
#endif

	astr = sp_repr_attr (object->repr, attr);

	/* fixme: we should really collect updates */
	
	if (strcmp (attr, "sodipodi:sides") == 0) {
		if (astr)
			star->sides = (gint) strtol (astr, NULL, 10);
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:cx") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->center.x = n;
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:cy") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->center.y = n;
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:r1") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->r1 = n;
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:r2") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->r2 = n;
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:arg1") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->arg1 = n;
		sp_shape_set_shape (shape);
		return;
	} else if (strcmp (attr, "sodipodi:arg2") == 0) {
		n = sp_svg_read_length (&unit, astr, 0.0);
		star->arg2 = n;
		sp_shape_set_shape (shape);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static gchar *
sp_star_description (SPItem * item)
{
	return g_strdup ("Star");
}

static void
sp_star_set_shape (SPShape *shape)
{
	SPStar *star;
	gint   i;
	gint   sides;
	gdouble cx, cy, r1, r2, arg1, arg2, darg;
	SPCurve * c;
	
	star = SP_STAR (shape);

	sp_object_request_modified (SP_OBJECT (star), SP_OBJECT_MODIFIED_FLAG);

	sp_path_clear (SP_PATH (shape));
	
	if ((star->r1 < 1e-12) || (star->r2 < 1e-12)) return;
	if (star->sides < 3) return;
	
	c = sp_curve_new ();
	
	sides = star->sides;

	cx = star->center.x;
	cy = star->center.y;
	r1 = star->r1;
	r2 = star->r2;
	arg1 = star->arg1;
	arg2 = star->arg2;
	darg = 2.0*M_PI/(double)sides;
	
	/* i = 0 */
	sp_curve_moveto (c, cx + r1*cos(arg1), cy + r1*sin(arg1));
	sp_curve_lineto (c, cx + r2*cos(arg2), cy + r2*sin(arg2));
	for (i = 1; i<sides; i++) {
		arg1 += darg;
		arg2 += darg;
		sp_curve_lineto (c, cx + r1*cos(arg1), cy + r1*sin(arg1));
		sp_curve_lineto (c, cx + r2*cos(arg2), cy + r2*sin(arg2));
	}
	
	sp_curve_closepath (c);
	sp_path_add_bpath (SP_PATH (star), c, TRUE, NULL);
	sp_curve_unref (c);
}

static void
sp_star_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}

static void
sp_star_knot1_set (SPItem   *item,
		   const ArtPoint *p,
		   guint state)
{
	SPStar *star;
	gdouble dx, dy, arg1, darg1;

	star = SP_STAR (item);

	dx = p->x - star->center.x;
	dy = p->y - star->center.y;

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
sp_star_knot2_set (SPItem   *item,
		   const ArtPoint *p,
		   guint state)
{
	SPStar *star;
	gdouble dx, dy;

	star = SP_STAR (item);

	dx = p->x - star->center.x;
	dy = p->y - star->center.y;

	if (state & GDK_CONTROL_MASK) {
		star->r2   = hypot (dx, dy);
		star->arg2 = star->arg1 + M_PI/star->sides;
	} else {
		star->r2   = hypot (dx, dy);
		star->arg2 = atan2 (dy, dx);
	}
}

static void
sp_star_knot1_get (SPItem *item,
		   ArtPoint *p)
{
	SPStar *star;

	g_return_if_fail (item != NULL);
	g_return_if_fail (p != NULL);

	star = SP_STAR(item);

	p->x = star->r1 * cos(star->arg1) + star->center.x;
	p->y = star->r1 * sin(star->arg1) + star->center.y;
}

static void
sp_star_knot2_get (SPItem *item,
		   ArtPoint *p)
{
	SPStar *star;

	g_return_if_fail (item != NULL);
	g_return_if_fail (p != NULL);

	star = SP_STAR(item);

	p->x = star->r2 * cos(star->arg2) + star->center.x;
	p->y = star->r2 * sin(star->arg2) + star->center.y;
}

static SPKnotHolder *
sp_star_knot_holder (SPItem    *item,
		     SPDesktop *desktop)
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
	knot_holder = sp_knot_holder_new (desktop, item);
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
sp_star_set (SPStar * star,
	     gint sides,
	     gdouble cx, gdouble cy,
	     gdouble r1, gdouble r2,
	     gdouble arg1, gdouble arg2)
{
	g_return_if_fail (star != NULL);
	g_return_if_fail (SP_IS_STAR (star));
	
	star->sides = sides;
	star->center.x = cx;
	star->center.y = cy;
	star->r1 = r1;
	star->r2 = r2;
	star->arg1 = arg1;
	star->arg2 = arg2;
	
	sp_shape_set_shape (SP_SHAPE(star));
}

static GSList * 
sp_star_snappoints (SPItem * item, GSList * points)
{
	SPStar *star;
	ArtPoint * p, p1, p2;
	gdouble affine[6];
	
	star = SP_STAR(item);
	
	/* we use two points of star */
	p1 = p2 = star->center;
	p1.x += star->r1 * cos(star->arg1);
	p1.y += star->r1 * sin(star->arg1);
	p2.x += star->r2 * cos(star->arg2);
	p2.y += star->r2 * sin(star->arg2);
	sp_item_i2d_affine (item, affine);
	
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p1, affine);
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	art_affine_point (p, &p2, affine);
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	art_affine_point (p, &star->center, affine);
	points = g_slist_append (points, p);
	
	return points;
}

/* Generate context menu item section */

static void
sp_star_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

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

