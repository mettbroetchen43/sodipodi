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
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "helper/art-utils.h"
#include "helper/nr-plain-stuff.h"
#include "svg/svg.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"
#include "document.h"
#include "uri-references.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "selection.h"
#include "style.h"
/* fixme: I do not like that (Lauris) */
#include "dialogs/item-properties.h"
#include "dialogs/object-attributes.h"
#include "sp-root.h"
#include "sp-anchor.h"
#include "sp-clippath.h"
#include "sp-item.h"

#define noSP_ITEM_DEBUG_IDLE

static void sp_item_class_init (SPItemClass * klass);
static void sp_item_init (SPItem * item);
static void sp_item_destroy (GtkObject * object);

static void sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_item_read_attr (SPObject *object, const gchar *key);
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

static SPObjectClass * parent_class;

GtkType
sp_item_get_type (void)
{
	static GtkType item_type = 0;
	if (!item_type) {
		GtkTypeInfo item_info = {
			"SPItem",
			sizeof (SPItem),
			sizeof (SPItemClass),
			(GtkClassInitFunc) sp_item_class_init,
			(GtkObjectInitFunc) sp_item_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		item_type = gtk_type_unique (sp_object_get_type (), &item_info);
	}
	return item_type;
}

static void
sp_item_class_init (SPItemClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_item_destroy;

	sp_object_class->build = sp_item_build;
	sp_object_class->read_attr = sp_item_read_attr;
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

	art_affine_identity (item->affine);
	item->display = NULL;

	item->clip = NULL;

	if (!object->style) object->style = sp_style_new_from_object (SP_OBJECT (item));
}

static void
sp_item_destroy (GtkObject * object)
{
	SPItem * item;

	item = (SPItem *) object;

	while (item->display) {
		nr_arena_item_destroy (item->display->arenaitem);
		item->display = sp_item_view_list_remove (item->display, item->display);
	}

	if (item->clip) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (item->clip), item);
		item->clip = (SPClipPath *) sp_object_hunref (SP_OBJECT (item->clip), SP_OBJECT (item));
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_item_read_attr (object, "transform");
	sp_item_read_attr (object, "style");
	sp_item_read_attr (object, "clip-path");
	sp_item_read_attr (object, "sodipodi:insensitive");
}

static void
sp_item_clip_destroyed (SPClipPath *cp, SPItem *item)
{
	g_warning ("Item %s clip path %s destroyed", SP_OBJECT_ID (item), SP_OBJECT_ID (cp));
}

static void
sp_item_clip_modified (SPClipPath *cp, guint flags, SPItem *item)
{
	g_warning ("Item %s clip path %s modified", SP_OBJECT_ID (item), SP_OBJECT_ID (cp));
}

static void
sp_item_read_attr (SPObject * object, const gchar * key)
{
	SPItem *item;
	const gchar *astr;

	item = SP_ITEM (object);

	astr = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "transform")) {
		gdouble a[6];
		art_affine_identity (a);
		if (astr != NULL) sp_svg_read_affine (a, astr);
		sp_item_set_item_transform (item, a);
	}

	if (!strcmp (key, "clip-path")) {
		SPObject *cp;
		cp = sp_uri_reference_resolve (SP_OBJECT_DOCUMENT (object), astr);
		if (item->clip) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (item->clip), item);
			item->clip = (SPClipPath *) sp_object_hunref (SP_OBJECT (item->clip), object);
		}
		if (SP_IS_CLIPPATH (cp)) {
			SPItemView *v;
			item->clip = (SPClipPath *) sp_object_href (cp, object);
			gtk_signal_connect (GTK_OBJECT (item->clip), "destroy",
					    GTK_SIGNAL_FUNC (sp_item_clip_destroyed), item);
			gtk_signal_connect (GTK_OBJECT (item->clip), "modified",
					    GTK_SIGNAL_FUNC (sp_item_clip_modified), item);
			for (v = item->display; v != NULL; v = v->next) {
				NRArenaItem *ai;
				ai = sp_clippath_show (item->clip, v->arena);
				nr_arena_item_set_clip (v->arenaitem, ai);
				nr_arena_item_unref (ai);
			}
		}
	}

	if (!strcmp (key, "sodipodi:insensitive")) {
		SPItemView * v;
		item->sensitive = (astr == NULL);
		for (v = item->display; v != NULL; v = v->next) {
			nr_arena_item_set_sensitive (v->arenaitem, item->sensitive);
		}
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);

	/* fixme: */
	if (!strcmp (key, "style") ||
	    !strcmp (key, "font-size") ||
	    !strcmp (key, "fill-cmyk") ||
	    !strcmp (key, "fill") ||
	    !strcmp (key, "stroke-cmyk") ||
	    !strcmp (key, "stroke") ||
	    !strcmp (key, "opacity")) {
		sp_style_read_from_object (object->style, object);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
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
		gdouble i2vp[6], vp2i[6];
		gdouble aw, ah;
		/* fixme: It is somewhat dangerous, yes (lauris) */
		sp_item_i2vp_affine (item, i2vp);
		art_affine_invert (vp2i, i2vp);
		aw = sp_distance_d_matrix_d_transform (1.0, vp2i);
		ah = sp_distance_d_matrix_d_transform (1.0, vp2i);
		/* sqrt ((actual_width) ** 2 + (actual_height) ** 2)) / sqrt (2) */
		style->stroke_width.computed = style->stroke_width.value * sqrt (aw * aw + ah * ah) * M_SQRT1_2;
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

#if 0
	if (!style->real_opacity_set) {
		SPObject *parent;
		style->real_opacity = style->opacity;
		parent = object->parent;
		while (parent && !parent->style) parent = parent->parent;
		/* fixme: Currently it does not work, if parent has not set real_opacity (but it shouldn't happen) */
		if (parent) style->real_opacity = style->real_opacity * parent->style->real_opacity;
		style->real_opacity_set = TRUE;
	}
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
	gint len;

	item = SP_ITEM (object);

	len = sp_svg_write_affine (c, 256, item->affine);
	sp_repr_set_attr (repr, "transform", (len > 0) ? c : NULL);
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
sp_item_invoke_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (bbox != NULL);
	g_assert (transform != NULL);

	bbox->x0 = bbox->y0 = 1e18;
	bbox->x1 = bbox->y1 = -1e18;

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox)
		SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox (item, bbox, transform);
}

void sp_item_bbox_desktop (SPItem *item, ArtDRect *bbox)
{
	gdouble i2d[6];

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (bbox != NULL);

	sp_item_i2d_affine (item, i2d);

	sp_item_invoke_bbox (item, bbox, i2d);
}

SPKnotHolder *
sp_item_knot_holder (SPItem *item, SPDesktop *desktop)
{
	SPKnotHolder *knot_holder = NULL;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->knot_holder)
		knot_holder = (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->knot_holder) (item, desktop);

	return knot_holder;
}

static GSList * 
sp_item_private_snappoints (SPItem * item, GSList * points) 
{
        ArtDRect bbox;
	ArtPoint * p;
	gdouble i2d[6];

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	sp_item_i2d_affine (item, i2d);
	sp_item_invoke_bbox (item, &bbox, i2d);

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

GSList * sp_item_snappoints (SPItem * item)
{
        GSList * points = NULL;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->snappoints)
	        points = (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->snappoints) (item, points);
	return points;
}

void
sp_item_print (SPItem * item, GnomePrintContext * gpc)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (gpc != NULL);

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->print)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->print) (item, gpc);
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

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description)
		return (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description) (item);

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

	if (((SPItemClass *) (((GtkObject *) item)->klass))->show)
		ai = ((SPItemClass *) (((GtkObject *) item)->klass))->show (item, arena);

	if (ai != NULL) {
		item->display = sp_item_view_new_prepend (item->display, item, arena, ai);
		nr_arena_item_set_transform (ai, item->affine);
		nr_arena_item_set_opacity (ai, SP_SCALE24_TO_FLOAT (SP_OBJECT_STYLE (item)->opacity.value));
		nr_arena_item_set_sensitive (ai, item->sensitive);
		if (item->clip) {
			NRArenaItem *ac;
			ac = sp_clippath_show (item->clip, arena);
			nr_arena_item_set_clip (ai, ac);
			nr_arena_item_unref (ac);
		}
		gtk_object_set_user_data (GTK_OBJECT (ai), item);
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

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide) (item, arena);
}

gboolean
sp_item_paint (SPItem *item, ArtPixBuf *buf, gdouble affine[])
{
	NRArena *arena;
	NRArenaItem *root;
	NRRectL bbox;
	NRGC gc;
	NRBuffer *b;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (buf != NULL);
	g_assert (affine != NULL);

	sp_document_ensure_up_to_date (SP_OBJECT_DOCUMENT (item));

	/* Create new arena */
	arena = gtk_type_new (NR_TYPE_ARENA);
	/* Create ArenaItem and set transform */
	root = sp_item_show (item, arena);
	nr_arena_item_set_transform (root, affine);
	/* Set area of interest */
	bbox.x0 = 0;
	bbox.y0 = 0;
	bbox.x1 = buf->width;
	bbox.y1 = buf->height;
	/* Update to renderable state */
	art_affine_identity (gc.affine);
	nr_arena_item_invoke_update (root, &bbox, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);
	/* Get RGBA buffer */
	b = nr_buffer_get (NR_IMAGE_R8G8B8A8, buf->width, buf->height, TRUE, FALSE);
	/* Render */
	nr_arena_item_invoke_render (root, &bbox, b);
	/* Free Arena and ArenaItem */
	sp_item_hide (item, arena);
	gtk_object_unref (GTK_OBJECT (arena));
	/* Copy buffer to output */
	nr_render_r8g8b8a8_r8g8b8a8_alpha (buf->pixels, buf->width, buf->height, buf->rowstride, b->px, b->rs, 255);
	/* Release RGBA buffer */
	nr_buffer_free (b);

	return FALSE;
}

void
sp_item_write_transform (SPItem *item, SPRepr *repr, gdouble *transform)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (repr != NULL);

	if (!transform) {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	} else {
		if (((SPItemClass *) (((GtkObject *) item)->klass))->write_transform) {
			gdouble ltrans[6];
			memcpy (ltrans, transform, 6 * sizeof (gdouble));
			((SPItemClass *) (((GtkObject *) item)->klass))->write_transform (item, repr, ltrans);
		} else {
			guchar t[80];
			if (sp_svg_write_affine (t, 80, item->affine)) {
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

	if (((SPItemClass *) (((GtkObject *) item)->klass))->event)
		return ((SPItemClass *) (((GtkObject *) item)->klass))->event (item, event);

	return FALSE;
}

/* Sets item private transform (not propagated to repr) */

void
sp_item_set_item_transform (SPItem *item, const gdouble *transform)
{
	SPItemView *v;
	gint i;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (transform != NULL);

	for (i = 0; i < 6; i++) {
		if (fabs (transform[i] - item->affine[i]) > 1e-9) break;
	}
	if (i >= 6) return;

	memcpy (item->affine, transform, 6 * sizeof (gdouble));

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_item_set_transform (v->arenaitem, transform);
	}

	sp_object_request_modified (SP_OBJECT (item), SP_OBJECT_MODIFIED_FLAG);
}

gdouble *
sp_item_i2doc_affine (SPItem * item, gdouble affine[])
{
	SPRoot *root;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (SP_OBJECT_PARENT (item)) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) SP_OBJECT_PARENT (item);
	}

	g_return_val_if_fail (SP_IS_ROOT (item), NULL);

	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	art_affine_multiply (affine, affine, root->viewbox.c);
	art_affine_multiply (affine, affine, item->affine);

	return affine;
}

/* Transformation to normalized (0,0-1,1) viewport */

gdouble *
sp_item_i2vp_affine (SPItem *item, gdouble affine[])
{
	SPRoot *root;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (SP_OBJECT_PARENT (item)) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) SP_OBJECT_PARENT (item);
	}

	g_return_val_if_fail (SP_IS_ROOT (item), NULL);

	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	art_affine_multiply (affine, affine, root->viewbox.c);

	affine[0] /= root->width.computed;
	affine[1] /= root->height.computed;
	affine[2] /= root->width.computed;
	affine[3] /= root->height.computed;

	return affine;
}

/* fixme: This is EVIL!!! */

gdouble *
sp_item_i2d_affine (SPItem *item, gdouble affine[])
{
	gdouble doc2dt[6];

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	sp_item_i2doc_affine (item, affine);
	art_affine_scale (doc2dt, 0.8, -0.8);
	doc2dt[5] = sp_document_height (SP_OBJECT_DOCUMENT (item));
	art_affine_multiply (affine, affine, doc2dt);

	return affine;
}

void
sp_item_set_i2d_affine (SPItem *item, gdouble affine[])
{
	gdouble p2d[6], d2p[6], i2p[6];

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (affine != NULL);

	if (SP_OBJECT_PARENT (item)) {
		sp_item_i2d_affine ((SPItem *) SP_OBJECT_PARENT (item), p2d);
	} else {
		art_affine_scale (p2d, 0.8, -0.8);
		p2d[5] = sp_document_height (SP_OBJECT_DOCUMENT (item));
	}

	art_affine_invert (d2p, p2d);

	art_affine_multiply (i2p, affine, d2p);

	sp_item_set_item_transform (item, i2p);
}

gdouble *
sp_item_dt2i_affine (SPItem *item, SPDesktop *dt, gdouble affine[])
{
	gdouble i2dt[6];

	/* fixme: Implement the right way (Lauris) */
	sp_item_i2d_affine (item, i2dt);
	art_affine_invert (affine, i2dt);

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

	if (SP_ITEM_CLASS (((GtkObject *) (item))->klass)->menu)
		(* SP_ITEM_CLASS (((GtkObject *) (item))->klass)->menu) (item, desktop, menu);
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
	new->prev = NULL;
	if (list) list->prev = new;
	new->item = item;
	new->arena = arena;
	new->arenaitem = arenaitem;

	return new;
}

SPItemView *
sp_item_view_list_remove (SPItemView * list, SPItemView * view)
{
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
}

/* Convert distances into SVG units */

static const SPUnit *absolute = NULL;
static const SPUnit *percent = NULL;
static const SPUnit *em = NULL;
static const SPUnit *ex = NULL;

gdouble
sp_item_distance_to_svg_viewport (SPItem *item, gdouble distance, const SPUnit *unit)
{
	gdouble i2doc[6], dx, dy;
	gdouble a2u, u2a;

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	sp_item_i2doc_affine (item, i2doc);
	dx = i2doc[0] + i2doc[2];
	dy = i2doc[1] + i2doc[3];
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
	gdouble i2doc[6], dx, dy;
	gdouble a2u, u2a;

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	g_return_val_if_fail (item != NULL, distance);
	g_return_val_if_fail (SP_IS_ITEM (item), distance);
	g_return_val_if_fail (unit != NULL, distance);

	sp_item_i2doc_affine (item, i2doc);
	dx = i2doc[0] + i2doc[2];
	dy = i2doc[1] + i2doc[3];
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

