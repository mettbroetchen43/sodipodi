#define SP_OBJECTGROUP_C

#include "sp-object-group.h"
#include "sp-object-repr.h"

static void sp_objectgroup_class_init (SPObjectGroupClass *klass);
static void sp_objectgroup_init (SPObjectGroup *objectgroup);
static void sp_objectgroup_destroy (GtkObject *object);

static void sp_objectgroup_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_objectgroup_add_child (SPObject * object, SPRepr * child);
static void sp_objectgroup_remove_child (SPObject * object, SPRepr * child);
static void sp_objectgroup_set_order (SPObject * object);

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
	sp_object_class->add_child = sp_objectgroup_add_child;
	sp_object_class->remove_child = sp_objectgroup_remove_child;
	sp_object_class->set_order = sp_objectgroup_set_order;
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
	SPObjectGroup * objectgroup;
	SPObject * spobject;

	objectgroup = SP_OBJECTGROUP (object);

	while (objectgroup->children) {
		spobject = SP_OBJECT (objectgroup->children->data);
		spobject->parent = NULL;
		gtk_object_unref ((GtkObject *) spobject);
		objectgroup->children = g_slist_remove_link (objectgroup->children, objectgroup->children);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void sp_objectgroup_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPObjectGroup * objectgroup;
	const GList * l;
	SPRepr * crepr;
	const gchar * name;
	GtkType type;
	SPObject * child;

	objectgroup = SP_OBJECTGROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->build)
		(* SP_OBJECT_CLASS (parent_class)->build) (object, document, repr);

	l = sp_repr_children (repr);

	while (l != NULL) {
		crepr = (SPRepr *) l->data;
		name = sp_repr_name (crepr);
		g_assert (name != NULL);
		type = sp_object_type_lookup (name);
		if (gtk_type_is_a (type, SP_TYPE_OBJECT)) {
			child = gtk_type_new (type);
			child->parent = object;
			objectgroup->children = g_slist_append (objectgroup->children, child);
			sp_object_invoke_build (child, document, crepr);
		}
		l = l->next;
	}
}

static void
sp_objectgroup_add_child (SPObject * object, SPRepr * child)
{
	SPObjectGroup * objectgroup;
	const gchar * name;
	GtkType type;
	SPObject * childobject;

	objectgroup = SP_OBJECTGROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->add_child)
		(* SP_OBJECT_CLASS (parent_class)->add_child) (object, child);

	name = sp_repr_name (child);
	g_assert (name != NULL);
	type = sp_object_type_lookup (name);
	g_return_if_fail (type > GTK_TYPE_NONE);

	childobject = gtk_type_new (type);
	childobject->parent = object;

	objectgroup->children = g_slist_append (objectgroup->children, childobject);
	sp_object_invoke_build (childobject, object->document, child);

	sp_objectgroup_set_order (object);
}

static void
sp_objectgroup_remove_child (SPObject * object, SPRepr * child)
{
	SPObjectGroup * objectgroup;
	GSList * l;
	SPObject * childobject;

	objectgroup = SP_OBJECTGROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->remove_child)
		(* SP_OBJECT_CLASS (parent_class)->remove_child) (object, child);

	for (l = objectgroup->children; l != NULL; l = l->next) {
		childobject = (SPObject *) l->data;
		if (childobject->repr == child) {
			objectgroup->children = g_slist_remove (objectgroup->children, childobject);
			childobject->parent = NULL;
			gtk_object_unref (GTK_OBJECT (childobject));
			return;
		}
	}

	g_assert_not_reached ();
}

static gint
sp_objectgroup_compare_children_pos (gconstpointer a, gconstpointer b)
{
	return sp_repr_compare_position (SP_OBJECT (a)->repr, SP_OBJECT (b)->repr);
}

/* fixme: all this is horribly ineffective */

static void
sp_objectgroup_set_order (SPObject * object)
{
	SPObjectGroup * objectgroup;
	GSList * neworder;
	GSList * l;

	objectgroup = SP_OBJECTGROUP (object);

	neworder = g_slist_copy (objectgroup->children);
	neworder = g_slist_sort (neworder, sp_objectgroup_compare_children_pos);

	for (l = neworder; l != NULL; l = l->next) {
		if (l->data == objectgroup->children->data) {
			objectgroup->children = g_slist_remove (objectgroup->children, objectgroup->children->data);
		} else {
			objectgroup->children = g_slist_remove (objectgroup->children, l->data);
		}
	}

	objectgroup->children = neworder;
}

