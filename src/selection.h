#ifndef SP_SELECTION_H
#define SP_SELECTION_H

/*
 * SPSelection
 *
 * A collection of items
 * Produces signals, when changed
 * Will listen, if items destroyed & behave accordingly
 *
 */

#include <gtk/gtk.h>
#include "sp-item.h"

#ifndef SP_DESKTOP_DEFINED
#define SP_DESKTOP_DEFINED
typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;
#endif

#define SP_TYPE_SELECTION            (sp_selection_get_type ())
#define SP_SELECTION(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SELECTION, SPSelection))
#define SP_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SELECTION, SPSelectionClass))
#define SP_IS_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SELECTION))
#define SP_IS_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SELECTION))

#ifndef SP_SELECTION_DEFINED
#define SP_SELECTION_DEFINED
typedef struct _SPSelection SPSelection;
#endif
typedef struct _SPSelectionClass SPSelectionClass;

struct _SPSelection {
	GtkObject object;
	SPDesktop * desktop;
	GSList * reprs;
	GSList * items;
};

struct _SPSelectionClass {
	GtkObjectClass parent_class;

	void (* changed) (SPSelection * selection);
};

/* Standard Gtk function */

GtkType sp_selection_get_type (void);

/* Constructor */

SPSelection * sp_selection_new (SPDesktop * desktop);

/* This is private methid & will be removed from header */

void sp_selection_changed (SPSelection * selection);

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
void sp_selection_empty (SPSelection * selection);
const GSList * sp_selection_item_list (SPSelection * selection);
const GSList * sp_selection_repr_list (SPSelection * selection);
SPItem * sp_selection_item (SPSelection * selection);
SPRepr * sp_selection_repr (SPSelection * selection);

ArtDRect * sp_selection_bbox (SPSelection * selection, ArtDRect * bbox);

#endif
