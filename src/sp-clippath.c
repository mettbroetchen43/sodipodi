#define __SP_CLIPPATH_C__

/*
 * SVG <clipPath> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-arena.h"
#include "display/nr-arena-group.h"
#include "sp-item.h"
#include "sp-clippath.h"

static void sp_clippath_class_init (SPClipPathClass *klass);
static void sp_clippath_init (SPClipPath *clippath);

static void sp_clippath_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_clippath_modified (SPObject *object, guint flags);
static SPRepr *sp_clippath_write (SPObject *object, SPRepr *repr, guint flags);

static SPObjectGroupClass *parent_class;

GType
sp_clippath_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPClipPathClass),
			NULL, NULL,
			(GClassInitFunc) sp_clippath_class_init,
			NULL, NULL,
			sizeof (SPClipPath),
			16,
			(GInstanceInitFunc) sp_clippath_init,
		};
		type = g_type_register_static (SP_TYPE_OBJECTGROUP, "SPClipPath", &info, 0);
	}
	return type;
}

static void
sp_clippath_class_init (SPClipPathClass *klass)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;

	gobject_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_OBJECTGROUP);

	sp_object_class->update = sp_clippath_update;
	sp_object_class->modified = sp_clippath_modified;
	sp_object_class->write = sp_clippath_write;
}

static void
sp_clippath_init (SPClipPath *clippath)
{
	/* Nothing special */
}

static void
sp_clippath_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPObjectGroup *og;
	SPClipPath *cp;
	SPObject *child;
	GSList *l;

	og = SP_OBJECTGROUP (object);
	cp = SP_CLIPPATH (object);

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
sp_clippath_modified (SPObject *object, guint flags)
{
	SPObjectGroup *og;
	SPClipPath *cp;
	SPObject *child;
	GSList *l;

	og = SP_OBJECTGROUP (object);
	cp = SP_CLIPPATH (object);

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
sp_clippath_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPClipPath *cp;

	cp = SP_CLIPPATH (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("clipPath");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

NRArenaItem *
sp_clippath_show (SPClipPath *cp, NRArena *arena, unsigned int key)
{
	NRArenaItem *ai, *ac, *ar;
	SPObject *child;

	g_return_val_if_fail (cp != NULL, NULL);
	g_return_val_if_fail (SP_IS_CLIPPATH (cp), NULL);
	g_return_val_if_fail (arena != NULL, NULL);
	g_return_val_if_fail (NR_IS_ARENA (arena), NULL);

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);

	ar = NULL;
	for (child = SP_OBJECTGROUP (cp)->children; child != NULL; child = child->next) {
		if (SP_IS_ITEM (child)) {
			ac = sp_item_show (SP_ITEM (child), arena, key);
			if (ac) {
				nr_arena_item_add_child (ai, ac, ar);
				g_object_unref (G_OBJECT (ac));
				ar = ac;
			}
		}
	}

	return ai;
}

void
sp_clippath_hide (SPClipPath *cp, unsigned int key)
{
	SPObject *child;

	g_return_if_fail (cp != NULL);
	g_return_if_fail (SP_IS_CLIPPATH (cp));

	for (child = SP_OBJECTGROUP (cp)->children; child != NULL; child = child->next) {
		if (SP_IS_ITEM (child)) {
			sp_item_hide (SP_ITEM (child), key);
		}
	}
}

