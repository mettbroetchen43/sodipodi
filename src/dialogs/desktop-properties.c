#define __SP_DESKTOP_PROPERTIES_C__

/*
 * Desktop configuration dialog
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) Lauris Kaplinski 2000-2002
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <gtk/gtknotebook.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhseparator.h>

#include "macros.h"
#include "helper/sp-intl.h"
#include "helper/window.h"
#include "helper/unit-menu.h"
#include "svg/svg.h"
#include "widgets/sp-color-selector.h"
#include "widgets/sp-color-preview.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop.h"
#include "../desktop-handles.h"
#include "../sp-namedview.h"

#include "desktop-properties.h"

static GtkWidget *sp_desktop_dialog_new (void);

static void sp_dtw_activate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_dtw_desactivate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_dtw_update (GtkWidget *dialog, SPDesktop *desktop);

static GtkWidget *sp_color_picker_new (unsigned char *colorkey, unsigned char *alphakey, unsigned char *title, guint32 rgba);
static void sp_color_picker_set_rgba32 (GtkWidget *cp, guint32 rgba);
static void sp_color_picker_clicked (GObject *cp, void *data);

static GtkWidget *dlg = NULL;

static void
sp_dtw_dialog_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (SODIPODI, dlg);

	dlg = NULL;
}

void
sp_desktop_dialog (void)
{
	if (!dlg) {
		dlg = sp_desktop_dialog_new ();
		g_signal_connect (G_OBJECT (dlg), "destroy", G_CALLBACK (sp_dtw_dialog_destroy), NULL);
		gtk_widget_show (dlg);
	}

	gtk_window_present (GTK_WINDOW (dlg));
}

static void
sp_dtw_whatever_toggled (GtkToggleButton *tb, GtkWidget *dialog)
{
	SPDesktop *dt;
	SPDocument *doc;
	SPRepr *repr;
	const guchar *key;

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	doc = SP_DT_DOCUMENT (dt);

	repr = SP_OBJECT_REPR (dt->namedview);
	key = gtk_object_get_data (GTK_OBJECT (tb), "key");

	sp_document_set_undo_sensitive (doc, FALSE);
	sp_repr_set_boolean (repr, key, gtk_toggle_button_get_active (tb));
	sp_document_set_undo_sensitive (doc, TRUE);
}

static void
sp_dtw_whatever_changed (GtkAdjustment *adjustment, GtkWidget *dialog)
{
	SPDesktop *dt;
	SPDocument *doc;
	SPRepr *repr;
	SPUnitSelector *us;
	const guchar *key;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	doc = SP_DT_DOCUMENT (dt);

	repr = SP_OBJECT_REPR (dt->namedview);
	key = gtk_object_get_data (GTK_OBJECT (adjustment), "key");
	us = gtk_object_get_data (GTK_OBJECT (adjustment), "unit_selector");

	g_snprintf (c, 32, "%g%s", adjustment->value, sp_unit_selector_get_unit (us)->abbr);

	sp_document_set_undo_sensitive (doc, FALSE);
	sp_repr_set_attr (repr, key, c);
	sp_document_set_undo_sensitive (doc, TRUE);
}

static void
sp_dtw_grid_snap_distance_changed (GtkAdjustment *adjustment, GtkWidget *dialog)
{
	SPRepr *repr;
	SPUnitSelector *us;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	us = gtk_object_get_data (GTK_OBJECT (dialog), "grid_snap_units");

	g_snprintf (c, 32, "%g%s", adjustment->value, sp_unit_selector_get_unit (us)->abbr);
	sp_repr_set_attr (repr, "gridtolerance", c);
}

#if 0
static void
sp_dtw_grid_color_set (GnomeColorPicker *cp, guint r, guint g, guint b, guint a)
{
	SPRepr *repr;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	sp_svg_write_color (c, 32, ((r << 16) & 0xff000000) | ((g << 8) & 0xff0000) | (b & 0xff00));
	sp_repr_set_attr (repr, "gridcolor", c);
	sp_repr_set_double (repr, "gridopacity", (a / 65535.0));
}
#endif

static void
sp_dtw_guides_snap_distance_changed (GtkAdjustment *adjustment, GtkWidget *dialog)
{
	SPRepr *repr;
	SPUnitSelector *us;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	us = gtk_object_get_data (GTK_OBJECT (dialog), "guide_snap_units");

	g_snprintf (c, 32, "%g%s", adjustment->value, sp_unit_selector_get_unit (us)->abbr);
	sp_repr_set_attr (repr, "guidetolerance", c);
}

#if 0
static void
sp_dtw_guides_color_set (GnomeColorPicker *cp, guint r, guint g, guint b, guint a)
{
	SPRepr *repr;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	sp_svg_write_color (c, 32, ((r << 16) & 0xff000000) | ((g << 8) & 0xff0000) | (b & 0xff00));
	sp_repr_set_attr (repr, "guidecolor", c);
	sp_repr_set_double (repr, "guideopacity", (a / 65535.0));
}
#endif

#if 0
static void
sp_dtw_guides_hi_color_set (GnomeColorPicker *cp, guint r, guint g, guint b, guint a)
{
	SPRepr *repr;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	sp_svg_write_color (c, 32, ((r << 16) & 0xff000000) | ((g << 8) & 0xff0000) | (b & 0xff00));
	sp_repr_set_attr (repr, "guidehicolor", c);
	sp_repr_set_double (repr, "guidehiopacity", (a / 65535.0));
}
#endif

static GtkWidget *
sp_desktop_dialog_new (void)
{
	GtkWidget *dialog, *nb, *l, *t, *b, *us, *sb, *cp;
	GtkObject *a;

	dialog = sp_window_new (_("Desktop settings"), FALSE);

	nb = gtk_notebook_new ();
	gtk_widget_show (nb);
	gtk_container_add (GTK_CONTAINER (dialog), nb);

	/* Grid settings */

	/* Notebook tab */
	l = gtk_label_new (_("Grid"));
	gtk_widget_show (l);
	t = gtk_table_new (9, 2, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_table_set_col_spacings (GTK_TABLE (t), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), t, l);

	/* Checkbuttons */
	b = gtk_check_button_new_with_label (_("Show grid"));
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", "showgrid");
	gtk_object_set_data (GTK_OBJECT (dialog), "showgrid", b);
	g_signal_connect (G_OBJECT (b), "toggled", G_CALLBACK (sp_dtw_whatever_toggled), dialog);

	b = gtk_check_button_new_with_label (_("Snap to grid"));
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", "snaptogrid");
	gtk_object_set_data (GTK_OBJECT (dialog), "snaptogrid", b);
	g_signal_connect (G_OBJECT (b), "toggled", G_CALLBACK (sp_dtw_whatever_toggled), dialog);

	l = gtk_label_new (_("Grid units:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	gtk_widget_show (us);
	gtk_table_attach (GTK_TABLE (t), us, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "grid_units", us);

	l = gtk_label_new (_("Origin X:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "gridoriginx");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "gridoriginx", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_whatever_changed), dialog);

	l = gtk_label_new (_("Origin Y:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "gridoriginy");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "gridoriginy", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_whatever_changed), dialog);

	l = gtk_label_new (_("Spacing X:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "gridspacingx");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "gridspacingx", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (GTK_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_whatever_changed), dialog);

	l = gtk_label_new (_("Spacing Y:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "gridspacingy");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "gridspacingy", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_whatever_changed), dialog);

	l = gtk_label_new (_("Snap units:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 6, 7, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE);
	gtk_widget_show (us);
	gtk_table_attach (GTK_TABLE (t), us, 1, 2, 6, 7, GTK_EXPAND | GTK_FILL, 0, 0, 0);
#if 0
	gtk_signal_connect (GTK_OBJECT (us), "set_unit", GTK_SIGNAL_FUNC (sp_dtw_grid_snap_units_set), dialog);
#endif
	gtk_object_set_data (GTK_OBJECT (dialog), "grid_snap_units", us);

	l = gtk_label_new (_("Snap distance:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 7, 8, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (dialog), "gridtolerance", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (sb), GTK_ADJUSTMENT (a));
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 7, 8, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_grid_snap_distance_changed), dialog);

	l = gtk_label_new (_("Grid color:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 8, 9, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	cp = sp_color_picker_new ("gridcolor", "gridopacity", _("Grid color"), 0);
	gtk_widget_show (cp);
	gtk_table_attach (GTK_TABLE (t), cp, 1, 2, 8, 9, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_object_set_data (G_OBJECT (dialog), "gridcolor", cp);

	/* Guidelines page */
	l = gtk_label_new (_("Guides"));
	gtk_widget_show (l);
	t = gtk_table_new (5, 2, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_table_set_col_spacings (GTK_TABLE (t), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), t, l);

	b = gtk_check_button_new_with_label (_("Show guides"));
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", "showguides");
	gtk_object_set_data (GTK_OBJECT (dialog), "showguides", b);
	g_signal_connect (G_OBJECT (b), "toggled", G_CALLBACK (sp_dtw_whatever_toggled), dialog);
	gtk_widget_set_sensitive (b, FALSE);

	b = gtk_check_button_new_with_label (_("Snap to guides"));
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", "snaptoguides");
	gtk_object_set_data (GTK_OBJECT (dialog), "snaptoguides", b);
	g_signal_connect (G_OBJECT (b), "toggled", G_CALLBACK (sp_dtw_whatever_toggled), dialog);

	l = gtk_label_new (_("Snap units:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE);
	gtk_widget_show (us);
	gtk_table_attach (GTK_TABLE (t), us, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
#if 0
	gtk_signal_connect (GTK_OBJECT (us), "set_unit", GTK_SIGNAL_FUNC (sp_dtw_guides_snap_units_set), dialog);
#endif
	gtk_object_set_data (GTK_OBJECT (dialog), "guide_snap_units", us);

	l = gtk_label_new (_("Snap distance:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (dialog), "guidetolerance", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	g_signal_connect (GTK_OBJECT (a), "value_changed", G_CALLBACK (sp_dtw_guides_snap_distance_changed), dialog);

	l = gtk_label_new (_("Guides color:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	cp = sp_color_picker_new ("guidecolor", "guideopacity", _("Guideline color"), 0);
	gtk_widget_show (cp);
	gtk_table_attach (GTK_TABLE (t), cp, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "guidecolor", cp);

	l = gtk_label_new (_("Highlight color:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	cp = sp_color_picker_new ("guidehicolor", "guidehiopacity", _("Highlighted guideline color"), 0);
	gtk_widget_show (cp);
	gtk_table_attach (GTK_TABLE (t), cp, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "guidehicolor", cp);

	/* Page page */
	l = gtk_label_new (_("Page"));
	gtk_widget_show (l);
	t = gtk_table_new (1, 1, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_table_set_col_spacings (GTK_TABLE (t), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), t, l);

	b = gtk_check_button_new_with_label (_("Show border"));
	gtk_widget_show (b);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (b), "key", "showborder");
	gtk_object_set_data (GTK_OBJECT (dialog), "showborder", b);
	g_signal_connect (G_OBJECT (b), "toggled", G_CALLBACK (sp_dtw_whatever_toggled), dialog);

	/* fixme: We should listen namedview changes here as well */
	g_signal_connect (G_OBJECT (SODIPODI), "activate_desktop", G_CALLBACK (sp_dtw_activate_desktop), dialog);
	g_signal_connect (G_OBJECT (SODIPODI), "desactivate_desktop", G_CALLBACK (sp_dtw_desactivate_desktop), dialog);
	sp_dtw_update (dialog, SP_ACTIVE_DESKTOP);

	return dialog;
}

static void
sp_dtw_activate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog)
{
	sp_dtw_update (dialog, desktop);
}

static void
sp_dtw_desactivate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog)
{
	sp_dtw_update (dialog, NULL);
}

static void
sp_dtw_update (GtkWidget *dialog, SPDesktop *desktop)
{
	if (!desktop) {
		GObject *cp, *w;
		gtk_widget_set_sensitive (dialog, FALSE);
		cp = g_object_get_data (G_OBJECT (dialog), "gridcolor");
		w = g_object_get_data (cp, "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
		cp = g_object_get_data (G_OBJECT (dialog), "guidecolor");
		w = g_object_get_data (cp, "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
		cp = g_object_get_data (G_OBJECT (dialog), "guidecolor");
		w = g_object_get_data (cp, "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
	} else {
		static const SPUnit *pt;
		SPNamedView *nv;
		GtkWidget *cp, *w;
		GtkObject *o;
		gdouble val;

		if (!pt) pt = sp_unit_get_by_abbreviation ("pt");

		nv = desktop->namedview;

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (TRUE));
		gtk_widget_set_sensitive (dialog, TRUE);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "showgrid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), nv->showgrid);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "snaptogrid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), nv->snaptogrid);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "grid_units");
		sp_unit_selector_set_unit (SP_UNIT_SELECTOR (o), nv->gridunit);

		val = nv->gridoriginx;
		sp_convert_distance (&val, pt, nv->gridunit);
		o = gtk_object_get_data (GTK_OBJECT (dialog), "gridoriginx");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), val);
		val = nv->gridoriginy;
		sp_convert_distance (&val, pt, nv->gridunit);
		o = gtk_object_get_data (GTK_OBJECT (dialog), "gridoriginy");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), val);
		val = nv->gridspacingx;
		sp_convert_distance (&val, pt, nv->gridunit);
		o = gtk_object_get_data (GTK_OBJECT (dialog), "gridspacingx");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), val);
		val = nv->gridspacingy;
		sp_convert_distance (&val, pt, nv->gridunit);
		o = gtk_object_get_data (GTK_OBJECT (dialog), "gridspacingy");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), val);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "grid_snap_units");
		sp_unit_selector_set_unit (SP_UNIT_SELECTOR (o), nv->gridtoleranceunit);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "gridtolerance");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), nv->gridtolerance);

		cp = gtk_object_get_data (GTK_OBJECT (dialog), "gridcolor");
		sp_color_picker_set_rgba32 (cp, nv->gridcolor);
		w = g_object_get_data (G_OBJECT (cp), "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "showguides");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), nv->showgrid);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "snaptoguides");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), nv->snaptogrid);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "guide_snap_units");
		sp_unit_selector_set_unit (SP_UNIT_SELECTOR (o), nv->guidetoleranceunit);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "guidetolerance");
		gtk_adjustment_set_value (GTK_ADJUSTMENT (o), nv->guidetolerance);

		cp = g_object_get_data (G_OBJECT (dialog), "guidecolor");
		sp_color_picker_set_rgba32 (cp, nv->guidecolor);
		w = g_object_get_data (G_OBJECT (cp), "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);

		cp = g_object_get_data (G_OBJECT (dialog), "guidehicolor");
		sp_color_picker_set_rgba32 (cp, nv->guidehicolor);
		w = g_object_get_data (G_OBJECT (cp), "window");
		if (w) gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);

		o = gtk_object_get_data (GTK_OBJECT (dialog), "showborder");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (o), nv->showborder);

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (FALSE));
	}
}

static void
sp_color_picker_destroy (GtkObject *cp, gpointer data)
{
	GtkObject *w;

	w = g_object_get_data (G_OBJECT (cp), "window");

	if (w) gtk_object_destroy (w);
}

static GtkWidget *
sp_color_picker_new (unsigned char *colorkey, unsigned char *alphakey, unsigned char *title, guint32 rgba)
{
	GtkWidget *b, *cpv;

	b = gtk_button_new ();

	g_object_set_data (G_OBJECT (b), "title", title);

	cpv = sp_color_preview_new (rgba);
#if 0
	sp_color_preview_set_show_solid  (SP_COLOR_PREVIEW (rgba), FALSE);
#endif
	gtk_widget_show (cpv);
	gtk_container_add (GTK_CONTAINER (b), cpv);
	g_object_set_data (G_OBJECT (b), "preview", cpv);

	g_object_set_data (G_OBJECT (b), "colorkey", colorkey);
	g_object_set_data (G_OBJECT (b), "alphakey", alphakey);

	g_signal_connect (G_OBJECT (b), "destroy", G_CALLBACK (sp_color_picker_destroy), NULL);
	g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sp_color_picker_clicked), NULL);

	return b;
}

static void
sp_color_picker_set_rgba32 (GtkWidget *cp, guint32 rgba)
{
	SPColorPreview *cpv;
	SPColorSelector *csel;

	cpv = g_object_get_data (G_OBJECT (cp), "preview");
	sp_color_preview_set_rgba32 (cpv, rgba);

	csel = g_object_get_data (G_OBJECT (cp), "selector");
	if (csel) sp_color_selector_set_rgba32 (csel, rgba);

	g_object_set_data (G_OBJECT (cp), "color", GUINT_TO_POINTER (rgba));
}

static void
sp_color_picker_window_destroy (GtkObject *object, GObject *cp)
{
	g_object_set_data (G_OBJECT (cp), "window", NULL);
	g_object_set_data (G_OBJECT (cp), "selector", NULL);
}

static void
sp_color_picker_color_mod (SPColorSelector *csel, GObject *cp)
{
	guint32 rgba;
	SPColorPreview *cpv;
	SPRepr *repr;
	guchar c[32];
	unsigned char *colorkey, *alphakey;

	if (g_object_get_data (G_OBJECT (cp), "update")) return;

	rgba = sp_color_selector_get_rgba32 (csel);

	cpv = g_object_get_data (G_OBJECT (cp), "preview");
	colorkey = g_object_get_data (G_OBJECT (cp), "colorkey");
	alphakey = g_object_get_data (G_OBJECT (cp), "alphakey");
	sp_color_preview_set_rgba32 (cpv, rgba);

	if (!SP_ACTIVE_DESKTOP) return;

	repr = SP_OBJECT_REPR (SP_ACTIVE_DESKTOP->namedview);

	sp_svg_write_color (c, 32, rgba);
	sp_repr_set_attr (repr, colorkey, c);
	sp_repr_set_double (repr, alphakey, (rgba & 0xff) / 255.0);

}

static void
sp_color_picker_clicked (GObject *cp, void *data)
{
	GtkWidget *w;

	w = g_object_get_data (cp, "window");
	if (!w) {
		GtkWidget *vb, *csel, *hs, *hb, *b;
		w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (w), g_object_get_data (cp, "title"));
		gtk_container_set_border_width (GTK_CONTAINER (w), 4);
		g_object_set_data (cp, "window", w);
		g_signal_connect (G_OBJECT (w), "destroy", G_CALLBACK (sp_color_picker_window_destroy), cp);

		vb = gtk_vbox_new (FALSE, 4);
		gtk_container_add (GTK_CONTAINER (w), vb);

		csel = sp_color_selector_new ();
		gtk_box_pack_start (GTK_BOX (vb), csel, TRUE, TRUE, 0);
		sp_color_selector_set_rgba32 (SP_COLOR_SELECTOR (csel), GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (cp), "color")));
		g_signal_connect (G_OBJECT (csel), "dragged", G_CALLBACK (sp_color_picker_color_mod), cp);
		g_signal_connect (G_OBJECT (csel), "changed", G_CALLBACK (sp_color_picker_color_mod), cp);
		g_object_set_data (cp, "selector", csel);

		hs = gtk_hseparator_new ();
		gtk_box_pack_start (GTK_BOX (vb), hs, FALSE, FALSE, 0);

		hb = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

		b = gtk_button_new_with_label (_("Close"));
		gtk_box_pack_end (GTK_BOX (hb), b, FALSE, FALSE, 0);

		gtk_widget_show_all (w);
	} else {
		gtk_window_present (GTK_WINDOW (w));
	}
}

