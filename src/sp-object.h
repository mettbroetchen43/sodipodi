#ifndef __SP_OBJECT_H__
#define __SP_OBJECT_H__

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

/* SPObject flags */
enum {
	/* Generic */
	SP_OBJECT_IN_DESTRUCTION_FLAG = (1 << 0),
	SP_OBJECT_CLONED_FLAG = (1 << 1),

	/* Async modification flags */
	SP_OBJECT_MODIFIED_FLAG = (1 << 5),
	SP_OBJECT_CHILD_MODIFIED_FLAG = (1 << 6),
	SP_OBJECT_PARENT_MODIFIED_FLAG = (1 << 7),
	SP_OBJECT_STYLE_MODIFIED_FLAG = (1 << 8),
	SP_OBJECT_VIEWPORT_MODIFIED_FLAG = (1 << 9),
	SP_OBJECT_USER_MODIFIED_FLAG_A = (1 << 10),
	SP_OBJECT_USER_MODIFIED_FLAG_B = (1 << 11),
	SP_OBJECT_USER_MODIFIED_FLAG_C = (1 << 12),
	SP_OBJECT_USER_MODIFIED_FLAG_D = (1 << 13),
	SP_OBJECT_USER_MODIFIED_FLAG_E = (1 << 14),
	SP_OBJECT_USER_MODIFIED_FLAG_F = (1 << 15),

	/* Update */
	SP_OBJECT_UPDATE_FLAG = (1 << 16)
};

/* Flags that mark object as modified */
/* Object, Child, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_STATE 0xff60

/* Flags that will propagate downstreams */
/* Parent, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_CASCADE 0xff80

/* Generic */
#define SP_OBJECT_FLAGS(obj) (SP_OBJECT(obj)->flags)
#define SP_OBJECT_SET_FLAGS(obj,flag) G_STMT_START{ (SP_OBJECT_FLAGS (obj) |= (flag)); }G_STMT_END
#define SP_OBJECT_UNSET_FLAGS(obj,flag) G_STMT_START{ (SP_OBJECT_FLAGS (obj) &= ~(flag)); }G_STMT_END

#define SP_OBJECT_IS_CLONED(o) (SP_OBJECT_FLAGS (o) & SP_OBJECT_CLONED_FLAG)
#define SP_OBJECT_IS_MODIFIED(o) (SP_OBJECT_FLAGS (o) & SP_OBJECT_MODIFIED_FLAG)
#define SP_OBJECT_CHILD_IS_MODIFIED(o) (SP_OBJECT_FLAGS (o) & SP_OBJECT_CHILD_MODIFIED_FLAG)

/* Write flags */
#define SP_OBJECT_WRITE_BUILD (1 << 0)
#define SP_OBJECT_WRITE_SODIPODI (1 << 1)

/* Convenience stuff */
#define SP_OBJECT_ID(o) (SP_OBJECT (o)->id)
#define SP_OBJECT_REPR(o) (SP_OBJECT (o)->repr)
#define SP_OBJECT_DOCUMENT(o) (SP_OBJECT (o)->document)
#define SP_OBJECT_PARENT(o) (SP_OBJECT (o)->parent)
#define SP_OBJECT_NEXT(o) (SP_OBJECT (o)->next)
#define SP_OBJECT_HREFCOUNT(o) (SP_OBJECT (o)->hrefcount)
#define SP_OBJECT_STYLE(o) (SP_OBJECT (o)->style)
#define SP_OBJECT_TITLE(o) (SP_OBJECT (o)->title)
#define SP_OBJECT_DESCRIPTION(o) (SP_OBJECT (o)->description)

#include <glib-object.h>
#include "xml/repr.h"
#include "forward.h"

typedef void (* SPObjectMethod) (SPObject *object, gpointer data);

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

typedef struct _SPCtx SPCtx;

struct _SPCtx {
	unsigned int flags;
};

struct _SPObject {
	GObject object;
	guint hrefcount; /* number os xlink:href references */
	guint32 flags;
	SPDocument *document; /* Document we are part of */
	SPObject *parent; /* Our parent (only one allowed) */
	SPObject *next; /* Next object in linked list */
	SPRepr *repr; /* Our xml representation */
	gchar *id; /* Our very own unique id */
	SPStyle *style;
	const guchar *title; /* Our title, if any */
	const guchar *description; /* Our description, if any */
};

struct _SPObjectClass {
	GObjectClass parent_class;

	void (* build) (SPObject *object, SPDocument *doc, SPRepr *repr);
	void (* release) (SPObject *object);

	/* Virtual handlers of repr signals */
	void (* child_added) (SPObject *object, SPRepr *child, SPRepr *ref);
	void (* remove_child) (SPObject *object, SPRepr *child);

	void (* order_changed) (SPObject *object, SPRepr *child, SPRepr *old, SPRepr *new);

	void (* set) (SPObject *object, unsigned int key, const unsigned char *value);

	void (* read_content) (SPObject *object);

	/* Update handler */
	void (* update) (SPObject *object, SPCtx *ctx, unsigned int flags);
	/* Modification handler */
	void (* modified) (SPObject *object, guint flags);
	/* Style change notifier */
	void (* style_modified) (SPObject *object, guint flags);

	/* Compute next sequence number */
	gint (* sequence) (SPObject *object, gint seq);
	void (* forall) (SPObject *object, SPObjectMethod func, gpointer data);

	SPRepr * (* write) (SPObject *object, SPRepr *repr, guint flags);
};

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
SPObject *sp_object_detach (SPObject *parent, SPObject *object);
SPObject *sp_object_detach_unref (SPObject *parent, SPObject *object);

void sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr, gboolean cloned);
void sp_object_invoke_release (SPObject *object);

void sp_object_set (SPObject *object, unsigned int key, const unsigned char *value);

void sp_object_read_attr (SPObject *object, const gchar *key);

/* Styling */

/* Modification */
void sp_object_request_update (SPObject *object, unsigned int flags);
void sp_object_invoke_update (SPObject *object, SPCtx *ctx, unsigned int flags);
void sp_object_request_modified (SPObject *object, unsigned int flags);
void sp_object_invoke_modified (SPObject *object, unsigned int flags);

/* Sequence */
gint sp_object_sequence (SPObject *object, gint seq);

void sp_object_invoke_forall (SPObject *object, SPObjectMethod func, gpointer data);
/* Write object to repr */
SPRepr *sp_object_invoke_write (SPObject *object, SPRepr *repr, guint flags);

/* Public */

const guchar *sp_object_getAttribute (SPObject *object, const guchar *key, SPException *ex);
void sp_object_setAttribute (SPObject *object, const guchar *key, const guchar *value, SPException *ex);
void sp_object_removeAttribute (SPObject * object, const guchar *key, SPException *ex);

/* Style */

const guchar *sp_object_get_style_property (SPObject *object, const gchar *key, const gchar *def);

#endif
