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

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include <libart_lgpl/art_rect.h>
#include "xml/repr.h"

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

#define SP_TYPE_DOCUMENT            (sp_document_get_type ())
#define SP_DOCUMENT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCUMENT, SPItem))
#define SP_DOCUMENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCUMENT, SPItemClass))
#define SP_IS_DOCUMENT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCUMENT))
#define SP_IS_DOCUMENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCUMENT))

struct _SPDocument {
	GtkObject object;
	SPRoot * root;		/* Our SPRoot */
	gdouble width;		/* Width of document */
	gdouble height;		/* Height of document */
	GHashTable * iddef;	/* id dictionary */
	gchar * uri;		/* Document uri */
	gchar * base;		/* Document base URI */
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

/*
 * Access methods
 */

SPRoot * sp_document_root (SPDocument * document);
gdouble sp_document_width (SPDocument * document);
gdouble sp_document_height (SPDocument * document);
const gchar * sp_document_uri (SPDocument * document);
const gchar * sp_document_base (SPDocument * document);

/*
 * Dictionary
 */

void sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object);
void sp_document_undef_id (SPDocument * document, const gchar * id);
SPObject * sp_document_lookup_id (SPDocument * document, const gchar * id);

/*
 * Misc
 */

GSList * sp_document_items_in_box (SPDocument * document, ArtDRect * box);
SPItem * sp_document_add_repr (SPDocument * document, SPRepr * repr);





#if 0
/* Old SPDocument */
#ifndef SP_DOCUMENT_DEFINED
#define SP_DOCUMENT_DEFINED
#define SPDocument SPRoot
#endif

#define SP_DOCUMENT SP_ROOT
#define SP_IS_DOCUMENT SP_IS_ROOT

/* Destructor */

void sp_document_destroy (SPDocument * document);

/* Constructors */

SPDocument * sp_document_new (void);
SPDocument * sp_document_new_from_file (const gchar * filename);

/* methods */


gdouble sp_document_page_width (SPDocument * document);
gdouble sp_document_page_height (SPDocument * document);

#endif

#endif
