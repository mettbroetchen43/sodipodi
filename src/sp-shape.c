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

#include "macros.h"
#include "helper/sp-intl.h"
#include "helper/art-utils.h"
#include "svg/svg.h"
#include "dialogs/fill-style.h"
#include "display/nr-arena-shape.h"
#include "uri-references.h"
#include "attributes.h"
#include "print.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-paint-server.h"
#include "style.h"
#include "sp-root.h"
#include "sp-marker.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);

static void sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_shape_release (SPObject *object);

static void sp_shape_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_shape_modified (SPObject *object, unsigned int flags);

static void sp_shape_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
void sp_shape_print (SPItem * item, SPPrintContext * ctx);
static NRArenaItem *sp_shape_show (SPItem *item, NRArena *arena, unsigned int key);

static void sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

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

	if (shape->marker_start) {
		sp_signal_disconnect_by_data (shape->marker_start, object);
		shape->marker_start = sp_object_hunref (shape->marker_start, object);
	}
	if (shape->marker_mid) {
		sp_signal_disconnect_by_data (shape->marker_mid, object);
		shape->marker_mid = sp_object_hunref (shape->marker_mid, object);
	}
	if (shape->marker_end) {
		sp_signal_disconnect_by_data (shape->marker_end, object);
		shape->marker_end = sp_object_hunref (shape->marker_end, object);
	}
	if (shape->curve) {
		shape->curve = sp_curve_unref (shape->curve);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_shape_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	SPItem *item;
	SPShape *shape;

	item = (SPItem *) object;
	shape = (SPShape *) object;

	if (((SPObjectClass *) (parent_class))->update)
		(* ((SPObjectClass *) (parent_class))->update) (object, ctx, flags);

	if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (object);
		if (style->stroke_width.unit == SP_CSS_UNIT_PERCENT) {
			SPItemCtx *ictx;
			SPItemView *v;
			double aw;
			ictx = (SPItemCtx *) ctx;
			aw = 1.0 / NR_MATRIX_DF_EXPANSION (&ictx->i2vp);
			style->stroke_width.computed = style->stroke_width.value * aw;
			for (v = ((SPItem *) (shape))->display; v != NULL; v = v->next) {
				nr_arena_shape_set_style ((NRArenaShape *) v->arenaitem, style);
			}
		}
	}

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

	if (shape->marker_start && shape->curve) {
		SPItemView *v;
		ArtBpath *bp;
		for (v = item->display; v != NULL; v = v->next) {
			if (v->pkey) sp_marker_hide ((SPMarker *) shape->marker_start, v->pkey);
		}
		for (bp = shape->curve->bpath; bp->code != ART_END; bp++) {
			if ((bp->code == ART_MOVETO) || (bp->code == ART_MOVETO_OPEN)) {
				NRMatrixF m;
				nr_matrix_f_set_translate (&m, bp->x3, bp->y3);
				for (v = item->display; v != NULL; v = v->next) {
					NRArenaItem *ai;
					if (!v->pkey) v->pkey = sp_item_display_key_new ();
					ai = sp_marker_show (SP_MARKER (shape->marker_start), NR_ARENA_ITEM_ARENA (v->arenaitem), v->pkey);
					/* fixme: Order (Lauris) */
					nr_arena_item_add_child (v->arenaitem, ai, NULL);
					/* fixme: This is not correct (Lauris) */
					nr_arena_item_set_transform (ai, &m);
					nr_arena_item_unref (ai);
				}
			}
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

/* Marker stuff */

static void
sp_shape_marker_start_release (SPObject *marker, SPShape *shape)
{
	SPItem *item;

	item = (SPItem *) shape;

	if (shape->marker_start) {
		SPItemView *v;
		/* Hide marker */
		for (v = item->display; v != NULL; v = v->next) {
			sp_marker_hide ((SPMarker *) (shape->marker_start), v->pkey);
			/* fixme: Do we need explicit remove here? (Lauris) */
			/* nr_arena_item_set_mask (v->arenaitem, NULL); */
		}
		/* Detach marker */
		sp_signal_disconnect_by_data (shape->marker_start, item);
		shape->marker_start = sp_object_hunref (shape->marker_start, item);
	}
}

static void
sp_shape_marker_start_modified (SPObject *marker, guint flags, SPItem *item)
{
	/* I think mask does update automagically */
	/* g_warning ("Item %s mask %s modified", SP_OBJECT_ID (item), SP_OBJECT_ID (mask)); */
}

void
sp_shape_set_marker (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPItem *item;
	SPShape *shape;

	item = (SPItem *) object;
	shape = (SPShape *) object;

	switch (key) {
	case SP_PROP_MARKER_START: {
		SPObject *mrk;
		mrk = sp_uri_reference_resolve (SP_OBJECT_DOCUMENT (object), value);
		if (mrk != shape->marker_start) {
			if (shape->marker_start) {
				SPItemView *v;
				/* Detach marker */
				sp_signal_disconnect_by_data (shape->marker_start, item);
				/* Hide marker */
				for (v = item->display; v != NULL; v = v->next) {
					sp_marker_hide ((SPMarker *) (shape->marker_start), v->pkey);
					/* fixme: Do we need explicit remove here? (Lauris) */
					/* nr_arena_item_set_mask (v->arenaitem, NULL); */
				}
				shape->marker_start = sp_object_hunref (shape->marker_start, object);
			}
			if (SP_IS_MARKER (mrk)) {
#if 0
				SPItemView *v;
#endif
				shape->marker_start = sp_object_href (mrk, object);
				g_signal_connect (G_OBJECT (shape->marker_start), "release", G_CALLBACK (sp_shape_marker_start_release), shape);
				g_signal_connect (G_OBJECT (shape->marker_start), "modified", G_CALLBACK (sp_shape_marker_start_modified), shape);
#if 0
				for (v = item->display; v != NULL; v = v->next) {
					NRArenaItem *ai;
					if (!v->pkey) v->pkey = sp_item_display_key_new ();
					ai = sp_marker_show (SP_MARKER (shape->marker_start), NR_ARENA_ITEM_ARENA (v->arenaitem), v->pkey);
					/* fixme: Order (Lauris) */
					nr_arena_item_add_child (v->arenaitem, ai, NULL);
					nr_arena_item_unref (ai);
				}
#endif
			}
		}
		break;
	}
	default:
		break;
	}
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

