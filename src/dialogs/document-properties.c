#define __SP_DOCUMENT_PROPERTIES_C__

/*
 * Desktop configuration dialog
 *
 * This file is part of Sodipodi http://www.sodipodi.com
 * Licensed under GNU General Public License, see file
 * COPYING for details
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) Lauris Kaplinski 2000-2002
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>

#include <libarikkei/arikkei-strlib.h>

#include <gtk/gtk.h>

#include "macros.h"

#include "helper/sp-intl.h"
#include "helper/window.h"
#include "helper/unit-menu.h"

#include "../sodipodi.h"
#include "../document.h"
#include "../desktop.h"
#include "../desktop-handles.h"
#include "../sp-namedview.h"

#include "document-properties.h"

/*
 * Very-very basic document properties dialog
 *
 */ 

static GtkWidget *dialog = NULL;
static const struct _papers {
	gchar  *name;
	gdouble width;
	gdouble height;
} papers[] = {
  { "A3", 29.7 * (72.0 / 2.54), 42.0 * (72.0 / 2.54) },
  { "A4", 21.0 * (72.0 / 2.54), 29.7 * (72.0 / 2.54) },
  { "A5", 14.85 * (72.0 / 2.54), 21.0 * (72.0 / 2.54) },
  { "B4", 25.7528 * (72.0 / 2.54), 36.4772 * (72.0 / 2.54) },
  { "B5", 17.6389 * (72.0 / 2.54), 25.0472 * (72.0 / 2.54) },
  { "B5-Japan", 18.2386 * (72.0 / 2.54), 25.7528 * (72.0 / 2.54) },
  { "Letter", 21.59 * (72.0 / 2.54), 27.94 * (72.0 / 2.54) },
  { "Legal", 21.59 * (72.0 / 2.54), 35.56 * (72.0 / 2.54) },
  { "Ledger", 27.9 * (72.0 / 2.54), 43.2 * (72.0 / 2.54) },
  { "Half-Letter", 21.59 * (72.0 / 2.54), 14.0 * (72.0 / 2.54) },
  { "Executive", 18.45 * (72.0 / 2.54), 26.74 * (72.0 / 2.54) },
  { "Tabloid", 28.01 * (72.0 / 2.54), 43.2858 * (72.0 / 2.54) },
  { "Monarch", 9.8778 * (72.0 / 2.54), 19.12 * (72.0 / 2.54) },
  { "SuperB", 29.74 * (72.0 / 2.54), 43.2858 * (72.0 / 2.54) },
  { "Envelope-Commercial", 10.5128 * (72.0 / 2.54), 24.2 * (72.0 / 2.54) },
  { "Envelope-Monarch", 9.8778 * (72.0 / 2.54), 19.12 * (72.0 / 2.54) },
  { "Envelope-DL", 11.0 * (72.0 / 2.54), 22.0 * (72.0 / 2.54) },
  { "Envelope-C5", 16.2278 * (72.0 / 2.54) },
  { "EuroPostcard", 10.5128 * (72.0 / 2.54), 14.8167 * (72.0 / 2.54) },
  { "A0", 84.1 * (72.0 / 2.54), 118.9 * (72.0 / 2.54) },
  { "A1", 59.4 * (72.0 / 2.54), 84.1 * (72.0 / 2.54) },
  { "A2", 42.0 * (72.0 / 2.54), 59.4 * (72.0 / 2.54) },
  { NULL, 0.0, 0.0 }
};

static GtkWidget *sp_doc_dialog_new (void);
static void sp_doc_dialog_activate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_doc_dialog_desactivate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_doc_dialog_update (GtkWidget *dialog, SPDocument *doc);

static void
sp_doc_dialog_destroy (GObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (SODIPODI, object);

	dialog = NULL;
}

void
sp_document_dialog (void)
{
	if (!dialog) {
		dialog = sp_doc_dialog_new ();
		g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (sp_doc_dialog_destroy), NULL);
	}

	gtk_window_present ((GtkWindow *) dialog);
}

static void
sp_doc_dialog_paper_selected (GtkWidget *widget, const struct _papers *paper)
{
	GtkWidget *ww, *hw, *rb;

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	ww = gtk_object_get_data (GTK_OBJECT (dialog), "widthsb");
	hw = gtk_object_get_data (GTK_OBJECT (dialog), "heightsb");
	rb = gtk_object_get_data (GTK_OBJECT (dialog), "portraitrb");

	if (paper) {
		SPUnitSelector *us;
		static const SPUnit *pt = NULL;
		const SPUnit *unit;
		GtkAdjustment *a;
		gdouble w, h;
		gboolean portrait = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb));

		gtk_widget_set_sensitive (ww, FALSE);
		gtk_widget_set_sensitive (hw, FALSE);
		if (!pt) pt = sp_unit_get_by_abbreviation ("pt");
		us = gtk_object_get_data (GTK_OBJECT (dialog), "units");
		unit = sp_unit_selector_get_unit (us);
		w = portrait ? paper->width : paper->height;
		a = gtk_object_get_data (GTK_OBJECT (dialog), "width");
		sp_convert_distance (&w, pt, unit);
		gtk_adjustment_set_value (a, w);
		h = portrait ? paper->height : paper->width;
		a = gtk_object_get_data (GTK_OBJECT (dialog), "height");
		sp_convert_distance (&h, pt, unit);
		gtk_adjustment_set_value (a, h);
	} else {
		gtk_widget_set_sensitive (ww, TRUE);
		gtk_widget_set_sensitive (hw, TRUE);
	}
}

static void
sp_doc_dialog_portrait_toggled (GtkWidget *widget, const struct _papers *paper)
{
	GtkWidget *rb;
	gboolean portrait;
	GtkAdjustment *aw, *ah;
	gdouble w, h;

	if (!dialog || gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	aw = gtk_object_get_data (GTK_OBJECT (dialog), "width");
	ah = gtk_object_get_data (GTK_OBJECT (dialog), "height");
	rb = gtk_object_get_data (GTK_OBJECT (dialog), "portraitrb");

	w = gtk_adjustment_get_value (aw);
	h = gtk_adjustment_get_value (ah);
	portrait = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb));

	/* maybe we need to swap values */
	if (!!portrait && h > w) {
		gtk_adjustment_set_value (aw, w);
		gtk_adjustment_set_value (ah, h);
	} else {
		gtk_adjustment_set_value (aw, h);
		gtk_adjustment_set_value (ah, w);
	}
}

static void
sp_doc_dialog_whatever_changed (GtkAdjustment *adjustment, GtkWidget *dialog)
{
	SPDesktop *dt;
	SPDocument *doc;
	SPRepr *repr;
	SPUnitSelector *us;
	const guchar *key;
	guchar c0[32], c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	doc = SP_DT_DOCUMENT (dt);

	repr = sp_document_repr_root (doc);
	key = gtk_object_get_data (GTK_OBJECT (adjustment), "key");
	us = gtk_object_get_data (GTK_OBJECT (adjustment), "unit_selector");

	arikkei_dtoa_simple (c0, 32, adjustment->value, 6, 0, FALSE);
	g_snprintf (c, 32, "%s%s", c0, sp_unit_selector_get_unit (us)->abbr);
	sp_repr_set_attr (repr, key, c);

	sp_document_done (doc);
}

static GtkWidget *
sp_doc_dialog_new (void)
{
	GtkWidget *dialog, *nb, *vb, *hb, *l, *om, *m, *i, *f, *t, *us, *sb, *rb;
	GtkObject *a;
	const struct _papers *paper;

	dialog = sp_window_new (_("Document settings"), FALSE, TRUE);

	nb = gtk_notebook_new ();
	gtk_widget_show (nb);
	gtk_container_add (GTK_CONTAINER (dialog), nb);

	/* Page settings */

	/* Notebook tab */
	l = gtk_label_new (_("Page"));
	gtk_widget_show (l);
	vb = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vb);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

	/* Paper menu */
	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	l = gtk_label_new (_("Paper size:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (hb), l, FALSE, FALSE, 0);
	om = gtk_option_menu_new ();
	gtk_widget_show (om);
	gtk_box_pack_start (GTK_BOX (hb), om, TRUE, TRUE, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "papers", om);

	m = gtk_menu_new ();
	gtk_widget_show (m);
	
	for (paper = papers; paper->name != NULL; ++paper) {
		i = gtk_menu_item_new_with_label (paper->name);
		gtk_widget_show (i);
		g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (sp_doc_dialog_paper_selected), (gpointer) paper);
		gtk_menu_append (GTK_MENU (m), i);
	}

	/* Portrait/Landscape */
	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	rb = gtk_radio_button_new_with_label (NULL, _("Landscape"));
	gtk_widget_show (rb);
	gtk_box_pack_end (GTK_BOX (hb), rb, FALSE, FALSE, 0);

	rb = gtk_radio_button_new_with_label (
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (rb)), _("Portrait"));
	gtk_widget_show (rb);
	gtk_box_pack_end (GTK_BOX (hb), rb, FALSE, FALSE, 0);

	gtk_object_set_data (GTK_OBJECT (dialog), "portraitrb", rb);
	g_signal_connect (G_OBJECT (rb), "toggled", G_CALLBACK (sp_doc_dialog_portrait_toggled), NULL);

	i = gtk_menu_item_new_with_label (_("Custom"));
	gtk_widget_show (i);
	g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (sp_doc_dialog_paper_selected), NULL);
	gtk_menu_prepend (GTK_MENU (m), i);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);

	/* Custom paper frame */
	f = gtk_frame_new (_("Custom paper"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, FALSE, FALSE, 0);

	t = gtk_table_new (9, 2, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_table_set_col_spacings (GTK_TABLE (t), 4);
	gtk_container_add (GTK_CONTAINER (f), t);

	l = gtk_label_new (_("Units:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	gtk_widget_show (us);
	gtk_table_attach (GTK_TABLE (t), us, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "units", us);

	l = gtk_label_new (_("Width:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, 1e-6, 1e6, 1.0, 10.0, 0.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "width");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "width", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "widthsb", sb);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_doc_dialog_whatever_changed), dialog);

	l = gtk_label_new (_("Height:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, 1e-6, 1e6, 1.0, 10.0, 0.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "height");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "height", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "heightsb", sb);
	g_signal_connect (G_OBJECT (a), "value_changed", G_CALLBACK (sp_doc_dialog_whatever_changed), dialog);

	/* fixme: We should listen namedview changes here as well */
	g_signal_connect (G_OBJECT (SODIPODI), "activate_desktop", G_CALLBACK (sp_doc_dialog_activate_desktop), dialog);
	g_signal_connect (G_OBJECT (SODIPODI), "desactivate_desktop", G_CALLBACK (sp_doc_dialog_desactivate_desktop), dialog);
	sp_doc_dialog_update (dialog, SP_ACTIVE_DOCUMENT);

	return dialog;
}

static void
sp_doc_dialog_activate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog)
{
	sp_doc_dialog_update (dialog, (desktop) ? SP_DT_DOCUMENT (desktop) : NULL);
}

static void
sp_doc_dialog_desactivate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog)
{
	sp_doc_dialog_update (dialog, NULL);
}

static void
sp_doc_dialog_update (GtkWidget *dialog, SPDocument *doc)
{
	if (!doc) {
		gtk_widget_set_sensitive (dialog, FALSE);
	} else {
		gdouble docw, doch;
		const struct _papers *paper;
		gint pos;
		GtkWidget *ww, *hw, *om, *rb;
		SPUnitSelector *us;
		static const SPUnit *pt = NULL;
		const SPUnit *unit;
		GtkAdjustment *a;

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (TRUE));
		gtk_widget_set_sensitive (dialog, TRUE);

		docw = sp_document_width (doc);
		doch = sp_document_height (doc);

		pos = 1;

		for (paper = papers; paper->name != NULL; ++paper) {
			gdouble pw, ph;
			pw = paper->width;
			ph = paper->height;
			if ((fabs (docw - pw) < 1.0) && (fabs (doch - ph) < 1.0)) break;
			pos += 1;
		}

		ww = gtk_object_get_data (GTK_OBJECT (dialog), "widthsb");
		hw = gtk_object_get_data (GTK_OBJECT (dialog), "heightsb");
		om = gtk_object_get_data (GTK_OBJECT (dialog), "papers");
		rb = gtk_object_get_data (GTK_OBJECT (dialog), "portraitrb");

		if (paper->name) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (om), pos);
			gtk_widget_set_sensitive (ww, FALSE);
			gtk_widget_set_sensitive (hw, FALSE);
		} else {
			gtk_option_menu_set_history (GTK_OPTION_MENU (om), 0);
			gtk_widget_set_sensitive (ww, TRUE);
			gtk_widget_set_sensitive (hw, TRUE);
		}

		if (!pt) pt = sp_unit_get_by_abbreviation ("pt");
		us = gtk_object_get_data (GTK_OBJECT (dialog), "units");
		unit = sp_unit_selector_get_unit (us);
		a = gtk_object_get_data (GTK_OBJECT (dialog), "width");
		sp_convert_distance (&docw, pt, unit);
		gtk_adjustment_set_value (a, docw);
		a = gtk_object_get_data (GTK_OBJECT (dialog), "height");
		sp_convert_distance (&doch, pt, unit);
		gtk_adjustment_set_value (a, doch);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb), doch > docw);

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (FALSE));
	}
}
