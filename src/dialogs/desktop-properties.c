#define SP_DESKTOP_PROPERTIES_C

#include <glade/glade.h>
#include "desktop-properties.h"
#include "../sodipodi.h"
#include "../desktop-handles.h"
#include "../svg/svg.h"

/*
 * Very-very basic desktop properties dialog
 *
 */ 

GladeXML  * guides_xml = NULL;
GtkWidget * guides_dialog = NULL;

GtkSpinButton * tolerance;
GtkToggleButton * snap_to_guides;

static void sp_guides_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);

void
sp_guides_dialog (void)
{
	if (guides_dialog == NULL) {
		guides_xml = glade_xml_new (SODIPODI_GLADEDIR "/desktop.glade", "guides_dialog");
		glade_xml_signal_autoconnect (guides_xml);
		guides_dialog = glade_xml_get_widget (guides_xml, "guides_dialog");
		
		tolerance = (GtkSpinButton *) glade_xml_get_widget (guides_xml, "tolerance");
		snap_to_guides = (GtkToggleButton *) glade_xml_get_widget (guides_xml, "snap_to_guides");

		gtk_signal_connect_while_alive (GTK_OBJECT (sodipodi), "activate_desktop",
						GTK_SIGNAL_FUNC (sp_guides_dialog_setup), NULL,
						GTK_OBJECT (guides_dialog));
	} else {
		if (!GTK_WIDGET_VISIBLE (guides_dialog))
			gtk_widget_show (guides_dialog);
	}

	sp_guides_dialog_setup (SODIPODI, SP_ACTIVE_DESKTOP, NULL);
}

/*
 * Fill entries etc. with default values
 */

static void
sp_guides_dialog_setup (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data)
{
	g_assert (sodipodi != NULL);
	g_assert (SP_IS_SODIPODI (sodipodi));
	g_assert (guides_dialog != NULL);

	if (!desktop) {
		gtk_widget_set_sensitive (GTK_WIDGET (snap_to_guides), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (tolerance), FALSE);
		return;
	} else {
		gtk_widget_set_sensitive (GTK_WIDGET (snap_to_guides), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (tolerance), TRUE);
	}

	gtk_toggle_button_set_active (snap_to_guides, desktop->namedview->snaptoguides);

	gtk_spin_button_set_value (tolerance, desktop->namedview->guidetolerance);
}

void
sp_guides_dialog_close (void)
{
	g_assert (guides_dialog != NULL);

	if (GTK_WIDGET_VISIBLE (guides_dialog))
		gtk_widget_hide (guides_dialog);
}


void
sp_guides_dialog_apply (void)
{
	SPDesktop * desktop;
	SPRepr * repr;
	gdouble t;

	g_assert (guides_dialog != NULL);

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail (desktop != NULL);
	repr = SP_OBJECT (desktop->namedview)->repr;

	if (gtk_toggle_button_get_active (snap_to_guides)) {
		sp_repr_set_attr (repr, "snaptoguides", "true");
	} else {
		sp_repr_set_attr (repr, "snaptoguides", NULL);
	}

	t = gtk_spin_button_get_value_as_float (tolerance);

	sp_repr_set_double_attribute (repr, "guidetolerance", t);
}


