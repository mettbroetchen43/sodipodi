/*
 * sodipodi - an ambitious vector drawing program
 * Copyright (c) Lauris Kaplinski 1999
 * Distributable under GPL version 2 (or later)
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>

#include "sodipodi.h"
#include "mdi.h"
#include "mdi-child.h"

int
main (int argc, char *argv[])
{
	SPDocument * doc;
	SPMDIChild * child;
	gboolean sp_use_gui = TRUE;

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

	if (sp_use_gui) {
		gnome_init ("sodipodi", VERSION, argc, argv);
		glade_gnome_init ();

		sp_mdi_create ();

		doc = sp_document_new ();
		g_assert (doc != NULL);

		child = sp_mdi_child_new (doc);

		gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));

		gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));

		gtk_main ();
	}
	return 0;
}

