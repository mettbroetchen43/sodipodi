#define OBJECT_STROKE_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "text-edit.h"
#include "../sp-text.h"
#include "../selection.h"

void text_dialog_ok (GtkButton * button, gpointer data);
void text_dialog_apply (GtkButton * button, gpointer data);
void text_dialog_cancel (GtkButton * button, gpointer data);

void text_dialog_changed (GtkText * text, gpointer data);

static void text_dialog_show (SPRepr * repr);
static void text_dialog_hide (void);

static void apply_font (SPRepr * repr);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;
gchar * fontname = NULL;
GtkText * wtext;
GtkCList * wfamily;
GtkCombo * wstyle;
GtkSpinButton * wsize;
const gchar * familyname = NULL;

void
sp_text_edit_selection_changed (void)
{
}

void sp_text_edit_dialog (void)
{
#if 0
	SPRepr * repr;

	repr = sp_selection_repr ();
	if (repr == NULL)
		return;

	text_dialog_show (repr);

	return;
#endif
}

void
text_dialog_ok (GtkButton * button, gpointer data)
{
#if 0
	SPRepr * repr;

	repr = sp_selection_repr ();

	if (repr != NULL)
		apply_font (repr);

#if 0
	sp_broker_selection_changed ();
#endif

	text_dialog_hide ();
#endif
}

void
text_dialog_apply (GtkButton * button, gpointer data)
{
#if 0
	SPRepr * repr;

	repr = sp_selection_repr ();

	if (repr != NULL)
		apply_font (repr);

#if 0
	sp_broker_selection_changed ();
#endif
#endif
}

void
text_dialog_cancel (GtkButton * button, gpointer data)
{
#if 0
	text_dialog_hide ();
#endif
}

void
family_select_row (GtkCList * c, gint row, gint column)
{
#if 0
	gpointer data;
	data = gtk_clist_get_row_data (wfamily, row);
	familyname = g_quark_to_string (GPOINTER_TO_INT (data));
#endif
}

void
family_unselect_row (GtkCList * c, gint row, gint column)
{
	familyname = NULL;
}

static void
apply_font (SPRepr * repr)
{
#if 0
	gchar * str;
	double size;
	SPCSSAttr * css;
	gchar c[64];

	if (repr == NULL)
		return;
	if (strcmp (sp_repr_name (repr), "text") != 0)
		return;

	str = gtk_editable_get_chars (GTK_EDITABLE (wtext), 0, -1);
	sp_repr_set_content (repr, str);
	g_free (str);

	css = sp_repr_css_attr_inherited (repr, "style");

	if (familyname != NULL)
		sp_repr_css_set_property (css, "font-family", g_strdup (familyname));
	str = gtk_entry_get_text (wstyle->entry);
	if (strcmp (str, "Normal") == 0) {
		sp_repr_css_set_property (css, "font-weight", g_strdup ("normal"));
		sp_repr_css_set_property (css, "font-style", g_strdup ("normal"));
	}
	if (strcmp (str, "Italic") == 0) {
		sp_repr_css_set_property (css, "font-weight", g_strdup ("normal"));
		sp_repr_css_set_property (css, "font-style", g_strdup ("italic"));
	}
	if (strcmp (str, "Bold") == 0) {
		sp_repr_css_set_property (css, "font-weight", g_strdup ("bold"));
		sp_repr_css_set_property (css, "font-style", g_strdup ("normal"));
	}
	if (strcmp (str, "Bold-Italic") == 0) {
		sp_repr_css_set_property (css, "font-weight", g_strdup ("bold"));
		sp_repr_css_set_property (css, "font-style", g_strdup ("italic"));
	}
	size = gtk_spin_button_get_value_as_float (wsize);
	snprintf (c, 64, "%f", size);
	sp_repr_css_set_property (css, "font-size", g_strdup (c));

	sp_repr_css_change (repr, css, "style");
	sp_repr_css_attr_unref (css);
#endif
}

static void
text_dialog_set_values (SPRepr * repr)
{
#if 0
	SPCSSAttr * css;
	const gchar * str, * sfamily, * sstyle;
	gint row;
	GnomeFontWeight weight;
	gboolean italic;
	double size;

	if (repr == NULL)
		return;
	if (strcmp (sp_repr_name (repr), "text") != 0)
		return;

	str = sp_repr_content (repr);
	gtk_editable_delete_text (GTK_EDITABLE (wtext), 0, -1);
	gtk_text_insert (wtext, NULL, NULL, NULL, str, strlen (str));

	css = sp_repr_css_attr_inherited (repr, "style");
	sfamily = sp_font_read_family (css);
	row = gtk_clist_find_row_from_data (wfamily, GINT_TO_POINTER (g_quark_from_string (sfamily)));
	if (row > 0)
		gtk_clist_select_row (wfamily, row, 0);
	weight = sp_font_read_weight (css);
	italic = sp_font_read_italic (css);
	sstyle = "Normal";
	if (weight == GNOME_FONT_BOOK) {
		if (italic) sstyle = "Italic";
	} else {
		sstyle = "Bold";
		if (italic) sstyle = "Bold-Italic";
	}
	gtk_entry_set_text (wstyle->entry, sstyle);
	size = sp_font_read_size (css);
	gtk_spin_button_set_value (wsize, size);
#endif
}


static void
text_dialog_show (SPRepr * repr)
{
#if 0
	GList * fonts, * l;
	SPFontFamilyInfo * family;
	gint row;

	if (dialog == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/text-dialog.glade", "text_dialog");
		g_assert (xml != NULL);

		glade_xml_signal_autoconnect (xml);

		dialog = glade_xml_get_widget (xml, "text_dialog");
		g_assert (dialog != NULL);

		wtext = (GtkText *) glade_xml_get_widget (xml, "text");
		wfamily = (GtkCList *) glade_xml_get_widget (xml, "family");
		wstyle = (GtkCombo *) glade_xml_get_widget (xml, "style");
		wsize = (GtkSpinButton *) glade_xml_get_widget (xml, "size");
		fonts = sp_font_family_list ();
		for (l = fonts; l != NULL; l = l->next) {
			family = (SPFontFamilyInfo *) l->data;
			row = gtk_clist_append (wfamily, &family->name);
			gtk_clist_set_row_data (wfamily, row, GINT_TO_POINTER (g_quark_from_string (family->name)));
		}

		text_dialog_set_values (repr);

	} else {
		text_dialog_set_values (repr);
		gtk_widget_show (dialog);
	}
#endif
}

static void
text_dialog_hide (void)
{
#if 0
	g_assert (dialog != NULL);
	g_assert (GTK_WIDGET_VISIBLE (dialog));

	gtk_widget_hide (dialog);

	familyname = NULL;
#endif
}

