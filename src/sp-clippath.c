#define __SP_CLIPPATH_C__

/*
 * SVG <g> implementation
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
static void sp_clippath_destroy (GtkObject *object);

static void sp_clippath_build (SPObject *object, SPDocument *document, SPRepr *repr);
static SPRepr *sp_clippath_write (SPObject *object, SPRepr *repr, guint flags);

static SPObjectGroupClass *parent_class;

GtkType
sp_clippath_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPClipPath",
			sizeof (SPClipPath),
			sizeof (SPClipPathClass),
			(GtkClassInitFunc) sp_clippath_class_init,
			(GtkObjectInitFunc) sp_clippath_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_OBJECTGROUP, &info);
	}
	return type;
}

static void
sp_clippath_class_init (SPClipPathClass *klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (SP_TYPE_OBJECTGROUP);

	gtk_object_class->destroy = sp_clippath_destroy;

	sp_object_class->build = sp_clippath_build;
	sp_object_class->write = sp_clippath_write;
}

static void
sp_clippath_init (SPClipPath *clippath)
{
}

static void
sp_clippath_destroy (GtkObject *object)
{
	SPClipPath *cp;

	cp = SP_CLIPPATH (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void sp_clippath_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPClipPath *cp;

	cp = SP_CLIPPATH (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);
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
sp_clippath_show (SPClipPath *cp, NRArena *arena)
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
			ac = sp_item_show (SP_ITEM (child), arena);
			if (ac) {
				nr_arena_item_add_child (ai, ac, ar);
				gtk_object_unref (GTK_OBJECT(ac));
				ar = ac;
			}
		}
	}

	return ai;
}


