#ifndef __SP_SELECTION_H__
#define __SP_SELECTION_H__

/*
 * Per-desktop selection container
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define SP_TYPE_SELECTION            (sp_selection_get_type ())
#define SP_SELECTION(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SELECTION, SPSelection))
#define SP_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SELECTION, SPSelectionClass))
#define SP_IS_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SELECTION))
#define SP_IS_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SELECTION))

#include "forward.h"
#include "sp-item.h"

struct _SPSelection {
	GtkObject object;
	SPDesktop *desktop;
	GSList *reprs;
	GSList *items;
	guint idle;
	guint flags;
};

struct _SPSelectionClass {
	GtkObjectClass parent_class;

	void (* changed) (SPSelection *selection);

	/* fixme: use fine granularity */
	void (* modified) (SPSelection *selection, guint flags);
};

GtkType sp_selection_get_type (void);

/* Constructor */

SPSelection * sp_selection_new (SPDesktop * desktop);

/* This are private methods & will be removed from this file */

void sp_selection_changed (SPSelection * selection);
void sp_selection_update_statusbar (SPSelection * selection);

gboolean sp_selection_item_selected (SPSelection * selection, SPItem * item);
gboolean sp_selection_repr_selected (SPSelection * selection, SPRepr * repr);
#define sp_selection_is_empty(s) (s->items == NULL)

void sp_selection_add_item (SPSelection * selection, SPItem * item);
void sp_selection_add_repr (SPSelection * selection, SPRepr * repr);
void sp_selection_set_item (SPSelection * selection, SPItem * item);
void sp_selection_set_repr (SPSelection * selection, SPRepr * repr);
void sp_selection_remove_item (SPSelection * selection, SPItem * item);
void sp_selection_remove_repr (SPSelection * selection, SPRepr * repr);
void sp_selection_set_item_list (SPSelection * selection, const GSList * list);
void sp_selection_set_repr_list (SPSelection * selection, const GSList * list);
#define sp_selection_set_empty(s) sp_selection_empty (s)
void sp_selection_empty (SPSelection * selection);
const GSList * sp_selection_item_list (SPSelection * selection);
const GSList * sp_selection_repr_list (SPSelection * selection);
SPItem * sp_selection_item (SPSelection * selection);
SPRepr * sp_selection_repr (SPSelection * selection);

ArtDRect *sp_selection_bbox (SPSelection *selection, ArtDRect *bbox);
ArtDRect *sp_selection_bbox_document (SPSelection *selection, ArtDRect *bbox);
GSList * sp_selection_snappoints (SPSelection * selection);

#endif
