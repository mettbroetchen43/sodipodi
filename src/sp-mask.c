#define __SP_MASK_C__

/*
 * SVG <mask> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-arena.h"
#include "display/nr-arena-group.h"
#include "document.h"
#include "sp-item.h"
#include "sp-mask.h"

struct _SPMaskView {
	SPMaskView *next;
	unsigned int key;
	NRArenaItem *arenaitem;
};

static void sp_mask_class_init (SPMaskClass *klass);
static void sp_mask_init (SPMask *mask);

static void sp_mask_release (SPObject * object);
static void sp_mask_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_mask_remove_child (SPObject *object, SPRepr *child);
static void sp_mask_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_mask_modified (SPObject *object, guint flags);
static SPRepr *sp_mask_write (SPObject *object, SPRepr *repr, guint flags);

SPMaskView *sp_mask_view_new_prepend (SPMaskView *list, unsigned int key, NRArenaItem *arenaitem);
SPMaskView *sp_mask_view_list_remove (SPMaskView *list, SPMaskView *view);

static SPObjectGroupClass *parent_class;

GType
sp_mask_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPMaskClass),
			NULL, NULL,
			(GClassInitFunc) sp_mask_class_init,
			NULL, NULL,
			sizeof (SPMask),
			16,
			(GInstanceInitFunc) sp_mask_init,
		};
		type = g_type_register_static (SP_TYPE_OBJECTGROUP, "SPMask", &info, 0);
	}
	return type;
}

static void
sp_mask_class_init (SPMaskClass *klass)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;

	gobject_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_OBJECTGROUP);

	sp_object_class->release = sp_mask_release;
	sp_object_class->child_added = sp_mask_child_added;
	sp_object_class->remove_child = sp_mask_remove_child;
	sp_object_class->update = sp_mask_update;
	sp_object_class->modified = sp_mask_modified;
	sp_object_class->write = sp_mask_write;
}

static void
sp_mask_init (SPMask *mask)
{
	mask->display = NULL;
}

static void
sp_mask_release (SPObject * object)
{
	SPMask *cp;

	cp = SP_MASK (object);

	while (cp->display) {
		/* We simply unref and let item to manage this in handler */
		cp->display = sp_mask_view_list_remove (cp->display, cp->display);
	}

	if (((SPObjectClass *) (parent_class))->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_mask_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPMask *cp;
	SPObject *ochild;

	cp = SP_MASK (object);

	/* Invoke SPObjectGroup implementation */
	((SPObjectClass *) (parent_class))->child_added (object, child, ref);

	/* Show new object */
	ochild = sp_document_lookup_id (SP_OBJECT_DOCUMENT (object), sp_repr_attr (child, "id"));
	if (SP_IS_ITEM (ochild)) {
		SPMaskView *v;
		for (v = cp->display; v != NULL; v = v->next) {
			NRArenaItem *ac;
			ac = sp_item_show (SP_ITEM (ochild), NR_ARENA_ITEM_ARENA (v->arenaitem), v->key);
			if (ac) {
				nr_arena_item_add_child (v->arenaitem, ac, NULL);
				nr_arena_item_unref (ac);
			}
		}
	}
}

static void
sp_mask_remove_child (SPObject *object, SPRepr *child)
{
	SPMask *cp;

	cp = SP_MASK (object);

	/* Invoke SPObjectGroup implementation */
	((SPObjectClass *) (parent_class))->remove_child (object, child);
}

static void
sp_mask_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPObjectGroup *og;
	SPMask *cp;
	SPObject *child;
	GSList *l;

	og = SP_OBJECTGROUP (object);
	cp = SP_MASK (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	l = NULL;
	for (child = og->children; child != NULL; child = child->next) {
		g_object_ref (G_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_invoke_update (child, ctx, flags);
		}
		g_object_unref (G_OBJECT (child));
	}
}

static void
sp_mask_modified (SPObject *object, guint flags)
{
	SPObjectGroup *og;
	SPMask *cp;
	SPObject *child;
	GSList *l;

	og = SP_OBJECTGROUP (object);
	cp = SP_MASK (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	l = NULL;
	for (child = og->children; child != NULL; child = child->next) {
		g_object_ref (G_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (SP_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_invoke_modified (child, flags);
		}
		g_object_unref (G_OBJECT (child));
	}
}

static SPRepr *
sp_mask_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPMask *cp;

	cp = SP_MASK (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("mask");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

NRArenaItem *
sp_mask_show (SPMask *cp, NRArena *arena, unsigned int key)
{
	NRArenaItem *ai, *ac;
	SPObject *child;

	g_return_val_if_fail (cp != NULL, NULL);
	g_return_val_if_fail (SP_IS_MASK (cp), NULL);
	g_return_val_if_fail (arena != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA (arena), NULL);

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
	cp->display = sp_mask_view_new_prepend (cp->display, key, ai);

	for (child = SP_OBJECTGROUP (cp)->children; child != NULL; child = child->next) {
		if (SP_IS_ITEM (child)) {
			ac = sp_item_show (SP_ITEM (child), arena, key);
			if (ac) {
				/* The order is not important in mask */
				nr_arena_item_add_child (ai, ac, NULL);
				nr_arena_item_unref (ac);
			}
		}
	}

	return ai;
}

void
sp_mask_hide (SPMask *cp, unsigned int key)
{
	SPMaskView *v;
	SPObject *child;

	g_return_if_fail (cp != NULL);
	g_return_if_fail (SP_IS_MASK (cp));

	for (child = SP_OBJECTGROUP (cp)->children; child != NULL; child = child->next) {
		if (SP_IS_ITEM (child)) {
			sp_item_hide (SP_ITEM (child), key);
		}
	}

	for (v = cp->display; v != NULL; v = v->next) {
		if (v->key == key) {
			/* We simply unref and let item to manage this in handler */
			cp->display = sp_mask_view_list_remove (cp->display, v);
			return;
		}
	}

	g_assert_not_reached ();
}

/* Mask views */

SPMaskView *
sp_mask_view_new_prepend (SPMaskView *list, unsigned int key, NRArenaItem *arenaitem)
{
	SPMaskView *new;

	new = g_new (SPMaskView, 1);

	new->next = list;
	new->key = key;
	new->arenaitem = nr_arena_item_ref (arenaitem);

	return new;
}

SPMaskView *
sp_mask_view_list_remove (SPMaskView *list, SPMaskView *view)
{
	if (view == list) {
		list = list->next;
	} else {
		SPMaskView *prev;
		prev = list;
		while (prev->next != view) prev = prev->next;
		prev->next = view->next;
	}

	nr_arena_item_unref (view->arenaitem);
	g_free (view);

	return list;
}

