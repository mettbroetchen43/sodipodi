#define SP_OBJECT_GROUP_C

/*
 * SPObject
 *
 * This is most abstract of all typed objects
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include "sp-object.h"

static void sp_object_class_init (SPObjectClass * klass);
static void sp_object_init (SPObject * object);
static void sp_object_destroy (GtkObject * object);

static void sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr);

static void sp_object_read_attr (SPObject * object, const gchar * key);

/* Real handlers of repr signals */

static void sp_object_repr_destroy (SPRepr * repr, gpointer data);

static void sp_object_repr_change_attr (SPRepr * repr, const gchar * key, gpointer data);
static void sp_object_repr_change_content (SPRepr * repr, gpointer data);
static void sp_object_repr_change_order (SPRepr * repr, gpointer data);
static void sp_object_repr_add_child (SPRepr * repr, SPRepr * child, gpointer data);
static void sp_object_repr_remove_child (SPRepr * repr, SPRepr * child, gpointer data);

static gboolean sp_object_repr_order_changed_pre (SPRepr * repr, gint order, gpointer data);
static gboolean sp_object_repr_attr_changed_pre (SPRepr * repr, const gchar * key, const gchar * value, gpointer data);
static gboolean sp_object_repr_content_changed_pre (SPRepr * repr, const gchar * value, gpointer data);

static gchar * sp_object_get_unique_id (SPObject * object, const gchar * defid);

static void sp_object_set_title (SPObject * object, SPRepr * repr);
static void sp_object_set_description (SPObject * object, SPRepr * repr);

static GtkObjectClass * parent_class;

GtkType
sp_object_get_type (void)
{
	static GtkType object_type = 0;
	if (!object_type) {
		GtkTypeInfo object_info = {
			"SPObject",
			sizeof (SPObject),
			sizeof (SPObjectClass),
			(GtkClassInitFunc) sp_object_class_init,
			(GtkObjectInitFunc) sp_object_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		object_type = gtk_type_unique (gtk_object_get_type (), &object_info);
	}
	return object_type;
}

static void
sp_object_class_init (SPObjectClass * klass)
{
	GtkObjectClass * gtk_object_class;

	gtk_object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	gtk_object_class->destroy = sp_object_destroy;

	klass->build = sp_object_build;
	klass->read_attr = sp_object_read_attr;
}

static void
sp_object_init (SPObject * object)
{
	object->document = NULL;
	object->parent = NULL;
	object->repr = NULL;
	object->id = NULL;
	object->title = NULL;
	object->description = NULL;
}

static void
sp_object_destroy (GtkObject * object)
{
	SPObject * spobject;

	spobject = (SPObject *) object;

	/* Parent refcount us, so there shouldn't be any */

	g_assert (spobject->parent == NULL);

	if (spobject->id) {
		if (spobject->document)
			sp_document_undef_id (spobject->document, spobject->id);
		g_free (spobject->id);
	}

	if (spobject->repr) {
		/* Signals will be disconnected, if we are destroyed */
		sp_repr_unref (spobject->repr);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/*
 * SPObject specific build method
 */

static void
sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	const GList * l;
	const gchar * name;

	/* Set object title and description */

	l = sp_repr_children (repr);

	while (l != NULL) {
		name = sp_repr_name ((SPRepr *) l->data);
		g_assert (name != NULL);
		if (strcmp (name, "title") == 0) {
			sp_object_set_title (object, (SPRepr *) l->data);
		}
		if (strcmp (name, "description") == 0) {
			sp_object_set_description (object, (SPRepr *) l->data);
		}
		l = l->next;
	}
}

void
sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	const gchar * id;
	gchar * realid;
	gboolean ret;

	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));

	g_assert (object->document == NULL);
	g_assert (object->repr == NULL);
	g_assert (object->id == NULL);

	/* Bookkeeping */

	object->document = document;
	object->repr = repr;
	sp_repr_ref (repr);

	/* Define ID */

	id = sp_repr_attr (repr, "id");
	realid = sp_object_get_unique_id (object, id);
	g_assert (realid != NULL);
	sp_document_def_id (document, realid, object);
	object->id = realid;

	/* Redefine ID, if required */

	if ((id == NULL) || (strcmp (id, realid) != 0)) {
		ret = sp_repr_set_attr (repr, "id", realid);
		g_assert (ret);
	}

	/* Invoke derived methods, if any */

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->build)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->build) (object, document, repr);

	/* Signalling (should be connected AFTER processing derived methods */

	sp_repr_set_signal (repr, "destroy", sp_object_repr_destroy, object);
	sp_repr_set_signal (repr, "change_attr", sp_object_repr_attr_changed_pre, object);
	sp_repr_set_signal (repr, "attr_changed", sp_object_repr_change_attr, object);
	sp_repr_set_signal (repr, "change_content", sp_object_repr_content_changed_pre, object);
	sp_repr_set_signal (repr, "content_changed", sp_object_repr_change_content, object);
	sp_repr_set_signal (repr, "change_order", sp_object_repr_order_changed_pre, object);
	sp_repr_set_signal (repr, "order_changed", sp_object_repr_change_order, object);
	sp_repr_set_signal (repr, "child_added", sp_object_repr_add_child, object);
	sp_repr_set_signal (repr, "remove_child", sp_object_repr_remove_child, object);
}

static void
sp_object_read_attr (SPObject * object, const gchar * key)
{
	const gchar * id;

	g_assert (SP_IS_DOCUMENT (object->document));
	g_assert (object->id != NULL);
	g_assert (key != NULL);

	if (strcmp (key, "id") == 0) {
		id = sp_repr_attr (object->repr, "id");
		g_assert (id != NULL);

		if (strcmp (id, object->id) == 0) return;

		g_assert (!sp_document_lookup_id (object->document, id));

		sp_document_undef_id (object->document, object->id);
		g_free (object->id);
		object->id = g_strdup (id);
		sp_document_def_id (object->document, object->id, object);
		return;
	}
}

void
sp_object_invoke_read_attr (SPObject * object, const gchar * key)
{
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (key != NULL);

	g_assert (SP_IS_DOCUMENT (object->document));
	g_assert (object->repr != NULL);
	g_assert (object->id != NULL);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr) (object, key);
}

static gchar *
sp_object_get_unique_id (SPObject * object, const gchar * id)
{
	static gint count = 0;
	const gchar * name;
	gchar * realid;
	gchar * b;
	gint len;

	g_assert (SP_IS_OBJECT (object));
	g_assert (SP_IS_DOCUMENT (object->document));

	count++;

	realid = NULL;

	if (id != NULL) {
		if (sp_document_lookup_id (object->document, id) == NULL) {
			realid = g_strdup (id);
			g_assert (realid != NULL);
		}
	}

	if (realid == NULL) {
		name = sp_repr_name (object->repr);
		g_assert (name != NULL);
		len = strlen (name) + 17;
		b = alloca (len);
		g_assert (b != NULL);
	}

	while (realid == NULL) {
		g_snprintf (b, len, "%s%d", name, count);
		if (sp_document_lookup_id (object->document, b) == NULL) {
			realid = g_strdup (b);
			g_assert (realid != NULL);
		} else {
			count++;
		}
	}

	return realid;
}

/*
 * Repr cannot be destroyed while "destroy" connected, because we ref it
 */

static void
sp_object_repr_destroy (SPRepr * repr, gpointer data)
{
	g_assert_not_reached ();
}

static gboolean
sp_object_repr_attr_changed_pre (SPRepr * repr, const gchar * key, const gchar * value, gpointer data)
{
	SPObject * object;
	gpointer defid;

	g_assert (repr != NULL);
	g_assert (key != NULL);
	g_assert (data != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	if (strcmp (key, "id") == 0) {
		defid = sp_document_lookup_id (object->document, value);
		if (defid == object) return TRUE;
		if (defid) return FALSE;
	}

	return sp_document_change_attr_requested (object->document, object, key, value);
}

static void
sp_object_repr_change_attr (SPRepr * repr, const gchar * key, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (key != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	sp_object_invoke_read_attr (object, key);
}

static gboolean
sp_object_repr_content_changed_pre (SPRepr * repr, const gchar * value, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	return sp_document_change_content_requested (object->document, object, value);
}

static void
sp_object_repr_change_content (SPRepr * repr, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_content)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_content) (object);
}

static gboolean
sp_object_repr_order_changed_pre (SPRepr * repr, gint order, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	return sp_document_change_order_requested (object->document, object, order);
}

/* fixme: */

static void
sp_object_repr_change_order (SPRepr * repr, gpointer data)
{
	SPObject * object, * parent;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	parent = object->parent;

	if (((SPObjectClass *)(((GtkObject *) parent)->klass))->set_order)
		(*((SPObjectClass *)(((GtkObject *) parent)->klass))->set_order) (parent);
}

static void
sp_object_repr_add_child (SPRepr * repr, SPRepr * child, gpointer data)
{
	SPObject * object;
	const gchar * name;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);
	g_assert (sp_repr_parent (child) == repr);

	name = sp_repr_name (child);

	/*
	 * fixme: probably we should have <title> and <description> objects
	 * to handle foreign namespaces, connect modified signals etc.
	 */

	if (strcmp (name, "title") == 0) sp_object_set_title (object, repr);
	if (strcmp (name, "description") == 0) sp_object_set_description (object, repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->add_child)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->add_child) (object, child);
}

static void
sp_object_repr_remove_child (SPRepr * repr, SPRepr * child, gpointer data)
{
	SPObject * object;
	const gchar * name;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr == repr);

	name = sp_repr_name (child);

	if (strcmp (name, "title") == 0) object->title = NULL;
	if (strcmp (name, "description") == 0) object->title = NULL;

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child) (object, child);
}

static void
sp_object_set_title (SPObject * object, SPRepr * repr)
{
	const gchar * content;

	content = sp_repr_content (repr);

	object->title = content;
}

static void
sp_object_set_description (SPObject * object, SPRepr * repr)
{
	const gchar * content;

	content = sp_repr_content (repr);

	object->description = content;
}

