#define SP_DEFS_C

/*
 * fixme: We should really check childrens validity - currently everything
 * flips in
 */

#include "sp-defs.h"

static void sp_defs_class_init (SPDefsClass * klass);
static void sp_defs_init (SPDefs * defs);
static void sp_defs_destroy (GtkObject * object);

static void sp_defs_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_defs_add_child (SPObject * object, SPRepr * child);
static void sp_defs_remove_child (SPObject * object, SPRepr * child);

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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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
	sp_object_class->add_child = sp_defs_add_child;
	sp_object_class->remove_child = sp_defs_remove_child;
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
	SPObject * spobject;

	defs = (SPDefs *) object;

	while (defs->children) {
		spobject = SP_OBJECT (defs->children->data);
		spobject->parent = NULL;
		gtk_object_unref (GTK_OBJECT (spobject));
		defs->children = g_slist_remove_link (defs->children, defs->children);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void sp_defs_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPDefs * defs;
	SPRepr * crepr;
	SPObject * child;
	const GList * l;

	defs = SP_DEFS (object);

	if (SP_OBJECT_CLASS (parent_class)->build)
		(* SP_OBJECT_CLASS (parent_class)->build) (object, document, repr);

	l = sp_repr_children (repr);

	while (l != NULL) {
		crepr = (SPRepr *) l->data;
		child = gtk_type_new (SP_TYPE_DEFS);
		child->parent = object;
		defs->children = g_slist_prepend (defs->children, child);
		sp_object_invoke_build (child, document, crepr, SP_OBJECT_IS_CLONED (object));
		l = l->next;
	}
}

static void
sp_defs_add_child (SPObject * object, SPRepr * child)
{
	SPDefs * defs;
	SPObject * childobject;

	defs = SP_DEFS (object);

	if (SP_OBJECT_CLASS (parent_class)->add_child)
		(* SP_OBJECT_CLASS (parent_class)->add_child) (object, child);

	childobject = gtk_type_new (SP_TYPE_DEFS);
	childobject->parent = object;
	defs->children = g_slist_prepend (defs->children, childobject);
	sp_object_invoke_build (childobject, object->document, child, SP_OBJECT_IS_CLONED (object));
}

static void
sp_defs_remove_child (SPObject * object, SPRepr * child)
{
	SPDefs * defs;
	GSList * l;
	SPObject * childobject;

	defs = SP_DEFS (object);

	if (SP_OBJECT_CLASS (parent_class)->remove_child)
		(* SP_OBJECT_CLASS (parent_class)->remove_child) (object, child);

	for (l = defs->children; l != NULL; l = l->next) {
		childobject = (SPObject *) l->data;
		if (childobject->repr == child) {
			defs->children = g_slist_remove (defs->children, childobject);
			childobject->parent = NULL;
			gtk_object_destroy (GTK_OBJECT (childobject));
			return;
		}
	}
	g_assert_not_reached ();
}



