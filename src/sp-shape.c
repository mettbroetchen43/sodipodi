#define __SP_SHAPE_C__

/*
 * Base class for shapes, including <path> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libnr/nr-pixblock.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>

#include "helper/sp-intl.h"
#include "helper/art-utils.h"
#include "svg/svg.h"
#include "dialogs/fill-style.h"
#include "display/nr-arena-shape.h"
#include "print.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-paint-server.h"
#include "style.h"
#include "sp-root.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);

static void sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_shape_release (SPObject *object);

static void sp_shape_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_shape_modified (SPObject *object, unsigned int flags);

#if 0
static SPRepr *sp_shape_write (SPObject *object, SPRepr *repr, guint flags);
#endif

static void sp_shape_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
void sp_shape_print (SPItem * item, SPPrintContext * ctx);
static NRArenaItem *sp_shape_show (SPItem *item, NRArena *arena, unsigned int key);

static void sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

#if 0
static gchar * sp_shape_description (SPItem * item);
static void sp_shape_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform);
#endif

static SPItemClass *parent_class;

unsigned int
sp_shape_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPShapeClass),
			NULL, NULL,
			(GClassInitFunc) sp_shape_class_init,
			NULL, NULL,
			sizeof (SPShape),
			16,
			(GInstanceInitFunc) sp_shape_init,
		};
		type = g_type_register_static (SP_TYPE_ITEM, "SPShape", &info, 0);
	}
	return type;
}

static void
sp_shape_class_init (SPShapeClass *klass)
{
	SPObjectClass *sp_object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;

	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	path_class = (SPPathClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	sp_object_class->build = sp_shape_build;
	sp_object_class->release = sp_shape_release;
	sp_object_class->update = sp_shape_update;
	sp_object_class->modified = sp_shape_modified;

	item_class->bbox = sp_shape_bbox;
	item_class->print = sp_shape_print;
	item_class->show = sp_shape_show;
	item_class->menu = sp_shape_menu;
}

static void
sp_shape_init (SPShape *shape)
{
	/* Nothing here */
}

/* fixme: Better place (Lauris) */

static guint
sp_shape_find_version (SPObject *object)
{

	while (object) {
		if (SP_IS_ROOT (object)) {
			return SP_ROOT (object)->sodipodi;
		}
		object = SP_OBJECT_PARENT (object);
	}

	return 0;
}

static void
sp_shape_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	guint version;

	version = sp_shape_find_version (object);

	if ((version > 0) && (version < 25)) {
		SPCSSAttr *css;
		const guchar *val;
		gdouble dval;
		gchar c[32];
		gboolean changed;
		/* Have to check for percentage opacities */
		css = sp_repr_css_attr (repr, "style");
		/* We force style rewrite at moment (Lauris) */
		changed = TRUE;
		val = sp_repr_css_property (css, "opacity", NULL);
		if (val && strchr (val, '%')) {
			dval = sp_svg_read_percentage (val, 1.0);
			g_snprintf (c, 32, "%g", dval);
			sp_repr_css_set_property (css, "opacity", c);
			changed = TRUE;
		}
		val = sp_repr_css_property (css, "fill-opacity", NULL);
		if (val && strchr (val, '%')) {
			dval = sp_svg_read_percentage (val, 1.0);
			g_snprintf (c, 32, "%g", dval);
			sp_repr_css_set_property (css, "fill-opacity", c);
			changed = TRUE;
		}
		val = sp_repr_css_property (css, "stroke-opacity", NULL);
		if (val && strchr (val, '%')) {
			dval = sp_svg_read_percentage (val, 1.0);
			g_snprintf (c, 32, "%g", dval);
			sp_repr_css_set_property (css, "stroke-opacity", c);
			changed = TRUE;
		}
		if (changed) sp_repr_css_set (repr, css, "style");
		sp_repr_css_attr_unref (css);
	}

	if (((SPObjectClass *) (parent_class))->build)
		(*((SPObjectClass *) (parent_class))->build) (object, document, repr);
}

static void
sp_shape_release (SPObject *object)
{
	SPShape *shape;

	shape = (SPShape *) object;

	if (shape->curve) {
		shape->curve = sp_curve_unref (shape->curve);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_shape_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	SPShape *shape;

	shape = SP_SHAPE (object);

	if (((SPObjectClass *) (parent_class))->update)
		(* ((SPObjectClass *) (parent_class))->update) (object, ctx, flags);

	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		SPItemView *v;
		NRRectF paintbox;
		/* This is suboptimal, because changing parent style schedules recalculation */
		/* But on the other hand - how can we know that parent does not tie style and transform */
		sp_item_invoke_bbox (SP_ITEM (object), &paintbox, NULL, TRUE);
		for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
			if (flags & SP_OBJECT_MODIFIED_FLAG) {
				nr_arena_shape_set_path (NR_ARENA_SHAPE (v->arenaitem), shape->curve, TRUE, NULL);
			}
			nr_arena_shape_set_paintbox (NR_ARENA_SHAPE (v->arenaitem), &paintbox);
		}
	}
}

static void
sp_shape_modified (SPObject *object, unsigned int flags)
{
	SPShape *shape;

	shape = SP_SHAPE (object);

	if (((SPObjectClass *) (parent_class))->modified)
		(* ((SPObjectClass *) (parent_class))->modified) (object, flags);

	if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
		SPItemView *v;
		for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
			nr_arena_shape_set_style (NR_ARENA_SHAPE (v->arenaitem), object->style);
		}
	}
}

static void
sp_shape_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPShape *shape;

	shape = SP_SHAPE (item);

	if (shape->curve) {
		NRMatrixF a;
		NRBPath bp;
		nr_matrix_f_from_d (&a, transform);
		bp.path = SP_CURVE_BPATH (shape->curve);
		nr_path_matrix_f_bbox_f_union (&bp, &a, bbox, 0.25);
	}
}

void
sp_shape_print (SPItem *item, SPPrintContext *ctx)
{
	SPShape *shape;
	NRRectF pbox, dbox, bbox;
	NRMatrixF i2d;

	shape = SP_SHAPE (item);

	if (!shape->curve) return;

	/* fixme: Think (Lauris) */
	sp_item_invoke_bbox (item, &pbox, NULL, TRUE);
	dbox.x0 = 0.0;
	dbox.y0 = 0.0;
	dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
	dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
	sp_item_bbox_desktop (item, &bbox);
	sp_item_i2d_affine (item, &i2d);

	if (SP_OBJECT_STYLE (item)->fill.type != SP_PAINT_TYPE_NONE) {
		NRBPath bp;
		bp.path = shape->curve->bpath;
		sp_print_fill (ctx, &bp, &i2d, SP_OBJECT_STYLE (item), &pbox, &dbox, &bbox);
	}

	if (SP_OBJECT_STYLE (item)->stroke.type != SP_PAINT_TYPE_NONE) {
		NRBPath bp;
		bp.path = shape->curve->bpath;
		sp_print_stroke (ctx, &bp, &i2d, SP_OBJECT_STYLE (item), &pbox, &dbox, &bbox);
	}
}

static NRArenaItem *
sp_shape_show (SPItem *item, NRArena *arena, unsigned int key)
{
	SPObject *object;
	SPShape *shape;
	NRRectF paintbox;
	NRArenaItem *arenaitem;

	object = SP_OBJECT (item);
	shape = SP_SHAPE (item);

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_SHAPE);
	nr_arena_shape_set_style (NR_ARENA_SHAPE (arenaitem), object->style);
	nr_arena_shape_set_path (NR_ARENA_SHAPE (arenaitem), shape->curve, TRUE, NULL);
	sp_item_invoke_bbox (item, &paintbox, NULL, TRUE);
	nr_arena_shape_set_paintbox (NR_ARENA_SHAPE (arenaitem), &paintbox);

	return arenaitem;
}

/* Generate context menu item section */

static void
sp_shape_fill_settings (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);

	sp_fill_style_dialog ();
}

static void
sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (((SPItemClass *) parent_class)->menu)
		((SPItemClass *) parent_class)->menu (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Shape"));
	m = gtk_menu_new ();
	/* Item dialog */
	w = gtk_menu_item_new_with_label (_("Fill settings"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_shape_fill_settings), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

/* Shape section */
void
sp_shape_set_shape (SPShape *shape)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_SHAPE (shape));

	if (SP_SHAPE_CLASS (G_OBJECT_GET_CLASS (shape))->set_shape)
		SP_SHAPE_CLASS (G_OBJECT_GET_CLASS (shape))->set_shape (shape);
}

void
sp_shape_set_curve (SPShape *shape, SPCurve *curve, unsigned int owner)
{
	if (shape->curve) {
		shape->curve = sp_curve_unref (shape->curve);
	}
	if (curve) {
		if (owner) {
			shape->curve = sp_curve_ref (curve);
		} else {
			shape->curve = sp_curve_copy (curve);
		}
	}
	sp_object_request_update (SP_OBJECT (shape), SP_OBJECT_MODIFIED_FLAG);
}

/* Return duplicate of curve or NULL */
SPCurve *
sp_shape_get_curve (SPShape *shape)
{
	if (shape->curve) {
		return sp_curve_copy (shape->curve);
	}
	return NULL;
}

/* NOT FOR GENERAL PUBLIC UNTIL SORTED OUT (Lauris) */
void
sp_shape_set_curve_insync (SPShape *shape, SPCurve *curve, unsigned int owner)
{
	if (shape->curve) {
		shape->curve = sp_curve_unref (shape->curve);
	}
	if (curve) {
		if (owner) {
			shape->curve = sp_curve_ref (curve);
		} else {
			shape->curve = sp_curve_copy (curve);
		}
	}
}

#if 0
static SPRepr *
sp_shape_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPPath *path;

	path = SP_PATH (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		if (!path->comp) return NULL;
		repr = sp_repr_new ("path");
	}

	if (path->independent || (flags & SP_SHAPE_WRITE_PATH)) {
		SPPathComp *pathcomp;
		ArtBpath *abp;
		gchar *str;

		/* fixme: Here used to be [g_assert (path->comp)] (Lauris) */

		pathcomp = path->comp->data;
		g_assert (pathcomp);
		abp = sp_curve_first_bpath (pathcomp->curve);
		str = sp_svg_write_path (abp);
		g_assert (str != NULL);
		sp_repr_set_attr (repr, "d", str);
		g_free (str);
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}
#endif

#if 0
static void
sp_shape_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform)
{
	SPPath *path;
	SPShape *shape;

	path = SP_PATH (item);
	shape = SP_SHAPE (item);

	if (path->independent) {
		SPPathComp *comp;
		NRBPath dpath, spath;
		NRMatrixF ctm;
		double ex;
		gchar *svgpath;
		SPStyle *style;
		ex = NR_MATRIX_DF_EXPANSION (transform);
		comp = (SPPathComp *) path->comp->data;
		nr_matrix_multiply_fdf (&ctm, NR_MATRIX_D_FROM_DOUBLE (comp->affine), &item->transform);
		spath.path = comp->curve->bpath;
		nr_path_duplicate_transform (&dpath, &spath, &ctm);
		svgpath = sp_svg_write_path (dpath.path);
		sp_repr_set_attr (repr, "d", svgpath);
		g_free (svgpath);
		nr_free (dpath.path);
		/* And last but not least */
		style = SP_OBJECT_STYLE (item);
		if (style->stroke.type != SP_PAINT_TYPE_NONE) {
			if (!NR_DF_TEST_CLOSE (ex, 1.0, NR_EPSILON_D)) {
				guchar *str;
				/* Scale changed, so we have to adjust stroke width */
				style->stroke_width.computed *= ex;
				if (style->stroke_dash.n_dash != 0) {
					int i;
					for (i = 0; i < style->stroke_dash.n_dash; i++) style->stroke_dash.dash[i] *= ex;
					style->stroke_dash.offset *= ex;
				}
				str = sp_style_write_difference (style, SP_OBJECT_STYLE (SP_OBJECT_PARENT (item)));
				sp_repr_set_attr (repr, "style", str);
				g_free (str);
			}
		}
		sp_repr_set_attr (repr, "transform", NULL);
	} else {
		guchar t[80];
		if (sp_svg_transform_write (t, 80, &item->transform)) {
			sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
		} else {
			sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
		}
	}
}
#endif

#if 0
void
sp_shape_remove_comp (SPPath *path, SPPathComp *comp)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_shape_set_path (NR_ARENA_SHAPE (v->arenaitem), NULL, FALSE, NULL);
	}

	if (SP_PATH_CLASS (parent_class)->remove_comp)
		SP_PATH_CLASS (parent_class)->remove_comp (path, comp);
}

void
sp_shape_add_comp (SPPath *path, SPPathComp *comp)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_shape_set_path (NR_ARENA_SHAPE (v->arenaitem), comp->curve, comp->private, comp->affine);
	}

	if (SP_PATH_CLASS (parent_class)->add_comp)
		SP_PATH_CLASS (parent_class)->add_comp (path, comp);
}

void
sp_shape_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
#if 0
		cs = (SPCanvasShape *) v->canvasitem;
		sp_canvas_shape_change_bpath (cs, comp->curve);
#endif
	}

	if (SP_PATH_CLASS (parent_class)->change_bpath)
		SP_PATH_CLASS (parent_class)->change_bpath (path, comp, curve);
}
#endif

