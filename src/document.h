#ifndef SP_DOCUMENT_H
#define SP_DOCUMENT_H

/*
 * SPDocument
 *
 * This is toplevel class, implementing a gateway from repr to most
 * editing properties, like canvases (desktops), undo stacks etc.
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
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

typedef struct _SPDocumentPrivate SPDocumentPrivate;

#define SP_TYPE_DOCUMENT            (sp_document_get_type ())
#define SP_DOCUMENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCUMENT, SPDocument))
#define SP_DOCUMENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCUMENT, SPItemClass))
#define SP_IS_DOCUMENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCUMENT))
#define SP_IS_DOCUMENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCUMENT))

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include <libart_lgpl/art_rect.h>
#include "xml/repr.h"
#include "forward.h"

struct _SPDocument {
	GtkObject object;

	SPDocumentPrivate * private;
};

struct _SPDocumentClass {
	GtkObjectClass parent_class;

	void (* modified) (SPDocument *document, guint flags);

	void (* uri_set) (SPDocument *document, const guchar *uri);
	void (* resized) (SPDocument *document, gdouble width, gdouble height);
};

GtkType sp_document_get_type (void);

/*
 * Constructor
 *
 * Fetches document from URI, or creates new, if NULL
 */

SPDocument * sp_document_new (const gchar * uri);
SPDocument * sp_document_new_from_mem (const gchar * buffer, gint length);

SPDocument *sp_document_ref (SPDocument *doc);
SPDocument *sp_document_unref (SPDocument *doc);

/*
 * Access methods
 */

SPReprDoc * sp_document_repr_doc (SPDocument * document);
SPRepr * sp_document_repr_root (SPDocument * document);
#define SP_DOCUMENT_ROOT(d) sp_document_root (d)
SPRoot * sp_document_root (SPDocument * document);
gdouble sp_document_width (SPDocument * document);
gdouble sp_document_height (SPDocument * document);
#define SP_DOCUMENT_URI(d) sp_document_uri (d)
const gchar * sp_document_uri (SPDocument * document);
const gchar * sp_document_base (SPDocument * document);

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

void sp_document_request_modified (SPDocument *document);

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

void sp_document_set_uri (SPDocument *document, const guchar *uri);
void sp_document_set_size (SPDocument *doc, gdouble width, gdouble height);

#endif
