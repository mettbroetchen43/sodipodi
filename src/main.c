#define __MAIN_C__

/*
 * Sodipodi - an ambitious vector drawing program
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Davide Puricelli <evo@debian.org>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Masatake YAMATO  <jet@gyve.org>
 *   F.J.Franklin <F.J.Franklin@sheffield.ac.uk>
 *   Michael Meeks <michael@helixcode.com>
 *   Chema Celorio <chema@celorio.com>
 *   Pawel Palucha
 * ... and various people who have worked with various projects
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include <config.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif
#include <string.h>

#include <popt.h>
#include <tree.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <glade/glade.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-init.h>
#include <libgnomeui/gnome-window-icon.h>

#ifdef ENABLE_BONOBO
#include <liboaf/liboaf.h>
#include "bonobo/sodipodi-bonobo.h"
#include "bonobo/svg-doc-factory.h"
#endif

#include "file.h"
#include "document.h"
#include "toolbox.h"
#include "interface.h"

#if 1
/* fixme: This are required for temporary slideshow hack */
#include "slide-context.h"
#endif

#if 1
/* fixme: Ths is needed for export, remove if we implement generic helper */
#include <math.h>
#include "helper/png-write.h"
#include "sp-item.h"
#endif

#include "sodipodi-private.h"

enum {
	SP_ARG_NONE,
	SP_ARG_NOGUI,
	SP_ARG_GUI,
	SP_ARG_FILE,
	SP_ARG_PRINT,
	SP_ARG_EXPORT_PNG,
	SP_ARG_EXPORT_DPI,
	SP_ARG_EXPORT_AREA,
	SP_ARG_SLIDESHOW,
	SP_ARG_LAST
};

int sp_main_gui (int argc, char **argv);
int sp_main_console (int argc, char **argv);
static GSList *sp_process_args (poptContext ctx);

static guchar *sp_global_printer = NULL;
static gboolean sp_global_slideshow = FALSE;
static guchar *sp_export_png = NULL;
static guchar *sp_export_dpi = NULL;
static guchar *sp_export_area = NULL;

struct poptOption options[] = {
	{"without-gui", 'z', POPT_ARG_NONE, NULL, SP_ARG_NOGUI,
	 N_("Do not use X server (only process files from console)"),
	 NULL},
	{"with-gui", 'x', POPT_ARG_NONE, NULL, SP_ARG_GUI,
	 N_("Try to use X server even if $DISPLAY is not set)"),
	 NULL},
	{"file", 'f', POPT_ARG_STRING, NULL, SP_ARG_FILE,
	 N_("Open specified document(s) (option string may be excluded)"),
	 N_("FILENAME")},
	{"print", 'p', POPT_ARG_STRING, &sp_global_printer, SP_ARG_PRINT,
	 N_("Print document(s) to specified output file (use '| program' for pipe)"),
	 N_("FILENAME")},
	{"export-png", 'e', POPT_ARG_STRING, &sp_export_png, SP_ARG_EXPORT_PNG,
	 N_("Export document to png file"),
	 N_("FILENAME")},
	{"export-dpi", 'e', POPT_ARG_STRING, &sp_export_dpi, SP_ARG_EXPORT_DPI,
	 N_("Exported bitmap resolution in pixels per inch (default 72.0)"),
	 N_("DPI")},
	{"export-area", 'e', POPT_ARG_STRING, &sp_export_area, SP_ARG_EXPORT_AREA,
	 N_("Exported area in millimeters (default is full document)"),
	 N_("x0:y0:x1:y1")},
	{"slideshow", 's', POPT_ARG_NONE, &sp_global_slideshow, SP_ARG_SLIDESHOW,
	 N_("Show given files one-by-one, switch to next on any key/mouse event"),
	 NULL},
	{NULL, '\0', 0, NULL, 0, NULL, NULL}
};

static void
main_save_preferences (GtkObject * object, gpointer data)
{
	sodipodi_save_preferences (SP_SODIPODI (object));
}

int
main (int argc, char **argv)
{
	gboolean use_gui;
	gint result, i;

#ifdef __FreeBSD__
	fpsetmask(fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);

	LIBXML_TEST_VERSION

	/*
	 * We must set LC_NUMERIC to default, or otherwise
	 * we'll end with localised SVG files :-(
	 */

	setlocale (LC_NUMERIC, "C");
	use_gui = (getenv ("DISPLAY") != NULL);

	/* Test whether with/without GUI is forced */
	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-z") ||
		    !strcmp (argv[i], "--without-gui") ||
		    !strcmp (argv[i], "-p") ||
		    !strncmp (argv[i], "--print", 7) ||
		    !strcmp (argv[i], "-e") ||
		    !strncmp (argv[i], "--export-png", 12)) {
			use_gui = FALSE;
			break;
		} else if (!strcmp (argv[i], "-x") || !strcmp (argv[i], "--with-gui")) {
			use_gui = TRUE;
			break;
		}
	}

	if (use_gui) {
		result = sp_main_gui (argc, argv);
	} else {
		result = sp_main_console (argc, argv);
	}

#ifdef __FreeBSD__
	fpresetsticky(FP_X_DZ|FP_X_INV);
	fpsetmask(FP_X_DZ|FP_X_INV);
#endif
	return result;
}

int
sp_main_gui (int argc, char **argv)
{
	poptContext ctx = NULL;
	GSList *fl;

	/* Setup gnome */
	gnome_init_with_popt_table ("sodipodi", VERSION, argc, argv, options, 0, &ctx);
	/* Collect own arguments */
	fl = sp_process_args (ctx);
	/* Setup libglade */
	glade_gnome_init ();

	/* Set default icon */
	if (g_file_test (GNOME_ICONDIR "/sodipodi.png", G_FILE_TEST_ISFILE | G_FILE_TEST_ISLINK)) {
		gnome_window_icon_set_default_from_file (GNOME_ICONDIR "/sodipodi.png");
	} else {
		g_warning ("Could not find %s", GNOME_ICONDIR "/sodipodi.png");
	}

	if (!sp_global_slideshow) {
		sodipodi = sodipodi_application_new ();
		sodipodi_load_preferences (sodipodi);
		gtk_signal_connect (GTK_OBJECT (sodipodi), "destroy", GTK_SIGNAL_FUNC (main_save_preferences), NULL);
		sp_maintoolbox_create ();
		sodipodi_unref ();

		while (fl) {
			SPDocument *doc;
			SPViewWidget *dtw;
			doc = sp_document_new ((const gchar *) fl->data);
			if (doc) {
				dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
				sp_document_unref (doc);
				if (dtw) sp_create_window (dtw, TRUE);
			}
			fl = g_slist_remove (fl, fl->data);
		}
	} else {
		GSList *slides = NULL;
		SPDocument * doc;
		SPViewWidget *dtw;
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
			dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
			if (dtw) {
				sp_desktop_set_event_context (SP_DESKTOP_WIDGET (dtw)->desktop, SP_TYPE_SLIDE_CONTEXT);
				gtk_object_set_data (GTK_OBJECT (SP_DESKTOP_WIDGET (dtw)->desktop), "slides", slides);
				sp_create_window (dtw, FALSE);
			}
		}
		
		sodipodi_unref ();
	}

	poptFreeContext (ctx);

	gtk_main ();

	return 0;
}

int
sp_main_console (int argc, char **argv)
{
	poptContext ctx = NULL;
	GSList * fl;
	guchar *printer;
	ArtDRect area;
	gdouble dpi;
	gboolean has_area;

	/* We are started in text mode */

	ctx = poptGetContext (NULL, argc, (const char **) argv, options, 0);
	g_return_val_if_fail (ctx != NULL, 1);

	fl = sp_process_args (ctx);

	poptFreeContext (ctx);

	if (fl == NULL) {
		g_print ("Nothing to do!\n");
		exit (0);
	}

	/* Check for and set up printing path */
	printer = NULL;
	if (sp_global_printer != NULL) {
		if ((sp_global_printer[0] != '|') && (sp_global_printer[0] != '/')) {
			gchar *cwd;
			/* Gnome-print appends relative paths to $HOME by default */
			cwd = g_get_current_dir ();
			printer = g_concat_dir_and_file (cwd, sp_global_printer);
			g_free (cwd);
		} else {
			printer = g_strdup (sp_global_printer);
		}
	}

	/* Check for and set up exporting path */
	has_area = FALSE;
	dpi = 72.0;
	if (sp_export_png) {
		if (sp_export_dpi) {
			dpi = atof (sp_export_dpi);
			if ((dpi < 0.1) || (dpi > 10000.0)) {
				g_warning ("DPI value %s out of range [0.1 - 10000.0]", sp_export_dpi);
				sp_export_png = NULL;
			}
			g_print ("dpi is %g\n", dpi);
		}
		if (sp_export_area) {
			if (sscanf (sp_export_area, "%lg:%lg:%lg:%lg", &area.x0, &area.y0, &area.x1, &area.y1) == 4) {
				area.x0 *= (72.0 / 25.4);
				area.y0 *= (72.0 / 25.4);
				area.x1 *= (72.0 / 25.4);
				area.y1 *= (72.0 / 25.4);
				has_area = TRUE;
			} else {
				g_warning ("Export area '%s' illegal (use 'x0:y0:x1:y1)", sp_export_area);
			}
		}
	}

	/* Start up gtk, without requiring X */
	gtk_type_init();
	sodipodi = sodipodi_application_new ();

	while (fl) {
		SPDocument *doc;
		doc = sp_document_new ((gchar *) fl->data);
		if (doc == NULL) {
			g_warning ("Specified document %s cannot be opened (is it valid SVG file?)", (gchar *) fl->data);
		} else {
			if (printer) {
				sp_do_file_print_to_file (doc, printer);
			}
			if (sp_export_png) {
				gint width, height;
				if (!has_area) {
					area.x0 = 0.0;
					area.x1 = 0.0;
					area.x1 = sp_document_width (doc);
					area.y1 = sp_document_height (doc);
				}
				width = (gint) floor ((area.x1 - area.x0) * dpi / 72.0 + 0.5);
				height = (gint) floor ((area.y1 - area.y0) * dpi / 72.0 + 0.5);
				g_print ("Width %d height %d\n", width, height);
				if ((width >= 16) || (height >= 16) || (width < 65536) || (height < 65536)) {
					ArtPixBuf *pixbuf;
					art_u8 *pixels;
					gdouble affine[6], t;
					/* fixme: Move this to helper */
					pixels = art_new (art_u8, width * height * 4);
					memset (pixels, 0, width * height * 4);
					pixbuf = art_pixbuf_new_rgba (pixels, width, height, width * 4);

					/* Go to document coordinates */
					t = area.y0;
					area.y0 = sp_document_height (doc) - area.y1;
					area.y1 = sp_document_height (doc) - t;

					/* In document coordinates
					 * 1) a[0] * x0 + a[2] * y0 + a[4] = 0.0
					 * 2) a[1] * x0 + a[3] * y0 + a[5] = 0.0
					 * 3) a[0] * x1 + a[2] * y0 + a[4] = width
					 * 4) a[1] * x0 + a[3] * y1 + a[5] = height
					 * 5) a[1] = 0.0;
					 * 6) a[2] = 0.0;
					 *
					 * (1,3) a[0] * x1 - a[0] * x0 = width
					 * a[0] = width / (x1 - x0)
					 * (2,4) a[3] * y1 - a[3] * y0 = height
					 * a[3] = height / (y1 - y0)
					 * (1) a[4] = -a[0] * x0
					 * (2) a[5] = -a[3] * y0
					 */

					affine[0] = width / (area.x1 - area.x0);
					affine[1] = 0.0;
					affine[2] = 0.0;
					affine[3] = height / (area.y1 - area.y0);
					affine[4] = -affine[0] * area.x0;
					affine[5] = -affine[3] * area.y0;

					sp_item_paint (SP_ITEM (sp_document_root (doc)), pixbuf, affine);

					sp_png_write_rgba (sp_export_png, pixbuf);

					art_pixbuf_free (pixbuf);
				} else {
					g_warning ("Calculated bitmap dimensions %d %d out of range (16 - 65535)", width, height);
				}
			}
		}
		fl = g_slist_remove (fl, fl->data);
	}

	if (printer) g_free (printer);

	sodipodi_unref ();

	return 0;
}

#if 0
int
main (int argc, char **argv)
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

		ctx = poptGetContext (NULL, argc, (const char **) argv, options, 0);
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

		if (g_file_test (GNOME_ICONDIR "/sodipodi.png", G_FILE_TEST_ISFILE | G_FILE_TEST_ISLINK)) {
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
			gtk_signal_connect (GTK_OBJECT (sodipodi), "destroy", GTK_SIGNAL_FUNC (main_save_preferences), NULL);
			sp_maintoolbox_create ();
			sodipodi_unref ();

			while (fl) {
				SPDocument * doc;
				SPViewWidget *dtw;
				doc = sp_document_new ((const gchar *) fl->data);
				if (doc) {
					dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
					sp_document_unref (doc);
					if (dtw) sp_create_window (dtw, TRUE);
				}
				fl = g_slist_remove (fl, fl->data);
			}
		} else {
			GSList *slides = NULL;
			SPDocument * doc;
			SPViewWidget *dtw;
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
				dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
				if (dtw) {
					sp_desktop_set_event_context (SP_DESKTOP_WIDGET (dtw)->desktop, SP_TYPE_SLIDE_CONTEXT);
					gtk_object_set_data (GTK_OBJECT (SP_DESKTOP_WIDGET (dtw)->desktop), "slides", slides);
					sp_create_window (dtw, FALSE);
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
#endif

static GSList *
sp_process_args (poptContext ctx)
{
	GSList * fl;
	const gchar ** args, *fn;
	gint i, a;

	fl = NULL;

#if 1
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

