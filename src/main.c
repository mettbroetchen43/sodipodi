/*
 * sodipodi - an ambitious vector drawing program
 * Copyright (c) Lauris Kaplinski et al 1999-2000
 * Distributable under GPL version 2 (or later)
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <locale.h>

#include <gnome.h>
#include <glade/glade.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include "sodipodi.h"

#ifdef ENABLE_BONOBO
#include "bonobo/sodipodi-bonobo.h"
#include "bonobo/svg-doc-factory.h"
#endif

#include "mdi.h"
#include "mdi-child.h"
#include "file.h"

#include "session.h"

enum {SP_ARG_NONE, SP_ARG_FILE, SP_ARG_PRINT, SP_ARG_LAST};

static GSList * sp_process_args (poptContext ctx);
gchar * sp_global_printer = NULL;

struct poptOption options[] = {
  {
        "without-gui",
        'z',
        POPT_ARG_NONE,
        NULL,
        0,
        N_("Do not use GUI. NB! if exist, should be FIRST argument!"),
        NULL
  },
  {
        "file",
        'f',
        POPT_ARG_STRING,
        NULL,
        SP_ARG_FILE,
        N_("Open specified file (option string may be excluded)"),
        N_("FILENAME")
  },
  {
        "print",
        'p',
        POPT_ARG_STRING,
        &sp_global_printer,
        SP_ARG_PRINT,
        N_("Print files to specified file (use '| program' for pipe)"),
        N_("FILENAME")
  },
  { NULL, '\0', 0, NULL, 0 }
};

int
main (int argc, char *argv[])
{
	GnomeClient * client;
	GnomeClientFlags flags;
	gchar * prefix;
	SPDocument * doc;
	SPMDIChild * child;

	poptContext ctx;

	GSList * fl;
	gboolean use_gui;
	gboolean restored;

#ifdef ENABLE_BONOBO
	gboolean bonobo_client = FALSE;
#endif

#ifdef __FreeBSD__
	fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

	use_gui = TRUE;

	/* Test, if first arg is "-z" or "--without-gui" */

	if (argc > 1) {
		if ((strcmp (argv[1], "-z") == 0) ||
		    (strcmp (argv[1], "--without-gui") == 0)) {

			use_gui = FALSE;
		}
	}

	if (!use_gui) {

		/* We are started in text mode */

		ctx = poptGetContext (NULL, argc, argv, options, 0);
		g_return_val_if_fail (ctx != NULL, 1);

		fl = sp_process_args (ctx);

		poptFreeContext (ctx);

		if (fl == NULL) {
			g_print ("Nothing to do!\n");
			exit (0);
		}

		/* Start up gtk, without requiring X */

		gtk_type_init();

		setlocale (LC_NUMERIC, "C");

		while (fl) {
			doc = sp_document_new ((gchar *) fl->data);
			if (doc == NULL) {
				g_log ("Sodipodi",
					G_LOG_LEVEL_MESSAGE,
					"Cannot open file %s",
					(gchar *) fl->data);
			} else {
				if (sp_global_printer != NULL) {
					sp_do_file_print_to_file (doc, sp_global_printer);
				}
			}
			fl = g_slist_remove (fl, fl->data);
		}
	} else {
		/* Use GUI */

#ifdef ENABLE_BONOBO
		/* Check, if we are executed as sodipodi-bonobo */

		if (strcmp (argv[0], "sodipodi-bonobo") == 0) {
			bonobo_client = TRUE;
		}

		CORBA_exception_init (&ev);

		gnome_CORBA_init_with_popt_table ("sodipodi", VERSION,
			&argc, argv, options, 0, &ctx,
			GNORBA_INIT_SERVER_FUNC, &ev);

		CORBA_exception_free (&ev);

		orb = gnome_CORBA_ORB ();

		if (bonobo_init (orb, NULL, NULL) == FALSE)
			g_error (_("Could not initialize Bonobo"));

#else /* ENABLE_BONOBO */

		gnome_init_with_popt_table ("sodipodi", VERSION, argc, argv,
                                    	    options, 0, &ctx);
#endif /* ENABLE_BONOBO */

/*		fl = sp_process_args (ctx);*/
		fl = NULL;

		glade_gnome_init ();

		/* We must set LC_NUMERIC to default, or otherwise
		 * we'll end with localised SVG files :-(
		 */

		setlocale (LC_NUMERIC, "C");

		/* Session management stuff */

		client = gnome_master_client ();
		gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			GTK_SIGNAL_FUNC (sp_sm_save_yourself), argv[0]);
		gtk_signal_connect (GTK_OBJECT (client), "die",
			GTK_SIGNAL_FUNC (sp_sm_die), NULL);

		/* Create main MDI object */

		sp_mdi_create ();

		restored = FALSE;
		flags = gnome_client_get_flags (client);
		if (flags & GNOME_CLIENT_RESTORED) {
			prefix = gnome_client_get_config_prefix (client);
			gnome_config_push_prefix (prefix);
			restored = sp_sm_restore_children ();
			gnome_config_pop_prefix ();
		}

#ifdef ENABLE_BONOBO

		/* No reading files & opening windows */

		sp_svg_doc_factory_init ();

		if (!bonobo_client) {

#endif /* ENABLE_BONOBO */

		if (!restored) {

			/* Nothing was restored, continue with local initializing */

			child = NULL;
			while (fl) {
				doc = sp_document_new ((gchar *) fl->data);
				if (doc == NULL) {
					g_log ("Sodipodi",
						G_LOG_LEVEL_MESSAGE,
						"Cannot open file %s",
						(gchar *) fl->data);
				}  else {
					child = sp_mdi_child_new (doc);
					g_return_val_if_fail (child != NULL, 1);
					gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
					gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));
				}
				fl = g_slist_remove (fl, fl->data);
			}

			if (child == NULL) {
				/* No files were loaded */
				doc = sp_document_new (NULL);
				g_return_val_if_fail (doc != NULL, 1);
				child = sp_mdi_child_new (doc);
				g_return_val_if_fail (child != NULL, 1);
				gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
				gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));
			}

		}

#ifdef ENABLE_BONOBO
		}
#endif /* ENABLE_BONOBO */

		poptFreeContext (ctx);

#ifdef ENABLE_BONOBO
		bonobo_main ();
#else
		gtk_main ();
#endif

	}

#ifdef __FreeBSD__
	fpresetsticky(FP_X_DZ|FP_X_INV);
	fpsetmask(FP_X_DZ|FP_X_INV);
#endif
	return 0;
}

static GSList *
sp_process_args (poptContext ctx)
{
	GSList * fl;
	const gchar * fn;
	const gchar ** args;
	gint i, a;

	fl = NULL;

	while ((a = poptGetNextOpt (ctx)) >= 0) {
		switch (a) {
		case SP_ARG_FILE:
			fn = poptGetOptArg (ctx);
			if (fn != NULL) {
				fl = g_slist_append (fl, (gpointer) fn);
			}
			break;
		default:
			break;
		}
	}

	args = poptGetArgs (ctx);

	if (args != NULL) {
		for (i = 0; args[i] != NULL; i++) {
			fl = g_slist_append (fl, (gpointer) args[i]);
		}
	}

	return fl;
}

