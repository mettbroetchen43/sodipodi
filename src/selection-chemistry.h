#ifndef __SP_SELECTION_CHEMISTRY_H__
#define __SP_SELECTION_CHEMISTRY_H__

/*
 * Miscellanous operations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "selection.h"

void sp_edit_cleanup (gpointer data);

void sp_selection_delete (GtkWidget * widget);
void sp_selection_duplicate (GtkWidget * widget);
void sp_edit_clear_all (gpointer data);

void sp_selection_group (GtkWidget * widget);
void sp_selection_ungroup (gpointer data);

void sp_selection_raise (GtkWidget * widget);
void sp_selection_raise_to_top (GtkWidget * widget);
void sp_selection_lower (GtkWidget * widget);
void sp_selection_lower_to_bottom (GtkWidget * widget);

void sp_undo (GtkWidget * widget);
void sp_redo (GtkWidget * widget);
void sp_selection_cut (GtkWidget * widget);
void sp_selection_copy (GtkWidget * widget);
void sp_selection_paste (GtkWidget * widget);

void sp_selection_apply_affine (SPSelection * selection, double affine[6]);
void sp_selection_remove_transform (void);
void sp_selection_scale_absolute (SPSelection * selection, double x0, double x1, double y0, double y1);
void sp_selection_scale_relative (SPSelection * selection, ArtPoint * align, double dx, double dy);
void sp_selection_rotate_relative (SPSelection * selection, ArtPoint * center, gdouble angle);
void sp_selection_skew_relative (SPSelection * selection, ArtPoint * align, double dx, double dy);
void sp_selection_move_relative (SPSelection * selection, double dx, double dy);

void sp_selection_rotate_90 (void);
void sp_selection_move_screen (gdouble sx, gdouble sy);
void sp_selection_item_next (void);
void sp_selection_item_prev (void);

/* selection cycling */

typedef enum
{
	SP_CYCLE_SIMPLE,
	SP_CYCLE_VISIBLE, /* cycle only visible items */
	SP_CYCLE_FOCUS /* readjust visible area to view selected item */
} SPCycleType;

/* fixme: This should be moved into preference repr */
#ifndef __SP_SELECTION_CHEMISTRY_C__
extern SPCycleType SP_CYCLING;
#else
SPCycleType SP_CYCLING = SP_CYCLE_FOCUS;
#endif

#endif


