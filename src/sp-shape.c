#define SP_SHAPE_C

#include <gnome.h>
#include "display/canvas-shape.h"
#include "sp-path-component.h"
#include "sp-shape-style.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);
static void sp_shape_destroy (GtkObject *object);

static void sp_shape_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_shape_description (SPItem * item);
static void sp_shape_read (SPItem * item, SPRepr * repr);
static void sp_shape_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);
static GnomeCanvasItem * sp_shape_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);

static void sp_shape_remove_comp (SPPath * path, SPPathComp * comp);
static void sp_shape_add_comp (SPPath * path, SPPathComp * comp);
static void sp_shape_change_bpath (SPPath * path, SPPathComp * comp, ArtBpath * bpath);

static SPPathClass * parent_class;

GtkType
sp_shape_get_type (void)
{
	static GtkType shape_type = 0;

	if (!shape_type) {
		GtkTypeInfo shape_info = {
			"SPShape",
			sizeof (SPShape),
			sizeof (SPShapeClass),
			(GtkClassInitFunc) sp_shape_class_init,
			(GtkObjectInitFunc) sp_shape_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		shape_type = gtk_type_unique (sp_path_get_type (), &shape_info);
	}
	return shape_type;
}

static void
sp_shape_class_init (SPShapeClass * klass)
{
	GtkObjectClass * object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	path_class = (SPPathClass *) klass;

	parent_class = gtk_type_class (sp_path_get_type ());

	object_class->destroy = sp_shape_destroy;

	item_class->print = sp_shape_print;
	item_class->description = sp_shape_description;
	item_class->read = sp_shape_read;
	item_class->read_attr = sp_shape_read_attr;
	item_class->show = sp_shape_show;

	path_class->remove_comp = sp_shape_remove_comp;
	path_class->add_comp = sp_shape_add_comp;
	path_class->change_bpath = sp_shape_change_bpath;
}

static void
sp_shape_init (SPShape *shape)
{
	shape->fill = sp_fill_default ();
	sp_fill_ref (shape->fill);
	shape->stroke = sp_stroke_default ();
	sp_stroke_ref (shape->stroke);
}

static void
sp_shape_destroy (GtkObject *object)
{
	SPShape *shape;

	shape = SP_SHAPE (object);

	if (shape->fill)
		sp_fill_unref (shape->fill);
	if (shape->stroke)
		sp_stroke_unref (shape->stroke);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_shape_print (SPItem * item, GnomePrintContext * gpc)
{

	double r, g, b;

	SPPath *path;
	SPShape * shape;
	SPPathComp * comp;
	GSList * l;
	ArtBpath * bpath;

	gnome_print_gsave (gpc);

	path = SP_PATH (item);
	shape = SP_SHAPE (item);

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		if (comp->bpath != NULL) {
			gnome_print_gsave (gpc);
			gnome_print_concat (gpc, comp->affine);
			bpath = comp->bpath;

			gnome_print_newpath (gpc);
			gnome_print_moveto (gpc,
				bpath->x3,
				bpath->y3);

			for (bpath = bpath + 1; bpath->code != ART_END; bpath++) {
				switch (bpath->code) {
					case ART_MOVETO:
						gnome_print_closepath (gpc);
						gnome_print_moveto (gpc,
							bpath->x3,
							bpath->y3);
						break;
					case ART_MOVETO_OPEN:
						gnome_print_moveto (gpc,
							bpath->x3,
							bpath->y3);
						break;
					case ART_LINETO:
						gnome_print_lineto (gpc,
							bpath->x3,
							bpath->y3);
						break;
					case ART_CURVETO:
						gnome_print_curveto (gpc,
							bpath->x1,
							bpath->y1,
							bpath->x2,
							bpath->y2,
							bpath->x3,
							bpath->y3);
						break;
					default:
						break;
				}
			}
			gnome_print_closepath (gpc);

			if (shape->fill->type == SP_FILL_COLOR) {
				r = (double) ((shape->fill->color >> 24) & 0xff) / 255.0;
				g = (double) ((shape->fill->color >> 16) & 0xff) / 255.0;
				b = (double) ((shape->fill->color >>  8) & 0xff) / 255.0;
				gnome_print_gsave (gpc);
				gnome_print_setrgbcolor (gpc, r, g, b);
				gnome_print_eofill (gpc);
				gnome_print_grestore (gpc);
			}
			if (shape->stroke->type == SP_STROKE_COLOR) {
				r = (double) ((shape->stroke->color >> 24) & 0xff) / 255.0;
				g = (double) ((shape->stroke->color >> 16) & 0xff) / 255.0;
				b = (double) ((shape->stroke->color >>  8) & 0xff) / 255.0;
				gnome_print_setrgbcolor (gpc, r, g, b);

				gnome_print_setlinewidth (gpc, shape->stroke->width);
				gnome_print_setlinejoin (gpc, shape->stroke->join);
				gnome_print_setlinecap (gpc, shape->stroke->cap);
				gnome_print_gsave (gpc);
				gnome_print_stroke (gpc);
				gnome_print_grestore (gpc);
			}
		}
		gnome_print_grestore (gpc);
	}
	gnome_print_grestore (gpc);
}

static gchar *
sp_shape_description (SPItem * item)
{
	return g_strdup (_("A path - whatever it means"));
}

static void
sp_shape_read (SPItem * item, SPRepr * repr)
{
	if (SP_ITEM_CLASS (parent_class)->read)
		SP_ITEM_CLASS (parent_class)->read (item, repr);

	sp_shape_read_attr (item, repr, "style");
}

static void
sp_shape_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	SPShape * shape;
	SPCSSAttr * css;
	SPFill * fill;
	SPStroke * stroke;
	SPCanvasShape * cs;
	GSList * l;

	shape = SP_SHAPE (item);

#ifdef SHAPE_VERBOSE
g_print ("sp_shape_read_attr: %s\n", attr);
#endif

	if (strcmp (attr, "style") == 0) {
		css = sp_repr_css_attr_inherited (repr, attr);
		fill = sp_fill_new ();
		stroke = sp_stroke_new ();
		sp_fill_read (fill, css);
		sp_stroke_read (stroke, css);
		sp_repr_css_attr_unref (css);
		sp_fill_unref (shape->fill);
		shape->fill = fill;
		sp_stroke_unref (shape->stroke);
		shape->stroke = stroke;

		for (l = item->display; l != NULL; l = l->next) {
			cs = SP_CANVAS_SHAPE (l->data);
			sp_canvas_shape_set_fill (cs, shape->fill);
			sp_canvas_shape_set_stroke (cs, shape->stroke);
		}
		return;
	}

	if (SP_ITEM_CLASS (parent_class)->read_attr)
		SP_ITEM_CLASS (parent_class)->read_attr (item, repr, attr);
}

static GnomeCanvasItem *
sp_shape_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	SPShape * shape;
	SPPath * path;
	SPCanvasShape * cs;
	SPPathComp * comp;
	GSList * l;

	shape = SP_SHAPE (item);
	path = SP_PATH (item);

	cs = (SPCanvasShape *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_SHAPE, NULL);
	g_return_val_if_fail (cs != NULL, NULL);

	sp_canvas_shape_set_fill (cs, shape->fill);
	sp_canvas_shape_set_stroke (cs, shape->stroke);

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		sp_canvas_shape_add_component (cs, comp->bpath, comp->private, comp->affine);
	}

	return (GnomeCanvasItem *) cs;
}

static void
sp_shape_remove_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	GSList * l;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (l = item->display; l != NULL; l = l->next) {
		cs = (SPCanvasShape *) l->data;
		sp_canvas_shape_clear (cs);
	}

	if (SP_PATH_CLASS (parent_class)->remove_comp)
		SP_PATH_CLASS (parent_class)->remove_comp (path, comp);
}

static void
sp_shape_add_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	GSList * l;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	for (l = item->display; l != NULL; l = l->next) {
		cs = (SPCanvasShape *) l->data;
		sp_canvas_shape_add_component (cs, comp->bpath, comp->private, comp->affine);
	}

	if (SP_PATH_CLASS (parent_class)->add_comp)
		SP_PATH_CLASS (parent_class)->add_comp (path, comp);
}

static void
sp_shape_change_bpath (SPPath * path, SPPathComp * comp, ArtBpath * bpath)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	GSList * l;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (l = item->display; l != NULL; l = l->next) {
		cs = (SPCanvasShape *) l->data;
		sp_canvas_shape_change_bpath (cs, comp->bpath);
	}

	if (SP_PATH_CLASS (parent_class)->change_bpath)
		SP_PATH_CLASS (parent_class)->change_bpath (path, comp, bpath);
}

#if 0
/* Utility */

void
sp_shape_add_bpath (SPShape * shape, ArtBpath * bpath, double affine[])
{
	SPItem * item;
	SPCanvasShape * cs;
	SPPathComp * comp;
	GSList * l;

	g_assert (shape != NULL);
	g_assert (bpath != NULL);

	item = (SPItem *) shape;

	for (l = item->display; l != NULL; l = l->next) {
		cs = SP_CANVAS_SHAPE (l->data);
		sp_canvas_shape_add_component (cs, bpath, FALSE, affine);
	}

	comp = sp_path_comp_new (bpath, FALSE, affine);
	sp_path_add_component (SP_PATH (shape), comp);
}

void
sp_shape_add_bpath_identity (SPShape * shape, ArtBpath * bpath)
{
	double affine[6];

	art_affine_identity (affine);

	sp_shape_add_bpath (shape, bpath, affine);
}

void
sp_shape_set_bpath (SPShape * shape, ArtBpath * bpath, double affine[])
{
	sp_shape_clear (shape);
	sp_shape_add_bpath (shape, bpath, affine);
}

void
sp_shape_set_bpath_identity (SPShape * shape, ArtBpath * bpath)
{
	sp_shape_clear (shape);
	sp_shape_add_bpath_identity (shape, bpath);
}
#endif

#if 0
void
sp_shape_set_stroke (SPShape * shape, SPStroke * stroke)
{
	SPStroke * s;

	s = shape->stroke;
	shape->stroke = stroke;
	sp_stroke_ref (stroke);
	sp_stroke_unref (s);

	gnome_canvas_item_request_update ((GnomeCanvasItem *) shape);
}
#endif

#if 0
void
sp_shape_clear (SPShape * shape)
{
	SPItem * item;
	SPCanvasShape * cs;
	GSList * l;

	item = (SPItem *) shape;

	for (l = item->display; l != NULL; l = l->next) {
		cs = SP_CANVAS_SHAPE (l->data);
		sp_canvas_shape_clear (cs);
	}

	sp_path_clear (SP_PATH (shape));
}
#endif

/* Unimplemented */

void
sp_shape_set_stroke (SPShape * shape, SPStroke * stroke)
{
	g_print ("unimplemented: sp_shape_set_stroke\n");
}
