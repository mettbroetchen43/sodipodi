#define SP_TEXT_EDIT_C

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomeprint/gnome-font-dialog.h>
#include "../xml/repr.h"
#include "../svg/svg.h"
#include "../sodipodi.h"
#include "../desktop-handles.h"
#include "text-edit.h"

static void sp_text_read_selection (void);

static void sp_text_show_dialog (void);
static void sp_text_hide_dialog (void);

/* glade gui handlers */

void sp_text_dialog_apply (GnomePropertyBox * propertybox, gint pagenum);
void sp_text_dialog_close (void);

void sp_text_family_select_row (GtkCList * c, gint row, gint column);
void sp_text_family_unselect_row (GtkCList * c, gint row, gint column);

void sp_text_dialog_text_changed (GtkEditable * editable, gpointer data);
static void sp_text_dialog_font_changed (GnomeFontSelection * fontsel, GnomeFont * font, gpointer data);

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;
static GtkWidget * fontsel = NULL;
static GtkWidget * preview = NULL;

gchar * fontname = NULL;
GtkText * ttext;

static SPCSSAttr * css = NULL;

#if 0
static guint view_changed_id = 0;
static guint sel_destroy_id = 0;
#endif

static guint sel_changed_id = 0;

#if 0
static SPSelection * sel_current;
#endif

void sp_text_edit_dialog (void)
{
	if (xml == NULL) {
		GtkWidget * vb;
		xml = glade_xml_new (SODIPODI_GLADEDIR "/text-dialog.glade", "text_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "text_dialog");
		ttext = (GtkText *) glade_xml_get_widget (xml, "text");
		vb = glade_xml_get_widget (xml, "vbox");
		fontsel = gnome_font_selection_new ();
		gtk_signal_connect (GTK_OBJECT (fontsel), "font_set",
				    GTK_SIGNAL_FUNC (sp_text_dialog_font_changed), NULL);
		gtk_widget_show (fontsel);
		gtk_box_pack_start ((GtkBox *) vb, fontsel, TRUE, TRUE, 4);
		preview = gnome_font_preview_new ();
		gtk_widget_show (preview);
		gtk_box_pack_start ((GtkBox *) vb, preview, TRUE, TRUE, 4);
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
	const gchar * str;
	const gchar * family;
	GnomeFontWeight weight;
	gboolean italic;
	double size;
	SPSVGUnit units;
	GnomeFont * font;
 
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

	if (str) {
		gtk_editable_insert_text (GTK_EDITABLE (ttext), str, strlen (str), &pos);
	}

	css = sp_repr_css_attr_new ();

	settings = sp_repr_css_attr_inherited (repr, "style");

	family = sp_repr_css_property (settings, "font-family", "Helvetica");
	str = sp_repr_css_property (settings, "font-weight", "normal");
	weight = sp_svg_read_font_weight (str);
	str = sp_repr_css_property (settings, "font-style", "normal");
	italic = sp_svg_read_font_italic (str);
	str = sp_repr_css_property (settings, "font-size", "12pt");
	size = sp_svg_read_length (&units, str);

	font = gnome_font_new_closest (family, weight, italic, size);

	if (font != NULL) {
		gnome_font_selection_set_font ((GnomeFontSelection *) fontsel, font);
		gnome_font_unref (font);
	}

	sp_repr_css_attr_unref (settings);
}

void
sp_text_dialog_apply (GnomePropertyBox * propertybox, gint pagenum)
{
	SPRepr * repr;
	const gchar * str;
	GnomeFontWeight weight;
	double size;
	SPCSSAttr * css;
	gchar c[64];
	GnomeFont * font;

	repr = sp_selection_repr (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	if (repr == NULL) return;
	if (strcmp (sp_repr_name (repr), "text") != 0) return;

	str = gtk_editable_get_chars (GTK_EDITABLE (ttext), 0, -1);
	sp_repr_set_content (repr, str);

	css = sp_repr_css_attr_new ();

	font = gnome_font_selection_get_font ((GnomeFontSelection *) fontsel);

	str = gnome_font_get_family_name (font);
	sp_repr_css_set_property (css, "font-family", g_strdup (str));

	weight = gnome_font_get_weight_code (font);
	if (weight < GNOME_FONT_SEMI) {
		str = "normal";
	} else {
		str = "bold";
	}
	sp_repr_css_set_property (css, "font-weight", g_strdup (str));

	if (gnome_font_is_italic (font)) {
		str = "italic";
	} else {
		str = "normal";
	}
	sp_repr_css_set_property (css, "font-style", g_strdup (str));

	size = gnome_font_get_size (font);
	snprintf (c, 64, "%f", size);
	sp_repr_css_set_property (css, "font-size", g_strdup (c));

	gnome_font_unref (font);

	sp_repr_css_change (repr, css, "style");
	sp_repr_css_attr_unref (css);

	sp_document_done (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP));
}

void
sp_text_dialog_close (void)
{
	sp_text_hide_dialog ();
}


void
sp_text_dialog_text_changed (GtkEditable * editable, gpointer data)
{
	const gchar * str;

	str = gtk_editable_get_chars ((GtkEditable *) ttext, 0, -1);
	if (strlen (str) > 0) {
		gnome_font_preview_set_phrase ((GnomeFontPreview *) preview, str);
	} else {
		gnome_font_preview_set_phrase ((GnomeFontPreview *) preview, NULL);
	}

	gnome_property_box_changed (GNOME_PROPERTY_BOX (dialog));
}

static void
sp_text_dialog_font_changed (GnomeFontSelection * fontsel, GnomeFont * font, gpointer data)
{
	gnome_font_preview_set_font ((GnomeFontPreview *) preview, font);

	gnome_property_box_changed (GNOME_PROPERTY_BOX (dialog));
}

/*
 * selection handlers
 *
 */

static void
sp_text_sel_changed (Sodipodi * sodipodi, SPSelection * selection, gpointer data)
{
	sp_text_read_selection ();
}

#if 0
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
#endif

static void
sp_text_show_dialog (void)
{
	g_return_if_fail (dialog != NULL);

	if (sel_changed_id < 1) {
		sel_changed_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "change_selection",
						     GTK_SIGNAL_FUNC (sp_text_sel_changed), NULL);
	}

#if 0
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
#endif

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

	if (sel_changed_id > 0) {
		gtk_signal_disconnect (GTK_OBJECT (SODIPODI), sel_changed_id);
		sel_changed_id = 0;
	}

#if 0
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
#endif
}

