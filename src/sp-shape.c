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
#include <math.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libnr/nr-pixblock.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>
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
static void sp_shape_modified (SPObject *object, guint flags);
static void sp_shape_style_modified (SPObject *object, guint flags);
static SPRepr *sp_shape_write (SPObject *object, SPRepr *repr, guint flags);

void sp_shape_print (SPItem * item, SPPrintContext * ctx);
static gchar * sp_shape_description (SPItem * item);
static NRArenaItem *sp_shape_show (SPItem *item, NRArena *arena);
static void sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

void sp_shape_remove_comp (SPPath * path, SPPathComp * comp);
void sp_shape_add_comp (SPPath * path, SPPathComp * comp);
void sp_shape_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve);

static SPPathClass * parent_class;

GType
sp_shape_get_type (void)
{
	static GType shape_type = 0;

	if (!shape_type) {
		GTypeInfo shape_info = {
			sizeof (SPShapeClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_shape_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPShape),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_shape_init,
		};
		shape_type = g_type_register_static (SP_TYPE_PATH, "SPShape", &shape_info, 0);
	}
	return shape_type;
}

static void
sp_shape_class_init (SPShapeClass * klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	path_class = (SPPathClass *) klass;

	parent_class = gtk_type_class (sp_path_get_type ());

	sp_object_class->build = sp_shape_build;
	sp_object_class->write = sp_shape_write;
	sp_object_class->modified = sp_shape_modified;
	sp_object_class->style_modified = sp_shape_style_modified;

	item_class->print = sp_shape_print;
	item_class->description = sp_shape_description;
	item_class->show = sp_shape_show;
	item_class->menu = sp_shape_menu;

	path_class->remove_comp = sp_shape_remove_comp;
	path_class->add_comp = sp_shape_add_comp;
	path_class->change_bpath = sp_shape_change_bpath;

	klass->set_shape = NULL;
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
		/* We foce style rewrite at moment (Lauris) */
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

#if 0
	if ((version > 0) && (version < 25)) {
		SPStyle *style;
		/* Potential invalid gradient position */
		style = object->style;
		if (style->fill.type = SP_PAINT_TYPE_PAINTSERVER) {
			if (SP_IS_LINEARGRADIENT (SP_STYLE_FILL_SERVER (style))) {
				SPLinearGradient *lg;
				lg = SP_LINEARGRADIENT (SP_STYLE_FILL_SERVER (style));
				if (SP_GRADIENT (lg)->href && (SP_GRADIENT (lg)->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX)) {
					/* Let's assume it is normalized private gradient */
					/* Item is built, so bbox should qualify */
				}
			}
		}
	}
#endif
}

static SPRepr *
sp_shape_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPPath *path;

	path = SP_PATH (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("path");
	}

	if (path->independent || (flags & SP_SHAPE_WRITE_PATH)) {
		SPPathComp *pathcomp;
		ArtBpath *abp;
		gchar *str;
		g_assert (path->comp);
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

static void
sp_shape_modified (SPObject *object, guint flags)
{
	SPShape *shape;
	SPItemView *v;

	shape = SP_SHAPE (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->modified)
		(* ((SPObjectClass *) (parent_class))->modified) (object, flags);

	if ((flags & SP_OBJECT_MODIFIED_FLAG) | (flags & SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		NRRectF paintbox;
		/* This is suboptimal, because changing parent style schedules recalculation */
		/* But on the other hand - how can we know that parent does not tie style and transform */
		sp_item_invoke_bbox (SP_ITEM (object), &paintbox, NULL, TRUE);
		for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
			nr_arena_shape_set_paintbox (NR_ARENA_SHAPE (v->arenaitem), &paintbox);
		}
	}
}

static void
sp_shape_style_modified (SPObject *object, guint flags)
{
	SPShape *shape;
	SPItemView *v;

	shape = SP_SHAPE (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->style_modified)
		(* ((SPObjectClass *) (parent_class))->style_modified) (object, flags);

	for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
		nr_arena_shape_set_style (NR_ARENA_SHAPE (v->arenaitem), object->style);
	}
}

void
sp_shape_print (SPItem *item, SPPrintContext *ctx)
{
	SPPath *path;
	SPPathComp *comp;
	NRRectF pbox, dbox, bbox;
	NRMatrixF i2d;

	path = SP_PATH (item);

	if (!path->comp) return;
	comp = path->comp->data;
	if (!comp->curve) return;

	/* fixme: Think (Lauris) */
	sp_item_invoke_bbox (item, &pbox, NULL, TRUE);
	dbox.x0 = 0.0;
	dbox.y0 = 0.0;
	dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
	dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
	sp_item_bbox_desktop (item, &bbox);
	sp_item_i2d_affine (item, &i2d);

	if (SP_OBJECT_STYLE (item)->fill.type != SP_PAINT_TYPE_NONE) {
		NRMatrixF ctm;
		NRBPath bp;
		nr_matrix_multiply_fdf (&ctm, (NRMatrixD *) comp->affine, &i2d);
		bp.path = comp->curve->bpath;
		sp_print_fill (ctx, &bp, &ctm, SP_OBJECT_STYLE (item), &pbox, &dbox, &bbox);
	}

	if (SP_OBJECT_STYLE (item)->stroke.type != SP_PAINT_TYPE_NONE) {
		NRMatrixF ctm;
		NRBPath bp;
		nr_matrix_multiply_fdf (&ctm, (NRMatrixD *) comp->affine, &i2d);
		bp.path = comp->curve->bpath;
		sp_print_stroke (ctx, &bp, &ctm, SP_OBJECT_STYLE (item), &pbox, &dbox, &bbox);
	}
}

static gchar *
sp_shape_description (SPItem * item)
{
	return g_strdup (_("A path - whatever it means"));
}

static NRArenaItem *
sp_shape_show (SPItem *item, NRArena *arena)
{
	SPObject *object;
	SPShape *shape;
	SPPath *path;
	NRArenaItem *arenaitem;
	SPPathComp *comp;

	object = SP_OBJECT (item);
	shape = SP_SHAPE (item);
	path = SP_PATH (item);

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_SHAPE);
	nr_arena_shape_set_style (NR_ARENA_SHAPE (arenaitem), object->style);
	if (path->comp) {
		NRRectF paintbox;
		comp = (SPPathComp *) path->comp->data;
		nr_arena_shape_set_path (NR_ARENA_SHAPE (arenaitem), comp->curve, comp->private, comp->affine);
		sp_item_invoke_bbox (SP_ITEM (object), &paintbox, NULL, TRUE);
		nr_arena_shape_set_paintbox (NR_ARENA_SHAPE (arenaitem), &paintbox);
	}

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

/* Shape section */
void
sp_shape_set_shape (SPShape *shape)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_SHAPE (shape));

	if (SP_SHAPE_CLASS (G_OBJECT_GET_CLASS(shape))->set_shape)
		SP_SHAPE_CLASS (G_OBJECT_GET_CLASS(shape))->set_shape (shape);
}
