/*
 * sodipodi - an ambitious vector drawing program
 * Copyright (c) Lauris Kaplinski et al. 1999-2000
 * Distributable under GPL version 2 (or later)
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <locale.h>

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>
#include <tree.h> /* libxml */

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include "sodipodi-private.h"

#ifdef ENABLE_BONOBO
#include <liboaf/liboaf.h>

#include "bonobo/sodipodi-bonobo.h"
#include "bonobo/svg-doc-factory.h"
#endif

#include "document.h"
#include "file.h"
#include "toolbox.h"
#include "interface.h"

/* fixme: These are required for temporary slideshow hack */
#include "desktop.h"
#include "slide-context.h"

enum {SP_ARG_NONE, SP_ARG_NOGUI, SP_ARG_FILE, SP_ARG_PRINT, SP_ARG_SLIDESHOW, SP_ARG_LAST};

static GSList * sp_process_args (poptContext ctx);
gchar * sp_global_printer = NULL;
gboolean sp_global_slideshow = FALSE;

struct poptOption options[] = {
  {
        "without-gui",
        'z',
        POPT_ARG_NONE,
        NULL,
        SP_ARG_NOGUI,
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
        N_("Print files to specified output file (use '| program' for pipe)"),
        N_("FILENAME")
  },
  {
        "slideshow",
        's',
        POPT_ARG_NONE,
        &sp_global_slideshow,
        SP_ARG_SLIDESHOW,
        N_("Show given files one-by-one, switch to next on any key/mouse event"),
        NULL
  },
  { NULL, '\0', 0, NULL, 0, NULL, NULL }
};

static void
main_save_preferences (GtkObject * object, gpointer data)
{
	sodipodi_save_preferences (SP_SODIPODI (object));
}

int
main (int argc, char *argv[])
{
#if 0
	GnomeClient * client;
	GnomeClientFlags flags;
	gchar * prefix;
#endif
	SPDocument * doc;

	poptContext ctx = NULL;

	GSList * fl;
	gboolean use_gui;
#if 0
	gboolean restored;
#endif

#ifdef ENABLE_BONOBO
	gboolean bonobo_client = FALSE;
#endif

#ifdef __FreeBSD__
	fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

	LIBXML_TEST_VERSION

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

		gnomelib_register_popt_table (oaf_popt_options, _("Oaf options"));
		gnome_init_with_popt_table ("sodipodi", VERSION,
					    argc, argv, options, 0, &ctx);
		orb = oaf_init (argc, argv);

		CORBA_exception_free (&ev);

		if (bonobo_init (orb, NULL, NULL) == FALSE)
			g_error (_("Could not initialize Bonobo"));

#else /* ENABLE_BONOBO */

		gnome_init_with_popt_table ("sodipodi", VERSION, argc, argv,
                                    	    options, 0, &ctx);
#endif /* ENABLE_BONOBO */

#if 1
		fl = sp_process_args (ctx);
#else
		fl = NULL;
#endif
		glade_gnome_init ();

		/* We must set LC_NUMERIC to default, or otherwise
		 * we'll end with localised SVG files :-(
		 */

		setlocale (LC_NUMERIC, "C");

		/* Set default icon */

		if (g_file_exists (GNOME_ICONDIR "/sodipodi.png")) {
			gnome_window_icon_set_default_from_file (GNOME_ICONDIR "/sodipodi.png");
		} else {
			g_warning ("Could not find %s", GNOME_ICONDIR "/sodipodi.png");
		}

#if 0
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
#endif

#ifdef ENABLE_BONOBO

		/* No reading files & opening windows */

		sp_svg_doc_factory_init ();

		if (!bonobo_client) {

#endif /* ENABLE_BONOBO */

#if 0
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
				/* No files were loaded 
				doc = sp_document_new (NULL);
				g_return_val_if_fail (doc != NULL, 1);
				child = sp_mdi_child_new (doc);
				g_return_val_if_fail (child != NULL, 1);
				gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
				gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));
				*/
			  sp_maintoolbox_create ();
			}

		}
#endif

		if (!sp_global_slideshow) {
			sodipodi = sodipodi_application_new ();
			sodipodi_load_preferences (sodipodi);
			gtk_signal_connect (GTK_OBJECT (sodipodi), "destroy",
					    GTK_SIGNAL_FUNC (main_save_preferences), NULL);
			sp_maintoolbox_create ();
			sodipodi_unref ();

			while (fl) {
				SPDocument * doc;
				SPDesktop * dt;
				doc = sp_document_new ((const gchar *) fl->data);
				if (doc) {
					dt = sp_desktop_new (doc, sp_document_namedview (doc, NULL));
					sp_document_unref (doc);
					if (dt) sp_create_window (dt, TRUE);
				}
				fl = g_slist_remove (fl, fl->data);
			}
		} else {
			GSList *slides = NULL;
			SPDocument * doc;
			SPDesktop * dt;
			/* fixme: This is terrible hack */
			sodipodi = sodipodi_application_new ();
			sodipodi_load_preferences (sodipodi);
			gtk_signal_connect (GTK_OBJECT (sodipodi), "destroy",
					    GTK_SIGNAL_FUNC (main_save_preferences), NULL);
			sp_maintoolbox_create ();
			sodipodi_unref ();

			while (fl) {
				doc = sp_document_new ((const gchar *) fl->data);
				if (doc) {
					slides = g_slist_append (slides, doc);
				}
				fl = g_slist_remove (fl, fl->data);
			}

			if (slides) {
				doc = slides->data;
				slides = g_slist_remove (slides, doc);
				dt = sp_desktop_new (doc, sp_document_namedview (doc, NULL));
				if (dt) {
					sp_desktop_set_event_context (dt, SP_TYPE_SLIDE_CONTEXT);
					gtk_object_set_data (GTK_OBJECT (dt), "slides", slides);
					sp_create_window (dt, FALSE);
				}
#if 0
				sp_document_unref (doc);
#endif
			}

			sodipodi_unref ();
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
	const gchar ** args;
	gint i;

	fl = NULL;

#if 0
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
#endif
	args = poptGetArgs (ctx);
	if (args != NULL) {
		for (i = 0; args[i] != NULL; i++) {
			fl = g_slist_append (fl, (gpointer) args[i]);
		}
	}

	return fl;
}

