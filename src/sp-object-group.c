#define SP_OBJECTGROUP_C

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

static SPObject * sp_objectgroup_get_child_by_repr (SPObjectGroup * og, SPRepr * repr);

static SPObjectClass * parent_class;

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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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
}

static void
sp_objectgroup_init (SPObjectGroup *objectgroup)
{
	objectgroup->children = NULL;
	objectgroup->transparent = FALSE;
}

static void
sp_objectgroup_destroy (GtkObject *object)
{
	SPObjectGroup * og;

	og = SP_OBJECTGROUP (object);

	while (og->children) {
		SPObject * child;
		child = og->children;
		og->children = child->next;
		child->parent = NULL;
		child->next = NULL;
		gtk_object_unref (GTK_OBJECT (child));
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void sp_objectgroup_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPObjectGroup * og;
	SPObject * last;
	SPRepr * rchild;

	og = SP_OBJECTGROUP (object);

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
			og->children = child;
		}
		sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
		last = child;
	}
}

static void
sp_objectgroup_child_added (SPObject * object, SPRepr * child, SPRepr * ref)
{
	SPObjectGroup * og;
	GtkType type;
	SPObject * ochild, * prev;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	type = sp_object_type_lookup (sp_repr_name (child));
	ochild = gtk_type_new (type);
	ochild->parent = object;

	prev = sp_objectgroup_get_child_by_repr (og, ref);

	if (!prev) {
		ochild->next = og->children;
		og->children = ochild;
	} else {
		ochild->next = prev->next;
		prev->next = ochild;
	}

	sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));
}

static void
sp_objectgroup_remove_child (SPObject * object, SPRepr * child)
{
	SPObjectGroup * og;
	SPObject * prev, * ochild;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	prev = NULL;
	ochild = og->children;
	while (ochild && ochild->repr != child) {
		prev = ochild;
		ochild = ochild->next;
	}

	if (prev) {
		prev->next = ochild->next;
	} else {
		og->children = ochild->next;
	}
	ochild->parent = NULL;
	ochild->next = NULL;
	gtk_object_unref (GTK_OBJECT (ochild));
}

static void
sp_objectgroup_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new)
{
	SPObjectGroup * og;
	SPObject * ochild, * oold, * onew;

	og = SP_OBJECTGROUP (object);

	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old, new);

	ochild = sp_objectgroup_get_child_by_repr (og, child);
	oold = sp_objectgroup_get_child_by_repr (og, old);
	onew = sp_objectgroup_get_child_by_repr (og, new);

	if (oold) {
		oold->next = ochild->next;
	} else {
		og->children = ochild->next;
	}
	if (onew) {
		ochild->next = onew->next;
		onew->next = ochild;
	} else {
		ochild->next = og->children;
		og->children = ochild;
	}
}

static SPObject *
sp_objectgroup_get_child_by_repr (SPObjectGroup * og, SPRepr * repr)
{
	SPObject * o;

	if (!repr) return NULL;

	o = og->children;
	while (o->repr != repr) o = o->next;

	return o;
}
