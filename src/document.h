#ifndef __SP_DOCUMENT_H__
#define __SP_DOCUMENT_H__

/*
 * Typed SVG document implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef enum {
	SPXMinYMin,
	SPXMidYMin,
	SPXMaxYMin,
	SPXMinYMid,
	SPXMidYMid,
	SPXMaxYMid,
	SPXMinYMax,
	SPXMidYMax,
	SPXMaxYMax
} SPAspect;

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include <libart_lgpl/art_rect.h>
#include "xml/repr.h"
#include "forward.h"

typedef struct _SPDocumentPrivate SPDocumentPrivate;

struct _SPDocument {
	GtkObject object;

	guint public : 1;

	SPReprDoc *rdoc; /* Our SPReprDoc */
	SPRepr *rroot; /* Root element of SPReprDoc */
	SPObject *root; /* Our SPRoot */

	guchar *uri; /* URI string or NULL */
	guchar *base;
	guchar *name;

	/* fixme: remove this */
	SPDocumentPrivate * private;

	/* Last action key */
	const guchar *actionkey;
	/* Handler ID */
	guint modified_id;
};

struct _SPDocumentClass {
	GtkObjectClass parent_class;

	void (* modified) (SPDocument *document, guint flags);
	void (* uri_set) (SPDocument *document, const guchar *uri);
	void (* resized) (SPDocument *document, gdouble width, gdouble height);
};

/*
 * Fetches document from URI, or creates new, if NULL
 * Public document appear in document list
 */

SPDocument *sp_document_new (const gchar *uri, gboolean public);
SPDocument *sp_document_new_from_mem (const gchar *buffer, gint length, gboolean public);

SPDocument *sp_document_ref (SPDocument *doc);
SPDocument *sp_document_unref (SPDocument *doc);

/*
 * Access methods
 */

#define sp_document_repr_doc(d) (SP_DOCUMENT (d)->rdoc)
#define sp_document_repr_root(d) (SP_DOCUMENT (d)->rroot)
#define sp_document_root(d) (SP_DOCUMENT (d)->root)
#define SP_DOCUMENT_ROOT(d)  (SP_DOCUMENT (d)->root)

gdouble sp_document_width (SPDocument * document);
gdouble sp_document_height (SPDocument * document);
#define SP_DOCUMENT_URI(d) (SP_DOCUMENT (d)->uri)
#define SP_DOCUMENT_NAME(d) (SP_DOCUMENT (d)->name)
#define SP_DOCUMENT_BASE(d) (SP_DOCUMENT (d)->base)

const GSList * sp_document_namedview_list (SPDocument * document);
SPNamedView * sp_document_namedview (SPDocument * document, const gchar * name);

/*
 * Dictionary
 */

void sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object);
void sp_document_undef_id (SPDocument * document, const gchar * id);
SPObject * sp_document_lookup_id (SPDocument * document, const gchar * id);

/*
 * Undo & redo
 */

void sp_document_set_undo_sensitive (SPDocument * document, gboolean sensitive);

void sp_document_clear_undo (SPDocument * document);
void sp_document_clear_redo (SPDocument * document);

void sp_document_child_added (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *ref);
void sp_document_child_removed (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *ref);
void sp_document_attr_changed (SPDocument *doc, SPObject *object, const guchar *key, const guchar *oldval, const guchar *newval);
void sp_document_content_changed (SPDocument *doc, SPObject *object, const guchar *oldcontent, const guchar *newcontent);
void sp_document_order_changed (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *oldref, SPRepr *newref);

/* Object modification root handler */
void sp_document_request_modified (SPDocument *doc);
gint sp_document_ensure_up_to_date (SPDocument *doc);

/* Save all previous actions to stack, as one undo step */
void sp_document_done (SPDocument *document);
void sp_document_maybe_done (SPDocument *document, const guchar *key);

/* Undo and redo */
void sp_document_undo (SPDocument * document);
void sp_document_redo (SPDocument * document);

/* Adds repr to document, returning created object (if any) */
/* Items will be added to root (fixme: should be namedview root) */
/* Non-item objects will go to root-level defs group */
SPObject *sp_document_add_repr (SPDocument *document, SPRepr *repr);

#if 0
/* Deletes repr from document */
/* fixme: This is not needed anymore - remove it (Lauris) */
/* Instead simply unparent repr */
void sp_document_del_repr (SPDocument *document, SPRepr *repr);
#endif

/* Resource management */
gboolean sp_document_add_resource (SPDocument *document, const guchar *key, SPObject *object);
gboolean sp_document_remove_resource (SPDocument *document, const guchar *key, SPObject *object);
const GSList *sp_document_get_resource_list (SPDocument *document, const guchar *key);

/*
 * Ideas: How to overcome style invalidation nightmare
 *
 * 1. There is reference request dictionary, that contains
 * objects (styles) needing certain id. Object::build checks
 * final id against it, and invokes necesary methods
 *
 * 2. Removing referenced object is simply prohibited -
 * needs analyse, how we can deal with situations, where
 * we simply want to ungroup etc. - probably we need
 * Repr::reparent method :( [Or was it ;)]
 *
 */

/*
 * Misc
 */

GSList * sp_document_items_in_box (SPDocument * document, ArtDRect * box);
GSList * sp_document_partial_items_in_box (SPDocument * document, ArtDRect * box);

void sp_document_set_uri (SPDocument *document, const guchar *uri);
void sp_document_set_size_px (SPDocument *doc, gdouble width, gdouble height);

#endif
