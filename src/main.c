/*
 * sodipodi - an ambitious vector drawing program
 * Copyright (c) Lauris Kaplinski 1999
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
#include "mdi.h"
#include "mdi-child.h"
#include "file.h"

#include "session.h"

/* TODO - allow multiple file's to be specifed at startup? */

/* Extract command line options which apply before we start up
 * gnome so we can't use gnome_init_with_popt_table for these
 * Would be nice to call the same mechanism as gnome uses to
 * avoid duplicating arg checking code. ??
 */
void extract_args (int argc, char *argv[],
		   gboolean *, gboolean *,
		   gchar **filename,
		   gchar **to_filename);

/* Command line options
 * -ffile filename to load at startup
 * -pfile print a file without starting GUI
 * -tfile file to print -p file to
 */

/* NOTE, use of extract_args results in these three variables not being used,
 * see again note about wanting to use gnome's command line cracking without
 * having to start GUI to allow for printing without an X environment
 */
static gchar *arg_filename = NULL;
static gchar *arg_print_file = NULL;
static gchar *arg_to_file = NULL;

struct poptOption options[] = {
  {
        NULL,
        'z',
        POPT_ARG_NONE,
        NULL,
        0,
        N_("Do not initialize GUI. NB! should be FIRST argument"),
        NULL
  },
  {
        "file",
        'f',
        POPT_ARG_STRING,
        &arg_filename,
        0,
        N_("Filename to load on startup"),
        N_("FILENAME")
  },
  {
        "print",
        'p',
        POPT_ARG_STRING,
        &arg_print_file,
        0,
        N_("Print file"),
        N_("FILENAME")
  },
  {
        "to",
        't',
        POPT_ARG_STRING,
        &arg_to_file,
        0,
        N_("Destination filename for print output"),
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

	gboolean sp_use_gui = TRUE;
	gboolean print_file = FALSE;
	gchar *filename = NULL;
	gchar *to_filename = NULL;

#ifdef __FreeBSD__
	fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

#if 0
	/* Check command line args before starting up gnome to avoid
	 * requiring X start-up when using sodipodi to print a file
	 */
	extract_args (argc, argv,
		      &sp_use_gui, &print_file, &filename, &to_filename);
#endif
	if (argc > 1) {
		if (strcmp (argv[1], "-z") == 0) {
			sp_use_gui = FALSE;
		}
	}

	if (sp_use_gui) {

		poptContext pctx;

		gnome_init_with_popt_table ("sodipodi", VERSION, argc, argv,
                                    	    options, 0, &pctx);
#if 0
		/* fixme: - leftover args */

		extract_args (argc, argv,
			&sp_use_gui, &print_file, &filename, &to_filename);
#endif

		/* Would normally check args here, but extract_args has
		 * already done that, and we just needed to use
		 * gnome_init_with_popt_table to avoid gnome complaining
		 * about invalid options
		 */

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

		flags = gnome_client_get_flags (client);
		if (flags & GNOME_CLIENT_RESTORED) {
			prefix = gnome_client_get_config_prefix (client);
			gnome_config_push_prefix (prefix);
			sp_sm_restore_children ();
			gnome_config_pop_prefix ();
		}

		/* fixme: */

		if (filename == NULL) {

			doc = sp_document_new ();

		} else {

			doc = sp_document_new_from_file (filename);
			if (doc == NULL) {
                                g_print("%s : File not found\n", filename);
                                doc = sp_document_new ();
                        }
		}

		g_assert (doc != NULL);

		child = sp_mdi_child_new (doc);

		gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
		gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));

		/* fixme: Is this right? */

		poptFreeContext (pctx);

		gtk_main ();

	} else {

		/* fixme: */

		extract_args (argc, argv,
			&sp_use_gui, &print_file, &filename, &to_filename);

		/* printing a file */

		/* Start up gtk, without requiring X */

		gtk_type_init();

		setlocale (LC_NUMERIC, "C");

                doc = sp_document_new_from_file (filename);
                g_assert (doc != NULL);

                if (to_filename == NULL) {
			/* sp_do_file_print (doc); */
			g_print("SORRY, can't send directly to printer\n");
			g_print("Add -t file.ps to dump to postscript\n");
			/* How do you get a GnomePrinter without getting a
			 * dialog box...which we can't get bacause we haven't
			 * started the GUI ???
			 */
                } else {
                        sp_do_file_print_to_file (doc, to_filename);
                }
	}

#ifdef __FreeBSD__
	fpresetsticky(FP_X_DZ|FP_X_INV);
	fpsetmask(FP_X_DZ|FP_X_INV);
#endif
	return 0;
}

/* Look for -f load_filename, -p print_filename, and -t print_to_file 
 * args
 */

void
extract_args (int argc, char *argv[],
	      gboolean *use_gui, gboolean *print_file,
	      gchar **filename, gchar **to_filename)
{ 
	int ch;
	int count = 0;

  	*use_gui = TRUE;
  	*print_file = FALSE;
  	*filename = NULL;
  	*to_filename = NULL;

	while ((ch = getopt(argc,argv,"p:f:t:z")) != EOF)  {

		count++;

		switch (ch) {

		case (int) 'p' :
                        if (optarg == NULL) {
                                fprintf (stderr,
                                         "Filename required with -p\n");
                        } else {
                                *filename = g_strdup (optarg);
				*print_file = TRUE;
                        }

		break;

		case (int) 'f' :
			if (optarg == NULL) {
				fprintf (stderr, 
					 "Filename required with -f\n");
			} else {
				*filename = g_strdup (optarg);
			}
		break;

		case (int) 't' :
			if (optarg == NULL) {
				fprintf (stderr, 
					 "Filename required with -t\n");
			} else {
				*to_filename = g_strdup (optarg);
			}
		break;

		default:
		break;
		}
	}

	/* TODO: Should really check for -f file and -p file conflict */

	/* No GUI required when printing */
	if (*print_file == TRUE) {
		*use_gui = FALSE;
	}

	/* If no args found, but one was specified, take the first as
	 * a filename. eg ./sodipodi file.svg
	 */
	if (count == 0 && argc > 1) {
		*filename = g_strdup (argv[1]);
	}
}
