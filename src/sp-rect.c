#define __SP_RECT_C__

/*
 * SVG <rect> implementation
 *
 * Authors:
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
#include <libnr/nr-values.h>
#include <libnr/nr-macros.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>
#include "helper/art-utils.h"
#include "svg/svg.h"
#include "style.h"
#include "document.h"
#include "dialogs/object-attributes.h"
#include "sp-rect.h"

#define noRECT_VERBOSE

static void sp_rect_class_init (SPRectClass *class);
static void sp_rect_init (SPRect *rect);

static void sp_rect_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_rect_read_attr (SPObject *object, const gchar *attr);
static void sp_rect_modified (SPObject *object, guint flags);
static SPRepr *sp_rect_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_rect_description (SPItem * item);
static GSList * sp_rect_snappoints (SPItem * item, GSList * points);
static void sp_rect_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);
static void sp_rect_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_rect_rect_properties (GtkMenuItem *menuitem, SPAnchor *anchor);

static void sp_rect_glue_set_shape (SPShape * shape);
static void sp_rect_set_shape (SPRect * rect);
static SPKnotHolder *sp_rect_knot_holder (SPItem *item, SPDesktop *desktop);

static SPShapeClass *parent_class;

GtkType
sp_rect_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPRect",
			sizeof (SPRect),
			sizeof (SPRectClass),
			(GtkClassInitFunc) sp_rect_class_init,
			(GtkObjectInitFunc) sp_rect_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_SHAPE, &info);
	}
	return type;
}

static void
sp_rect_class_init (SPRectClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPShapeClass * shape_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;
	shape_class = (SPShapeClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	sp_object_class->build = sp_rect_build;
	sp_object_class->write = sp_rect_write;
	sp_object_class->read_attr = sp_rect_read_attr;
	sp_object_class->modified = sp_rect_modified;

	item_class->description = sp_rect_description;
	item_class->snappoints = sp_rect_snappoints;
	item_class->write_transform = sp_rect_write_transform;
	item_class->menu = sp_rect_menu;
	item_class->knot_holder = sp_rect_knot_holder;

	shape_class->set_shape = sp_rect_glue_set_shape;
}

static void
sp_rect_init (SPRect * rect)
{
	SP_PATH (rect) -> independent = FALSE;
	rect->x.set = FALSE;
	rect->x.computed = 0.0;
	rect->y.set = FALSE;
	rect->y.computed = 0.0;
	rect->width.set = FALSE;
	rect->width.computed = 0.0;
	rect->height.set = FALSE;
	rect->height.computed = 0.0;
	rect->rx.set = FALSE;
	rect->rx.computed = 0.0;
	rect->ry.set = FALSE;
	rect->ry.computed = 0.0;
}

static void
sp_rect_build (SPObject * object, SPDocument * document, SPRepr * repr)
{

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

	sp_rect_read_attr (object, "x");
	sp_rect_read_attr (object, "y");
	sp_rect_read_attr (object, "width");
	sp_rect_read_attr (object, "height");
	sp_rect_read_attr (object, "rx");
	sp_rect_read_attr (object, "ry");
}

static void
sp_rect_read_attr (SPObject *object, const gchar *attr)
{
	SPRect *rect;
	const gchar *str;
	gulong unit;

	rect = SP_RECT (object);

#ifdef RECT_VERBOSE
	g_print ("sp_rect_read_attr: attr %s\n", attr);
#endif

	str = sp_repr_attr (object->repr, attr);

	/* fixme: We should really collect updates */
	/* fixme: We need real error processing some time */

	if (!strcmp (attr, "x")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->x.value, &rect->x.computed)) {
			rect->x.set = TRUE;
			rect->x.unit = unit;
		} else {
			rect->x.set = FALSE;
			rect->x.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	} else if (!strcmp (attr, "y")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->y.value, &rect->y.computed)) {
			rect->y.set = TRUE;
			rect->y.unit = unit;
		} else {
			rect->y.set = FALSE;
			rect->y.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	} else if (!strcmp (attr, "width")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->width.value, &rect->width.computed) && (rect->width.value > 0.0)) {
			rect->width.set = TRUE;
			rect->width.unit = unit;
		} else {
			rect->width.set = FALSE;
			rect->width.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	} else if (!strcmp (attr, "height")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->height.value, &rect->height.computed) && (rect->height.value > 0.0)) {
			rect->height.set = TRUE;
			rect->height.unit = unit;
		} else {
			rect->height.set = FALSE;
			rect->height.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	} else if (!strcmp (attr, "rx")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->rx.value, &rect->rx.computed) && (rect->rx.value > 0.0)) {
			rect->rx.set = TRUE;
			rect->rx.unit = unit;
		} else {
			rect->rx.set = FALSE;
			rect->rx.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	} else if (!strcmp (attr, "ry")) {
		if (sp_svg_length_read_lff (str, &unit, &rect->ry.value, &rect->ry.computed) && rect->ry.value > 0.0) {
			rect->ry.set = TRUE;
			rect->ry.unit = unit;
		} else {
			rect->ry.set = FALSE;
			rect->ry.computed = 0.0;
		}
		sp_rect_set_shape (rect);
	}

	if (((SPObjectClass *) parent_class)->read_attr)
		((SPObjectClass *) parent_class)->read_attr (object, attr);

}

static void
sp_rect_modified (SPObject *object, guint flags)
{
	if ((flags & SP_OBJECT_STYLE_MODIFIED_FLAG) || (flags & SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
		sp_rect_set_shape (SP_RECT (object));
	}

	if (((SPObjectClass *) parent_class)->modified)
		((SPObjectClass *) parent_class)->modified (object, flags);
}

static SPRepr *
sp_rect_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPRect *rect;

	rect = SP_RECT (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("rect");
	}

	sp_repr_set_double (repr, "width", rect->width.computed);
	sp_repr_set_double (repr, "height", rect->height.computed);
	sp_repr_set_double_default (repr, "rx", rect->rx.computed, 0.0, 1e-6);
	sp_repr_set_double_default (repr, "ry", rect->ry.computed, 0.0, 1e-6);
	sp_repr_set_double (repr, "x", rect->x.computed);
	sp_repr_set_double (repr, "y", rect->y.computed);

	if (((SPObjectClass *) parent_class)->write)
		((SPObjectClass *) parent_class)->write (object, repr, flags);

	return repr;
}

static gchar *
sp_rect_description (SPItem * item)
{
	SPRect *rect;

	rect = SP_RECT (item);

	return g_strdup_printf ("Rectangle %g %g %g %g", rect->x.computed, rect->y.computed, rect->width.computed, rect->height.computed);
}

static void
sp_rect_glue_set_shape (SPShape *shape)
{
	SPRect *rect;

	rect = SP_RECT (shape);

	sp_rect_set_shape (rect);
}

static void
sp_rect_update_length (SPSVGLength *length, gdouble em, gdouble ex, gdouble scale)
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
sp_rect_compute_values (SPRect *rect)
{
	SPStyle *style;
	gdouble i2vp[6], vp2i[6];
	gdouble aw, ah;
	gdouble d;

	style = SP_OBJECT_STYLE (rect);

	/* fixme: It is somewhat dangerous, yes (Lauris) */
	/* fixme: And it is terribly slow too (Lauris) */
	/* fixme: In general we want to keep viewport scales around */
	sp_item_i2vp_affine (SP_ITEM (rect), i2vp);
	art_affine_invert (vp2i, i2vp);
	aw = sp_distance_d_matrix_d_transform (1.0, vp2i);
	ah = sp_distance_d_matrix_d_transform (1.0, vp2i);
	/* sqrt ((actual_width) ** 2 + (actual_height) ** 2)) / sqrt (2) */
	d = sqrt (aw * aw + ah * ah) * M_SQRT1_2;

	sp_rect_update_length (&rect->x, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_rect_update_length (&rect->y, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_rect_update_length (&rect->width, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_rect_update_length (&rect->height, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_rect_update_length (&rect->rx, style->font_size.computed, style->font_size.computed * 0.5, d);
	sp_rect_update_length (&rect->ry, style->font_size.computed, style->font_size.computed * 0.5, d);
}

#define C1 0.554

static void
sp_rect_set_shape (SPRect * rect)
{
	double x, y, w, h, w2, h2, rx, ry;
	SPCurve * c;
	
	sp_path_clear (SP_PATH (rect));

	/* fixme: Maybe track, whether we have em,ex,% (Lauris) */
	/* fixme: Alternately we can use ::modified to keep everything up-to-date (Lauris) */
	sp_rect_compute_values (rect);

	if ((rect->height.computed < 1e-18) || (rect->width.computed < 1e-18)) return;

	c = sp_curve_new ();

	x = rect->x.computed;
	y = rect->y.computed;
	w = rect->width.computed;
	h = rect->height.computed;
	w2 = w / 2;
	h2 = h / 2;
	if (rect->rx.set) {
		rx = CLAMP (rect->rx.computed, 0.0, rect->width.computed / 2);
	} else if (rect->ry.set) {
		rx = CLAMP (rect->ry.computed, 0.0, rect->width.computed / 2);
	} else {
		rx = 0.0;
	}
	if (rect->ry.set) {
		ry = CLAMP (rect->ry.computed, 0.0, rect->height.computed / 2);
	} else if (rect->rx.set) {
		ry = CLAMP (rect->rx.computed, 0.0, rect->height.computed / 2);
	} else {
		ry = 0.0;
	}

	if ((rx > 1e-18) && (ry > 1e-18)) {
		sp_curve_moveto (c, x + rx, y + 0.0);
		sp_curve_curveto (c, x + rx * (1 - C1), y + 0.0, x + 0.0, y + ry * (1 - C1), x + 0.0, y + ry);
		if (ry < h2) sp_curve_lineto (c, x + 0.0, y + h - ry);
		sp_curve_curveto (c, x + 0.0, y + h - ry * (1 - C1), x + rx * (1 - C1), y + h, x + rx, y + h);
		if (rx < w2) sp_curve_lineto (c, x + w - rx, y + h);
		sp_curve_curveto (c, x + w - rx * (1 - C1), y + h, x + w, y + h - ry * (1 - C1), x + w, y + h - ry);
		if (ry < h2) sp_curve_lineto (c, x + w, y + ry);
		sp_curve_curveto (c, x + w, y + ry * (1 - C1), x + w - rx * (1 - C1), y + 0.0, x + w - rx, y + 0.0);
		if (rx < w2) sp_curve_lineto (c, x + rx, y + 0.0);
	} else {
		sp_curve_moveto (c, x + 0.0, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + h);
		sp_curve_lineto (c, x + w, y + h);
		sp_curve_lineto (c, x + w, y + 0.0);
		sp_curve_lineto (c, x + 0.0, y + 0.0);
	}

	sp_curve_closepath_current (c);
	sp_path_add_bpath (SP_PATH (rect), c, TRUE, NULL);
	sp_curve_unref (c);
}

/* fixme: Think (Lauris) */

void
sp_rect_set (SPRect * rect, gdouble x, gdouble y, gdouble width, gdouble height)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->x.computed = x;
	rect->y.computed = y;
	rect->width.computed = width;
	rect->height.computed = height;

	sp_rect_set_shape (rect);
}

void
sp_rect_set_rx(SPRect * rect, gboolean set, gdouble value)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->rx.set = set;
	if (set)
		rect->rx.computed = value;
	sp_rect_set_shape (rect);
}

void
sp_rect_set_ry(SPRect * rect, gboolean set, gdouble value)
{
	g_return_if_fail (rect != NULL);
	g_return_if_fail (SP_IS_RECT (rect));

	rect->ry.set = set;
	if (set)
		rect->ry.computed = value;
	sp_rect_set_shape (rect);
}

static GSList * 
sp_rect_snappoints (SPItem *item, GSList *points)
{
	SPRect *rect;
	ArtPoint * p, p1, p2, p3, p4;
	gdouble affine[6];

	rect = SP_RECT (item);

	/* we use corners of rect only */
	p1.x = rect->x.computed;
	p1.y = rect->y.computed;
	p2.x = p1.x + rect->width.computed;
	p2.y = p1.y;
	p3.x = p1.x;
	p3.y = p1.y + rect->height.computed;
	p4.x = p2.x;
	p4.y = p3.y;

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

/* fixme: Use preferred units somehow (Lauris) */
/* fixme: Alternately preserve whatever units there are (lauris) */

static void
sp_rect_write_transform (SPItem *item, SPRepr *repr, gdouble *transform)
{
	SPRect *rect;
	gdouble rev[6];
	gdouble px, py, sw, sh;
	guchar t[80];
	SPStyle *style;

	rect = SP_RECT (item);

	/* Calculate rect start in parent coords */
	px = transform[0] * rect->x.computed + transform[2] * rect->y.computed + transform[4];
	py = transform[1] * rect->x.computed + transform[3] * rect->y.computed + transform[5];

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
	/* fixme: Would be nice to preserve units here */
	sp_repr_set_double_attribute (repr, "width", rect->width.computed * sw);
	sp_repr_set_double_attribute (repr, "height", rect->height.computed * sh);
	sp_repr_set_double_attribute (repr, "rx", rect->rx.computed * sw);
	sp_repr_set_double_attribute (repr, "ry", rect->ry.computed * sh);

	/* Find start in item coords */
	art_affine_invert (rev, transform);
	sp_repr_set_double_attribute (repr, "x", px * rev[0] + py * rev[2]);
	sp_repr_set_double_attribute (repr, "y", px * rev[1] + py * rev[3]);

	if (sp_svg_write_affine (t, 80, transform)) {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
	} else {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	}

	/* And last but not least */
	style = SP_OBJECT_STYLE (item);
	if (style->stroke.type != SP_PAINT_TYPE_NONE) {
		if (!NR_DF_TEST_CLOSE (sw, 1.0, NR_EPSILON_D) || !NR_DF_TEST_CLOSE (sh, 1.0, NR_EPSILON_D)) {
			double scale;
			guchar *str;
			/* Scale changed, so we have to adjust stroke width */
			scale = sqrt (fabs (sw * sh));
			style->stroke_width.computed *= scale;
			if (style->stroke_dash.n_dash != 0) {
				int i;
				for (i = 0; i < style->stroke_dash.n_dash; i++) style->stroke_dash.dash[i] *= scale;
				style->stroke_dash.offset *= scale;
			}
			str = sp_style_write_difference (style, SP_OBJECT_STYLE (SP_OBJECT_PARENT (item)));
			sp_repr_set_attr (SP_OBJECT_REPR (item), "style", str);
			g_free (str);
		}
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

static void
sp_rect_rx_get (SPItem *item, ArtPoint *p)
{
	SPRect *rect;

	rect = SP_RECT(item);

	p->x = rect->x.computed + rect->rx.computed;
	p->y = rect->y.computed;
}

static void
sp_rect_rx_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPRect *rect;

	rect = SP_RECT(item);
	
	if (state & GDK_CONTROL_MASK) {
		gdouble temp = MIN (rect->height.computed, rect->width.computed) / 2.0;
		rect->rx.computed = rect->ry.computed = CLAMP (p->x - rect->x.computed, 0.0, temp);
	} else {
		rect->rx.computed = CLAMP (p->x - rect->x.computed, 0.0, rect->width.computed / 2.0);
	}
}


static void
sp_rect_ry_get (SPItem *item, ArtPoint *p)
{
	SPRect *rect;

	rect = SP_RECT(item);

	p->x = rect->x.computed;
	p->y = rect->y.computed + rect->ry.computed;
}

static void
sp_rect_ry_set (SPItem *item, const ArtPoint *p, guint state)
{
	SPRect *rect;

	rect = SP_RECT(item);
	
	if (state & GDK_CONTROL_MASK) {
		gdouble temp = MIN (rect->height.computed, rect->width.computed) / 2.0;
		rect->rx.computed = rect->ry.computed = CLAMP (p->y - rect->y.computed, 0.0, temp);
	} else {
		rect->ry.computed = CLAMP (p->y - rect->y.computed, 0.0, rect->height.computed / 2.0);
	}
}


static SPKnotHolder *
sp_rect_knot_holder (SPItem *item, SPDesktop *desktop)
{
	SPRect *rect;
	SPKnotHolder *knot_holder;

	rect = SP_RECT (item);
	knot_holder = sp_knot_holder_new (desktop, item, NULL);
	
	sp_knot_holder_add (knot_holder, sp_rect_rx_set, sp_rect_rx_get);
	sp_knot_holder_add (knot_holder, sp_rect_ry_set, sp_rect_ry_get);
	
	return knot_holder;
}
