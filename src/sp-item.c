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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include <string.h>
#include <assert.h>

#include <libnr/nr-rect.h>

#include "macros.h"
#include "helper/sp-intl.h"
#include "svg/svg.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"
#include "attributes.h"
#include "document.h"
#include "uri-references.h"

#include "style.h"
#include "print.h"
#include "sp-anchor.h"
#include "sp-clippath.h"
#include "sp-mask.h"
#include "sp-item.h"

#define noSP_ITEM_DEBUG_IDLE

static void sp_item_class_init (SPItemClass *klass);
static void sp_item_init (SPItem *item);

static void sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_item_release (SPObject *object);
static void sp_item_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_item_update (SPObject *object, SPCtx *ctx, guint flags);
static SPRepr *sp_item_write (SPObject *object, SPRepr *repr, guint flags);

static gchar * sp_item_private_description (SPItem * item);
static int sp_item_private_snappoints (SPItem *item, NRPointF *p, int size, const NRMatrixF *transform);

static SPObjectClass *parent_class;

GType
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
	GObjectClass *object_class;
	SPObjectClass *sp_object_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_OBJECT);

	sp_object_class->build = sp_item_build;
	sp_object_class->release = sp_item_release;
	sp_object_class->set = sp_item_set;
	sp_object_class->update = sp_item_update;
	sp_object_class->write = sp_item_write;

	klass->description = sp_item_private_description;
	klass->snappoints = sp_item_private_snappoints;
}

static void
sp_item_init (SPItem *item)
{
	SPObject *object;

	object = SP_OBJECT (item);

	item->sensitive = TRUE;
	item->visible = TRUE;
	item->printable = TRUE;

	nr_matrix_f_set_identity (&item->transform);

	item->display = NULL;

	item->clip = NULL;
	item->mask = NULL;

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
	sp_object_read_attr (object, "mask");
	sp_object_read_attr (object, "sodipodi:insensitive");
	sp_object_read_attr (object, "sodipodi:invisible");
	sp_object_read_attr (object, "sodipodi:nonprintable");
}

static void
sp_item_release (SPObject * object)
{
	SPItem *item;

	item = (SPItem *) object;

	while (item->display) {
		NRArenaItem *ai;
		ai = item->display;
		if (item->clip) {
			sp_clippath_hide (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (ai));
			nr_arena_item_set_clip (ai, NULL);
		}
		if (item->mask) {
			sp_mask_hide (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (ai));
			nr_arena_item_set_mask (ai, NULL);
		}
		item->display = nr_arena_item_view_list_remove (item->display, item->display);
		nr_arena_item_unparent (ai);
	}

	if (item->clip) {
		sp_signal_disconnect_by_data (item->clip, object);
		item->clip = sp_object_hunref (SP_OBJECT (item->clip), object);
	}

	if (item->mask) {
		sp_signal_disconnect_by_data (item->mask, object);
		item->mask = sp_object_hunref (SP_OBJECT (item->mask), object);
	}

	if (((SPObjectClass *) (parent_class))->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_item_clip_release (SPClipPath *cp, SPItem *item)
{
	if (item->clip) {
		SPItemView *v;
		/* Hide clippath */
		for (v = item->display; v != NULL; v = v->view.next) {
			sp_clippath_hide (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (v));
			nr_arena_item_set_clip (v, NULL);
		}
		/* Detach clippath */
		sp_signal_disconnect_by_data (item->clip, item);
		item->clip = sp_object_hunref (SP_OBJECT (item->clip), item);
	}
}

static void
sp_item_clip_modified (SPClipPath *cp, guint flags, SPItem *item)
{
	/* I think cliPath does update automagically */
	/* g_warning ("Item %s clip path %s modified", SP_OBJECT_ID (item), SP_OBJECT_ID (cp)); */
}

static void
sp_item_mask_release (SPMask *mask, SPItem *item)
{
	if (item->mask) {
		SPItemView *v;
		/* Hide mask */
		for (v = item->display; v != NULL; v = v->view.next) {
			sp_mask_hide (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (v));
			nr_arena_item_set_mask (v, NULL);
		}
		/* Detach mask */
		sp_signal_disconnect_by_data (item->mask, item);
		item->mask = sp_object_hunref (SP_OBJECT (item->mask), item);
	}
}

static void
sp_item_mask_modified (SPMask *mask, guint flags, SPItem *item)
{
	/* I think mask does update automagically */
	/* g_warning ("Item %s mask %s modified", SP_OBJECT_ID (item), SP_OBJECT_ID (mask)); */
}

static void
sp_item_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPItem *item;
	SPItemView *v;
	unsigned int bval;

	item = (SPItem *) object;

	switch (key) {
	case SP_ATTR_TRANSFORM: {
		NRMatrixF t;
		if (value && sp_svg_transform_read (value, &t)) {
			sp_item_set_item_transform (item, &t);
		} else {
			sp_item_set_item_transform (item, NULL);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	}
	case SP_PROP_CLIP_PATH: {
		SPObject *cp;
		cp = sp_uri_reference_resolve (SP_OBJECT_DOCUMENT (object), value);
		if (cp != item->clip) {
			if (item->clip) {
				SPItemView *v;
				/* Detach clippath */
				sp_signal_disconnect_by_data (item->clip, item);
				/* Hide clippath */
				for (v = item->display; v != NULL; v = v->view.next) {
					sp_clippath_hide (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (v));
					nr_arena_item_set_clip (v, NULL);
				}
				item->clip = sp_object_hunref (item->clip, object);
			}
			if (SP_IS_CLIPPATH (cp)) {
				NRRectF bbox;
				SPItemView *v;
				item->clip = sp_object_href (cp, object);
				g_signal_connect (G_OBJECT (item->clip), "release", G_CALLBACK (sp_item_clip_release), item);
				g_signal_connect (G_OBJECT (item->clip), "modified", G_CALLBACK (sp_item_clip_modified), item);
				sp_item_invoke_bbox (item, &bbox, NULL, TRUE);
				for (v = item->display; v != NULL; v = v->view.next) {
					NRArenaItem *ai;
					if (!v->view.key) NR_ARENA_ITEM_SET_KEY (v, sp_item_display_key_new (3));
					ai = sp_clippath_show (SP_CLIPPATH (item->clip),
							       NR_ARENA_ITEM_ARENA (v),
							       NR_ARENA_ITEM_GET_KEY (v));
					nr_arena_item_set_clip (v, ai);
					nr_arena_item_unref (ai);
					sp_clippath_set_bbox (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (v), &bbox);
				}
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	}
	case SP_PROP_MASK: {
		SPObject *m;
		m = sp_uri_reference_resolve (SP_OBJECT_DOCUMENT (object), value);
		if (m != item->mask) {
			if (item->mask) {
				SPItemView *v;
				/* Detach mask */
				sp_signal_disconnect_by_data (item->mask, item);
				/* Hide mask */
				for (v = item->display; v != NULL; v = v->view.next) {
					sp_mask_hide (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (v));
					nr_arena_item_set_mask (v, NULL);
				}
				item->mask = sp_object_hunref (item->mask, object);
			}
			if (SP_IS_MASK (m)) {
				NRRectF bbox;
				SPItemView *v;
				item->mask = sp_object_href (m, object);
				g_signal_connect (G_OBJECT (item->mask), "release", G_CALLBACK (sp_item_mask_release), item);
				g_signal_connect (G_OBJECT (item->mask), "modified", G_CALLBACK (sp_item_mask_modified), item);
				sp_item_invoke_bbox (item, &bbox, NULL, TRUE);
				for (v = item->display; v != NULL; v = v->view.next) {
					NRArenaItem *ai;
					if (!v->view.key) NR_ARENA_ITEM_SET_KEY (v, sp_item_display_key_new (3));
					ai = sp_mask_show (SP_MASK (item->mask),
							   NR_ARENA_ITEM_ARENA (v),
							   NR_ARENA_ITEM_GET_KEY (v));
					nr_arena_item_set_mask (v, ai);
					nr_arena_item_unref (ai);
					sp_mask_set_bbox (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (v), &bbox);
				}
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	}
	case SP_ATTR_SODIPODI_INSENSITIVE:
		item->sensitive = TRUE;
		if (value && sp_svg_boolean_read (value, &bval)) {
			item->sensitive = !bval;
		}
		for (v = item->display; v != NULL; v = v->view.next) {
			nr_arena_item_set_sensitive (v, item->sensitive);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_SODIPODI_INVISIBLE:
		item->visible = TRUE;
		if (value && sp_svg_boolean_read (value, &bval)) {
			item->visible = !bval;
		}
		for (v = item->display; v != NULL; v = v->view.next) {
			if (!(v->view.flags & SP_ITEM_SHOW_PRINT)) {
				nr_arena_item_set_visible (v, item->visible);
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_SODIPODI_NONPRINTABLE:
		item->printable = TRUE;
		if (value && sp_svg_boolean_read (value, &bval)) {
			item->printable = !bval;
		}
		for (v = item->display; v != NULL; v = v->view.next) {
			if (v->view.flags & SP_ITEM_SHOW_PRINT) {
				nr_arena_item_set_visible (v, item->printable);
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
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
sp_item_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPItem *item;

	item = SP_ITEM (object);

	if (((SPObjectClass *) (parent_class))->update)
		(* ((SPObjectClass *) (parent_class))->update) (object, ctx, flags);

	if (flags & (SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG)) {
		SPItemView *v;
		if (flags & SP_OBJECT_MODIFIED_FLAG) {
			for (v = item->display; v != NULL; v = v->view.next) {
				nr_arena_item_set_transform (v, &item->transform);
			}
		}
		if ((item->clip) || (item->mask)) {
			NRRectF bbox;
			sp_item_invoke_bbox (item, &bbox, NULL, TRUE);
			if (item->clip) {
				for (v = item->display; v != NULL; v = v->view.next) {
					sp_clippath_set_bbox (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (v), &bbox);
				}
			}
			if (item->mask) {
				for (v = item->display; v != NULL; v = v->view.next) {
					sp_mask_set_bbox (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (v), &bbox);
				}
			}
		}
		if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
			for (v = item->display; v != NULL; v = v->view.next) {
				nr_arena_item_set_opacity (v, SP_SCALE24_TO_FLOAT (object->style->opacity.value));
			}
		}
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
sp_item_invoke_print (SPItem *item, SPPrintContext *ctx)
{
	if (item->printable) {
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

unsigned int
sp_item_get_extra_transform (SPItem *item, NRMatrixD *transform)
{
	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->get_extra_transform)
		return ((SPItemClass *) G_OBJECT_GET_CLASS (item))->get_extra_transform (item, transform);

	return 0;
}

/* fixme: Logic (Lauris) */

SPItem *
sp_item_get_viewport (SPItem *item, NRRectF *viewport, NRMatrixD *i2vp)
{
	SPObject *object;

	object = (SPObject *) item;

	nr_matrix_d_set_identity (i2vp);
	nr_rect_f_set_empty (viewport);

	/* Walk down the tree searching viewport */
	while (!item->has_viewport) {
		/* Fail if no parent or parent is not item */
		if (!object->parent || !SP_IS_ITEM (object->parent)) return NULL;
		/* Apply item->parent transform */
		nr_matrix_multiply_ddf (i2vp, i2vp, &item->transform);
		object = object->parent;
		item = (SPItem *) object;
		if (item->has_extra_transform) {
			/* Apply extra transform */
			sp_item_get_extra_transform (item, i2vp);
		}
	}

	/* Item has viewport defined itself */
	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->get_viewport) {
		((SPItemClass *) G_OBJECT_GET_CLASS (item))->get_viewport (item, viewport);
		return item;
	}

	return NULL;
}

static int
sp_item_private_snappoints (SPItem *item, NRPointF *p, int size, const NRMatrixF *transform)
{
        NRRectF bbox;
	NRMatrixD i2dd;
	if (size < 4) return 0;
	nr_matrix_d_from_f (&i2dd, transform);
	sp_item_invoke_bbox (item, &bbox, &i2dd, TRUE);
	p[0].x = bbox.x0;
	p[0].y = bbox.y0;
	p[1].x = bbox.x1;
	p[1].y = bbox.y0;
	p[2].x = bbox.x1;
	p[2].y = bbox.y1;
	p[3].x = bbox.x0;
	p[3].y = bbox.y1;
	return 4;
}

int
sp_item_snappoints (SPItem *item, NRPointF *p, int size, const NRMatrixF *transform)
{
	g_return_val_if_fail (item != NULL, 0);
	g_return_val_if_fail (SP_IS_ITEM (item), 0);

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->snappoints)
	        return ((SPItemClass *) G_OBJECT_GET_CLASS(item))->snappoints (item, p, size, transform);

	return 0;
}

static gchar *
sp_item_private_description (SPItem * item)
{
	return g_strdup (_("Unknown item :-("));
}

unsigned char *
sp_item_description (SPItem * item)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->description)
		return ((SPItemClass *) G_OBJECT_GET_CLASS (item))->description (item);

	g_assert_not_reached ();
	return NULL;
}

unsigned int
sp_item_display_key_new (unsigned int numkeys)
{
	static unsigned int dkey = 0;

	dkey += numkeys;

	return dkey - numkeys;
}

NRArenaItem *
sp_item_invoke_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	NRArenaItem *ai;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (arena != NULL);
	g_assert (NR_IS_ARENA (arena));

	ai = NULL;

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->show)
		ai = ((SPItemClass *) G_OBJECT_GET_CLASS (item))->show (item, arena, key, flags);

	if (ai != NULL) {
		item->display = nr_arena_item_view_new_prepend (item->display, flags, key, ai);
		nr_arena_item_set_transform (ai, &item->transform);
		nr_arena_item_set_opacity (ai, SP_SCALE24_TO_FLOAT (SP_OBJECT_STYLE (item)->opacity.value));
		nr_arena_item_set_sensitive (ai, item->sensitive);
		if (flags & SP_ITEM_SHOW_PRINT) {
			nr_arena_item_set_visible (ai, item->printable);
		} else {
			nr_arena_item_set_visible (ai, item->visible);
		}
		if (item->clip) {
			NRArenaItem *ac;
			if (!item->display->key) NR_ARENA_ITEM_SET_KEY (item->display, sp_item_display_key_new (3));
			ac = sp_clippath_show (SP_CLIPPATH (item->clip), arena, NR_ARENA_ITEM_GET_KEY (item->display));
			nr_arena_item_set_clip (ai, ac);
			nr_arena_item_unref (ac);
		}
		if (item->mask) {
			NRArenaItem *ac;
			if (!item->display->key) NR_ARENA_ITEM_SET_KEY (item->display, sp_item_display_key_new (3));
			ac = sp_mask_show (SP_MASK (item->mask), arena, NR_ARENA_ITEM_GET_KEY (item->display));
			nr_arena_item_set_mask (ai, ac);
			nr_arena_item_unref (ac);
		}
		NR_ARENA_ITEM_SET_DATA (ai, item);
	}

	return ai;
}

void
sp_item_invoke_hide (SPItem *item, unsigned int key)
{
	SPItemView *v, *ref, *next;

	assert (item != NULL);
	assert (SP_IS_ITEM (item));

	if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->hide)
		((SPItemClass *) G_OBJECT_GET_CLASS (item))->hide (item, key);

	ref = NULL;
	v = item->display;
	while (v != NULL) {
		next = v->view.next;
		if (v->view.key == key) {
			if (item->clip) {
				sp_clippath_hide (SP_CLIPPATH (item->clip), NR_ARENA_ITEM_GET_KEY (v));
				nr_arena_item_set_clip (v, NULL);
			}
			if (item->mask) {
				sp_mask_hide (SP_MASK (item->mask), NR_ARENA_ITEM_GET_KEY (v));
				nr_arena_item_set_mask (v, NULL);
			}
			if (!ref) {
				item->display = v->view.next;
			} else {
				/* fixme: This is dangerous (Lauris) */
				ref->view.next = v->view.next;
			}
			nr_arena_item_unparent (v);
			/* g_free (v); */
		} else {
			ref = v;
		}
		v = next;
	}
}

void
sp_item_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform)
{
	unsigned char c[80];
	NRMatrixF ltrans;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (repr != NULL);

	if (transform) {
		ltrans = *transform;
		if (((SPItemClass *) G_OBJECT_GET_CLASS (item))->write_transform)
			((SPItemClass *) G_OBJECT_GET_CLASS (item))->write_transform (item, repr, &ltrans);
		transform = &ltrans;
	}

	if (sp_svg_transform_write (c, 80, transform)) {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", c);
	} else {
		sp_repr_set_attr (SP_OBJECT_REPR (item), "transform", NULL);
	}

	/* Due to direct manipulation transforms may be out of sync */
	sp_object_read_attr (SP_OBJECT (item), "transform");
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
		item->transform = *transform;
		sp_object_request_update (SP_OBJECT (item), SP_OBJECT_MODIFIED_FLAG);
	}
}

NRMatrixF *
sp_item_i2doc_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD td;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	nr_matrix_d_set_identity (&td);

	while (SP_OBJECT_PARENT (item)) {
		nr_matrix_multiply_ddf (&td, &td, &item->transform);
		item = (SPItem *) SP_OBJECT_PARENT (item);
		/* We may hit <defs> or similar (Lauris) */
		if (!SP_IS_ITEM (item)) break;
		if (item->has_extra_transform) {
			/* Apply parent extra transform so we stay in master coordinates */
			sp_item_get_extra_transform (item, &td);
		}
	}

	/* g_return_val_if_fail (SP_IS_ROOT (item), NULL); */
	/* root = SP_ROOT (item); */
	/* fixme: (Lauris) */
	nr_matrix_multiply_ddf (&td, &td, &item->transform);

	nr_matrix_f_from_d (affine, &td);

	return affine;
}

NRMatrixF *
sp_item_i2root_affine (SPItem *item, NRMatrixF *affine)
{
	NRMatrixD td;
#if 0
	SPRoot *root;
#endif

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	nr_matrix_d_set_identity (&td);

	while (SP_OBJECT_PARENT (item)) {
		nr_matrix_multiply_ddf (&td, &td, &item->transform);
		item = (SPItem *) SP_OBJECT_PARENT (item);
		/* We may hit <defs> or similar (Lauris) */
		if (!SP_IS_ITEM (item)) break;
		/* For some reason we do not use c2p here */
		/* fixme: Which coordinate system we want after all (Lauris) */
	}

	/* g_return_val_if_fail (SP_IS_ROOT (item), NULL); */

#if 0
	root = SP_ROOT (item);

	/* fixme: (Lauris) */
	nr_matrix_multiply_ddd (&td, &td, &root->c2p);
	nr_matrix_multiply_ddf (&td, &td, &item->transform);
#endif

	nr_matrix_f_from_d (affine, &td);

	return affine;
}

void
sp_item_get_bbox_document (SPItem *item, NRRectF *bb, unsigned int flags, unsigned int clear)
{
	NRMatrixF i2docf;
	NRMatrixD i2docd;

	sp_item_i2doc_affine (item, &i2docf);
	nr_matrix_d_from_f (&i2docd, &i2docf);

	sp_item_invoke_bbox_full (item, bb, &i2docd, flags, clear);
}

#if 0

/* Item views */

static SPItemView *
sp_item_view_new_prepend (SPItemView * list, SPItem * item, unsigned int flags, unsigned int key, NRArenaItem *arenaitem)
{
	SPItemView * new;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (arenaitem != NULL);
	g_assert (NR_IS_ARENA_ITEM (arenaitem));

	new = g_new (SPItemView, 1);

	new->next = list;
	new->flags = flags;
	new->key = key;
	new->arenaitem = arenaitem;

	return new;
}

static SPItemView *
sp_item_view_list_remove (SPItemView *list, SPItemView *view)
{
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
}

#endif

/* Utility */
int
sp_corner_snappoints (NRPointF *p, int size, const NRMatrixF *transform, float x0, float y0, float x1, float y1)
{
	unsigned int i;
	i = 0;
	if (i < size) {
		p[i].x = NR_MATRIX_DF_TRANSFORM_X (transform, x0, y0);
		p[i].y = NR_MATRIX_DF_TRANSFORM_Y (transform, x0, y0);
		i += 1;
	}
	if (i < size) {
		p[i].x = NR_MATRIX_DF_TRANSFORM_X (transform, x1, y0);
		p[i].y = NR_MATRIX_DF_TRANSFORM_Y (transform, x1, y0);
		i += 1;
	}
	if (i < size) {
		p[i].x = NR_MATRIX_DF_TRANSFORM_X (transform, x1, y1);
		p[i].y = NR_MATRIX_DF_TRANSFORM_Y (transform, x1, y1);
		i += 1;
	}
	if (i < size) {
		p[i].x = NR_MATRIX_DF_TRANSFORM_X (transform, x0, y1);
		p[i].y = NR_MATRIX_DF_TRANSFORM_Y (transform, x0, y1);
		i += 1;
	}
	return i;
}

