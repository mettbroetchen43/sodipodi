/*
 * sodipodi - an ambitious vector drawing program
 * Copyright (c) Lauris Kaplinski 1999
 * Distributable under GPL version 2
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>

#include "desktop.h"

int
main (int argc, char *argv[])
{
	SPApp * app;
	SPDocument * doc;
	SPDesktop * desktop;
	gboolean sp_use_gui = TRUE;

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

	if (sp_use_gui) {
		gnome_init ("sodipodi", VERSION, argc, argv);
		glade_gnome_init ();

		app = sp_app_new ();
		g_assert (app != NULL);

		doc = sp_document_new ();
		g_assert (doc != NULL);

		desktop = sp_desktop_new (app, doc);
		g_assert (desktop != NULL);

		gtk_main ();
	}
	return 0;
}

