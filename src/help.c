#define HELP_C

#include <gnome.h>
#include <glade/glade.h>
#include "help.h"

void
sp_help_about (void)
{
	GladeXML * xml;
	GtkWidget * dialog;

	xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "about");
	g_assert (xml != NULL);
	dialog = glade_xml_get_widget (xml, "about");

	gnome_dialog_run ((GnomeDialog *) dialog);
}
