#ifndef SP_SELECTION_CHEMISTRY_H
#define SP_SELECTION_CHEMISTRY_H

/*
 * selection-chemistry - find a better name for it!
 *
 * Here are collected UI handlers, manipulating with selections
 *
 */

#include "selection.h"

void sp_selection_delete (GtkWidget * widget);
void sp_selection_duplicate (GtkWidget * widget);

void sp_selection_group (GtkWidget * widget);
void sp_selection_ungroup (GtkWidget * widget);

void sp_selection_raise (GtkWidget * widget);
void sp_selection_raise_to_top (GtkWidget * widget);
void sp_selection_lower (GtkWidget * widget);
void sp_selection_lower_to_bottom (GtkWidget * widget);

void sp_undo (GtkWidget * widget);
void sp_redo (GtkWidget * widget);
void sp_selection_cut (GtkWidget * widget);
void sp_selection_copy (GtkWidget * widget);
void sp_selection_paste (GtkWidget * widget);

#endif
