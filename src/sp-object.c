#define __SP_OBJECT_C__
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

#include <stdlib.h>
#include <string.h>
#include <glib-object.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtksignal.h>
#include "xml/repr-private.h"
#include "document.h"
#include "style.h"
#include "sp-object-repr.h"
#include "sp-object.h"

#define noSP_OBJECT_DEBUG

static void sp_object_class_init (SPObjectClass * klass);
static void sp_object_init (SPObject * object);
static void sp_object_dispose (GObject * object);
static void sp_object_finalize (GObject * object);

static void sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_object_release (SPObject *object);

static void sp_object_read_attr (SPObject * object, const gchar * key);
static SPRepr *sp_object_private_write (SPObject *object, SPRepr *repr, guint flags);

/* Real handlers of repr signals */

static gboolean sp_object_repr_change_attr (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);
static void sp_object_repr_attr_changed (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);

static void sp_object_repr_content_changed (SPRepr *repr, const guchar *oldcontent, const guchar *newcontent, gpointer data);

static void sp_object_repr_child_added (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
static gboolean sp_object_repr_remove_child (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);

static void sp_object_repr_order_changed (SPRepr *repr, SPRepr *child, SPRepr *old, SPRepr *new, gpointer data);

static gchar * sp_object_get_unique_id (SPObject * object, const gchar * defid);

enum {RELEASE, MODIFIED, LAST_SIGNAL};

SPReprEventVector object_event_vector = {
	NULL, /* Destroy */
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

static GObjectClass *parent_class;
static guint object_signals[LAST_SIGNAL] = {0};

unsigned int
sp_object_get_type (void)
{
	static GType object_type = 0;
	if (!object_type) {
		GTypeInfo object_info = {
			sizeof (SPObjectClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_object_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPObject),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_object_init,
		};
		object_type = g_type_register_static (G_TYPE_OBJECT, "SPObject", &object_info, 0);
	}
	return object_type;
}

static void
sp_object_class_init (SPObjectClass * klass)
{
	GObjectClass * object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	object_signals[RELEASE] = g_signal_new ("release",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET (SPObjectClass, release),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
	object_signals[MODIFIED] = g_signal_new ("modified",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_FIRST,
                                                 G_STRUCT_OFFSET (SPObjectClass, modified),
                                                 NULL, NULL,
                                                 gtk_marshal_NONE__UINT,
                                                 G_TYPE_NONE, 1, G_TYPE_UINT);

	object_class->dispose = sp_object_dispose;
	object_class->finalize = sp_object_finalize;

	klass->build = sp_object_build;
	klass->release = sp_object_release;
	klass->read_attr = sp_object_read_attr;
	klass->write = sp_object_private_write;
}

static void
sp_object_init (SPObject * object)
{
#ifdef SP_OBJECT_DEBUG
	g_print("sp_object_init: id=%x, typename=%s\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif

	object->hrefcount = 0;
	object->document = NULL;
	object->parent = object->next = NULL;
	object->repr = NULL;
	object->id = NULL;
	object->style = NULL;
	object->title = NULL;
	object->description = NULL;
}

/* fixme: This is complete crap - copy destrouctor from gnome1 version (Lauris) */

static void
sp_object_dispose (GObject * object)
{
	SPObject *spobject;

	spobject = (SPObject *) object;

#ifdef SP_OBJECT_DEBUG
	g_print("sp_object_dispose: id=%x, typename=%s\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif
	if (!(SP_OBJECT_FLAGS (spobject) & SP_OBJECT_IN_DESTRUCTION_FLAG))
	{
#ifdef SP_OBJECT_DEBUG
		g_print("sp_object_dispose: id=%x, typename=%s enter IN_DESTRUCTION\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif
		SP_OBJECT_SET_FLAGS (spobject, SP_OBJECT_IN_DESTRUCTION_FLAG);

/* 		sp_object_invoke_release (spobject); */
		if (spobject->document) {
			g_signal_emit (object, object_signals[RELEASE], 0);
		}

		SP_OBJECT_UNSET_FLAGS (spobject, SP_OBJECT_IN_DESTRUCTION_FLAG);
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sp_object_finalize (GObject * object)
{
	SPObject *spobject;

	spobject = (SPObject *) object;

	if (((GObjectClass *) (parent_class))->finalize)
		(* ((GObjectClass *) (parent_class))->finalize) (object);
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

	g_object_ref (G_OBJECT (object));

	return object;
}

SPObject *
sp_object_unref (SPObject *object, SPObject *owner)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (!owner || SP_IS_OBJECT (owner), NULL);

	g_object_unref (G_OBJECT (object));

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
	g_object_unref (G_OBJECT (object));
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

	sp_object_invoke_release (object);

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

	sp_object_invoke_release (object);

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
#ifdef SP_OBJECT_DEBUG
	g_print("sp_object_build: id=%x, typename=%s\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif
}

static void
sp_object_release (SPObject * object)
{
#ifdef SP_OBJECT_DEBUG
	g_print("sp_object_release: id=%x, typename=%s\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif
	/* href holders HAVE to release it in signal handler */
	g_assert (object->hrefcount == 0);

	if (object->style) {
		object->style = sp_style_unref (object->style);
	}

	if (!SP_OBJECT_IS_CLONED (object)) {
		g_assert (object->id);
		sp_document_undef_id (object->document, object->id);
		g_free (object->id);
		object->id = NULL;
	} else {
		g_assert (!object->id);
	}

	sp_repr_remove_listener_by_data (object->repr, object);
	sp_repr_unref (object->repr);

	object->document = NULL;
	object->repr = NULL;
}

void
sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr, gboolean cloned)
{
	const gchar * id;
	gchar * realid;
	gboolean ret;

#ifdef SP_OBJECT_DEBUG
	g_print("sp_object_invoke_build: id=%x, typename=%s\n", object, g_type_name_from_instance((GTypeInstance*)object));
#endif
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
	if (cloned) SP_OBJECT_SET_FLAGS (object, SP_OBJECT_CLONED_FLAG);

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

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->build)
		(*((SPObjectClass *) G_OBJECT_GET_CLASS(object))->build) (object, document, repr);

	/* Signalling (should be connected AFTER processing derived methods */

	sp_repr_add_listener (repr, &object_event_vector, object);
}

void
sp_object_invoke_release (SPObject *object)
{
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));

	/* Parent refcount us, so there shouldn't be any */
	g_assert (!object->parent);
	g_assert (!object->next);
	g_assert (object->document);
	g_assert (object->repr);

	gtk_signal_emit (GTK_OBJECT (object), object_signals[RELEASE]);

	/* href holders HAVE to release it in signal handler */
	g_assert (object->hrefcount == 0);

	if (object->style) {
		object->style = sp_style_unref (object->style);
	}

	if (!SP_OBJECT_IS_CLONED (object)) {
		g_assert (object->id);
		sp_document_undef_id (object->document, object->id);
		g_free (object->id);
		object->id = NULL;
	} else {
		g_assert (!object->id);
	}

	sp_repr_remove_listener_by_data (object->repr, object);
	sp_repr_unref (object->repr);

	object->document = NULL;
	object->repr = NULL;
}

static void
sp_object_repr_child_added (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data)
{
	SPObject * object; 

	object = SP_OBJECT (data);

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->child_added)
		(*((SPObjectClass *)G_OBJECT_GET_CLASS(object))->child_added) (object, child, ref);

	sp_document_child_added (object->document, object, child, ref);
}

static gboolean
sp_object_repr_remove_child (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->remove_child)
		(* ((SPObjectClass *)G_OBJECT_GET_CLASS(object))->remove_child) (object, child);

	sp_document_child_removed (object->document, object, child, ref);

	return TRUE;
}

/* fixme: */

static void
sp_object_repr_order_changed (SPRepr * repr, SPRepr * child, SPRepr * old, SPRepr * new, gpointer data)
{
	SPObject * object;

	object = SP_OBJECT (data);

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->order_changed)
		(* ((SPObjectClass *)G_OBJECT_GET_CLASS(object))->order_changed) (object, child, old, new);

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

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->read_attr)
		(*((SPObjectClass *) G_OBJECT_GET_CLASS(object))->read_attr) (object, key);
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

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->read_content)
		(*((SPObjectClass *) G_OBJECT_GET_CLASS(object))->read_content) (object);

	sp_document_content_changed (object->document, object, oldcontent, newcontent);
}

void
sp_object_invoke_forall (SPObject *object, SPObjectMethod func, gpointer data)
{
	SPRepr *repr, *child;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (func != NULL);

	func (object, data);

#if 0

	if (((SPObjectClass *) (((GtkObject *) object)->klass))->forall)
		((SPObjectClass *) (((GtkObject *) object)->klass))->forall (object, func, data);
#else
	repr = SP_OBJECT_REPR (object);
	for (child = repr->children; child != NULL; child = child->next) {
		const unsigned char *id;
		SPObject *cho;
		id = sp_repr_attr (child, "id");
		cho = sp_document_lookup_id (SP_OBJECT_DOCUMENT (object), id);
		if (cho) sp_object_invoke_forall (cho, func, data);
	}
#endif
}

static SPRepr *
sp_object_private_write (SPObject *object, SPRepr *repr, guint flags)
{
	if (!repr && (flags & SP_OBJECT_WRITE_BUILD)) {
		repr = sp_repr_duplicate (SP_OBJECT_REPR (object));
	} else {
		sp_repr_set_attr (repr, "id", object->id);
	}

	return repr;
}

SPRepr *
sp_object_invoke_write (SPObject *object, SPRepr *repr, guint flags)
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);

	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->write) {
		if (!(flags & SP_OBJECT_WRITE_BUILD) && !repr) {
			repr = SP_OBJECT_REPR (object);
		}
		return ((SPObjectClass *) G_OBJECT_GET_CLASS(object))->write (object, repr, flags);
	} else {
		g_warning ("Class %s does not implement ::write", G_OBJECT_TYPE_NAME (object));
		if (!repr) {
			if (flags & SP_OBJECT_WRITE_BUILD) {
				repr = sp_repr_duplicate (SP_OBJECT_REPR (object));
			}
			/* fixme: else probably error (Lauris) */
		} else {
			sp_repr_merge (repr, SP_OBJECT_REPR (object), "id");
		}
		return repr;
	}
}

/* Modification */

void
sp_object_request_modified (SPObject *object, guint flags)
{
	gboolean propagate;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (!(flags & SP_OBJECT_PARENT_MODIFIED_FLAG));
	g_return_if_fail ((flags & SP_OBJECT_MODIFIED_FLAG) || (flags & SP_OBJECT_CHILD_MODIFIED_FLAG));
	g_return_if_fail (!((flags & SP_OBJECT_MODIFIED_FLAG) && (flags & SP_OBJECT_CHILD_MODIFIED_FLAG)));

	/* Check for propagate before we set any flags */
	/* Propagate means, that object is not passed through by modification request cascade yet */
	propagate = (!(SP_OBJECT_FLAGS (object) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG)));

	/* Just set object flags safe even if some have been set before */
	SP_OBJECT_SET_FLAGS (object, flags);

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
	g_return_if_fail (!(flags & ~SP_OBJECT_MODIFIED_CASCADE));

	flags |= (SP_OBJECT_FLAGS (object) & SP_OBJECT_MODIFIED_STATE);

	g_return_if_fail (flags != 0);

	/* Merge style if we have good reasons to think that parent style is changed */
	/* I am not sure, whether we should check only propagated flag */
	/* We are currently assuming, that style parsing is done immediately */
	/* I think this is correct (Lauris) */
	if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
		if (object->style && object->parent) {
			sp_style_merge_from_parent (object->style, object->parent->style);
		}
	}

	g_object_ref (G_OBJECT (object));
	g_signal_emit (G_OBJECT (object), object_signals[MODIFIED], 0, flags);
	g_object_unref (G_OBJECT (object));

	/* If style is modified, invoke style_modified virtual method */
	/* It is pure convenience, and should be used with caution */
	/* The cascade is created solely by modified method plus appropriate flag */
	/* Also, it merely signals, that actual style object has changed */
	if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
		if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->style_modified)
			(*((SPObjectClass *) G_OBJECT_GET_CLASS(object))->style_modified) (object, flags);
	}

	/*
	 * fixme: I am not sure - it was before class method, but moved it here
	 */

	SP_OBJECT_UNSET_FLAGS (object, SP_OBJECT_MODIFIED_STATE);
}

/* Sequence */
gint
sp_object_sequence (SPObject *object, gint seq)
{
	if (((SPObjectClass *) G_OBJECT_GET_CLASS(object))->sequence)
		return (*((SPObjectClass *) G_OBJECT_GET_CLASS(object))->sequence) (object, seq);

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
	const guchar *val;

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
	val = sp_repr_attr (object->repr, key);
	if (val) return val;
	if (object->parent) {
		return sp_object_get_style_property (object->parent, key, def);
	}

	return def;
}

