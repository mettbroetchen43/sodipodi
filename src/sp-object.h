#ifndef SP_OBJECT_H
#define SP_OBJECT_H

/*
 * SPObject
 *
 * This is most abstract of all typed objects
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#define SP_TYPE_OBJECT            (sp_object_get_type ())
#define SP_OBJECT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_OBJECT, SPObject))
#define SP_OBJECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECT, SPObjectClass))
#define SP_IS_OBJECT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_OBJECT))
#define SP_IS_OBJECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECT))

/* Async modifiaction flags */

#define SP_OBJECT_SET_FLAGS GTK_OBJECT_SET_FLAGS
#define SP_OBJECT_UNSET_FLAGS GTK_OBJECT_UNSET_FLAGS
#define SP_OBJECT_CLONED_FLAG (1 << 4)
#define SP_OBJECT_MODIFIED_FLAG (1 << 5)
#define SP_OBJECT_CHILD_MODIFIED_FLAG (1 << 6)
#define SP_OBJECT_PARENT_MODIFIED_FLAG (1 << 7)
#define SP_OBJECT_IS_CLONED(o) (GTK_OBJECT_FLAGS (o) & SP_OBJECT_CLONED_FLAG)
#define SP_OBJECT_IS_MODIFIED(o) (GTK_OBJECT_FLAGS (o) & SP_OBJECT_MODIFIED_FLAG)
#define SP_OBJECT_CHILD_IS_MODIFIED(o) (GTK_OBJECT_FLAGS (o) & SP_OBJECT_CHILD_MODIFIED_FLAG)

/* Convenience stuff */

#define SP_OBJECT_ID(o) (SP_OBJECT (o)->id)
#define SP_OBJECT_REPR(o) (SP_OBJECT (o)->repr)
#define SP_OBJECT_DOCUMENT(o) (SP_OBJECT (o)->document)
#define SP_OBJECT_PARENT(o) (SP_OBJECT (o)->parent)
#define SP_OBJECT_HREFCOUNT(o) (SP_OBJECT (o)->hrefcount)
#define SP_OBJECT_STYLE(o) (SP_OBJECT (o)->style)

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include "xml/repr.h"
#include "forward.h"

typedef enum {
	SP_NO_EXCEPTION,
	SP_INDEX_SIZE_ERR,
	SP_DOMSTRING_SIZE_ERR,
	SP_HIERARCHY_REQUEST_ERR,
	SP_WRONG_DOCUMENT_ERR,
	SP_INVALID_CHARACTER_ERR,
	SP_NO_DATA_ALLOWED_ERR,
	SP_NO_MODIFICATION_ALLOWED_ERR,
	SP_NOT_FOUND_ERR,
	SP_NOT_SUPPORTED_ERR,
	SP_INUSE_ATTRIBUTE_ERR,
	SP_INVALID_STATE_ERR,
	SP_SYNTAX_ERR,
	SP_INVALID_MODIFICATION_ERR,
	SP_NAMESPACE_ERR,
	SP_INVALID_ACCESS_ERR
} SPExceptionType;

typedef struct _SPException SPException;

struct _SPException {
	SPExceptionType code;
};

#define SP_EXCEPTION_INIT(ex) {(ex)->code = SP_NO_EXCEPTION;}
#define SP_EXCEPTION_IS_OK(ex) (!(ex) || ((ex)->code == SP_NO_EXCEPTION))

struct _SPObject {
	GtkObject object;
	guint hrefcount; /* number os xlink:href references */
	SPDocument *document; /* Document we are part of */
	SPObject *parent; /* Our parent (only one allowed) */
	SPObject *next; /* Next object in linked list */
	SPRepr *repr; /* Our xml representation */
	gchar *id; /* Our very own unique id */
	SPStyle *style;
	const gchar *title; /* Our title, if any */
	const gchar *description; /* Our description, if any */
};

struct _SPObjectClass {
	GtkObjectClass parent_class;
	void (* build) (SPObject * object, SPDocument * document, SPRepr * repr);

	/* Virtual handlers of repr signals */
	void (* child_added) (SPObject * object, SPRepr * child, SPRepr * ref);
	void (* remove_child) (SPObject * object, SPRepr * child);

	void (* order_changed) (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);

	void (* read_attr) (SPObject * object, const gchar * key);
	void (* read_content) (SPObject * object);

	/* Style change notifier */
	void (* style_changed) (SPObject *object, guint flags);
	/* Modification handler */
	void (* modified) (SPObject *object, guint flags);

	/* Compute next sequence number */
	gint (* sequence) (SPObject *object, gint seq);
};

GtkType sp_object_get_type (void);

/*
 * Refcounting
 *
 * Owner is here for debug reasons, you can set it to NULL safely
 * Ref should return object, NULL is error, unref return always NULL
 */

SPObject *sp_object_ref (SPObject *object, SPObject *owner);
SPObject *sp_object_unref (SPObject *object, SPObject *owner);

SPObject *sp_object_href (SPObject *object, gpointer owner);
SPObject *sp_object_hunref (SPObject *object, gpointer owner);

/*
 * Attaching/detaching
 *
 * Attach returns object itself, or NULL on error
 * Detach returns next object, NULL on error
 */

SPObject *sp_object_attach_reref (SPObject *parent, SPObject *object, SPObject *next);
SPObject *sp_object_detach_unref (SPObject *parent, SPObject *object);

void sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr, gboolean cloned);
void sp_object_invoke_read_attr (SPObject * object, const gchar * key);

/* Styling */

/* fixme: this is potentially dangerous - use load/set style instead (Lauris) */
/* flags are the same as for modification */
void sp_object_style_changed (SPObject *object, guint flags);

/* Modification */

void sp_object_request_modified (SPObject *object, guint flags);
void sp_object_modified (SPObject *object, guint flags);

/* Sequence */
gint sp_object_sequence (SPObject *object, gint seq);

/* Public */

const guchar *sp_object_getAttribute (SPObject *object, const guchar *key, SPException *ex);
void sp_object_setAttribute (SPObject *object, const guchar *key, const guchar *value, SPException *ex);
void sp_object_removeAttribute (SPObject * object, const guchar *key, SPException *ex);

/* Style */

const guchar *sp_object_get_style_property (SPObject *object, const gchar *key, const gchar *def);

#endif
