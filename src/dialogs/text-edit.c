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

#include <string.h>
#include <libnrtype/nr-type-directory.h>

#include <glib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <gtk/gtktext.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkimage.h>

#include "../helper/sp-intl.h"
#include "../widgets/font-selector.h"
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

static void sp_text_edit_dialog_set_default (GtkButton *button, GtkWidget *dlg);
static void sp_text_edit_dialog_apply (GtkButton *button, GtkWidget *dlg);
static void sp_text_edit_dialog_close (GtkButton *button, GtkWidget *dlg);

static void sp_text_edit_dialog_read_selection (GtkWidget *dlg, gboolean style, gboolean content);

static void sp_text_edit_dialog_text_changed (GtkText *txt, GtkWidget *dlg);
static void sp_text_edit_dialog_font_changed (SPFontSelector *fontsel, NRFont *font, GtkWidget *dlg);
static void sp_text_edit_dialog_any_toggled (GtkToggleButton *tb, GtkWidget *dlg);

static SPText *sp_ted_get_selected_text_item (void);

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
		GtkWidget *mainvb, *nb, *vb, *hb, *txt, *fontsel, *preview, *f, *tbl, *l, *px, *b, *hs;

		dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dlg), _("Text properties"));
		gtk_window_set_policy (GTK_WINDOW (dlg), TRUE, TRUE, FALSE);
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

		/* Vbox inside notebook */
		vb = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vb);

		txt = gtk_text_new (NULL, NULL);
		gtk_widget_show (txt);
		gtk_text_set_editable (GTK_TEXT (txt), TRUE);
		gtk_signal_connect (GTK_OBJECT (txt), "changed", GTK_SIGNAL_FUNC (sp_text_edit_dialog_text_changed), dlg);
		gtk_box_pack_start (GTK_BOX (vb), txt, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "text", txt);

		/* HBox containing font selection and layout */
		hb = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (hb);
		gtk_box_pack_start (GTK_BOX (vb), hb, TRUE, TRUE, 0);

		fontsel = sp_font_selector_new ();
		gtk_widget_show (fontsel);
		gtk_signal_connect (GTK_OBJECT (fontsel), "font_set", GTK_SIGNAL_FUNC (sp_text_edit_dialog_font_changed), dlg);
		gtk_box_pack_start (GTK_BOX (hb), fontsel, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "fontsel", fontsel);

		/* Layout */
		f = gtk_frame_new (_("Layout"));
		gtk_widget_show (f);
		gtk_box_pack_start (GTK_BOX (hb), f, FALSE, FALSE, 4);

		tbl = gtk_table_new (2, 4, FALSE);
		gtk_widget_show (tbl);
		gtk_table_set_row_spacings (GTK_TABLE (tbl), 4);
		gtk_table_set_col_spacings (GTK_TABLE (tbl), 4);
		gtk_container_add (GTK_CONTAINER (f), tbl);

		l = gtk_label_new (_("Alignment:"));
		gtk_widget_show (l);
		gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
		gtk_table_attach (GTK_TABLE (tbl), l, 0, 1, 0, 1, 0, 0, 4, 0);
		px = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_LEFT, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show (px);
		b = gtk_radio_button_new (NULL);
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_text_edit_dialog_any_toggled), dlg);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
		gtk_container_add (GTK_CONTAINER (b), px);
		gtk_table_attach (GTK_TABLE (tbl), b, 1, 2, 0, 1, 0, 0, 0, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "text_anchor_start", b);
		px = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_CENTER, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show (px);
		b = gtk_radio_button_new (gtk_radio_button_group (GTK_RADIO_BUTTON (b)));
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_text_edit_dialog_any_toggled), dlg);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
		gtk_container_add (GTK_CONTAINER (b), px);
		gtk_table_attach (GTK_TABLE (tbl), b, 2, 3, 0, 1, 0, 0, 0, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "text_anchor_middle", b);
		px = gtk_image_new_from_stock (GTK_STOCK_JUSTIFY_RIGHT, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show (px);
		b = gtk_radio_button_new (gtk_radio_button_group (GTK_RADIO_BUTTON (b)));
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_text_edit_dialog_any_toggled), dlg);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
		gtk_container_add (GTK_CONTAINER (b), px);
		gtk_table_attach (GTK_TABLE (tbl), b, 3, 4, 0, 1, 0, 0, 0, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "text_anchor_end", b);

		l = gtk_label_new (_("Orientation:"));
		gtk_widget_show (l);
		gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
		gtk_table_attach (GTK_TABLE (tbl), l, 0, 1, 1, 2, 0, 0, 4, 0);
		px = gtk_image_new_from_file (SODIPODI_GLADEDIR "/writing_mode_lr.xpm");
/*  		px = gnome_stock_pixmap_widget (dlg, SODIPODI_GLADEDIR "/writing_mode_lr.xpm"); */
		gtk_widget_show (px);
		b = gtk_radio_button_new (NULL);
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_text_edit_dialog_any_toggled), dlg);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
		gtk_container_add (GTK_CONTAINER (b), px);
		gtk_table_attach (GTK_TABLE (tbl), b, 1, 2, 1, 2, 0, 0, 0, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "writing_mode_lr", b);
		px = gtk_image_new_from_file (SODIPODI_GLADEDIR "/writing_mode_tb.xpm");
/*  		px = gnome_stock_pixmap_widget (dlg, SODIPODI_GLADEDIR "/writing_mode_tb.xpm"); */
		gtk_widget_show (px);
		b = gtk_radio_button_new (gtk_radio_button_group (GTK_RADIO_BUTTON (b)));
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_text_edit_dialog_any_toggled), dlg);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
		gtk_container_add (GTK_CONTAINER (b), px);
		gtk_table_attach (GTK_TABLE (tbl), b, 2, 3, 1, 2, 0, 0, 0, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "writing_mode_tb", b);

		preview = sp_font_preview_new ();
		gtk_widget_show (preview);
		gtk_box_pack_start (GTK_BOX (vb), preview, TRUE, TRUE, 4);
		gtk_object_set_data (GTK_OBJECT (dlg), "preview", preview);

		l = gtk_label_new (_("Text and font"));
		gtk_widget_show (l);
		gtk_notebook_append_page (GTK_NOTEBOOK (nb), vb, l);

		/* Buttons */
		hs = gtk_hseparator_new ();
		gtk_widget_show (hs);
		gtk_box_pack_start (GTK_BOX (mainvb), hs, FALSE, FALSE, 0);

		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_container_set_border_width (GTK_CONTAINER (hb), 4);
		gtk_box_pack_start (GTK_BOX (mainvb), hb, FALSE, FALSE, 0);

		b = gtk_button_new_with_label (_("Set as default"));
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_text_edit_dialog_set_default), dlg);
		gtk_box_pack_start (GTK_BOX (hb), b, FALSE, FALSE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "default", b);

		b = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
/*  		b = gnome_stock_button (GTK_STOCK_CLOSE); */
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_text_edit_dialog_close), dlg);
		gtk_box_pack_end (GTK_BOX (hb), b, FALSE, FALSE, 0);

		b = gtk_button_new_from_stock (GTK_STOCK_APPLY);
/*  		b = gnome_stock_button (GTK_STOCK_APPLY); */
		gtk_widget_show (b);
		gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_text_edit_dialog_apply), dlg);
		gtk_box_pack_end (GTK_BOX (hb), b, FALSE, FALSE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "apply", b);
	}

	gtk_signal_connect (GTK_OBJECT (SODIPODI), "modify_selection", GTK_SIGNAL_FUNC (sp_text_edit_dialog_modify_selection), dlg);
	gtk_signal_connect (GTK_OBJECT (SODIPODI), "change_selection", GTK_SIGNAL_FUNC (sp_text_edit_dialog_change_selection), dlg);

	sp_text_edit_dialog_read_selection (dlg, TRUE, TRUE);

	gtk_widget_show (dlg);
}

static void
sp_text_edit_dialog_modify_selection (Sodipodi *sodipodi, SPSelection *sel, guint flags, GtkWidget *dlg)
{
	gboolean style, content;

	style = ((flags & (SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG )) != 0);
	content = ((flags & (SP_OBJECT_CHILD_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG)) != 0);
	sp_text_edit_dialog_read_selection (dlg, style, content);
}

static void
sp_text_edit_dialog_change_selection (Sodipodi *sodipodi, SPSelection *sel, GtkWidget *dlg)
{
	sp_text_edit_dialog_read_selection (dlg, TRUE, TRUE);
}


static void
sp_text_edit_dialog_update_object (SPText *text, SPRepr *repr)
{
	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", GINT_TO_POINTER (TRUE));

	if (text) {
		GtkWidget *textw;
		guchar *str;

		textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");

		/* Content */
		str = gtk_editable_get_chars (GTK_EDITABLE (textw), 0, -1);
		sp_text_set_repr_text_multiline (text, str);
		g_free (str);
	}

	if (repr) {
		GtkWidget *fontsel, *preview, *b;
		SPCSSAttr *css;
		NRFont *font;
		guchar c[256];

		fontsel = gtk_object_get_data (GTK_OBJECT (dlg), "fontsel");
		preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");

		css = sp_repr_css_attr_new ();

		/* font */
		font = sp_font_selector_get_font (SP_FONT_SELECTOR (fontsel));
		nr_typeface_family_name_get (NR_FONT_TYPEFACE (font), c, 256);
		sp_repr_css_set_property (css, "font-family", c);
		nr_typeface_attribute_get (NR_FONT_TYPEFACE (font), "weight", c, 256);
		g_strdown (c);
		sp_repr_css_set_property (css, "font-weight", c);
		nr_typeface_attribute_get (NR_FONT_TYPEFACE (font), "style", c, 256);
		g_strdown (c);
		sp_repr_css_set_property (css, "font-style", c);
		snprintf (c, 64, "%g", NR_FONT_SIZE (font));
		sp_repr_css_set_property (css, "font-size", c);
		nr_font_unref (font);
		/* Layout */
		b = gtk_object_get_data (GTK_OBJECT (dlg), "text_anchor_start");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b))) {
			sp_repr_css_set_property (css, "text-anchor", "start");
		} else {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "text_anchor_middle");
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b))) {
				sp_repr_css_set_property (css, "text-anchor", "middle");
			} else {
				sp_repr_css_set_property (css, "text-anchor", "end");
			}
		}
		b = gtk_object_get_data (GTK_OBJECT (dlg), "writing_mode_lr");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b))) {
			sp_repr_css_set_property (css, "writing-mode", "lr");
		} else {
			sp_repr_css_set_property (css, "writing-mode", "tb");
		}

		sp_repr_css_change (repr, css, "style");
		sp_repr_css_attr_unref (css);
	}

	if (text) {
		sp_document_done (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP));
		sp_document_ensure_up_to_date (SP_OBJECT_DOCUMENT (text));
	}

	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", NULL);
}

static void
sp_text_edit_dialog_set_default (GtkButton *button, GtkWidget *dlg)
{
	GtkWidget *def;
	SPRepr *repr;

	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	repr = sodipodi_get_repr (SODIPODI, "tools.text");

	sp_text_edit_dialog_update_object (NULL, repr);

	gtk_widget_set_sensitive (def, FALSE);
}

static void
sp_text_edit_dialog_apply (GtkButton *button, GtkWidget *dlg)
{
	GtkWidget *apply, *def;
	SPText *text;
	SPRepr *repr;

	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", GINT_TO_POINTER (TRUE));

	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	text = sp_ted_get_selected_text_item ();
	if (text) {
		repr = SP_OBJECT_REPR (text);
	} else {
		repr = sodipodi_get_repr (SODIPODI, "tools.text");
	}

	sp_text_edit_dialog_update_object (text, repr);

	if (!text) {
		gtk_widget_set_sensitive (def, FALSE);
	}
	gtk_widget_set_sensitive (apply, FALSE);

	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", NULL);
}

static void
sp_text_edit_dialog_close (GtkButton *button, GtkWidget *dlg)
{
	gtk_object_destroy (GTK_OBJECT (dlg));
}

static guchar *
sp_text_edit_dialog_font_style_to_lookup (SPStyle *style)
{
	static guchar c[256];
	guchar *wstr, *sstr, *p;

	switch (style->font_weight.computed) {
	case SP_CSS_FONT_WEIGHT_100:
		wstr = "extra light";
		break;
	case SP_CSS_FONT_WEIGHT_200:
		wstr = "thin";
		break;
	case SP_CSS_FONT_WEIGHT_300:
		wstr = "light";
		break;
	case SP_CSS_FONT_WEIGHT_400:
	case SP_CSS_FONT_WEIGHT_NORMAL:
		wstr = NULL;
		break;
	case SP_CSS_FONT_WEIGHT_500:
		wstr = "medium";
		break;
	case SP_CSS_FONT_WEIGHT_600:
		wstr = "semi";
		break;
	case SP_CSS_FONT_WEIGHT_700:
	case SP_CSS_FONT_WEIGHT_BOLD:
		wstr = "bold";
		break;
	case SP_CSS_FONT_WEIGHT_800:
		wstr = "heavy";
		break;
	case SP_CSS_FONT_WEIGHT_900:
		wstr = "black";
		break;
	default:
		wstr = NULL;
		break;
	}

	switch (style->font_style.computed) {
	case SP_CSS_FONT_STYLE_NORMAL:
		sstr = NULL;
		break;
	case SP_CSS_FONT_STYLE_ITALIC:
		sstr = "italic";
		break;
	case SP_CSS_FONT_STYLE_OBLIQUE:
		sstr = "oblique";
		break;
	default:
		sstr = NULL;
		break;
	}

	p = c;
	if (wstr) {
		if (p != c) *p++ = ' ';
		memcpy (p, wstr, strlen (wstr));
		p += strlen (wstr);
	}
	if (sstr) {
		if (p != c) *p++ = ' ';
		memcpy (p, sstr, strlen (sstr));
		p += strlen (sstr);
	}
	*p = '\0';

	return c;
}

static void
sp_text_edit_dialog_read_selection (GtkWidget *dlg, gboolean dostyle, gboolean docontent)
{
	GtkWidget *notebook, *textw, *fontsel, *preview, *apply, *def;
	SPText *text;
	SPStyle *style;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;
	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", GINT_TO_POINTER (TRUE));

	notebook = gtk_object_get_data (GTK_OBJECT (dlg), "notebook");
	textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");
	fontsel = gtk_object_get_data (GTK_OBJECT (dlg), "fontsel");
	preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	text = sp_ted_get_selected_text_item ();

	if (text) {
		gtk_widget_set_sensitive (textw, TRUE);
		gtk_widget_set_sensitive (apply, FALSE);
		gtk_widget_set_sensitive (def, TRUE);
		style = SP_OBJECT_STYLE (text);
		if (docontent) {
			guchar *str;
			str = sp_text_get_string_multiline (text);
			if (str && *str) {
				int pos;
				pos = 0;
				gtk_text_freeze (GTK_TEXT (textw));
				gtk_editable_delete_text (GTK_EDITABLE (textw), 0, -1);
				gtk_editable_insert_text (GTK_EDITABLE (textw), str, strlen (str), &pos);
				gtk_text_thaw (GTK_TEXT (textw));
				sp_font_preview_set_phrase (SP_FONT_PREVIEW (preview), str);
				g_free (str);
			} else {
				gtk_editable_delete_text (GTK_EDITABLE (textw), 0, -1);
				sp_font_preview_set_phrase (SP_FONT_PREVIEW (preview), NULL);
			}
		}
	} else {
		SPRepr *repr;
		gtk_widget_set_sensitive (textw, FALSE);
		gtk_widget_set_sensitive (apply, FALSE);
		gtk_widget_set_sensitive (def, FALSE);
		repr = sodipodi_get_repr (SODIPODI, "tools.text");
		if (repr) {
			gtk_widget_set_sensitive (notebook, TRUE);
			style = sp_style_new ();
			sp_style_read_from_repr (style, repr);
		} else {
			gtk_widget_set_sensitive (notebook, FALSE);
			style = sp_style_new ();
		}
	}

	if (dostyle) {
		NRTypeFace *tf;
		NRFont *font;
		GtkWidget *b;
		guchar *c;

		c = sp_text_edit_dialog_font_style_to_lookup (style);
		tf = nr_type_directory_lookup_fuzzy (style->text->font_family.value, c);
		font = nr_font_new_default (tf, NR_TYPEFACE_METRICS_HORIZONTAL, style->font_size.computed);
		nr_typeface_unref (tf);

		if (font) {
			sp_font_selector_set_font (SP_FONT_SELECTOR (fontsel), font);
			sp_font_preview_set_font (SP_FONT_PREVIEW (preview), font);
			nr_font_unref (font);
		}

		if (style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_START) {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "text_anchor_start");
		} else if (style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_MIDDLE) {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "text_anchor_middle");
		} else {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "text_anchor_end");
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), TRUE);
		if (style->writing_mode.computed == SP_CSS_WRITING_MODE_LR) {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "writing_mode_lr");
		} else {
			b = gtk_object_get_data (GTK_OBJECT (dlg), "writing_mode_tb");
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), TRUE);
	}

	gtk_object_set_data (GTK_OBJECT (dlg), "blocked", NULL);
}

static void
sp_text_edit_dialog_text_changed (GtkText *txt, GtkWidget *dlg)
{
	GtkWidget *textw, *preview, *apply, *def;
	SPText *text;
	gchar *str;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	text = sp_ted_get_selected_text_item ();

	textw = gtk_object_get_data (GTK_OBJECT (dlg), "text");
	preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	str = gtk_editable_get_chars (GTK_EDITABLE (textw), 0, -1);
	if (str && *str) {
		sp_font_preview_set_phrase (SP_FONT_PREVIEW (preview), str);
	} else {
		sp_font_preview_set_phrase (SP_FONT_PREVIEW (preview), NULL);
	}
	if (str) g_free (str);

	if (text) {
		gtk_widget_set_sensitive (apply, TRUE);
	}
	gtk_widget_set_sensitive (def, TRUE);
}

static void
sp_text_edit_dialog_font_changed (SPFontSelector *fsel, NRFont *font, GtkWidget *dlg)
{
	GtkWidget *preview, *apply, *def;
	SPText *text;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	text = sp_ted_get_selected_text_item ();

	preview = gtk_object_get_data (GTK_OBJECT (dlg), "preview");
	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	sp_font_preview_set_font (SP_FONT_PREVIEW (preview), font);

	if (text) {
		gtk_widget_set_sensitive (apply, TRUE);
	}
	gtk_widget_set_sensitive (def, TRUE);
}

static void
sp_text_edit_dialog_any_toggled (GtkToggleButton *tb, GtkWidget *dlg)
{
	GtkWidget *apply, *def;
	SPText *text;

	if (gtk_object_get_data (GTK_OBJECT (dlg), "blocked")) return;

	text = sp_ted_get_selected_text_item ();

	apply = gtk_object_get_data (GTK_OBJECT (dlg), "apply");
	def = gtk_object_get_data (GTK_OBJECT (dlg), "default");

	if (text) {
		gtk_widget_set_sensitive (apply, TRUE);
	}
	gtk_widget_set_sensitive (def, TRUE);
}

static SPText *
sp_ted_get_selected_text_item (void)
{
	SPItem *item;
	if (!SP_ACTIVE_DESKTOP) return NULL;
	item = sp_selection_item (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	if (item && SP_IS_TEXT (item)) return SP_TEXT (item);
	return NULL;
}
