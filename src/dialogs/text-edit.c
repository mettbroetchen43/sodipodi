#define __SP_TEXT_EDIT_C__

/*
 * Text editing dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktext.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeprint/gnome-font-dialog.h>
#include <gal/widgets/e-unicode.h>

#include "../forward.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../style.h"
#include "../sp-text.h"

#include "text-edit.h"

static void sp_text_edit_dialog_modify_selection (Sodipodi *sodipodi, SPSelection *sel, guint flags, GtkWidget *dlg);
static void sp_text_edit_dialog_change_selection (Sodipodi *sodipodi, SPSelection *sel, GtkWidget *dlg);

static void sp_text_edit_dialog_apply (GtkButton *button, GtkWidget *dlg);
static void sp_text_edit_dialog_close (GtkButton *button, GtkWidget *dlg);

static void sp_text_edit_dialog_read_selection (GtkWidget *dlg);

static void sp_text_edit_dialog_text_changed (GtkText *txt, GtkWidget *dlg);
static void sp_text_edit_dialog_font_changed (GnomeFontSelection *fontsel, GnomeFont *font, GtkWidget *dlg);

static GtkWidget *dlg = NULL;

static void
sp_text_edit_dialog_destroy (GtkObject *object, gpointer data)
{
	gtk_signal_disconnect_by_data (GTK_OBJECT (SODIPODI), dlg);
	dlg = NULL;
}

static gint
sp_text_edit_dialog_delete (GtkWidget *dlg, GdkEvent *event, gpointer data)
{
	gtk_object_destroy (GTK_OBJECT (dlg));

	return TRUE;
}

void
sp_text_edit_dialog (void)
{
	if (!dlg) {
		GtkWidget *mainvb, *nb, *vb, *txt, *fontsel, *preview, *l, *hs, *hb, *b;

		dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dlg), _("Text properties"));
		gtk_signal_connect (GTK_OBJECT (dlg), "destroy", GTK_SIGNAL_FUNC (sp_text_edit_dialog_destroy), dlg);
		gtk_signal_connect (GTK_OBJECT (dlg), "delete_event", GTK_SIGNAL_FUNC (sp_text_edit_dialog_delete), dlg);

		mainvb = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (mainvb);
		gtk_container_add (GTK_CONTAINER (dlg), mainvb);

		nb = gtk_notebook_new ();
		gtk_widget_show (nb);
		gtk_container_set_border_width (GTK_CONTAINER (nb), 4);
		gtk_box_pack_start (GTK_BOX (mainvb), nb, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "notebook", nb);

		vb = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vb);

		txt = gtk_text_new (NULL, NULL);
		gtk_widget_show (txt);
		gtk_text_set_editable (GTK_TEXT (txt), TRUE);
		gtk_signal_connect (GTK_OBJECT (txt), "changed", GTK_SIGNAL_FUNC (sp_text_edit_dialog_text_changed), dlg);
		gtk_box_pack_start (GTK_BOX (vb), txt, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "text", txt);

		fontsel = gnome_font_selection_new ();
		gtk_widget_show (fontsel);
		gtk_signal_connect (GTK_OBJECT (fontsel), "font_set", GTK_SIGNAL_FUNC (sp_text_edit_dialog_font_changed), dlg);
		gtk_box_pack_start (GTK_BOX (vb), fontsel, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "fontsel", fontsel);

		preview = gnome_font_preview_new ();
		gtk_widget_show (preview);
		gtk_box_pack_start (GTK_BOX (vb), preview, TRUE, TRUE, 4);
		gtk_object_set_data (GTK_OBJECT (dlg), "preview", preview);

		l = gtk_label_new (_("Text and font"));
		gtk_widget_show (l);
		gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

		/* Buttons */
		hs = gtk_hseparator_new ();
		gtk_widget_show (hs);
		gtk_box_pack_start (GTK_BOX (mainvb), hs, FALSE, FALSE, 4);

		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_box_pack_start (GTK_BOX (mainvb), hb, FALSE, FALSE, 0);

		b = gnome_stock_button (GNOME_STOCK_BUTTON_CLOSE);
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_text_edit_dialog_close), dlg);
		gtk_box_pack_end (GTK_BOX (hb), b, FALSE, FALSE, 0);

		b = gnome_stock_button (GNOME_STOCK_BUTTON_APPLY);
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_text_edit_dialog_apply), dlg);
		gtk_box_pack_end (GTK_BOX (hb), b, FALSE, FALSE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "apply", b);
	}

	gtk_signal_connect (GTK_OBJECT (SODIPODI), "modify_selection", GTK_SIGNAL_FUNC (sp_text_edit_dialog_modify_selection), dlg);
	gtk_signal_connect (GTK_OBJECT (SODIPODI), "change_selection", GTK_SIGNAL_FUNC (sp_text_edit_dialog_change_selection), dlg);

	sp_text_edit_dialog_read_selection (dlg);

	gtk_widget_show (dlg);
}

static void
sp_text_edit_dialog_modify_selection (Sodipodi *sodipodi, SPSelection *sel, guint flags, GtkWidget *dlg)
{
	sp_text_edit_dialog_read_selection (dlg);
}

static void
sp_text_edit_dialog_change_selection (Sodipodi *sodipodi, SPSelection *sel, GtkWidget *dlg)
{
	sp_text_edit_dialog_read_selection (dlg);
}

static void
sp_text_edit_dialog_apply (GtkButton *button, GtkWidget *dlg)
{
	GtkWidget *apply;
	SPItem *item;

	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	gtk_widget_set_sensitive (apply, FALSE);

	if (!SP_ACTIVE_DESKTOP) return;
	item = sp_selection_item (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));

	if (item && SP_IS_TEXT (item)) {
		GtkWidget *textw, *fontsel, *preview;
		SPCSSAttr *css;
		GnomeFont *font;
		gint weight;
		const guchar *val;
		guchar *str;
		guchar c[64];

		gtk_object_set_data (GTK_OBJECT (dlg), "blocked", GINT_TO_POINTER (TRUE));

		textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");
		fontsel = gtk_object_get_data (GTK_OBJECT (dlg), "fontsel");
		preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");

		str = e_utf8_gtk_editable_get_text (GTK_EDITABLE (textw));
		sp_text_set_repr_text_multiline (SP_TEXT (item), str);
		g_free (str);

		css = sp_repr_css_attr_new ();

		font = gnome_font_selection_get_font (GNOME_FONT_SELECTION (fontsel));

		val = gnome_font_get_family_name (font);
		sp_repr_css_set_property (css, "font-family", val);

		weight = gnome_font_get_weight_code (font);
		if (weight < GNOME_FONT_SEMI) {
			val = "normal";
		} else {
			val = "bold";
		}
		sp_repr_css_set_property (css, "font-weight", val);

		if (gnome_font_is_italic (font)) {
			val = "italic";
		} else {
			val = "normal";
		}
		sp_repr_css_set_property (css, "font-style", val);

		snprintf (c, 64, "%g", gnome_font_get_size (font));
		sp_repr_css_set_property (css, "font-size", c);

		gnome_font_unref (font);

		sp_repr_css_change (SP_OBJECT_REPR (item), css, "style");
		sp_repr_css_attr_unref (css);

		sp_document_done (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP));
		sp_document_ensure_up_to_date (SP_OBJECT_DOCUMENT (item));

		gtk_object_set_data (GTK_OBJECT (dlg), "blocked", NULL);
	}
}

static void
sp_text_edit_dialog_close (GtkButton *button, GtkWidget *dlg)
{
	gtk_object_destroy (GTK_OBJECT (dlg));
}

static void
sp_text_edit_dialog_read_selection (GtkWidget *dlg)
{
	GtkWidget *notebook, *apply;
	SPItem *item;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	notebook = gtk_object_get_data (GTK_OBJECT (dlg), "notebook");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");

	if (!SP_ACTIVE_DESKTOP) return;
	item = sp_selection_item (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	if (item && SP_IS_TEXT (item)) {
		GtkWidget *textw, *fontsel, *preview;
		SPStyle *style;
		guchar *str;
		GnomeFont *font;

		gtk_object_set_data (GTK_OBJECT (dlg), "blocked", GINT_TO_POINTER (TRUE));

		style = SP_OBJECT_STYLE (item);

		textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");
		fontsel = gtk_object_get_data (GTK_OBJECT (dlg), "fontsel");
		preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");

		str = sp_text_get_string_multiline (SP_TEXT (item));
		if (str && *str) {
			e_utf8_gtk_editable_set_text (GTK_EDITABLE (textw), str);
			gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), str);
			g_free (str);
		} else {
			gtk_editable_delete_text (GTK_EDITABLE (textw), 0, -1);
			gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), NULL);
		}

		font = gnome_font_new_closest (style->text->font_family.value,
					       sp_text_font_weight_to_gp (style->font_weight.computed),
					       sp_text_font_italic_to_gp (style->font_style.computed),
					       style->font_size.computed);
		if (font) {
			gnome_font_selection_set_font (GNOME_FONT_SELECTION (fontsel), font);
			gnome_font_preview_set_font (GNOME_FONT_PREVIEW (preview), font);
			gnome_font_unref (font);
		}
		gtk_widget_set_sensitive (notebook, TRUE);

		gtk_object_set_data (GTK_OBJECT (dlg), "blocked", NULL);
	} else {
		gtk_widget_set_sensitive (notebook, FALSE);
	}

	gtk_widget_set_sensitive (apply, FALSE);
}

static void
sp_text_edit_dialog_text_changed (GtkText *txt, GtkWidget *dlg)
{
	GtkWidget *textw, *apply, *preview;
	gchar *str;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");

	str = e_utf8_gtk_editable_get_text (GTK_EDITABLE (textw));
	if (str && *str) {
		gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), str);
	} else {
		gnome_font_preview_set_phrase (GNOME_FONT_PREVIEW (preview), NULL);
	}
	if (str) g_free (str);

	gtk_widget_set_sensitive (apply, TRUE);
}

static void
sp_text_edit_dialog_font_changed (GnomeFontSelection *fontsel, GnomeFont *font, GtkWidget *dlg)
{
	GtkWidget *preview;
	GtkWidget *apply;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");

	gnome_font_preview_set_font (GNOME_FONT_PREVIEW (preview), font);

	gtk_widget_set_sensitive (apply, TRUE);
}

