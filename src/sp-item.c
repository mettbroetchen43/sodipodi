#define __SP_ITEM_C__

/*
 * Base class for visual SVG elements
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <math.h>
#include <string.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>

#include "macros.h"
#include "svg/svg.h"
#include "print.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"
#include "attributes.h"
#include "document.h"
#include "uri-references.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "selection.h"
#include "style.h"
#include "helper/sp-intl.h"
/* fixme: I do not like that (Lauris) */
#include "dialogs/item-properties.h"
#include "dialogs/object-attributes.h"
#include "sp-root.h"
#include "sp-anchor.h"
#include "sp-clippath.h"
#include "sp-item.h"

#define noSP_ITEM_DEBUG_IDLE

static void sp_item_class_init (SPItemClass *klass);
static void sp_item_init (SPItem *item);

static void sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_item_release (SPObject *object);
static void sp_item_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_item_modified (SPObject *object, guint flags);
static void sp_item_style_modified (SPObject *object, guint flags);
static SPRepr *sp_item_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_item_private_description (SPItem * item);
static GSList * sp_item_private_snappoints (SPItem * item, GSList * points);

static NRArenaItem *sp_item_private_show (SPItem *item, NRArena *arena);
static void sp_item_private_hide (SPItem * item, NRArena *arena);

static void sp_item_private_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);
static void sp_item_properties (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_select_this (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_reset_transformation (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_toggle_sensitivity (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_create_link (GtkMenuItem *menuitem, SPItem *item);

static SPObjectClass *parent_class;

unsigned int
sp_item_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPItemClass),
			NULL, NULL,
			(GClassInitFunc) sp_item_class_init,
			NULL, NULL,
			sizeof (SPItem),
			16,
			(GInstanceInitFunc) sp_item_init,
		};
		type = g_type_register_static (SP_TYPE_OBJECT, "SPItem", &info, 0);
	}
	return type;
}

static void
sp_item_class_init (SPItemClass *klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_OBJECT);

	sp_object_class->build = sp_item_build;
	sp_object_class->release = sp_item_release;
	sp_object_class->set = sp_item_set;
	sp_object_class->modified = sp_item_modified;
	sp_object_class->style_modified = sp_item_style_modified;
	sp_object_class->write = sp_item_write;

	klass->description = sp_item_private_description;
	klass->show = sp_item_private_show;
	klass->hide = sp_item_private_hide;
	klass->knot_holder = NULL;
	klass->menu = sp_item_private_menu;
	klass->snappoints = sp_item_private_snappoints;
}

static void
sp_item_init (SPItem *item)
{
	SPObject *object;

	object = SP_OBJECT (item);

	item->sensitive = TRUE;

	nr_matrix_f_set_identity (&item->transform);

	item->display = NULL;

	item->clip = NULL;

	if (!object->style) object->style = sp_style_new_from_object (SP_OBJECT (item));
}

static void
sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_object_read_attr (object, "transform");
	sp_object_read_attr (object, "style");
	sp_object_read_attr (object, "clip-path");
	sp_object_read_attr (object, "sodipodi:insensitive");
}

static void
sp_item_release (SPObject * object)
{
	SPItem *item;

	item = (SPItem *) object;

	while (item->display) {
		nr_arena_item_destroy (item->display->arenaitem);
		item->display = sp_item_view_list_remove (item->display, item->display);
	}

	if (item->clip) {
		g_signal_handlers_disconnect_matched (G_OBJECT(item->clip), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, (gpointer)item);
		item->clip = sp_object_hunref (SP_OBJECT (item->clip), object);
	}

	if (((SPObjectClass *) (parent_class))->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_item_clip_release (SPClipPath *cp, SPItem *item)
{
	g_warning ("Item %s clip path %s release", SP_OBJECT_ID (item), SP_OBJECT_ID (cp));
}

static void
sp_item_clip_modified (SPClipPath *cp, guint flags, SPItem *item)
{
	g_warning ("Item %s clip path %s modified", SP_OBJECT_ID (item), SP_OBJECT_ID (cp));
}

static void
sp_item_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPItem *item;

	item = SP_ITEM (object);

	switch (key) {
	case SP_ATTR_TRANSFORM: {
		NRMatrixF t;
		if (value && sp_svg_transform_read (value, &t)) {
			sp_item_set_item_transform (item, &t);
		} else {
			sp_item_set_item_transform (item, NULL);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	}
	case SP_ATTR_CLIP_PATH: {
		SPObject *cp;
		cp = sp_uri_reference_resolve (SP_OBJECT_DOCUMENT (object), value);
		if (item->clip) {
  			sp_signal_disconnect_by_data (item->clip, item);
			item->clip = sp_object_hunref (SP_OBJECT (item->clip), object);
		}
		if (SP_IS_CLIPPATH (cp)) {
			SPItemView *v;
			item->clip = sp_object_href (cp, object);
			g_signal_connect (G_OBJECT (item->clip), "release", G_CALLBACK (sp_item_clip_release), item);
			g_signal_connect (G_OBJECT (item->clip), "modified", G_CALLBACK (sp_item_clip_modified), item);
			for (v = item->display; v != NULL; v = v->next) {
				NRArenaItem *ai;
				ai = sp_clippath_show (SP_CLIPPATH (item->clip), v->arena);
				nr_arena_item_set_clip (v->arenaitem, ai);
				nr_arena_item_unref (ai);
			}
		}
		break;
	}
	case SP_ATTR_SODIPODI_INSENSITIVE: {
		SPItemView * v;
		item->sensitive = (!value);
		for (v = item->display; v != NULL; v = v->next) {
			nr_arena_item_set_sensitive (v->arenaitem, item->sensitive);
		}
		break;
	}
	case SP_ATTR_STYLE:
		sp_style_read_from_object (object->style, object);
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
		break;
	default:
		if (SP_ATTRIBUTE_IS_CSS (key)) {
			sp_style_read_from_object (object->style, object);
			sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
		} else {
			if (((SPObjectClass *) (parent_class))->set)
				(* ((SPObjectClass *) (parent_class))->set) (object, key, value);
		}
		break;
	}
}

static void
sp_item_modified (SPObject *object, guint flags)
{
	SPItem *item;
	SPStyle *style;

	item = SP_ITEM (object);
	style = SP_OBJECT_STYLE (object);

#ifdef SP_ITEM_DEBUG_IDLE
	g_print ("M");
#endif

	if (style->stroke_width.unit == SP_CSS_UNIT_PERCENT) {
		NRMatrixF i2vp, vp2i;
		gdouble aw;
		/* fixme: It is somewhat dangerous, yes (lauris) */
		sp_item_i2vp_affine (item, &i2vp);
		nr_matrix_f_invert (&vp2i, &i2vp);
		aw = NR_MATRIX_DF_EXPANSION (&vp2i);
		style->stroke_width.computed = style->stroke_width.value * aw;
	}
}

static void
sp_item_style_modified (SPObject *object, guint flags)
{
	SPItem *item;
	SPStyle *style;
	SPItemView *v;

	item = SP_ITEM (object);
	style = object->style;

	/* Set up inherited/relative style properties */

#ifdef SP_ITEM_DEBUG_IDLE
	g_print ("S");
#endif

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_item_set_opacity (v->arenaitem, SP_SCALE24_TO_FLOAT (object->style->opacity.value));
	}
}

static SPRepr *
sp_item_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPItem *item;
	guchar c[256];
	guchar *s;

	item = SP_ITEM (object);

	if (sp_svg_transform_write (c, 256, &item->transform)) {
		sp_repr_set_attr (repr, "transform", c);
	} else {
		sp_repr_set_attr (repr, "transform", NULL);
	}

	if (SP_OBJECT_PARENT (object)) {
		s = sp_style_write_difference (SP_OBJECT_STYLE (object), SP_OBJECT_STYLE (SP_OBJECT_PARENT (object)));
		sp_repr_set_attr (repr, "style", (s && *s) ? s : NULL);
		g_free (s);
	} else {
		sp_repr_set_attr (repr, "style", NULL);
	}

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "sodipodi:insensitive", item->sensitive ? NULL : "true");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

void
sp_item_invoke_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int clear)
{
	sp_item_invoke_bbox_full (item, bbox, transform, 0, clear);
}

void
sp_item_invoke_bbox_full (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags, unsigned int clear)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (bbox != NULL);

	if (clear) {
		bbox->x0 = bbox->y0 = 1e18;
		bbox->x1 = bbox->y1 = -1e18;
	}

	if (!transform) transform = &NR_MATRIX_D_IDENTITY;

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->bbox)
		((SPItemClass *) G_OBJECT_GET_CLASS (item))->bbox (item, bbox, transform, flags);
}

void
sp_item_bbox_desktop (SPItem *item, NRRectF *bbox)
{
	NRMatrixF i2d;
	NRMatrixD i2dd;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (bbox != NULL);

	sp_item_i2d_affine (item, &i2d);
	nr_matrix_d_from_f (&i2dd, &i2d);

	sp_item_invoke_bbox (item, bbox, &i2dd, TRUE);
}

SPKnotHolder *
sp_item_knot_holder (SPItem *item, SPDesktop *desktop)
{
	SPKnotHolder *knot_holder = NULL;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->knot_holder)
		knot_holder = ((SPItemClass *) G_OBJECT_GET_CLASS (item))->knot_holder (item, desktop);

	return knot_holder;
}

static GSList * 
sp_item_private_snappoints (SPItem * item, GSList * points) 
{
        NRRectF bbox;
	NRMatrixF i2d;
	NRMatrixD i2dd;
	ArtPoint *p;

	sp_item_i2d_affine (item, &i2d);
	nr_matrix_d_from_f (&i2dd, &i2d);
	sp_item_invoke_bbox (item, &bbox, &i2dd, TRUE);

	p = g_new (ArtPoint,1);
	p->x = bbox.x0;
	p->y = bbox.y0;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x1;
	p->y = bbox.y0;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x1;
	p->y = bbox.y1;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x0;
	p->y = bbox.y1;
	points = g_slist_append (points, p);

	return points;
}

GSList *
sp_item_snappoints (SPItem *item)
{
        GSList * points = NULL;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->snappoints)
	        points = ((SPItemClass *) G_OBJECT_GET_CLASS(item))->snappoints (item, points);

	return points;
}

void
sp_item_invoke_print (SPItem *item, SPPrintContext *ctx)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (ctx != NULL);

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->print) {
		if (!nr_matrix_f_test_identity (&item->transform, NR_EPSILON_F) ||
		    SP_OBJECT_STYLE (item)->opacity.value != SP_SCALE24_MAX) {
			sp_print_bind (ctx, &item->transform, SP_SCALE24_TO_FLOAT (SP_OBJECT_STYLE (item)->opacity.value));
			((SPItemClass *) G_OBJECT_GET_CLASS (item))->print (item, ctx);
			sp_print_release (ctx);
		} else {
			((SPItemClass *) G_OBJECT_GET_CLASS (item))->print (item, ctx);
		}
	}
}

static gchar *
sp_item_private_description (SPItem * item)
{
	return g_strdup (_("Unknown item :-("));
}

gchar *
sp_item_description (SPItem * item)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->description)
		return ((SPItemClass *) G_OBJECT_GET_CLASS (item))->description (item);

	g_assert_not_reached ();
	return NULL;
}

static NRArenaItem *
sp_item_private_show (SPItem *item, NRArena *arena)
{
	return NULL;
}

NRArenaItem *
sp_item_show (SPItem *item, NRArena *arena)
{
	NRArenaItem *ai;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (arena != NULL);
	g_assert (NR_IS_ARENA (arena));

	ai = NULL;

	if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->show)
		ai = ((SPItemClass *) G_OBJECT_GET_CLASS(item))->show (item, arena);

	if (ai != NULL) {
		item->display = sp_item_view_new_prepend (item->display, item, arena, ai);
		nr_arena_item_set_transform (ai, &item->transform);
		nr_arena_item_set_opacity (ai, SP_SCALE24_TO_FLOAT (SP_OBJECT_STYLE (item)->opacity.value));
		nr_arena_item_set_sensitive (ai, item->sensitive);
		if (item->clip) {
			NRArenaItem *ac;
			ac = sp_clippath_show (SP_CLIPPATH (item->clip), arena);
			nr_arena_item_set_clip (ai, ac);
			nr_arena_item_unref (ac);
		}
		g_object_set_data (G_OBJECT (ai), "sp-item", item);
	}

	return ai;
}

static void
sp_item_private_hide (SPItem * item, NRArena *arena)
{
	SPItemView *v;

	for (v = item->display; v != NULL; v = v->next) {
		if (v->arena == arena) {
			nr_arena_item_destroy (v->arenaitem);
			item->display = sp_item_view_list_remove (item->display, v);
			return;
		}
	}

	g_assert_not_reached ();
}

void
sp_item_hide (SPItem *item, NRArena *arena)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (arena != NULL);
	g_assert (NR_IS_ARENA (arena));

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->hide)
		((SPItemClass *) G_OBJECT_GET_CLASS (item))->hide (item, arena);
}

void
sp_item_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (repr != NULL);

	if (!transform) {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	} else {
		if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->write_transform) {
			NRMatrixF lt;
			lt = *transform;
			((SPItemClass *) G_OBJECT_GET_CLASS(item))->write_transform (item, repr, &lt);
		} else {
			guchar t[80];
			if (sp_svg_transform_write (t, 80, &item->transform)) {
				sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
			} else {
				sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", t);
			}
		}
	}
}

gint
sp_item_event (SPItem *item, SPEvent *event)
{
	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (SP_IS_ITEM (item), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->event)
		return ((SPItemClass *) G_OBJECT_GET_CLASS(item))->event (item, event);

	return FALSE;
}

/* Sets item private transform (not propagated to repr) */

void
sp_item_set_item_transform (SPItem *item, const NRMatrixF *transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	if (!transform) transform = &NR_MATRIX_F_IDENTITY;

	if (!NR_MATRIX_DF_TEST_CLOSE (transform, &item->transform, NR_EPSILON_F)) {
		SPItemView *v;
		item->transform = *transform;
		for (v = item->display; v != NULL; v = v->next) {
			nr_arena_item_set_transform (v->arenaitem, transform);
		}
		sp_object_request_modified (SP_OBJECT (item), SP_OBJECT_MODIFIED_FLAG);
	}
}

NRMatrixF *
sp_item_i2doc_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD td;
	SPRoot *root;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	nr_matrix_d_set_identity (&td);

	while (SP_OBJECT_PARENT (item)) {
		nr_matrix_multiply_ddf (&td, &td, &item->transform);
		item = (SPItem *) SP_OBJECT_PARENT (item);
	}

	g_return_val_if_fail (SP_IS_ROOT (item), NULL);

	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	nr_matrix_multiply_ddd (&td, &td, &root->viewbox);
	nr_matrix_multiply_ddf (&td, &td, &item->transform);

	nr_matrix_f_from_d (affine, &td);

	return affine;
}

NRMatrixF *
sp_item_i2root_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD td;
	SPRoot *root;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	nr_matrix_d_set_identity (&td);

	while (SP_OBJECT_PARENT (item)) {
		nr_matrix_multiply_ddf (&td, &td, &item->transform);
		item = (SPItem *) SP_OBJECT_PARENT (item);
	}

	g_return_val_if_fail (SP_IS_ROOT (item), NULL);

	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	nr_matrix_multiply_ddd (&td, &td, &root->viewbox);
	nr_matrix_multiply_ddf (&td, &td, &item->transform);

	nr_matrix_f_from_d (affine, &td);

	return affine;
}

/* Transformation to normalized (0,0-1,1) viewport */

NRMatrixF *
sp_item_i2vp_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD td;
	SPRoot *root;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	nr_matrix_d_set_identity (&td);

	while (SP_OBJECT_PARENT (item)) {
		nr_matrix_multiply_ddf (&td, &td, &item->transform);
		item = (SPItem *) SP_OBJECT_PARENT (item);
	}

	g_return_val_if_fail (SP_IS_ROOT (item), NULL);

	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	nr_matrix_multiply_ddd (&td, &td, &root->viewbox);

	td.c[0] /= root->width.computed;
	td.c[1] /= root->height.computed;
	td.c[2] /= root->width.computed;
	td.c[3] /= root->height.computed;

	nr_matrix_f_from_d (affine, &td);

	return affine;
}

/* fixme: This is EVIL!!! */

NRMatrixF *
sp_item_i2d_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD doc2dt;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	sp_item_i2doc_affine (item, affine);
	nr_matrix_d_set_scale (&doc2dt, 0.8, -0.8);
	doc2dt.c[5] = sp_document_height (SP_OBJECT_DOCUMENT (item));
	nr_matrix_multiply_ffd (affine, affine, &doc2dt);

	return affine;
}

void
sp_item_set_i2d_affine (SPItem *item, const NRMatrixF *affine)
{
	NRMatrixF p2d, d2p, i2p;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (affine != NULL);

	if (SP_OBJECT_PARENT (item)) {
		sp_item_i2d_affine ((SPItem *) SP_OBJECT_PARENT (item), &p2d);
	} else {
		nr_matrix_f_set_scale (&p2d, 0.8, -0.8);
		p2d.c[5] = sp_document_height (SP_OBJECT_DOCUMENT (item));
	}

	nr_matrix_f_invert (&d2p, &p2d);

	nr_matrix_multiply_fff (&i2p, affine, &d2p);

	sp_item_set_item_transform (item, &i2p);
}

NRMatrixF *
sp_item_dt2i_affine (SPItem *item, SPDesktop *dt, NRMatrixF *affine)
{
	NRMatrixF i2dt;

	/* fixme: Implement the right way (Lauris) */
	sp_item_i2d_affine (item, &i2dt);
	nr_matrix_f_invert (affine, &i2dt);

	return affine;
}

/* Generate context menu item section */

static void
sp_item_private_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget * i, * m, * w;
	gboolean insensitive;

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Item"));
	m = gtk_menu_new ();
	/* Item dialog */
	w = gtk_menu_item_new_with_label (_("Item Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Separator */
	w = gtk_menu_item_new ();
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Select item */
	w = gtk_menu_item_new_with_label (_("Select this"));
	if (sp_selection_item_selected (SP_DT_SELECTION (desktop), item)) {
		gtk_widget_set_sensitive (w, FALSE);
	} else {
		gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
		gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_select_this), item);
	}
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Reset transformations */
	w = gtk_menu_item_new_with_label (_("Reset transformation"));
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_reset_transformation), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Toggle sensitivity */
	insensitive = (sp_repr_attr (SP_OBJECT_REPR (item), "sodipodi:insensitive") != NULL);
	w = gtk_menu_item_new_with_label (insensitive ? _("Make sensitive") : _("Make insensitive"));
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_toggle_sensitivity), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Create link */
	w = gtk_menu_item_new_with_label (_("Create link"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_create_link), item);
	gtk_widget_set_sensitive (w, !SP_IS_ANCHOR (item));
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

void
sp_item_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	g_assert (SP_IS_ITEM (item));
	g_assert (GTK_IS_MENU (menu));

	if (((SPItemClass *) G_OBJECT_GET_CLASS(item))->menu)
		((SPItemClass *) G_OBJECT_GET_CLASS(item))->menu (item, desktop, menu);
}

static void
sp_item_properties (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);

	sp_item_dialog (item);
}

static void
sp_item_select_this (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);
}

static void
sp_item_reset_transformation (GtkMenuItem * menuitem, SPItem * item)
{
	g_assert (SP_IS_ITEM (item));

	sp_repr_set_attr (((SPObject *) item)->repr, "transform", NULL);
	sp_document_done (SP_OBJECT_DOCUMENT (item));
}

static void
sp_item_toggle_sensitivity (GtkMenuItem * menuitem, SPItem * item)
{
	const gchar * val;

	g_assert (SP_IS_ITEM (item));

	/* fixme: reprs suck */
	val = sp_repr_attr (SP_OBJECT_REPR (item), "sodipodi:insensitive");
	if (val != NULL) {
		val = NULL;
	} else {
		val = "1";
	}
	sp_repr_set_attr (SP_OBJECT_REPR (item), "sodipodi:insensitive", val);
	sp_document_done (SP_OBJECT_DOCUMENT (item));
}

static void
sp_item_create_link (GtkMenuItem *menuitem, SPItem *item)
{
	SPRepr *repr, *child;
	SPObject *object;
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));
	g_assert (!SP_IS_ANCHOR (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	repr = sp_repr_new ("a");
	sp_repr_add_child (SP_OBJECT_REPR (SP_OBJECT_PARENT (item)), repr, SP_OBJECT_REPR (item));
	object = sp_document_lookup_id (SP_OBJECT_DOCUMENT (item), sp_repr_attr (repr, "id"));
	g_return_if_fail (SP_IS_ANCHOR (object));
	child = sp_repr_duplicate (SP_OBJECT_REPR (item));
	sp_repr_unparent (SP_OBJECT_REPR (item));
	sp_repr_add_child (repr, child, NULL);
	sp_document_done (SP_OBJECT_DOCUMENT (object));

	sp_object_attributes_dialog (object, "SPAnchor");

	sp_selection_set_item (SP_DT_SELECTION (desktop), SP_ITEM (object));
}

/* Item views */

SPItemView *
sp_item_view_new_prepend (SPItemView * list, SPItem * item, NRArena *arena, NRArenaItem *arenaitem)
{
	SPItemView * new;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (arena != NULL);
	g_assert (NR_IS_ARENA (arena));
	g_assert (arenaitem != NULL);
	g_assert (NR_IS_ARENA_ITEM (arenaitem));

	new = g_new (SPItemView, 1);

	new->next = list;
#if 0
	new->prev = NULL;
	if (list) list->prev = new;
#endif
	new->item = item;
	new->arena = arena;
	new->arenaitem = arenaitem;

	return new;
}

SPItemView *
sp_item_view_list_remove (SPItemView *list, SPItemView *view)
{
#if 1
	if (view == list) {
		list = list->next;
	} else {
		SPItemView *prev;
		prev = list;
		while (prev->next != view) prev = prev->next;
		prev->next = view->next;
	}

	g_free (view);

	return list;

#else
	SPItemView * v;

	g_assert (list != NULL);
	g_assert (view != NULL);

	for (v = list; v != NULL; v = v->next) {
		if (v == view) {
			if (v->next) v->next->prev = v->prev;
			if (v->prev) {
				v->prev->next = v->next;
			} else {
				list = v->next;
			}
			g_free (v);
			return list;
		}
	}

	g_assert_not_reached ();

	return NULL;
#endif
}

/* Convert distances into SVG units */

static const SPUnit *absolute = NULL;
static const SPUnit *percent = NULL;
static const SPUnit *em = NULL;
static const SPUnit *ex = NULL;

gdouble
sp_item_distance_to_svg_viewport (SPItem *item, gdouble distance, const SPUnit *unit)
{
	NRMatrixF i2doc;
	double dx, dy;
	double a2u, u2a;

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	sp_item_i2doc_affine (item, &i2doc);
	dx = i2doc.c[0] + i2doc.c[2];
	dy = i2doc.c[1] + i2doc.c[3];
	u2a = sqrt (dx * dx + dy * dy) * M_SQRT1_2;
	a2u = u2a > 1e-9 ? 1 / u2a : 1e9;

	if (unit->base == SP_UNIT_DIMENSIONLESS) {
		/* Check for percentage */
		if (!percent) percent = sp_unit_get_by_abbreviation ("%");
		if (unit == percent) {
			/* Percentage of viewport */
			/* fixme: full viewport support (Lauris) */
			dx = sp_document_width (SP_OBJECT_DOCUMENT (item));
			dy = sp_document_height (SP_OBJECT_DOCUMENT (item));
			return 0.01 * distance * sqrt (dx * dx + dy * dy) * M_SQRT1_2;
		} else {
			/* Treat as userspace */
			return distance * unit->unittobase * u2a;
		}
	} else if (unit->base == SP_UNIT_VOLATILE) {
		/* Either em or ex */
		/* fixme: This need real care */
		if (!em) em = sp_unit_get_by_abbreviation ("em");
		if (!ex) ex = sp_unit_get_by_abbreviation ("ex");
		if (unit == em) {
			return distance * 12.0;
		} else {
			return distance * 10.0;
		}
	} else {
		/* Everything else can be done in one step */
		/* We just know, that pt == 1.25 * px */
		if (!absolute) absolute = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		sp_convert_distance_full (&distance, unit, absolute, u2a, 0.8);
		return distance;
	}
}

gdouble
sp_item_distance_to_svg_bbox (SPItem *item, gdouble distance, const SPUnit *unit)
{
	NRMatrixF i2doc;
	double dx, dy;
	double a2u, u2a;

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	sp_item_i2doc_affine (item, &i2doc);
	dx = i2doc.c[0] + i2doc.c[2];
	dy = i2doc.c[1] + i2doc.c[3];
	u2a = sqrt (dx * dx + dy * dy) * M_SQRT1_2;
	a2u = u2a > 1e-9 ? 1 / u2a : 1e9;

	if (unit->base == SP_UNIT_DIMENSIONLESS) {
		/* Check for percentage */
		if (!percent) percent = sp_unit_get_by_abbreviation ("%");
		if (unit == percent) {
			/* Percentage of viewport */
			/* fixme: full viewport support (Lauris) */
			g_warning ("file %s: line %d: Implement real item bbox percentage etc.", __FILE__, __LINE__);
			dx = sp_document_width (SP_OBJECT_DOCUMENT (item));
			dy = sp_document_height (SP_OBJECT_DOCUMENT (item));
			return 0.01 * distance * sqrt (dx * dx + dy * dy) * M_SQRT1_2;
		} else {
			/* Treat as userspace */
			return distance * unit->unittobase * u2a;
		}
	} else if (unit->base == SP_UNIT_VOLATILE) {
		/* Either em or ex */
		/* fixme: This need real care */
		if (!em) em = sp_unit_get_by_abbreviation ("em");
		if (!ex) ex = sp_unit_get_by_abbreviation ("ex");
		if (unit == em) {
			return distance * 12.0;
		} else {
			return distance * 10.0;
		}
	} else {
		/* Everything else can be done in one step */
		/* We just know, that pt == 1.25 * px */
		if (!absolute) absolute = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		sp_convert_distance_full (&distance, unit, absolute, u2a, 0.8);
		return distance;
	}
}

