#ifndef __SP_API_H__
#define __SP_API_H__

/*
 * Public API for external modules
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <glib.h>

G_BEGIN_DECLS

#include <glib-object.h>

#include "xml/repr.h"

/* Document tree */

/*
 * Signals
 *
 * void (* modified) (SPDocument *document, guint flags);
 * void (* uri_set) (SPDocument *document, const guchar *uri);
 * void (* resized) (SPDocument *document, gdouble width, gdouble height);
 */

typedef struct _SPDocument SPDocument;
typedef struct _SPDocumentClass SPDocumentClass;

#define SP_TYPE_DOCUMENT (sp_document_get_type ())
#define SP_DOCUMENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_DOCUMENT, SPDocument))
#define SP_IS_DOCUMENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_DOCUMENT))

GType sp_document_get_type (void);

/* Objects */

/*
 * Signals
 *
 * void (* release) (SPObject *object);
 * void (* modified) (SPObject *object, unsigned int flags);
 */

/* flags for modification handler */
/* Object itself was modified */
#define SP_OBJECT_MODIFIED_FLAG (1 << 0)
/* Some of object children was modified */
#define SP_OBJECT_CHILD_MODIFIED_FLAG (1 << 1)
/* Object parent was modified */
#define SP_OBJECT_PARENT_MODIFIED_FLAG (1 << 2)
/* Object (or parent) style was modified */
#define SP_OBJECT_STYLE_MODIFIED_FLAG (1 << 3)
/* Object (or parent) viewport was modified */
#define SP_OBJECT_VIEWPORT_MODIFIED_FLAG (1 << 4)
#define SP_OBJECT_USER_MODIFIED_FLAG_A (1 << 5)
#define SP_OBJECT_USER_MODIFIED_FLAG_B (1 << 6)
#define SP_OBJECT_USER_MODIFIED_FLAG_C (1 << 7)

/* Flags that mark object as modified */
/* Object, Child, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_STATE (SP_OBJECT_FLAGS_ALL & ~(SP_OBJECT_PARENT_MODIFIED_FLAG))
/* Flags that will propagate downstreams */
/* Parent, Style, Viewport, User */
#define SP_OBJECT_MODIFIED_CASCADE (SP_OBJECT_FLAGS_ALL & ~(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))

typedef struct _SPObject SPObject;
typedef struct _SPObjectClass SPObjectClass;

#define SP_TYPE_OBJECT (sp_object_get_type ())
#define SP_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_OBJECT, SPObject))
#define SP_IS_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_OBJECT))

GType sp_object_get_type (void);

typedef struct _SPItem SPItem;
typedef struct _SPItemClass SPItemClass;

#define SP_TYPE_ITEM (sp_item_get_type ())
#define SP_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ITEM, SPItem))
#define SP_IS_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ITEM))

GType sp_item_get_type (void);

/* Editing window */

typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;

#define SP_TYPE_DESKTOP (sp_desktop_get_type ())
#define SP_DESKTOP(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_DESKTOP, SPDesktop))
#define SP_IS_DESKTOP(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_DESKTOP))

GType sp_desktop_get_type (void);

/* Selection */

/*
 * Signals
 *
 * void (* changed) (SPSelection *selection);
 * void (* modified) (SPSelection *selection, guint flags);
 */

typedef struct _SPSelection SPSelection;
typedef struct _SPSelectionClass SPSelectionClass;

#define SP_TYPE_SELECTION (sp_selection_get_type ())
#define SP_SELECTION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_SELECTION, SPSelection))
#define SP_IS_SELECTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_SELECTION))

GType sp_selection_get_type (void);

/* Defined in sodipodi.c */
SPDocument *sodipodi_document_new (const unsigned char *uri, unsigned int advertize, unsigned int keepalive);
SPDocument *sodipodi_document_new_from_mem (const unsigned char *cdata, unsigned int length,
					    unsigned int advertize, unsigned int keepalive);
SPDesktop *sodipodi_get_active_desktop (void);

/* Defined in sp-document.c */
const unsigned char *sp_document_get_uri (SPDocument *doc);
const unsigned char *sp_document_get_name (SPDocument *doc);
SPReprDoc *sp_document_get_repr_doc (SPDocument *doc);
SPRepr *sp_document_get_repr_root (SPDocument *doc);
SPObject *sp_document_get_object_from_id (SPDocument *doc, const unsigned char *id);
SPObject *sp_document_get_object_from_repr (SPDocument *doc, SPRepr *repr);

/* Defined in document-undo.c */
/* Save all previous actions to stack, as one undo step */
void sp_document_done (SPDocument *document);
void sp_document_maybe_done (SPDocument *document, const unsigned char *key);
/* Cancel (and revert) current unsaved actions */
void sp_document_cancel (SPDocument *document);

/* Defined in desktop-handles.c */
SPSelection *sp_desktop_get_selection (SPDesktop *desktop);
SPDocument *sp_desktop_get_document (SPDesktop *desktop);

/* Defined in selection.c */
void sp_selection_set_empty (SPSelection *selection);
SPItem *sp_selection_get_item (SPSelection *selection);
void sp_selection_set_item (SPSelection *selection, SPItem *item);
SPRepr *sp_selection_get_repr (SPSelection *selection);
void sp_selection_set_repr (SPSelection *selection, SPRepr *repr);

G_END_DECLS

#endif

