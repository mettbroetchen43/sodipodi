#define __SP_TRANSFORMATION_C__

/*
 * Object transformation dialog
 *
 * Authors:
 *   Frank Felfe <innerspace@iname.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkstock.h>

#include "helper/sp-intl.h"
#include "helper/window.h"
#include "helper/unit-menu.h"
#include "widgets/icon.h"
#include "macros.h"
#include "sodipodi.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "desktop-handles.h"

/* Notebook pages */
/* These are hardcoded so do not play with them */

enum {
	SP_TRANSFORMATION_MOVE,
	SP_TRANSFORMATION_SCALE,
	SP_TRANSFORMATION_ROTATE,
	SP_TRANSFORMATION_SKEW
};

static void sp_transformation_dialog_present (unsigned int page);
static GtkWidget *sp_transformation_dialog_new (void);

static void sp_transformation_dialog_apply (GObject *object, GObject *dlg);
static void sp_transformation_dialog_close (GObject *object, GtkWidget *dlg);

static GtkWidget *sp_transformation_page_move_new (GObject *obj);
static void sp_transformation_move_update (GObject *dlg, SPSelection *selection);
static void sp_transformation_move_apply (GObject *dlg, SPSelection *selection, unsigned int copy);

static GtkWidget *sp_transformation_page_scale_new (GObject *obj);
static void sp_transformation_scale_update (GObject *dlg, SPSelection *selection);
static void sp_transformation_scale_apply (GObject *dlg, SPSelection *selection, unsigned int copy);

static GtkWidget *sp_transformation_page_rotate_new (GObject *obj);
static void sp_transformation_rotate_update (GObject *dlg, SPSelection *selection);
static void sp_transformation_rotate_apply (GObject *dlg, SPSelection *selection, unsigned int copy);

static void sp_transformation_skew_apply (GObject *dlg, SPSelection *selection, unsigned int copy);

static GtkWidget *dlg = NULL;

void
sp_transformation_dialog_move (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_MOVE);
}

void
sp_transformation_dialog_scale (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_SCALE);
}

void
sp_transformation_dialog_rotate (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_ROTATE);
}

void
sp_transformation_dialog_skew (void)
{
	sp_transformation_dialog_present (SP_TRANSFORMATION_SKEW);
}

static void
sp_transformation_dialog_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (SODIPODI, object);

	dlg = NULL;
}

static void
sp_transformation_dialog_present (unsigned int page)
{
	GtkWidget *nbook;

	if (!dlg) {
		dlg = sp_transformation_dialog_new ();
	}

	nbook = g_object_get_data (G_OBJECT (dlg), "notebook");
	gtk_notebook_set_page (GTK_NOTEBOOK (nbook), page);

#if 0
	sp_transformation_apply_button_reset ();
#endif

	gtk_widget_show (dlg);
	gtk_window_present (GTK_WINDOW (dlg));
}

static void
sp_transformation_dialog_update_selection (GObject *dlg, unsigned int page, SPSelection *selection)
{
	GtkWidget *apply;

	switch (page) {
	case SP_TRANSFORMATION_MOVE:
		sp_transformation_move_update (dlg, selection);
		break;
	case SP_TRANSFORMATION_SCALE:
		sp_transformation_scale_update (dlg, selection);
		break;
	case SP_TRANSFORMATION_ROTATE:
		sp_transformation_rotate_update (dlg, selection);
		break;
	case SP_TRANSFORMATION_SKEW:
	default:
		break;
	}

	apply = g_object_get_data (dlg, "apply");
	if (selection && !sp_selection_is_empty (selection)) {
		gtk_widget_set_sensitive (apply, TRUE);
	} else {
		gtk_widget_set_sensitive (apply, FALSE); 
	}
}

static void
sp_transformation_dialog_selection_changed (Sodipodi *sodipodi, SPSelection *selection, GObject *obj)
{
	GObject *notebook;
	int page;

	notebook = g_object_get_data (obj, "notebook");
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	sp_transformation_dialog_update_selection (obj, page, selection);
}

static void
sp_transformation_dialog_selection_modified (Sodipodi *sodipodi, SPSelection *selection, unsigned int flags, GObject *obj)
{
	GObject *notebook;
	int page;

	notebook = g_object_get_data (obj, "notebook");
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	sp_transformation_dialog_update_selection (obj, page, selection);
}

static void
sp_transformation_dialog_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint pagenum, GObject *dlg)
{
	SPSelection *sel;

	sel = (SP_ACTIVE_DESKTOP) ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL;

	sp_transformation_dialog_update_selection (dlg, pagenum, sel);
}

static GtkWidget *
sp_transformation_dialog_new (void)
{
	GtkWidget *dlg;
	GtkWidget *hb, *vb, *nbook, *page, *img, *cb, *bb, *b;
	SPSelection *sel;

	dlg = sp_window_new (_("Transform selection"), FALSE);

	/* Toplevel hbox */
	hb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hb);
	gtk_container_add (GTK_CONTAINER (dlg), hb);

	/* Toplevel vbox */
	vb = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (hb), vb, TRUE, TRUE, 0);
	
	/* Notebook for individual transformations */
	nbook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (vb), nbook, TRUE, TRUE, 0);
	g_object_set_data (G_OBJECT (dlg), "notebook", nbook);
#if 0
	/* Separator */
	hs = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vb), hs, FALSE, FALSE, 0);
#endif
	/* Apply to duplicate */
	cb = gtk_check_button_new_with_label (_("Apply to duplicate"));
	g_object_set_data (G_OBJECT (dlg), "duplicate", cb);
	gtk_box_pack_start (GTK_BOX (vb), cb, FALSE, FALSE, 0);
	/* Buttons */
	bb = gtk_hbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vb), bb, FALSE, FALSE, 0);
	b = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	g_object_set_data (G_OBJECT (dlg), "apply", b);
	gtk_box_pack_start (GTK_BOX (bb), b, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sp_transformation_dialog_apply), dlg);
	b = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (bb), b, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sp_transformation_dialog_close), dlg);

	/* Move page */
	img = sp_icon_new (SP_ICON_SIZE_NOTEBOOK, "move");
	page = sp_transformation_page_move_new (G_OBJECT (dlg));
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), page, img);
	g_object_set_data (G_OBJECT (dlg), "move", page);

	/* Scale page */
	img = sp_icon_new (SP_ICON_SIZE_NOTEBOOK, "scale");
	page = sp_transformation_page_scale_new (G_OBJECT (dlg));
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), page, img);
	g_object_set_data (G_OBJECT (dlg), "scale", page);

	/* Rotate page */
	img = sp_icon_new (SP_ICON_SIZE_NOTEBOOK, "rotate");
	page = sp_transformation_page_rotate_new (G_OBJECT (dlg));
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), page, img);
	g_object_set_data (G_OBJECT (dlg), "rotate", page);

	gtk_widget_show_all (vb);

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg), "destroy", G_CALLBACK (sp_transformation_dialog_destroy), NULL);
	g_signal_connect (G_OBJECT (SODIPODI), "change_selection", G_CALLBACK (sp_transformation_dialog_selection_changed), dlg);
	g_signal_connect (G_OBJECT (SODIPODI), "modify_selection", G_CALLBACK (sp_transformation_dialog_selection_modified), dlg);
	g_signal_connect (G_OBJECT (nbook), "switch_page", G_CALLBACK (sp_transformation_dialog_switch_page), dlg);

	sel = (SP_ACTIVE_DESKTOP) ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL;
	sp_transformation_dialog_update_selection (G_OBJECT (dlg), 0, sel);

	return dlg;
}

static void
sp_transformation_dialog_apply (GObject *object, GObject *dlg)
{
	SPDesktop *desktop;
	SPSelection *selection;
	GtkWidget *nbookw, *duplw;
	int page, duplicate;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail (desktop != NULL);
	selection = SP_DT_SELECTION (desktop);
	g_return_if_fail (!sp_selection_is_empty (selection));

	nbookw = g_object_get_data (dlg, "notebook");
	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (nbookw));

	duplw = g_object_get_data (dlg, "duplicate");
	duplicate = gtk_toggle_button_get_active ((GtkToggleButton *) duplw);

	switch (page) {
	case SP_TRANSFORMATION_MOVE:
		sp_transformation_move_apply (dlg, selection, duplicate);
		break;
	case SP_TRANSFORMATION_ROTATE:
		sp_transformation_rotate_apply (dlg, selection, duplicate);
		break;
	case SP_TRANSFORMATION_SCALE:
		sp_transformation_scale_apply (dlg, selection, duplicate);
		break;
	case SP_TRANSFORMATION_SKEW:
		sp_transformation_skew_apply (dlg, selection, duplicate);
		break;
	}
}

static void
sp_transformation_dialog_close (GObject *object, GtkWidget *dlg)
{
	gtk_widget_destroy (dlg);
}

/*
 * Move implementation
 */

static GtkWidget *
sp_transformation_page_move_new (GObject *obj)
{
	GtkWidget *frame, *vb, *tbl, *lbl, *img, *sb, *us;
	GtkAdjustment *adj;

	frame = gtk_frame_new (_("Move"));

	vb = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_container_add (GTK_CONTAINER (frame), vb);

	tbl = gtk_table_new (4, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (tbl), 4);
	gtk_table_set_col_spacings (GTK_TABLE (tbl), 4);
	gtk_box_pack_start (GTK_BOX (vb), tbl, FALSE, FALSE, 0);

	/* Unit selector */
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	g_object_set_data (obj, "move_units", us);
	if (SP_ACTIVE_DESKTOP) {
		sp_unit_selector_set_unit (SP_UNIT_SELECTOR (us), sp_desktop_get_default_unit (SP_ACTIVE_DESKTOP));
	}
	/* Horizontal */
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "arrows_hor");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 0, 1, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "move_position_x", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* Vertical */
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "arrows_ver");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 1, 2, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "move_position_y", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* Unit selector */
	lbl = gtk_label_new (_("Units:"));
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 0, 1, 2, 3, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (tbl), us, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_widget_show_all (vb);

	return frame;
}

static void
sp_transformation_move_update (GObject *dlg, SPSelection *selection)
{
	GtkWidget *page;

	page = g_object_get_data (dlg, "move");

	if (selection && !sp_selection_is_empty (selection)) {
		gtk_widget_set_sensitive (page, TRUE);
	} else {
		gtk_widget_set_sensitive (page, FALSE);
	}
}

static void
sp_transformation_move_apply (GObject *dlg, SPSelection *selection, unsigned int copy)
{
	SPUnitSelector *us;
	GtkAdjustment *ax, *ay;
	float x, y;

	us = g_object_get_data (dlg, "move_units");
	ax = g_object_get_data (dlg, "move_position_x");
	ay = g_object_get_data (dlg, "move_position_y");
	x = sp_unit_selector_get_value_in_points (us, ax);
	y = sp_unit_selector_get_value_in_points (us, ay);

	sp_selection_move_relative (selection, x, y, copy);

	if (selection) sp_document_done (SP_DT_DOCUMENT (selection->desktop));
}

/*
 * Scale implementation
 */

static gboolean
sp_transformation_scale_set_unit (SPUnitSelector *us, const SPUnit *old, const SPUnit *new, GObject *dlg)
{
	SPDesktop *desktop;
	SPSelection *selection;

	desktop = SP_ACTIVE_DESKTOP;
	if (!desktop) return FALSE;
	selection = SP_DT_SELECTION (desktop);
	if (sp_selection_is_empty (selection)) return FALSE;

	if ((old->base == SP_UNIT_ABSOLUTE) && (new->base == SP_UNIT_DIMENSIONLESS)) {
		SPUnitSelector *us;
		GtkAdjustment *ax, *ay;
		NRRectF bbox;
		float x, y;
		/* Absolute to percentage */
		g_object_set_data (dlg, "update", GUINT_TO_POINTER (TRUE));
		us = g_object_get_data (dlg, "scale_units");
		ax = g_object_get_data (dlg, "scale_dimension_x");
		ay = g_object_get_data (dlg, "scale_dimension_y");
		x = sp_units_get_points (ax->value, old);
		y = sp_units_get_points (ay->value, old);
		sp_selection_bbox (selection, &bbox);
		gtk_adjustment_set_value (ax, 100.0 * x / (bbox.x1 - bbox.x0));
		gtk_adjustment_set_value (ay, 100.0 * y / (bbox.y1 - bbox.y0));
		g_object_set_data (dlg, "update", GUINT_TO_POINTER (FALSE));
		return TRUE;
	} else if ((old->base == SP_UNIT_DIMENSIONLESS) && (new->base == SP_UNIT_ABSOLUTE)) {
		SPUnitSelector *us;
		GtkAdjustment *ax, *ay;
		NRRectF bbox;
		/* Percentage to absolute */
		g_object_set_data (dlg, "update", GUINT_TO_POINTER (TRUE));
		us = g_object_get_data (dlg, "scale_units");
		ax = g_object_get_data (dlg, "scale_dimension_x");
		ay = g_object_get_data (dlg, "scale_dimension_y");
		sp_selection_bbox (selection, &bbox);
		gtk_adjustment_set_value (ax, sp_points_get_units (0.01 * ax->value * (bbox.x1 - bbox.x0), new));
		gtk_adjustment_set_value (ay, sp_points_get_units (0.01 * ay->value * (bbox.y1 - bbox.y0), new));
		g_object_set_data (dlg, "update", GUINT_TO_POINTER (FALSE));
		return TRUE;
	}

	return FALSE;
}

static void
sp_transformation_scale_value_changed (GtkAdjustment *adj, GObject *dlg)
{
	GtkWidget *apply;

	if (g_object_get_data (dlg, "update")) return;

	apply = g_object_get_data (dlg, "apply");
	gtk_widget_set_sensitive (apply, TRUE);
}

static GtkWidget *
sp_transformation_page_scale_new (GObject *obj)
{
	GtkWidget *frame, *vb, *tbl, *lbl, *img, *sb, *us;
	GtkAdjustment *adj;

	frame = gtk_frame_new (_("Scale"));

	vb = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_container_add (GTK_CONTAINER (frame), vb);

	tbl = gtk_table_new (4, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (tbl), 4);
	gtk_table_set_col_spacings (GTK_TABLE (tbl), 4);
	gtk_box_pack_start (GTK_BOX (vb), tbl, FALSE, FALSE, 0);

	/* Unit selector */
	/* fixme: Default has to be percentage */
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	g_object_set_data (obj, "scale_units", us);
	sp_unit_selector_add_unit (SP_UNIT_SELECTOR (us), sp_unit_get_by_abbreviation ("%"), 0);
	sp_unit_selector_set_unit (SP_UNIT_SELECTOR (us), sp_unit_get_by_abbreviation ("%"));
	g_signal_connect (G_OBJECT (us), "set_unit", G_CALLBACK (sp_transformation_scale_set_unit), obj);
	/* Horizontal */
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "scale_hor");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 0, 1, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "scale_dimension_x", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	g_signal_connect (G_OBJECT (adj), "value_changed", G_CALLBACK (sp_transformation_scale_value_changed), obj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* Vertical */
	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "scale_ver");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 1, 2, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "scale_dimension_y", adj);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), adj);
	g_signal_connect (G_OBJECT (adj), "value_changed", G_CALLBACK (sp_transformation_scale_value_changed), obj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* Unit selector */
	lbl = gtk_label_new (_("Units:"));
	gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tbl), lbl, 0, 1, 2, 3, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (tbl), us, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_widget_show_all (vb);

	return frame;
}

static void
sp_transformation_scale_update (GObject *dlg, SPSelection *selection)
{
	GtkWidget *page;

	page = g_object_get_data (dlg, "scale");

	if (selection && !sp_selection_is_empty (selection)) {
		GtkAdjustment *ax, *ay;
		SPUnitSelector *us;
		const SPUnit *unit;
		NRRectF bbox;
		ax = g_object_get_data (dlg, "scale_dimension_x");
		ay = g_object_get_data (dlg, "scale_dimension_y");
		us = g_object_get_data (dlg, "scale_units");
		sp_selection_bbox (selection, &bbox);
		unit = sp_unit_selector_get_unit (us);
		if (unit->base == SP_UNIT_ABSOLUTE) {
			sp_unit_selector_set_value_in_points (us, ax, bbox.x1 - bbox.x0);
			sp_unit_selector_set_value_in_points (us, ay, bbox.y1 - bbox.y0);
		} else {
			gtk_adjustment_set_value (ax, 100.0);
			gtk_adjustment_set_value (ay, 100.0);
		}
		gtk_widget_set_sensitive (page, TRUE);
	} else {
		gtk_widget_set_sensitive (page, FALSE);
	}
}

static void
sp_transformation_scale_apply (GObject *dlg, SPSelection *selection, unsigned int copy)
{
	GtkAdjustment *ax, *ay;
	SPUnitSelector *us;
	const SPUnit *unit;
	NRRectF bbox;
	NRPointF c;
	float x, y;

	us = g_object_get_data (dlg, "scale_units");
	ax = g_object_get_data (dlg, "scale_dimension_x");
	ay = g_object_get_data (dlg, "scale_dimension_y");

	sp_selection_bbox (selection, &bbox);
	c.x = 0.5 * (bbox.x0 + bbox.x1);
	c.y = 0.5 * (bbox.y0 + bbox.y1);
	unit = sp_unit_selector_get_unit (us);
	if (unit->base == SP_UNIT_ABSOLUTE) {
		x = sp_unit_selector_get_value_in_points (us, ax);
		y = sp_unit_selector_get_value_in_points (us, ay);
		sp_selection_scale_relative (selection, &c, x / (bbox.x1 - bbox.x0), y / (bbox.y1 - bbox.y0), copy);
	} else {
		sp_selection_scale_relative (selection, &c, 0.01 * ax->value, 0.01 * ay->value, copy);
	}

	if (selection) sp_document_done (SP_DT_DOCUMENT (selection->desktop));
}

/*
 * Rotate implementation
 */

static void
sp_transformation_rotate_value_changed (GtkAdjustment *adj, GObject *dlg)
{
	GtkWidget *apply;

	if (g_object_get_data (dlg, "update")) return;

	apply = g_object_get_data (dlg, "apply");
	gtk_widget_set_sensitive (apply, TRUE);
}

static GtkWidget *
sp_transformation_page_rotate_new (GObject *obj)
{
	GtkWidget *frame, *vb, *tbl, *lbl, *img, *sb;
	GtkAdjustment *adj;

	frame = gtk_frame_new (_("Rotate"));

	vb = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_container_add (GTK_CONTAINER (frame), vb);

	tbl = gtk_table_new (1, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (tbl), 4);
	gtk_table_set_col_spacings (GTK_TABLE (tbl), 4);
	gtk_box_pack_start (GTK_BOX (vb), tbl, FALSE, FALSE, 0);

	img = sp_icon_new (SP_ICON_SIZE_BUTTON, "rotate_left");
	gtk_table_attach (GTK_TABLE (tbl), img, 0, 1, 0, 1, 0, 0, 0, 0);
	adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -1e6, 1e6, 0.01, 0.1, 0.1);
	g_object_set_data (obj, "rotate_angle", adj);
	g_signal_connect (G_OBJECT (adj), "value_changed", G_CALLBACK (sp_transformation_rotate_value_changed), obj);
	sb = gtk_spin_button_new (adj, 0.1, 2);
	gtk_table_attach (GTK_TABLE (tbl), sb, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	lbl = gtk_label_new (_("deg"));
	gtk_table_attach (GTK_TABLE (tbl), lbl, 2, 3, 0, 1, 0, 0, 0, 0);

	gtk_widget_show_all (vb);

	return frame;
}

static void
sp_transformation_rotate_update (GObject *dlg, SPSelection *selection)
{
	GtkWidget *page;

	page = g_object_get_data (dlg, "rotate");

	if (selection && !sp_selection_is_empty (selection)) {
		gtk_widget_set_sensitive (page, TRUE);
	} else {
		gtk_widget_set_sensitive (page, FALSE);
	}
}

static void
sp_transformation_rotate_apply (GObject *dlg, SPSelection *selection, unsigned int copy)
{
	GtkAdjustment *a;
	NRRectF bbox;
	NRPointF c;

	a = g_object_get_data (dlg, "rotate_angle");

	sp_selection_bbox (selection, &bbox);
	c.x = 0.5 * (bbox.x0 + bbox.x1);
	c.y = 0.5 * (bbox.y0 + bbox.y1);
	sp_selection_rotate_relative (selection, &c, a->value, copy);

	if (selection) sp_document_done (SP_DT_DOCUMENT (selection->desktop));
}

static void
sp_transformation_skew_apply (GObject *dlg, SPSelection *selection, unsigned int copy)
{
}

