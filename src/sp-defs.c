#define SP_DEFS_C

/*
 * fixme: We should really check childrens validity - currently everything
 * flips in
 */

#include "xml/repr-private.h"
#include "sp-object-repr.h"
#include "sp-defs.h"

static void sp_defs_class_init (SPDefsClass * klass);
static void sp_defs_init (SPDefs * defs);
static void sp_defs_destroy (GtkObject * object);

static void sp_defs_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_defs_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_defs_remove_child (SPObject * object, SPRepr * child);
static void sp_defs_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);

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

	gtk_object_class->destroy = sp_defs_destroy;

	sp_object_class->build = sp_defs_build;
	sp_object_class->child_added = sp_defs_child_added;
	sp_object_class->remove_child = sp_defs_remove_child;
	sp_object_class->order_changed = sp_defs_order_changed;
}

static void
sp_defs_init (SPDefs *defs)
{
	defs->children = NULL;
}

static void
sp_defs_destroy (GtkObject * object)
{
	SPDefs * defs;

	defs = (SPDefs *) object;

	while (defs->children) {
		SPObject * child;
		child = defs->children;
		child->parent = NULL;
		child->next = NULL;
		defs->children = child->next;
		gtk_object_unref (GTK_OBJECT (child));
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
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
		type = sp_object_type_lookup (sp_repr_name (rchild));
		child = gtk_type_new (type);
		child->parent = object;
		if (last) {
			last->next = child;
		} else {
			defs->children = child;
		}
		sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
		last = child;
	}
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

	type = sp_object_type_lookup (sp_repr_name (child));
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

static SPObject *
sp_defs_get_child_by_repr (SPDefs * defs, SPRepr * repr)
{
	SPObject * o;

	if (!repr) return NULL;

	o = defs->children;
	while (o->repr != repr) o = o->next;

	return o;
}


