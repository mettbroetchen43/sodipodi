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

#include <libnr/nr-types.h>
#include <glib-object.h>
#include "xml/repr.h"
#include "forward.h"

/* Version type. See SPRoot for more detail */
enum {
	SP_VERSION_SVG,
	SP_VERSION_SODIPODI,
	SP_VERSION_ORIGINAL,
};

typedef struct _SPDocumentPrivate SPDocumentPrivate;

struct _SPDocument {
	GObject object;

	unsigned int advertize : 1;

	SPReprDoc *rdoc; /* Our SPReprDoc */
	SPRepr *rroot; /* Root element of SPReprDoc */
	SPObject *root; /* Our SPRoot */

	guchar *uri; /* URI string or NULL */
	guchar *base;
	guchar *name;

	/* fixme: remove this */
	SPDocumentPrivate *priv;

	/* Last action key */
	const guchar *actionkey;

	/* Handler ID */
	/* guint modified_id; */

	/* Begin time */
	double begintime;
};

struct _SPDocumentClass {
	GObjectClass parent_class;

	void (* destroy) (SPDocument *document);

	void (* modified) (SPDocument *document, guint flags);
	void (* uri_set) (SPDocument *document, const guchar *uri);
	void (* resized) (SPDocument *document, gdouble width, gdouble height);
	/* Timer events */
	void (* begin) (SPDocument *doc, double dtime);
	void (* end) (SPDocument *doc, double dtime);
};

/*
 * Fetches document from URI, or creates new, if NULL
 * Public document appear in document list
 */

SPDocument *sp_document_new (const gchar *uri);
SPDocument *sp_document_new_from_mem (const gchar *buffer, gint length);

SPDocument *sp_document_ref (SPDocument *doc);
SPDocument *sp_document_unref (SPDocument *doc);

/*
 * Access methods
 */

SPReprDoc *sp_document_get_repr_doc (SPDocument *doc);
SPRepr *sp_document_get_repr_root (SPDocument *doc);
#define sp_document_repr_doc(d) (SP_DOCUMENT (d)->rdoc)
#define sp_document_repr_root(d) (SP_DOCUMENT (d)->rroot)
#define sp_document_root(d) (SP_DOCUMENT (d)->root)
#define SP_DOCUMENT_ROOT(d)  (SP_DOCUMENT (d)->root)

gdouble sp_document_width (SPDocument * document);
gdouble sp_document_height (SPDocument * document);
const unsigned char *sp_document_get_uri (SPDocument *doc);
const unsigned char *sp_document_get_name (SPDocument *doc);
#define SP_DOCUMENT_URI(d) (SP_DOCUMENT (d)->uri)
#define SP_DOCUMENT_NAME(d) (SP_DOCUMENT (d)->name)
#define SP_DOCUMENT_BASE(d) (SP_DOCUMENT (d)->base)

#define sp_document_set_advertize(d,v) (((SPDocument *)(d))->advertize = ((v) != 0))

/*
 * Dictionary
 */

void sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object);
void sp_document_undef_id (SPDocument * document, const gchar * id);
SPObject *sp_document_get_object_from_id (SPDocument *doc, const unsigned char *id);
SPObject *sp_document_get_object_from_repr (SPDocument *doc, SPRepr *repr);
#define sp_document_lookup_id sp_document_get_object_from_id

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
void sp_document_maybe_done (SPDocument *document, const unsigned char *key);
/* Cancel (and revert) current unsaved actions */
void sp_document_cancel (SPDocument *document);

/* Undo and redo */
void sp_document_undo (SPDocument * document);
void sp_document_redo (SPDocument * document);

/* Adds repr to document, returning created object (if any) */
/* Items will be added to root (fixme: should be namedview root) */
/* Non-item objects will go to root-level defs group */
SPObject *sp_document_add_repr (SPDocument *document, SPRepr *repr);

/* Returns the sequence number of object */
unsigned int sp_document_object_sequence_get (SPDocument *doc, SPObject *object);

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

#if 0
/*
 * Misc
 */

GSList *sp_document_items_in_box (SPDocument *document, NRRectD *box);
GSList *sp_document_partial_items_in_box (SPDocument *document, NRRectD *box);
#endif

void sp_document_set_uri (SPDocument *document, const guchar *uri);
void sp_document_set_size_px (SPDocument *doc, gdouble width, gdouble height);

guint sp_document_get_version (SPDocument *document, guint version_type);

/*
 * Timer
 *
 * Objects are required to use time services only from their parent document
 */

enum {
	SP_TIMER_IDLE,
	SP_TIMER_EXACT
};

/* Begin document and start timer */
double sp_document_begin (SPDocument *doc);
/* End document and stop timer */
double sp_document_end (SPDocument *doc);
/* Get time in seconds from document begin */
double sp_document_get_time (SPDocument *doc);

/* Create and set up timer */
unsigned int sp_document_add_timer (SPDocument *doc, double time, unsigned int klass,
				    unsigned int (* callback) (double, void *),
				    void *data);
/* Remove timer */
void sp_document_remove_timer (SPDocument *doc, unsigned int id);
/* Set up timer */
void sp_document_set_timer (SPDocument *doc, unsigned int id, double time, unsigned int klass);

#endif
