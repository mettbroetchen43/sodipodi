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

#include <config.h>

#include <math.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-paper.h>

#include "../helper/unit-menu.h"
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
static GList *papers = NULL;

static GtkWidget *sp_doc_dialog_new (void);
static void sp_doc_dialog_activate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_doc_dialog_desactivate_desktop (Sodipodi *sodipodi, SPDesktop *desktop, GtkWidget *dialog);
static void sp_doc_dialog_update (GtkWidget *dialog, SPDocument *doc);

static void
sp_doc_dialog_destroy (GtkObject *object, gpointer data)
{
	dialog = NULL;
}

void
sp_document_dialog (void)
{
	if (!dialog) {
		dialog = sp_doc_dialog_new ();
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (sp_doc_dialog_destroy), NULL);
	}

	gtk_widget_show (dialog);
}

static void
sp_doc_dialog_paper_selected (GtkWidget *widget, const GnomePaper *paper)
{
	GtkWidget *ww, *hw;

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	ww = gtk_object_get_data (GTK_OBJECT (dialog), "widthsb");
	hw = gtk_object_get_data (GTK_OBJECT (dialog), "heightsb");

	if (paper) {
		SPUnitSelector *us;
		static const SPUnit *pt = NULL;
		const SPUnit *unit;
		GtkAdjustment *a;
		gdouble w, h;
		gtk_widget_set_sensitive (ww, FALSE);
		gtk_widget_set_sensitive (hw, FALSE);
		if (!pt) pt = sp_unit_get_by_abbreviation ("pt");
		us = gtk_object_get_data (GTK_OBJECT (dialog), "units");
		unit = sp_unit_selector_get_unit (us);
		w = gnome_paper_pswidth (paper);
		a = gtk_object_get_data (GTK_OBJECT (dialog), "width");
		sp_convert_distance (&w, pt, unit);
		gtk_adjustment_set_value (a, w);
		h = gnome_paper_psheight (paper);
		a = gtk_object_get_data (GTK_OBJECT (dialog), "height");
		sp_convert_distance (&h, pt, unit);
		gtk_adjustment_set_value (a, h);
	} else {
		gtk_widget_set_sensitive (ww, TRUE);
		gtk_widget_set_sensitive (hw, TRUE);
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
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (dialog), "update")) return;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	doc = SP_DT_DOCUMENT (dt);

	repr = sp_document_repr_root (doc);
	key = gtk_object_get_data (GTK_OBJECT (adjustment), "key");
	us = gtk_object_get_data (GTK_OBJECT (adjustment), "unit_selector");

	g_snprintf (c, 32, "%g%s", adjustment->value, sp_unit_selector_get_unit (us)->abbr);

	sp_repr_set_attr (repr, key, c);

	sp_document_done (doc);
}

static GtkWidget *
sp_doc_dialog_new (void)
{
	GtkWidget *dialog, *nb, *vb, *hb, *l, *om, *m, *i, *f, *t, *us, *sb;
	GtkObject *a;
	GList *ll;

	dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Document settings"));

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
	if (!papers) papers = gnome_paper_name_list ();
	for (ll = papers; ll != NULL; ll = ll->next) {
		const GnomePaper * paper;
		paper = gnome_paper_with_name (ll->data);
		i = gtk_menu_item_new_with_label (gnome_paper_name (paper));
		gtk_widget_show (i);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_doc_dialog_paper_selected), (gpointer) paper);
		gtk_menu_append (GTK_MENU (m), i);
	}
	i = gtk_menu_item_new_with_label (_("Custom"));
	gtk_widget_show (i);
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (sp_doc_dialog_paper_selected), NULL);
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
	a = gtk_adjustment_new (0.0, 1e-6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "width");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "width", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "widthsb", sb);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_doc_dialog_whatever_changed), dialog);

	l = gtk_label_new (_("Height:"));
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_widget_show (l);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	a = gtk_adjustment_new (0.0, 1e-6, 1e6, 1.0, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (a), "key", "height");
	gtk_object_set_data (GTK_OBJECT (a), "unit_selector", us);
	gtk_object_set_data (GTK_OBJECT (dialog), "height", a);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 1.0, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (dialog), "heightsb", sb);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_doc_dialog_whatever_changed), dialog);

	/* fixme: We should listen namedview changes here as well */
	gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "activate_desktop",
					GTK_SIGNAL_FUNC (sp_doc_dialog_activate_desktop), dialog, GTK_OBJECT (dialog));
	gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "desactivate_desktop",
					GTK_SIGNAL_FUNC (sp_doc_dialog_desactivate_desktop), dialog, GTK_OBJECT (dialog));
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
		GList *l;
		gint pos;
		GtkWidget *ww, *hw, *om;
		SPUnitSelector *us;
		static const SPUnit *pt = NULL;
		const SPUnit *unit;
		GtkAdjustment *a;

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (TRUE));
		gtk_widget_set_sensitive (dialog, TRUE);

		docw = sp_document_width (doc);
		doch = sp_document_height (doc);

		pos = 1;

		for (l = papers; l != NULL; l = l->next) {
			gdouble pw, ph;
			const GnomePaper *paper;
			paper = gnome_paper_with_name ((gchar *) l->data);
			pw = gnome_paper_pswidth (paper);
			ph = gnome_paper_psheight (paper);
			if ((fabs (docw - pw) < 1.0) && (fabs (doch - ph) < 1.0)) break;
			pos += 1;
		}

		ww = gtk_object_get_data (GTK_OBJECT (dialog), "widthsb");
		hw = gtk_object_get_data (GTK_OBJECT (dialog), "heightsb");
		om = gtk_object_get_data (GTK_OBJECT (dialog), "papers");

		if (l != NULL) {
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

		gtk_object_set_data (GTK_OBJECT (dialog), "update", GINT_TO_POINTER (FALSE));
	}
}

#if 0
#define MM2PT(v) ((v) * 72.0 / 25.4)
#define PT2MM(v) ((v) * 25.4 / 72.0)

static void sp_document_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);
static gint sp_document_dialog_delete (GtkWidget *widget, GdkEvent *event);

static void paper_selected (GtkWidget * widget, gpointer data);

void
sp_document_dialog (void)
{
	if (dialog == NULL) {
		GtkWidget * papermenu;
		GtkWidget * menu;
		GtkWidget * item;
		GList * l, * pl;

		g_assert (xml == NULL);
		xml = glade_xml_new (SODIPODI_GLADEDIR "/document.glade", "document_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "document_dialog");
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
				    GTK_SIGNAL_FUNC (sp_document_dialog_delete), NULL);
		
		papermenu = glade_xml_get_widget (xml, "paper_size");

		menu = gtk_menu_new ();
		pl = gnome_paper_name_list ();
		for (l = pl; l != NULL; l = l->next) {
			const GnomePaper * paper;
			paper = gnome_paper_with_name (l->data);
			item = gtk_menu_item_new_with_label (gnome_paper_name (paper));
			gtk_widget_show (item);
			gtk_signal_connect (GTK_OBJECT (item), "activate",
					    GTK_SIGNAL_FUNC (paper_selected), (gpointer) paper);
			gtk_menu_append (GTK_MENU (menu), item);
		}
		item = gtk_menu_item_new_with_label (_("Custom"));
		gtk_widget_show (item);
		gtk_widget_show (menu);
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    GTK_SIGNAL_FUNC (paper_selected), NULL);
		gtk_menu_prepend (GTK_MENU (menu), item);

		gtk_option_menu_set_menu (GTK_OPTION_MENU (papermenu), menu);

		gtk_signal_connect_while_alive (GTK_OBJECT (sodipodi), "activate_desktop",
						GTK_SIGNAL_FUNC (sp_document_dialog_setup), NULL,
						GTK_OBJECT (dialog));
	} else {
		if (!GTK_WIDGET_VISIBLE (dialog))
			gtk_widget_show (dialog);
	}

	sp_document_dialog_setup (SODIPODI, SP_ACTIVE_DESKTOP, NULL);
}

/*
 * Fill entries etc. with default values
 */

static void
sp_document_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data)
{
	SPDocument * doc;
	GtkWidget * w;

	g_assert (sodipodi != NULL);
	g_assert (SP_IS_SODIPODI (sodipodi));
	g_assert (dialog != NULL);

	if (!desktop) return;

	doc = SP_DT_DOCUMENT (desktop);

	w = glade_xml_get_widget (xml, "paper_width");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (sp_document_width (doc)));

	w = glade_xml_get_widget (xml, "paper_height");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (sp_document_height (doc)));

	w = glade_xml_get_widget (xml, "paper_size");
	gtk_option_menu_set_history (GTK_OPTION_MENU (w), 0);
}

void
sp_document_dialog_close (GtkWidget * widget)
{
	g_assert (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog))
		gtk_widget_hide (dialog);
}


static gint
sp_document_dialog_delete (GtkWidget *widget, GdkEvent *event)
{
	sp_document_dialog_close (widget);

	return TRUE;
}

void
sp_document_dialog_apply (GtkWidget * widget)
{
	SPDocument * doc;
	SPRepr * repr;
	GtkWidget * w;
	gdouble t;

	g_assert (dialog != NULL);

	if (!SP_ACTIVE_DESKTOP) return;
	doc = SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP);

	repr = sp_document_repr_root (doc);

	w = glade_xml_get_widget (xml, "paper_width");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "width", MM2PT (t));

	w = glade_xml_get_widget (xml, "paper_height");
	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_repr_set_double_attribute (repr, "height", MM2PT (t));

	sp_document_done (doc);
}

static void
paper_selected (GtkWidget * widget, gpointer data)
{
	GtkWidget * w;

	if (data) {
		GnomePaper * paper;
		paper = (GnomePaper *) data;
		w = glade_xml_get_widget (xml, "custom_frame");
		gtk_widget_set_sensitive (w, FALSE);
		w = glade_xml_get_widget (xml, "paper_width");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (gnome_paper_pswidth (paper)));
		w = glade_xml_get_widget (xml, "paper_height");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), PT2MM (gnome_paper_psheight (paper)));
	} else {
		w = glade_xml_get_widget (xml, "custom_frame");
		gtk_widget_set_sensitive (w, TRUE);
	}
}

#endif
