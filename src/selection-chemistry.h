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

#include <libnr/nr-types.h>

#include "forward.h"
#include "helper/action.h"

/* Action based functionality */

void sp_selection_action_perform (SPAction *action, void *config, void *data);

/* Simple stuff */

void sp_edit_cleanup (gpointer object, gpointer data);

void sp_selection_delete (gpointer object, gpointer data);
void sp_selection_duplicate (gpointer object, gpointer data);
void sp_edit_clear_all (gpointer object, gpointer data);
void sp_edit_select_all (gpointer object, gpointer data);

void sp_selection_cut (GtkWidget *widget);
void sp_selection_copy (GtkWidget *widget);
void sp_selection_paste (GtkWidget *widget);

/* This uses generic transformation action */
/* Transforms selection optionally creating duplicate */
void sp_selection_apply_affine (SPSelection *selection, NRMatrixF *transform, unsigned int duplicate, unsigned int repeatable);

void sp_selection_remove_transform (void);
void sp_selection_scale_absolute (SPSelection *selection, double x0, double x1, double y0, double y1);

void sp_selection_scale_relative (SPSelection *selection, NRPointF *align, double dx, double dy, unsigned int duplicate);
void sp_selection_move_relative (SPSelection *selection, double dx, double dy, unsigned int duplicate);
void sp_selection_rotate_relative (SPSelection *selection, NRPointF *center, double theta_deg, unsigned int duplicate);

void sp_selection_skew_relative (SPSelection *selection, NRPointF *align, double dx, double dy);

void sp_selection_rotate_90 (void);
void sp_selection_move_screen (gdouble sx, gdouble sy);
void sp_selection_item_next (void);
void sp_selection_item_prev (void);

#endif


