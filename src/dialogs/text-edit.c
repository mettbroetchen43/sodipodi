#define SP_TEXT_EDIT_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../font-wrapper.h"
#include "../mdi-desktop.h"
#include "../selection.h"
#include "../desktop-handles.h"
#include "text-edit.h"

static void sp_text_read_selection (void);

static void sp_text_show_dialog (void);
static void sp_text_hide_dialog (void);

/* glade gui handlers */

void sp_text_dialog_apply (void);
void sp_text_dialog_close (void);

void sp_text_family_select_row (GtkCList * c, gint row, gint column);
void sp_text_family_unselect_row (GtkCList * c, gint row, gint column);

void sp_text_dialog_text_changed (GtkEditable * editable, gpointer data);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;

gchar * fontname = NULL;
GtkText * ttext;
GtkCList * clfamily;
GtkCombo * cstyle;
GtkSpinButton * ssize;
const gchar * familyname = NULL;

static SPCSSAttr * css = NULL;

static guint view_changed_id = 0;
static guint sel_destroy_id = 0;
static guint sel_changed_id = 0;
static SPSelection * sel_current;

void sp_text_edit_dialog (void)
{
	GList * fonts, * l;
	SPFontFamilyInfo * family;
	gint row;

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/text-dialog.glade", "text_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "text_dialog");
		clfamily = (GtkCList *) glade_xml_get_widget (xml, "family");
		fonts = sp_font_family_list ();
		for (l = fonts; l != NULL; l = l->next) {
			family = (SPFontFamilyInfo *) l->data;
			row = gtk_clist_append (clfamily, &family->name);
			gtk_clist_set_row_data (clfamily, row, GINT_TO_POINTER (g_quark_from_string (family->name)));
		}
		cstyle = (GtkCombo *) glade_xml_get_widget (xml, "style");
		ssize = (GtkSpinButton *) glade_xml_get_widget (xml, "size");
		ttext = (GtkText *) glade_xml_get_widget (xml, "text");
	}

	sp_text_read_selection ();
	sp_text_show_dialog ();
}

static void
sp_text_read_selection (void)
{
	SPSelection * selection;
	SPRepr * repr;
	SPCSSAttr * settings;
	gint pos;
	const gchar * str, * sfamily, * sstyle;
	gint row;
	GnomeFontWeight weight;
	gboolean italic;
	double size;
	SPSVGUnit units;
 
	g_return_if_fail (dialog != NULL);

	if (css != NULL) {
		sp_repr_css_attr_unref (css);
		css = NULL;
	}

#if 0
	if (contents != NULL) {
		g_free (contents);
		contents = NULL;
	}
#endif

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	repr = sp_selection_repr (selection);

	if (repr == NULL) return;
	if (strcmp (sp_repr_name (repr), "text") != 0) return;

	str = sp_repr_content (repr);
	gtk_editable_delete_text (GTK_EDITABLE (ttext), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (ttext), str, strlen (str), &pos);

	css = sp_repr_css_attr_new ();

	settings = sp_repr_css_attr_inherited (repr, "style");

	str = sp_repr_css_property (settings, "font-family", "Helvetica");
	sfamily = str;
	row = gtk_clist_find_row_from_data (clfamily, GINT_TO_POINTER (g_quark_from_string (sfamily)));
	if (row > 0) gtk_clist_select_row (clfamily, row, 0);

	str = sp_repr_css_property (settings, "font-weight", "normal");
	weight = sp_svg_read_font_weight (str);
	str = sp_repr_css_property (settings, "font-style", "normal");
	italic = sp_svg_read_font_italic (str);
	sstyle = "Normal";
	if (weight == GNOME_FONT_BOOK) {
		if (italic) sstyle = "Italic";
	} else {
		sstyle = "Bold";
		if (italic) sstyle = "Bold-Italic";
	}
	gtk_entry_set_text ((GtkEntry *) cstyle->entry, sstyle);

	str = sp_repr_css_property (settings, "font-size", "12pt");
	size = sp_svg_read_length (&units, str);
	gtk_spin_button_set_value (ssize, size);

	sp_repr_css_attr_unref (settings);
}

void
sp_text_dialog_apply (void)
{
	SPRepr * repr;
	gchar * str;
	double size;
	SPCSSAttr * css;
	gchar c[64];

	repr = sp_selection_repr (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	if (repr == NULL) return;
	if (strcmp (sp_repr_name (repr), "text") != 0) return;

	str = gtk_editable_get_chars (GTK_EDITABLE (ttext), 0, -1);
	sp_repr_set_content (repr, str);

	css = sp_repr_css_attr_new ();

	if (familyname != NULL)
		sp_repr_css_set_property (css, "font-family", g_strdup (familyname));
	str = gtk_entry_get_text ((GtkEntry *) cstyle->entry);
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
	size = gtk_spin_button_get_value_as_float (ssize);
	snprintf (c, 64, "%f", size);
	sp_repr_css_set_property (css, "font-size", g_strdup (c));

	sp_repr_css_change (repr, css, "style");
	sp_repr_css_attr_unref (css);
}

void
sp_text_dialog_close (void)
{
	sp_text_hide_dialog ();
}

void
sp_text_family_select_row (GtkCList * c, gint row, gint column)
{
	gpointer data;
	data = gtk_clist_get_row_data (clfamily, row);
	familyname = g_quark_to_string (GPOINTER_TO_INT (data));
}

void
sp_text_family_unselect_row (GtkCList * c, gint row, gint column)
{
	familyname = NULL;
}

void
sp_text_dialog_text_changed (GtkEditable * editable, gpointer data)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (dialog));
}

/*
 * selection handlers
 *
 */

static void
sp_text_sel_changed (SPSelection * selection, gpointer data)
{
	sp_text_read_selection ();
}

static void
sp_text_sel_destroy (GtkObject * object, gpointer data)
{
	SPSelection * selection;

	selection = SP_SELECTION (object);

	if (selection == sel_current) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}
}

static void
sp_text_view_changed (GnomeMDI * mdi, GtkWidget * widget, gpointer data)
{
	if (sel_current != NULL) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}

	sel_current = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sel_current != NULL) {
		if (sel_destroy_id < 1) {
			sel_destroy_id = gtk_signal_connect (GTK_OBJECT (sel_current), "destroy",
				GTK_SIGNAL_FUNC (sp_text_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_text_sel_changed), NULL);
		}
	}

	sp_text_read_selection ();
}

static void
sp_text_show_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	sel_current = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sel_current != NULL) {
		if (sel_destroy_id < 1) {
			sel_destroy_id = gtk_signal_connect (GTK_OBJECT (sel_current), "destroy",
				GTK_SIGNAL_FUNC (sp_text_sel_destroy), NULL);
		}
		if (sel_changed_id < 1) {
			sel_changed_id = gtk_signal_connect (GTK_OBJECT (sel_current), "changed",
				GTK_SIGNAL_FUNC (sp_text_sel_changed), NULL);
		}
	}

	if (view_changed_id < 1) {
		view_changed_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "view_changed",
			GTK_SIGNAL_FUNC (sp_text_view_changed), NULL);
	}
	if (!GTK_WIDGET_VISIBLE (dialog)) {
		gtk_widget_show (dialog);
	}
}

static void
sp_text_hide_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog)) {
		gtk_widget_hide (dialog);
	}

	if (view_changed_id > 0) {
		gtk_signal_disconnect (GTK_OBJECT (SODIPODI), view_changed_id);
		view_changed_id = 0;
	}

	if (sel_current != NULL) {
		if (sel_destroy_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_destroy_id);
			sel_destroy_id = 0;
		}
		if (sel_changed_id > 0) {
			gtk_signal_disconnect (GTK_OBJECT (sel_current), sel_changed_id);
			sel_changed_id = 0;
		}
	}
}

