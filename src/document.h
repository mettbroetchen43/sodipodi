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

#ifndef SP_OBJECT_DEFINED
#define SP_OBJECT_DEFINED
typedef struct _SPObject SPObject;
typedef struct _SPObjectClass SPObjectClass;
#endif

#ifndef SP_ITEM_DEFINED
#define SP_ITEM_DEFINED
typedef struct _SPItem SPItem;
typedef struct _SPItemClass SPItemClass;
#endif

#ifndef SP_ROOT_DEFINED
#define SP_ROOT_DEFINED
typedef struct _SPRoot SPRoot;
typedef struct _SPRootClass SPRootClass;
#endif

#ifndef SP_DOCUMENT_DEFINED
#define SP_DOCUMENT_DEFINED
typedef struct _SPDocument SPDocument;
typedef struct _SPDocumentClass SPDocumentClass;
#endif

typedef struct _SPDocumentPrivate SPDocumentPrivate;

#define SP_TYPE_DOCUMENT            (sp_document_get_type ())
#define SP_DOCUMENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCUMENT, SPItem))
#define SP_DOCUMENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCUMENT, SPItemClass))
#define SP_IS_DOCUMENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCUMENT))
#define SP_IS_DOCUMENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCUMENT))

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include <libart_lgpl/art_rect.h>
#include "xml/repr.h"
#include "sp-namedview.h"

struct _SPDocument {
	GtkObject object;

	SPDocumentPrivate * private;
};

struct _SPDocumentClass {
	GtkObjectClass parent_class;
};

GtkType sp_document_get_type (void);

/*
 * Constructor
 *
 * Fetches document from URI, or creates new, if NULL
 */

SPDocument * sp_document_new (const gchar * uri);

SPDocument * sp_document_new_from_mem (const gchar * buffer, gint length);

/*
 * Access methods
 */

SPReprDoc * sp_document_repr_doc (SPDocument * document);
SPRepr * sp_document_repr_root (SPDocument * document);
SPRoot * sp_document_root (SPDocument * document);
gdouble sp_document_width (SPDocument * document);
gdouble sp_document_height (SPDocument * document);
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

gboolean sp_document_change_order_requested (SPDocument * document, SPObject * object, gint order);
gboolean sp_document_change_attr_requested (SPDocument * document, SPObject * object, const gchar * key, const gchar * value);
gboolean sp_document_change_content_requested (SPDocument * document, SPObject * object, const gchar * value);

/* Save all previous actions to stack, as one undo step */

void sp_document_done (SPDocument * document);

/* Clear current actions, so these cannot be undone */

void sp_document_clear_actions (SPDocument * document);

void sp_document_undo (SPDocument * document);
void sp_document_redo (SPDocument * document);

/*
 * Actions
 */

/* Adds repr to document, returning created item, if any */
SPItem * sp_document_add_repr (SPDocument * document, SPRepr * repr);
/* Deletes repr from document */
void sp_document_del_repr (SPDocument * document, SPRepr * repr);

/*
 * Misc
 */

GSList * sp_document_items_in_box (SPDocument * document, ArtDRect * box);


#endif
