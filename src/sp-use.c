#define __SP_USE_C__

/*
 * SVG <svg> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <glib-object.h>
#include "helper/sp-intl.h"
#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "attributes.h"
#include "document.h"
#include "sp-object-repr.h"
#include "sp-use.h"

/* fixme: */
#include "desktop-events.h"

static void sp_use_class_init (SPUseClass *class);
static void sp_use_init (SPUse *use);

static void sp_use_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_use_release (SPObject *object);
static void sp_use_set (SPObject *object, unsigned int key, const unsigned char *value);
static SPRepr *sp_use_write (SPObject *object, SPRepr *repr, guint flags);
static void sp_use_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_use_modified (SPObject *object, guint flags);

static void sp_use_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static void sp_use_print (SPItem *item, SPPrintContext *ctx);
static gchar * sp_use_description (SPItem * item);
static NRArenaItem *sp_use_show (SPItem *item, NRArena *arena);
static void sp_use_hide (SPItem *item, NRArena *arena);

static void sp_use_href_changed (SPUse * use);

static SPItemClass * parent_class;

GType
sp_use_get_type (void)
{
	static GType use_type = 0;
	if (!use_type) {
		GTypeInfo use_info = {
			sizeof (SPUseClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_use_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPUse),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_use_init,
		};
		use_type = g_type_register_static (SP_TYPE_ITEM, "SPUse", &use_info, 0);
	}
	return use_type;
}

static void
sp_use_class_init (SPUseClass *class)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gobject_class = (GObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = g_type_class_ref (SP_TYPE_ITEM);

	sp_object_class->build = sp_use_build;
	sp_object_class->release = sp_use_release;
	sp_object_class->set = sp_use_set;
	sp_object_class->write = sp_use_write;
	sp_object_class->update = sp_use_update;
	sp_object_class->modified = sp_use_modified;

	item_class->bbox = sp_use_bbox;
	item_class->description = sp_use_description;
	item_class->print = sp_use_print;
	item_class->show = sp_use_show;
	item_class->hide = sp_use_hide;
}

static void
sp_use_init (SPUse * use)
{
	sp_svg_length_unset (&use->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&use->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&use->width, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&use->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
	use->href = NULL;
}

static void
sp_use_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPUse * use;

	use = SP_USE (object);

	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "width");
	sp_object_read_attr (object, "height");
	sp_object_read_attr (object, "xlink:href");

	if (use->href) {
		SPObject *refobj;
		refobj = sp_document_lookup_id (document, use->href);
		if (refobj) {
			SPRepr *childrepr;
			GType type;
			childrepr = SP_OBJECT_REPR (refobj);
			type = sp_repr_type_lookup (childrepr);
			g_return_if_fail (type > G_TYPE_NONE);
			if (g_type_is_a (type, SP_TYPE_ITEM)) {
				SPObject *childobj;
				childobj = g_object_new (type, 0);
				use->child = sp_object_attach_reref (object, childobj, NULL);
				sp_object_invoke_build (childobj, document, childrepr, TRUE);
			}
		}
	}
}

static void
sp_use_release (SPObject *object)
{
	SPUse *use;

	use = SP_USE (object);

	if (use->child) {
		use->child = sp_object_detach_unref (SP_OBJECT (object), use->child);
	}

	if (use->href) g_free (use->href);

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_use_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPUse *use;

	use = SP_USE (object);

	switch (key) {
	case SP_ATTR_X:
		if (!sp_svg_length_read (value, &use->x)) {
			sp_svg_length_unset (&use->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (!sp_svg_length_read (value, &use->y)) {
			sp_svg_length_unset (&use->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_WIDTH:
		if (!sp_svg_length_read (value, &use->width)) {
			sp_svg_length_unset (&use->width, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_HEIGHT:
		if (!sp_svg_length_read (value, &use->height)) {
			sp_svg_length_unset (&use->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_XLINK_HREF: {
		if (value) {
			if (use->href) {
				if (strcmp (value, use->href) == 0) return;
				g_free (use->href);
				use->href = g_strdup (value + 1);
			} else {
				use->href = g_strdup (value + 1);
			}
		} else {
			if (use->href) {
				g_free (use->href);
				use->href = NULL;
			}
		}
		sp_use_href_changed (use);
		break;
	}
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static SPRepr *
sp_use_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPUse *use;

	use = SP_USE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("use");
	}

	sp_repr_set_attr (repr, "id", object->id);
	sp_repr_set_attr (repr, "xlink:href", use->href);
	/* fixme: Implement x, y, width, height */

	return repr;
}

static void
sp_use_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) {
		sp_item_invoke_bbox_full (SP_ITEM (use->child), bbox, transform, flags, FALSE);
	}
}

static void
sp_use_print (SPItem *item, SPPrintContext *ctx)
{
	SPUse * use;

	use = SP_USE (item);

	/* fixme: transformations? (lauris) */
	if (use->child) sp_item_invoke_print (SP_ITEM (use->child), ctx);
}

static gchar *
sp_use_description (SPItem * item)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) return sp_item_description (SP_ITEM (use->child));

	return g_strdup ("Empty reference [SHOULDN'T HAPPEN]");
}

static NRArenaItem *
sp_use_show (SPItem *item, NRArena *arena)
{
	SPUse *use;

	use = SP_USE (item);

	if (use->child) {
		NRArenaItem *ai, *ac;
		ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
		nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);
		ac = sp_item_show (SP_ITEM (use->child), arena);
		if (ac) {
			nr_arena_item_add_child (ai, ac, NULL);
			g_object_unref (G_OBJECT(ac));
		}
		return ai;
	}
		
	return NULL;
}

static void
sp_use_hide (SPItem * item, NRArena *arena)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) sp_item_hide (SP_ITEM (use->child), arena);

	if (((SPItemClass *) parent_class)->hide)
		((SPItemClass *) parent_class)->hide (item, arena);
}

static void
sp_use_href_changed (SPUse * use)
{
	SPItem * item;

	item = SP_ITEM (use);

	if (use->child) {
		use->child = sp_object_detach_unref (SP_OBJECT (use), use->child);
	}

	if (use->href) {
		SPObject * refobj;
		refobj = sp_document_lookup_id (SP_OBJECT (use)->document, use->href);
		if (refobj) {
			SPRepr * repr;
			GType type;
			repr = refobj->repr;
			type = sp_repr_type_lookup (repr);
			g_return_if_fail (type > G_TYPE_NONE);
			if (g_type_is_a (type, SP_TYPE_ITEM)) {
				SPObject * childobj;
				SPItemView * v;
				childobj = g_object_new (type, 0);
				use->child = sp_object_attach_reref (SP_OBJECT (use), childobj, NULL);
				sp_object_invoke_build (childobj, SP_OBJECT (use)->document, repr, TRUE);
				for (v = item->display; v != NULL; v = v->next) {
					NRArenaItem *ai;
					ai = sp_item_show (SP_ITEM (childobj), v->arena);
					if (ai) {
						nr_arena_item_add_child (v->arenaitem, ai, NULL);
						g_object_unref (G_OBJECT(ai));
					}
				}
			}
		}
	}
}
static void
sp_use_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	SPUse *use_obj;
	SPObject *child;
	SPItemCtx *ictx, cctx;

	use_obj = SP_USE (object);
	ictx = (SPItemCtx *) ctx;
	cctx = *ictx;

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	child = use_obj->child;
	if (child) {
		g_object_ref (G_OBJECT (child));
		if (flags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			if (SP_IS_ITEM (child)) {
				SPItem *chi;
				chi = SP_ITEM (child);
				nr_matrix_multiply_dfd (&cctx.i2doc, &chi->transform, &ictx->i2doc);
				sp_object_invoke_update (child, (SPCtx *) &cctx, flags);
			} else {
				sp_object_invoke_update (child, ctx, flags);
			}
		}
		g_object_unref (G_OBJECT (child));
	}
}

static void
sp_use_modified (SPObject *object, guint flags)
{
	SPUse *use_obj;
	SPObject *child;

	use_obj = SP_USE (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	child = use_obj->child;
	if (child) {
		g_object_ref (G_OBJECT (child));
		if (flags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_invoke_modified (child, flags);
		}
		g_object_unref (G_OBJECT (child));
	}
}
