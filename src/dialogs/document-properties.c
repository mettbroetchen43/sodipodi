#define SP_DOCUMENT_PROPERTIES_C

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>
#include "document-properties.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../svg/svg.h"

/*
 * Very-very basic document properties dialog
 *
 */ 

#define MM2PT(v) ((v) * 72.0 / 25.4)
#define PT2MM(v) ((v) * 25.4 / 72.0)

static GladeXML  * xml = NULL;
static GtkWidget * dialog = NULL;

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


