#define SP_RECT_C

#include <gnome.h>
#include "sp-rect.h"

#define noRECT_VERBOSE

static void sp_rect_class_init (SPRectClass *class);
static void sp_rect_init (SPRect *rect);
static void sp_rect_destroy (GtkObject *object);

static void sp_rect_bbox (SPItem * item, ArtDRect * bbox);
static gchar * sp_rect_description (SPItem * item);
static void sp_rect_read (SPItem * item, SPRepr * repr);
static void sp_rect_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);

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
	GtkObjectClass *object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	object_class->destroy = sp_rect_destroy;

	item_class->bbox = sp_rect_bbox;
	item_class->description = sp_rect_description;
	item_class->read = sp_rect_read;
	item_class->read_attr = sp_rect_read_attr;
}

static void
sp_rect_init (SPRect * rect)
{
	SP_PATH (rect) -> independent = FALSE;
	rect->x = rect->y = 0.0;
	rect->width = rect->height = 0.0;
	rect->rx = rect->ry = 0.0;
	rect->bpath = NULL;
}

static void
sp_rect_destroy (GtkObject *object)
{
	SPRect *rect;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_RECT (object));

	rect = SP_RECT (object);

#if 0
	/* fixme: We should make shape with private component ;-) */
	if (rect->bpath)
		art_free (rect->bpath);
#endif

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static gchar *
sp_rect_description (SPItem * item)
{
	return g_strdup (_("Rectangle"));
}

#define C1 0.554

static ArtBpath *
rect_moveto (ArtBpath * bpath, gdouble x, gdouble y)
{
	bpath->code = ART_MOVETO;
	bpath->x3 = x;
	bpath->y3 = y;
	return ++bpath;
}

static ArtBpath *
rect_lineto (ArtBpath * bpath, gdouble x, gdouble y)
{
	bpath->code = ART_LINETO;
	bpath->x3 = x;
	bpath->y3 = y;
	return ++bpath;
}

static ArtBpath *
rect_curveto (ArtBpath * bpath, gdouble x1, gdouble y1, gdouble x2, gdouble y2, gdouble x3, gdouble y3)
{
	bpath->code = ART_CURVETO;
	bpath->x1 = x1;
	bpath->y1 = y1;
	bpath->x2 = x2;
	bpath->y2 = y2;
	bpath->x3 = x3;
	bpath->y3 = y3;
	return ++bpath;
}

static ArtBpath *
rect_end (ArtBpath * bpath)
{
	bpath->code = ART_END;
	return ++bpath;
}

static void
sp_rect_set_shape (SPRect * rect)
{
	double x, y, rx, ry;
	ArtBpath * p, * bpath;
	
	bpath = art_new (ArtBpath, 32);
	p = bpath;

	x = rect->x;
	y = rect->y;

	if ((rect->rx != 0.0) && (rect->ry != 0.0)) {
		rx = rect->rx;
		ry = rect->ry;
		if (rx > rect->width / 2) rx = rect->width / 2;
		if (ry > rect->height / 2) ry = rect->height / 2;

		p = rect_moveto (p, x + rx, y + 0.0);
		p = rect_curveto (p, x + rx * (1 - C1), y + 0.0, x + 0.0, y + ry * (1 - C1), x + 0.0, y + ry);
		if (ry < rect->height / 2)
			p = rect_lineto (p, x + 0.0, y + rect->height - ry);
		p = rect_curveto (p, x + 0.0, y + rect->height - ry * (1 - C1), x + rx * (1 - C1), y + rect->height, x + rx, y + rect->height);
		if (rx < rect->width / 2)
			p = rect_lineto (p, x + rect->width - rx, y + rect->height);
		p = rect_curveto (p, x + rect->width - rx * (1 - C1), y + rect->height, x + rect->width, y + rect->height - ry * (1 - C1), x + rect->width, y + rect->height - ry);
		if (ry < rect->height / 2)
			p = rect_lineto (p, x + rect->width, y + ry);
		p = rect_curveto (p, x + rect->width, y + ry * (1 - C1), x + rect->width - rx * (1 - C1), y + 0.0, x + rect->width - rx, y + 0.0);
		if (rx < rect->width / 2)
			p = rect_lineto (p, x + rx, y + 0.0);
	} else {
		p = rect_moveto (p, x + 0.0, y + 0.0);
		p = rect_lineto (p, x + 0.0, y + rect->height);
		p = rect_lineto (p, x + rect->width, y + rect->height);
		p = rect_lineto (p, x + rect->width, y + 0.0);
		p = rect_lineto (p, x + 0.0, y + 0.0);
	}

	p = rect_end (p);

	sp_path_clear (SP_PATH (rect));
#if 0
	if (rect->bpath) art_free (rect->bpath);
#endif
	rect->bpath = bpath;
	sp_path_add_bpath (SP_PATH (rect), bpath, TRUE, NULL);
}

static void
sp_rect_read (SPItem * item, SPRepr * repr)
{

	if (SP_ITEM_CLASS(parent_class)->read)
		(* SP_ITEM_CLASS(parent_class)->read) (item, repr);

	sp_rect_read_attr (item, repr, "x");
	sp_rect_read_attr (item, repr, "y");
	sp_rect_read_attr (item, repr, "width");
	sp_rect_read_attr (item, repr, "height");
	sp_rect_read_attr (item, repr, "rx");
	sp_rect_read_attr (item, repr, "ry");
}

static void
sp_rect_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	SPRect * rect;
	double n;

	rect = SP_RECT (item);

#ifdef RECT_VERBOSE
g_print ("sp_rect_read_attr: attr %s\n", attr);
#endif

	/* fixme: we should really collect updates */

	if (strcmp (attr, "x") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->x);
		rect->x = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->y);
		rect->y = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "width") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->width);
		rect->width = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "height") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->height);
		rect->height = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "rx") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->rx);
		rect->rx = n;
		sp_rect_set_shape (rect);
		return;
	}
	if (strcmp (attr, "ry") == 0) {
		n = sp_repr_get_double_attribute (repr, attr, rect->ry);
		rect->ry = n;
		sp_rect_set_shape (rect);
		return;
	}

	if (SP_ITEM_CLASS (parent_class)->read_attr)
		SP_ITEM_CLASS (parent_class)->read_attr (item, repr, attr);

}

static void
sp_rect_bbox (SPItem * item, ArtDRect * bbox)
{
	if (SP_ITEM_CLASS(parent_class)->bbox)
		(* SP_ITEM_CLASS(parent_class)->bbox) (item, bbox);
}

