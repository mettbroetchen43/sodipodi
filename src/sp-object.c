#define SP_OBJECT_C

/*
 * SPObject
 *
 * This is most abstract of all typed objects
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 1999-2000
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtksignal.h>
#include "xml/repr-private.h"
#include "document.h"
#include "style.h"
#include "sp-object-repr.h"
#include "sp-object.h"

static void sp_object_class_init (SPObjectClass * klass);
static void sp_object_init (SPObject * object);
static void sp_object_destroy (GtkObject * object);

static void sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_object_read_attr (SPObject * object, const gchar * key);

/* Real handlers of repr signals */

static gboolean sp_object_repr_change_attr (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);
static void sp_object_repr_attr_changed (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);

static void sp_object_repr_content_changed (SPRepr *repr, const guchar *oldcontent, const guchar *newcontent, gpointer data);

static void sp_object_repr_child_added (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
static gboolean sp_object_repr_remove_child (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);

static void sp_object_repr_order_changed (SPRepr *repr, SPRepr *child, SPRepr *old, SPRepr *new, gpointer data);

static gchar * sp_object_get_unique_id (SPObject * object, const gchar * defid);

enum {MODIFIED, LAST_SIGNAL};

SPReprEventVector object_event_vector = {
	NULL, /* Add child */
	sp_object_repr_child_added,
	sp_object_repr_remove_child,
	NULL, /* Child removed */
	sp_object_repr_change_attr,
	sp_object_repr_attr_changed,
	NULL, /* Change content */
	sp_object_repr_content_changed,
	NULL, /* change_order */
	sp_object_repr_order_changed
};

static GtkObjectClass *parent_class;
static guint object_signals[LAST_SIGNAL] = {0};

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

	object_signals[MODIFIED] = gtk_signal_new ("modified",
						   GTK_RUN_FIRST,
						   gtk_object_class->type,
						   GTK_SIGNAL_OFFSET (SPObjectClass, modified),
						   gtk_marshal_NONE__UINT,
						   GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	gtk_object_class_add_signals (gtk_object_class, object_signals, LAST_SIGNAL);

	gtk_object_class->destroy = sp_object_destroy;

	klass->build = sp_object_build;
	klass->read_attr = sp_object_read_attr;
}

static void
sp_object_init (SPObject * object)
{
	object->hrefcount = 0;
	object->document = NULL;
	object->parent = object->next = NULL;
	object->repr = NULL;
	object->id = NULL;
	object->style = NULL;
	object->title = NULL;
	object->description = NULL;
}

static void
sp_object_destroy (GtkObject * object)
{
	SPObject * spobject;

	spobject = (SPObject *) object;

	/* Parent refcount us, so there shouldn't be any */
	g_assert (!spobject->parent);
	g_assert (!spobject->next);
	g_assert (spobject->document);
	g_assert (spobject->repr);
	/* href holders HAVE to release it in "destroy" signal handler */
	g_assert (spobject->hrefcount == 0);

	if (spobject->style) {
		sp_style_unref (spobject->style);
		spobject->style = NULL;
	}

	if (!SP_OBJECT_IS_CLONED (object)) {
		g_assert (spobject->id);
		sp_document_undef_id (spobject->document, spobject->id);
		g_free (spobject->id);
	} else {
		g_assert (!spobject->id);
	}

	sp_repr_remove_listener_by_data (spobject->repr, spobject);
	sp_repr_unref (spobject->repr);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/*
 * Refcounting
 *
 * Owner is here for debug reasons, you can set it to NULL safely
 * Ref should return object, NULL is error, unref return always NULL
 */

SPObject *
sp_object_ref (SPObject *object, SPObject *owner)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (!owner || SP_IS_OBJECT (owner), NULL);

	gtk_object_ref (GTK_OBJECT (object));

	return object;
}

SPObject *
sp_object_unref (SPObject *object, SPObject *owner)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (!owner || SP_IS_OBJECT (owner), NULL);

	gtk_object_unref (GTK_OBJECT (object));

	return NULL;
}

SPObject *
sp_object_href (SPObject *object, gpointer owner)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);

	object->hrefcount++;

	return object;
}

SPObject *
sp_object_hunref (SPObject *object, gpointer owner)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (object->hrefcount > 0, NULL);

	object->hrefcount--;

	return NULL;
}

/*
 * Attaching/detaching
 */

SPObject *
sp_object_attach_reref (SPObject *parent, SPObject *object, SPObject *next)
{
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (parent), NULL);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (!next || SP_IS_OBJECT (next), NULL);
	g_return_val_if_fail (!object->parent, NULL);
	g_return_val_if_fail (!object->next, NULL);

	sp_object_ref (object, parent);
	gtk_object_unref (GTK_OBJECT (object));
	object->parent = parent;
	object->next = next;

	return object;
}

SPObject *
sp_object_detach (SPObject *parent, SPObject *object)
{
	SPObject *next;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (parent), NULL);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (object->parent == parent, NULL);

	next = object->next;
	object->parent = NULL;
	object->next = NULL;

	return next;
}

SPObject *
sp_object_detach_unref (SPObject *parent, SPObject *object)
{
	SPObject *next;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (parent), NULL);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (object->parent == parent, NULL);

	next = object->next;
	object->parent = NULL;
	object->next = NULL;
	sp_object_unref (object, parent);

	return next;
}

/*
 * SPObject specific build method
 */

static void
sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	/* Nothing specific here */
}

void
sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr, gboolean cloned)
{
	const gchar * id;
	gchar * realid;
	gboolean ret;

	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);

	g_assert (object->document == NULL);
	g_assert (object->repr == NULL);
	g_assert (object->id == NULL);

	/* Bookkeeping */

	object->document = document;
	object->repr = repr;
	sp_repr_ref (repr);
	if (cloned) GTK_OBJECT_SET_FLAGS (object, SP_OBJECT_CLONED_FLAG);

	/* If we are not cloned, force unique id */
	if (!SP_OBJECT_IS_CLONED (object)) {
		id = sp_repr_attr (repr, "id");
		realid = sp_object_get_unique_id (object, id);
		g_assert (realid != NULL);
		sp_document_def_id (document, realid, object);
		object->id = realid;
		/* Redefine ID, if required */
		if ((id == NULL) || (strcmp (id, realid) != 0)) {
			ret = sp_repr_set_attr (repr, "id", realid);
			if (!ret) {
				g_error ("Cannot change id %s -> %s - probably there is stale ref", id, realid);
			}
		}
	} else {
		g_assert (object->id == NULL);
	}

	/* Invoke derived methods, if any */

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->build)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->build) (object, document, repr);

	/* Signalling (should be connected AFTER processing derived methods */

	sp_repr_add_listener (repr, &object_event_vector, object);
}

static void
sp_object_repr_child_added (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data)
{
	SPObject * object; 

	object = SP_OBJECT (data);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->child_added)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->child_added) (object, child, ref);

	sp_document_child_added (object->document, object, child, ref);
}

static gboolean
sp_object_repr_remove_child (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child)
		(* ((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child) (object, child);

	sp_document_child_removed (object->document, object, child, ref);

	return TRUE;
}

/* fixme: */

static void
sp_object_repr_order_changed (SPRepr * repr, SPRepr * child, SPRepr * old, SPRepr * new, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	if (((SPObjectClass *) (((GtkObject *) object)->klass))->order_changed)
		(* ((SPObjectClass *)(((GtkObject *) object)->klass))->order_changed) (object, child, old, new);

	sp_document_order_changed (object->document, object, child, old, new);
}

static void
sp_object_read_attr (SPObject * object, const gchar * key)
{
	const gchar * id;

	g_assert (SP_IS_DOCUMENT (object->document));
	/* fixme: rething that cloning issue */
	g_assert (SP_OBJECT_IS_CLONED (object) || object->id != NULL);
	g_assert (key != NULL);

	if (strcmp (key, "id") == 0) {
		if (!SP_OBJECT_IS_CLONED (object)) {
			id = sp_repr_attr (object->repr, "id");
			g_assert (id != NULL);
			if (strcmp (id, object->id) == 0) return;
			g_assert (!sp_document_lookup_id (object->document, id));
			sp_document_undef_id (object->document, object->id);
			g_free (object->id);
			object->id = g_strdup (id);
			sp_document_def_id (object->document, object->id, object);
		} else {
			g_warning ("ID of cloned object changed, so document is out of sync");
		}
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
	/* fixme: rething that cloning issue */
	g_assert (SP_OBJECT_IS_CLONED (object) || object->id != NULL);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr) (object, key);
}

static gboolean
sp_object_repr_change_attr (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data)
{
	SPObject * object;
	gpointer defid;

	object = SP_OBJECT (data);

	if (strcmp (key, "id") == 0) {
		defid = sp_document_lookup_id (object->document, newval);
		if (defid == object) return TRUE;
		if (defid) return FALSE;
	}

	return TRUE;
}

static void
sp_object_repr_attr_changed (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	sp_object_invoke_read_attr (object, key);

	sp_document_attr_changed (object->document, object, key, oldval, newval);
}

static void
sp_object_repr_content_changed (SPRepr *repr, const guchar *oldcontent, const guchar *newcontent, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_content)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_content) (object);

	sp_document_content_changed (object->document, object, oldcontent, newcontent);
}

/* Styling */

/* fixme: this is potentially dangerous - use load/set style instead (Lauris) */
void
sp_object_style_changed (SPObject *object, guint flags)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->style_changed)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->style_changed) (object, flags);

	sp_object_request_modified (object, flags);
}

/* Modification */

void
sp_object_request_modified (SPObject *object, guint flags)
{
	gboolean propagate;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail ((flags == SP_OBJECT_MODIFIED_FLAG) || (flags == SP_OBJECT_CHILD_MODIFIED_FLAG));

	/* Check for propagate before we set any flags */
	propagate = ((GTK_OBJECT_FLAGS (object) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG)) == 0);

	/* Either one is true, so we can simply overwrite flag */
	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		SP_OBJECT_SET_FLAGS (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (flags & SP_OBJECT_CHILD_MODIFIED_FLAG) {
		SP_OBJECT_SET_FLAGS (object, SP_OBJECT_CHILD_MODIFIED_FLAG);
	}

	if (propagate) {
		if (object->parent) {
			sp_object_request_modified (object->parent, SP_OBJECT_CHILD_MODIFIED_FLAG);
		} else {
			sp_document_request_modified (object->document);
		}
	}
}

void
sp_object_modified (SPObject *object, guint flags)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail ((flags == 0) || (flags == SP_OBJECT_PARENT_MODIFIED_FLAG));

	flags |= (GTK_OBJECT_FLAGS (object) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG));

	g_return_if_fail (flags != 0);

	/* Clear flags to allow reentrancy */
	SP_OBJECT_UNSET_FLAGS (object, (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG));

	gtk_object_ref (GTK_OBJECT (object));
	gtk_signal_emit (GTK_OBJECT (object), object_signals[MODIFIED], flags);
	gtk_object_unref (GTK_OBJECT (object));
}

/* Sequence */
gint
sp_object_sequence (SPObject *object, gint seq)
{
	if (((SPObjectClass *)(((GtkObject *) object)->klass))->sequence)
		return (*((SPObjectClass *)(((GtkObject *) object)->klass))->sequence) (object, seq);

	return seq + 1;
}

const guchar *
sp_object_getAttribute (SPObject *object, const guchar *key, SPException *ex)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (*key != '\0', NULL);

	/* If exception is not clear, return */
	if (!SP_EXCEPTION_IS_OK (ex)) return NULL;

	return (const guchar *) sp_repr_attr (object->repr, key);
}

void
sp_object_setAttribute (SPObject *object, const guchar *key, const guchar *value, SPException *ex)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (key != NULL);
	g_return_if_fail (*key != '\0');
	g_return_if_fail (value != NULL);

	/* If exception is not clear, return */
	g_return_if_fail (SP_EXCEPTION_IS_OK (ex));

	if (!sp_repr_set_attr (object->repr, key, value)) {
		ex->code = SP_NO_MODIFICATION_ALLOWED_ERR;
	}
}

void
sp_object_removeAttribute (SPObject *object, const guchar *key, SPException *ex)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (key != NULL);
	g_return_if_fail (*key != '\0');

	/* If exception is not clear, return */
	g_return_if_fail (SP_EXCEPTION_IS_OK (ex));

	if (!sp_repr_set_attr (object->repr, key, NULL)) {
		ex->code = SP_NO_MODIFICATION_ALLOWED_ERR;
	}
}

/* Helper */

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

	name = sp_repr_name (object->repr);
	g_assert (name != NULL);
	len = strlen (name) + 17;
	b = alloca (len);
	g_assert (b != NULL);
	realid = NULL;

	if (id != NULL) {
		if (sp_document_lookup_id (object->document, id) == NULL) {
			realid = g_strdup (id);
			g_assert (realid != NULL);
		}
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

/* Style */

const guchar *
sp_object_get_style_property (SPObject *object, const gchar *key, const gchar *def)
{
	const gchar *style;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	style = sp_repr_attr (object->repr, "style");
	if (style) {
		gchar *p;
		gint len;
		len = strlen (key);
		for (p = strstr (style, key); p; p = strstr (style, key)) {
			p += len;
			while ((*p <= ' ') && *p) p++;
			if (*p++ != ':') break;
			while ((*p <= ' ') && *p) p++;
			if (*p) return p;
		}
	}
	if (object->parent) {
		return sp_object_get_style_property (object->parent, key, def);
	}

	return def;
}

