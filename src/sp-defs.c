#define __SP_DEFS_C__

/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme: We should really check childrens validity - currently everything
 * flips in
 */

#include "xml/repr-private.h"
#include "sp-object-repr.h"
#include "sp-defs.h"

static void sp_defs_class_init (SPDefsClass * klass);
static void sp_defs_init (SPDefs * defs);

static void sp_defs_build (SPObject *object, SPDocument * document, SPRepr * repr);
static void sp_defs_release (SPObject *object);
static void sp_defs_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_defs_remove_child (SPObject * object, SPRepr * child);
static void sp_defs_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);
static void sp_defs_modified (SPObject *object, guint flags);
static SPRepr *sp_defs_write (SPObject *object, SPRepr *repr, guint flags);

static SPObject * sp_defs_get_child_by_repr (SPDefs * defs, SPRepr * repr);

static SPObjectClass * parent_class;

GtkType
sp_defs_get_type (void)
{
	static GtkType defs_type = 0;
	if (!defs_type) {
		GtkTypeInfo defs_info = {
			"SPDefs",
			sizeof (SPDefs),
			sizeof (SPDefsClass),
			(GtkClassInitFunc) sp_defs_class_init,
			(GtkObjectInitFunc) sp_defs_init,
			NULL, NULL, NULL
		};
		defs_type = gtk_type_unique (sp_object_get_type (), &defs_info);
	}
	return defs_type;
}

static void
sp_defs_class_init (SPDefsClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	sp_object_class->build = sp_defs_build;
	sp_object_class->release = sp_defs_release;
	sp_object_class->child_added = sp_defs_child_added;
	sp_object_class->remove_child = sp_defs_remove_child;
	sp_object_class->order_changed = sp_defs_order_changed;
	sp_object_class->modified = sp_defs_modified;
	sp_object_class->write = sp_defs_write;
}

static void
sp_defs_init (SPDefs *defs)
{
	defs->children = NULL;
}

static void sp_defs_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPDefs * defs;
	SPObject * last;
	SPRepr * rchild;

	defs = SP_DEFS (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject * child;
		type = sp_repr_type_lookup (rchild);
		child = gtk_type_new (type);
		last ? last->next : defs->children = sp_object_attach_reref (object, child, NULL);
		sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
		last = child;
	}
}

static void
sp_defs_release (SPObject * object)
{
	SPDefs *defs;

	defs = (SPDefs *) object;

	while (defs->children) {
		defs->children = sp_object_detach_unref (SP_OBJECT (defs), defs->children);
	}

	if (((SPObjectClass *) (parent_class))->release)
		((SPObjectClass *) (parent_class))->release (object);
}

static void
sp_defs_child_added (SPObject * object, SPRepr * child, SPRepr * ref)
{
	SPDefs * defs;
	SPObject * ochild, * prev;
	GtkType type;

	defs = SP_DEFS (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	type = sp_repr_type_lookup (child);
	ochild = gtk_type_new (type);
	ochild->parent = object;

	prev = sp_defs_get_child_by_repr (defs, ref);

	if (!prev) {
		ochild->next = defs->children;
		defs->children = ochild;
	} else {
		ochild->next = prev->next;
		prev->next = ochild;
	}

	sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));
}

static void
sp_defs_remove_child (SPObject * object, SPRepr * child)
{
	SPDefs * defs;
	SPObject * prev, * ochild;

	defs = SP_DEFS (object);

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	prev = NULL;
	ochild = defs->children;
	while (ochild && ochild->repr != child) {
		prev = ochild;
		ochild = ochild->next;
	}

	if (prev) {
		prev->next = ochild->next;
	} else {
		defs->children = ochild->next;
	}
	ochild->parent = NULL;
	ochild->next = NULL;
	gtk_object_unref (GTK_OBJECT (ochild));
}

static void
sp_defs_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new)
{
	SPDefs * defs;
	SPObject * ochild, * oold, * onew;

	defs = SP_DEFS (object);

	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old, new);

	ochild = sp_defs_get_child_by_repr (defs, child);
	oold = sp_defs_get_child_by_repr (defs, old);
	onew = sp_defs_get_child_by_repr (defs, new);

	if (oold) {
		oold->next = ochild->next;
	} else {
		defs->children = ochild->next;
	}
	if (onew) {
		ochild->next = onew->next;
		onew->next = ochild;
	} else {
		ochild->next = defs->children;
		defs->children = ochild;
	}
}

static void
sp_defs_modified (SPObject *object, guint flags)
{
	SPDefs *defs;
	SPObject *child;
	GSList *l;

	defs = SP_DEFS (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	/* fixme: Not sure, whether we should cascade style modification here (Lauris) */
	flags &= SP_OBJECT_PARENT_MODIFIED_FLAG;

	l = NULL;
	for (child = defs->children; child != NULL; child = child->next) {
		gtk_object_ref (GTK_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (GTK_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_modified (child, flags);
		}
		gtk_object_unref (GTK_OBJECT (child));
	}
}

static SPRepr *
sp_defs_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPDefs *defs;
	SPObject *child;
	SPRepr *crepr;


	defs = SP_DEFS (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("defs");
		l = NULL;
		for (child = defs->children; child != NULL; child = child->next) {
			crepr = sp_object_invoke_write (child, NULL, flags);
			l = g_slist_prepend (l, crepr);
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = defs->children; child != NULL; child = child->next) {
			sp_object_invoke_write (child, SP_OBJECT_REPR (child), flags);
		}
	}

	if (((SPObjectClass *) (parent_class))->write)
		(* ((SPObjectClass *) (parent_class))->write) (object, repr, flags);

	return repr;
}

static SPObject *
sp_defs_get_child_by_repr (SPDefs * defs, SPRepr * repr)
{
	SPObject * o;

	if (!repr) return NULL;

	o = defs->children;
	while (o->repr != repr) o = o->next;

	return o;
}


