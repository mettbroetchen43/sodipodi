#ifndef SP_DOCUMENT_H
#define SP_DOCUMENT_H

/*
 * SPDocument
 *
 * Currently is synonym for SPRoot
 * but we probably add undo stack to it one day...
 *
 */

#include "sp-root.h"

#define SPDocument SPRoot
#define SP_DOCUMENT SP_ROOT
#define SP_IS_DOCUMENT SP_IS_ROOT

/* Destructor */

void sp_document_destroy (SPDocument * document);

/* Constructors */

SPDocument * sp_document_new (void);
SPDocument * sp_document_new_from_file (const gchar * filename);

/* methods */

SPItem * sp_document_add_repr (SPDocument * document, SPRepr * repr);

gdouble sp_document_page_width (SPDocument * document);
gdouble sp_document_page_height (SPDocument * document);

GSList * sp_document_items_in_box (SPDocument * document, ArtDRect * box);

#endif
