#define __SP_OBJECTGROUP_C__

/*
 * Abstract base class for all nodes
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "xml/repr-private.h"
#include "sp-object-group.h"
#include "sp-object-repr.h"

static void sp_objectgroup_class_init (SPObjectGroupClass *klass);
static void sp_objectgroup_init (SPObjectGroup *objectgroup);
static void sp_objectgroup_destroy (GtkObject *object);

static void sp_objectgroup_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_objectgroup_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_objectgroup_remove_child (SPObject * object, SPRepr * child);
static void sp_objectgroup_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);
static SPRepr *sp_objectgroup_write (SPObject *object, SPRepr *repr, guint flags);

static SPObject *sp_objectgroup_get_le_child_by_repr (SPObjectGroup *og, SPRepr *ref);

static SPObjectClass *parent_class;

GtkType
sp_objectgroup_get_type (void)
{
	static GtkType objectgroup_type = 0;
	if (!objectgroup_type) {
		GtkTypeInfo objectgroup_info = {
			"SPObjectGroup",
			sizeof (SPObjectGroup),
			sizeof (SPObjectGroupClass),
			(GtkClassInitFunc) sp_objectgroup_class_init,
			(GtkObjectInitFunc) sp_objectgroup_init,
			NULL, NULL, NULL
		};
		objectgroup_type = gtk_type_unique (sp_object_get_type (), &objectgroup_info);
	}
	return objectgroup_type;
}

static void
sp_objectgroup_class_init (SPObjectGroupClass *klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_objectgroup_destroy;

	sp_object_class->build = sp_objectgroup_build;
	sp_object_class->child_added = sp_objectgroup_child_added;
	sp_object_class->remove_child = sp_objectgroup_remove_child;
	sp_object_class->order_changed = sp_objectgroup_order_changed;
	sp_object_class->write = sp_objectgroup_write;
}

static void
sp_objectgroup_init (SPObjectGroup *objectgroup)
{
	objectgroup->children = NULL;
}

static void
sp_objectgroup_destroy (GtkObject *object)
{
	SPObjectGroup * og;

	og = SP_OBJECTGROUP (object);

	while (og->children) {
		og->children = sp_object_detach_unref (SP_OBJECT (og), og->children);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void sp_objectgroup_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPObjectGroup *og;
	SPObject *last;
	SPRepr *rchild;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject *child;
		type = sp_repr_type_lookup (rchild);
		if (gtk_type_is_a (type, SP_TYPE_OBJECT)) {
			child = gtk_type_new (type);
			(last) ? last->next : og->children = sp_object_attach_reref (object, child, NULL);
			sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
			last = child;
		}
	}
}

static void
sp_objectgroup_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPObjectGroup * og;
	GtkType type;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	type = sp_repr_type_lookup (child);
	if (gtk_type_is_a (type, SP_TYPE_OBJECT)) {
		SPObject *ochild, *prev;
		ochild = gtk_type_new (type);
		prev = sp_objectgroup_get_le_child_by_repr (og, ref);
		if (prev) {
			prev->next = sp_object_attach_reref (object, ochild, prev->next);
		} else {
			og->children = sp_object_attach_reref (object, ochild, og->children);
		}
		sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	}
}

static void
sp_objectgroup_remove_child (SPObject *object, SPRepr *child)
{
	SPObjectGroup *og;
	SPObject *ref, *oc;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	ref = NULL;
	oc = og->children;
	while (oc && (SP_OBJECT_REPR (oc) != child)) {
		ref = oc;
		oc = oc->next;
	}

	if (oc) {
		(ref) ? ref->next : og->children = sp_object_detach_unref (object, oc);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	}
}

static void
sp_objectgroup_order_changed (SPObject *object, SPRepr *child, SPRepr *old, SPRepr *new)
{
	SPObjectGroup *og;
	SPObject *ochild, *oold, *onew;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old, new);

	ochild = sp_objectgroup_get_le_child_by_repr (og, child);
	oold = sp_objectgroup_get_le_child_by_repr (og, old);
	onew = sp_objectgroup_get_le_child_by_repr (og, new);

	if (ochild) {
		(oold) ? oold->next : og->children = sp_object_detach (object, ochild);
		(onew) ? onew->next : og->children = sp_object_attach_reref (object, ochild, (onew) ? onew->next : og->children);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	}
}

static SPRepr *
sp_objectgroup_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPObjectGroup *group;
	SPObject *child;
	SPRepr *crepr;

	group = SP_OBJECTGROUP (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("g");
		l = NULL;
		for (child = group->children; child != NULL; child = child->next) {
			crepr = sp_object_invoke_write (child, NULL, flags);
			if (crepr) l = g_slist_prepend (l, crepr);
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = group->children; child != NULL; child = child->next) {
			sp_object_invoke_write (child, SP_OBJECT_REPR (child), flags);
		}
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static SPObject *
sp_objectgroup_get_le_child_by_repr (SPObjectGroup *og, SPRepr *ref)
{
	SPObject *o, *oc;
	SPRepr *r, *rc;

	if (!ref) return NULL;

	o = NULL;
	r = SP_OBJECT_REPR (og);
	rc = r->children;
	oc = og->children;

	while (rc) {
		if (rc == ref) return o;
		if (oc && (SP_OBJECT_REPR (oc) == rc)) {
			/* Rewing object */
			o = oc;
			oc = oc->next;
		}
		/* Rewind repr */
		rc = rc->next;
	}

	g_assert_not_reached ();

	return NULL;
}
