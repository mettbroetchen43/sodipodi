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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif
#include <string.h>
#include <signal.h>

#ifdef WITH_POPT
#include <popt.h>
#ifndef POPT_TABLEEND
#define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif /* Not def: POPT_TABLEEND */
#endif

#include <libxml/tree.h>
#include <glib-object.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkbox.h>

#include "macros.h"
#include "system.h"
#include "file.h"
#include "document.h"
#include "desktop.h"
#include "sp-object.h"
#include "toolbox.h"
#include "interface.h"
#include "print.h"
#include "slideshow.h"

#include "svg/svg.h"

#include "sodipodi-private.h"

#include "sp-namedview.h"
#include "sp-guide.h"
#include "sp-object-repr.h"

#include "module.h"

#include "helper/sp-intl.h"

#ifndef HAVE_BIND_TEXTDOMAIN_CODESET
#define bind_textdomain_codeset(p,c)
#endif
#ifndef HAVE_GTK_WINDOW_SET_DEFAULT_ICON_FROM_FILE
#define gtk_window_set_default_icon_from_file(f,v)
#endif

#ifdef WITH_POPT
enum {
	SP_ARG_NONE,
	SP_ARG_NOGUI,
	SP_ARG_GUI,
	SP_ARG_FILE,
	SP_ARG_PRINT,
	SP_ARG_EXPORT_PNG,
	SP_ARG_EXPORT_DPI,
	SP_ARG_EXPORT_AREA,
	SP_ARG_EXPORT_WIDTH,
	SP_ARG_EXPORT_HEIGHT,
	SP_ARG_EXPORT_BACKGROUND,
	SP_ARG_EXPORT_SVG,
	SP_ARG_SLIDESHOW,
	SP_ARG_BITMAP_ICONS,
	SP_ARG_LAST
};
#endif

int sp_main_gui (int argc, const char **argv);
int sp_main_console (int argc, const char **argv);
static void sp_do_export_png (SPDocument *doc);

/* fixme: We need this non-static, but better arrange it another way (Lauris) */
gboolean sp_bitmap_icons = FALSE;

static unsigned int stop = FALSE;
static guchar *sp_global_printer = NULL;
static gboolean sp_global_slideshow = FALSE;
static const unsigned char *sp_export_png = NULL;
static const unsigned char *sp_export_dpi = NULL;
static const unsigned char *sp_export_area = NULL;
static const unsigned char *sp_export_width = NULL;
static const unsigned char *sp_export_height = NULL;
static const unsigned char *sp_export_background = NULL;
static const unsigned char *sp_export_svg = NULL;

#ifdef WITH_POPT
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
	{"export-dpi", 'd', POPT_ARG_STRING, &sp_export_dpi, SP_ARG_EXPORT_DPI,
	 N_("The resolution used for converting SVG into bitmap (default 72.0)"),
	 N_("DPI")},
	{"export-area", 'a', POPT_ARG_STRING, &sp_export_area, SP_ARG_EXPORT_AREA,
	 N_("Exported area in millimeters (default is full document, 0,0 is lower-left corner)"),
	 N_("x0:y0:x1:y1")},
	{"export-width", 'w', POPT_ARG_STRING, &sp_export_width, SP_ARG_EXPORT_WIDTH,
	 N_("The width of generated bitmap in pixels (overwrites dpi)"), N_("WIDTH")},
	{"export-height", 't', POPT_ARG_STRING, &sp_export_height, SP_ARG_EXPORT_HEIGHT,
	 N_("The height of generated bitmap in pixels (overwrites dpi)"), N_("HEIGHT")},
	{"export-background", 'b', POPT_ARG_STRING, &sp_export_background, SP_ARG_EXPORT_HEIGHT,
	 N_("Background color of exported bitmap (any SVG supported color string)"), N_("COLOR")},
	{"export-svg", 0, POPT_ARG_STRING, &sp_export_svg, SP_ARG_EXPORT_SVG,
	 N_("Export document to plain SVG file (no \"xmlns:sodipodi\" namespace)"), N_("FILENAME")},
	{"slideshow", 's', POPT_ARG_NONE, &sp_global_slideshow, SP_ARG_SLIDESHOW,
	 N_("Show given files one-by-one, switch to next on any key/mouse event"), NULL},
	{"bitmap-icons", 'i', POPT_ARG_NONE, &sp_bitmap_icons, SP_ARG_BITMAP_ICONS,
	 N_("Prefer bitmap (xpm) icons to SVG ones"),
	 NULL},
	POPT_AUTOHELP POPT_TABLEEND
};
#endif

static GSList *sp_process_args (int argc, const char **argv);

#ifndef WIN32
/* Private segv handler */
static void sodipodi_segv_handler (int signum);
static void (* segv_handler) (int) = NULL;
#endif

int
main (int argc, const char **argv)
{
	gboolean use_gui;
	gint result, i;

#ifdef __FreeBSD__
	fpsetmask (fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	/* We must set LC_NUMERIC to default, or otherwise */
	/* we'll end with localised SVG files :-( */
	/* setlocale (LC_ALL, "C"); */

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	LIBXML_TEST_VERSION

#ifndef WIN32
	use_gui = (getenv ("DISPLAY") != NULL);
#else
	use_gui = TRUE;
#endif
	/* Test whether with/without GUI is forced */
	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-z") ||
		    !strcmp (argv[i], "--without-gui") ||
		    !strcmp (argv[i], "-p") ||
		    !strncmp (argv[i], "--print", 7) ||
		    !strcmp (argv[i], "-e") ||
		    !strncmp (argv[i], "--export-png", 12) ||
		    !strncmp (argv[i], "--export-svg", 12) ||
		    !strcmp (argv[i], "-v") ||
		    !strcmp (argv[i], "--version") ||
		    !strcmp (argv[i], "-h") ||
		    !strcmp (argv[i], "--help")) {
			use_gui = FALSE;
			break;
		} else if (!strcmp (argv[i], "-x") ||
			   !strcmp (argv[i], "--with-gui") ||
			   !strncmp (argv[i], "--display", 9)) {
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
sp_main_gui (int argc, const char **argv)
{
	GSList *fl = NULL;

#ifdef _DEBUG
	g_print ("This is debug console. It can be minimized, but do not close it\n");
	g_print ("Initializing Gtk+...\n");
#endif
	gtk_init (&argc, (char ***) &argv);
#ifdef _DEBUG
	g_print ("Creating new application...\n");
#endif
	sodipodi_application_new ();
#ifdef _DEBUG
	g_print ("Initializing modules...\n");
#endif
	sp_modules_init (&argc, argv, TRUE);

#ifdef _DEBUG
	g_print ("Registering objects...\n");
#endif
	/* fixme: Move these to some centralized location (Lauris) */
	sp_object_type_register ("sodipodi:namedview", SP_TYPE_NAMEDVIEW);
	sp_object_type_register ("sodipodi:guide", SP_TYPE_GUIDE);

#ifdef _DEBUG
	g_print ("Collecting arguments...\n");
#endif
	fl = sp_process_args (argc, argv);
	if (stop) exit (1);

#ifdef DATADIR
	/* Set default icon */
	/* fixme: use system.h */
	if (g_file_test (DATADIR "/pixmaps/sodipodi.png", G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_SYMLINK)) {
		gtk_window_set_default_icon_from_file (DATADIR "/pixmaps/sodipodi.png", NULL);
	}
#endif

	if (!sp_global_slideshow) {
#ifdef _DEBUG
	g_print ("Loading preferences...\n");
#endif
		sodipodi_load_preferences (sodipodi);
#ifdef _DEBUG
	g_print ("Loading extensions...\n");
#endif
		sodipodi_load_extensions (sodipodi);
#ifdef _DEBUG
	g_print ("Creating main toolbox...\n");
#endif
		sp_maintoolbox_create_toplevel ();
#ifdef _DEBUG
	g_print ("Unrefing application...\n");
#endif
		sodipodi_unref ();

		while (fl) {
			SPDocument *doc;
			doc = sodipodi_document_new ((const unsigned char *) fl->data, TRUE, TRUE);
			if (doc) {
				if (sp_export_png) {
					sp_do_export_png (doc);
				} else {
					SPViewWidget *dtw;
					dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
					if (dtw) sp_create_window (dtw, TRUE);
				}
				sp_document_unref (doc);
			}
			fl = g_slist_remove (fl, fl->data);
		}
#ifndef WIN32
		/* We have opened all documents and ready to enter main loop */
		segv_handler = signal (SIGSEGV, sodipodi_segv_handler);
		signal (SIGFPE, sodipodi_segv_handler);
		signal (SIGILL, sodipodi_segv_handler);
#endif
	} else {
		if (fl) {
			GtkWidget *ss;
			sodipodi_load_preferences (sodipodi);
			sodipodi_load_extensions (sodipodi);
			ss = sp_slideshow_new (fl);
			if (ss) gtk_widget_show (ss);
			sodipodi_unref ();
		} else {
#ifdef VERBOSE
			fprintf (stderr, "No slides to display\n");
#endif
			exit (0);
		}
	}

	/* sp_modules_menu_about_new (); */
#ifdef _DEBUG
	g_print ("Entering main loop...\n");
#endif
	gtk_main ();

	return 0;
}

int
sp_main_console (int argc, const char **argv)
{
	GSList * fl = NULL;

	/* We are started in text mode */
	g_type_init ();

#ifndef WITH_FONTCONFIG
	/* Still have to init gdk, or Xft does not work */
	gdk_init (&argc, (char ***) &argv);
#endif

	fl = sp_process_args (argc, argv);
	if (stop) exit (1);

	/* Start up g type system, without requiring X, do this before modules init */
        g_type_init();
        sodipodi_application_new();

#ifdef _DEBUG
	g_print ("Initializing modules...\n");
#endif
	sp_modules_init (&argc, argv, TRUE);

#ifdef _DEBUG
	g_print ("Registering objects...\n");
#endif
	/* fixme: Move these to some centralized location (Lauris) */
	sp_object_type_register ("sodipodi:namedview", SP_TYPE_NAMEDVIEW);
	sp_object_type_register ("sodipodi:guide", SP_TYPE_GUIDE);

	if (fl == NULL) {
		g_print ("Nothing to do!\n");
		exit (0);
	}

	sodipodi_load_preferences (sodipodi);
	sodipodi_load_extensions (sodipodi);

	while (fl) {
		SPDocument *doc;
		doc = sodipodi_document_new ((const unsigned char *) fl->data, FALSE, TRUE);
		if (doc == NULL) {
			g_warning ("Specified document %s cannot be opened (is it valid SVG file?)", (gchar *) fl->data);
		} else {
			if (sp_global_printer) {
                                sp_print_document(doc, FALSE, sp_global_printer);
			}
			if (sp_export_png) {
				sp_do_export_png (doc);
			}
			if (sp_export_svg) {
				SPReprDoc *rdoc;
				SPRepr *repr;
				rdoc = sp_repr_doc_new ("svg");
				repr = sp_repr_doc_get_root (rdoc);
				repr = sp_object_invoke_write (sp_document_root (doc), repr, SP_OBJECT_WRITE_BUILD);
				sp_repr_doc_write_file (sp_repr_get_doc (repr), sp_export_svg);
			}
		}
		fl = g_slist_remove (fl, fl->data);
	}

	sodipodi_unref ();

	return 0;
}

static void
sp_do_export_png (SPDocument *doc)
{
	ArtDRect area;
	gdouble dpi;
	gboolean has_area;
	gint width, height;
	guint32 bgcolor;

	/* Check for and set up exporting path */
	has_area = FALSE;
	dpi = 72.0;

	if (sp_export_dpi) {
		dpi = atof (sp_export_dpi);
		if ((dpi < 0.1) || (dpi > 10000.0)) {
			g_warning ("DPI value %s out of range [0.1 - 10000.0]", sp_export_dpi);
			return;
		}
		g_print ("dpi is %g\n", dpi);
	}

	if (sp_export_area) {
		/* Try to parse area (given in mm) */
		if (sscanf (sp_export_area, "%lg:%lg:%lg:%lg", &area.x0, &area.y0, &area.x1, &area.y1) == 4) {
			area.x0 *= (72.0 / 25.4);
			area.y0 *= (72.0 / 25.4);
			area.x1 *= (72.0 / 25.4);
			area.y1 *= (72.0 / 25.4);
			has_area = TRUE;
		} else {
			g_warning ("Export area '%s' illegal (use 'x0:y0:x1:y1)", sp_export_area);
			return;
		}
		if ((area.x0 >= area.x1) || (area.y0 >= area.y1)) {
			g_warning ("Export area '%s' has invalid values", sp_export_area);
			return;
		}
	} else {
		/* Export the whole document */
		area.x0 = 0.0;
		area.y0 = 0.0;
		area.x1 = sp_document_width (doc);
		area.y1 = sp_document_height (doc);
	}

	/* Kill warning */
	width = 0;
	height = 0;

	if (sp_export_width) {
		width = atoi (sp_export_width);
		if ((width < 1) || (width > 65536)) {
			g_warning ("Export width %d out of range (1 - 65536)", width);
			return;
		}
		dpi = (gdouble) width * 72.0 / (area.x1 - area.x0);
	}

	if (sp_export_height) {
		height = atoi (sp_export_height);
		if ((height < 1) || (height > 65536)) {
			g_warning ("Export height %d out of range (1 - 65536)", width);
			return;
		}
		dpi = (gdouble) height * 72.0 / (area.y1 - area.y0);
	}

	if (!sp_export_width) {
		width = (gint) ((area.x1 - area.x0) * dpi / 72.0 + 0.5);
	}

	if (!sp_export_height) {
		height = (gint) ((area.y1 - area.y0) * dpi / 72.0 + 0.5);
	}

	bgcolor = 0x00000000;
	if (sp_export_background) {
		bgcolor = sp_svg_read_color (sp_export_background, 0xffffff00);
		bgcolor |= 0xff;
	}
	g_print ("Background is %x\n", bgcolor);

	g_print ("Exporting %g %g %g %g to %d x %d rectangle\n", area.x0, area.y0, area.x1, area.y1, width, height);

	if ((width >= 1) || (height >= 1) || (width < 65536) || (height < 65536)) {
		sp_export_png_file (doc, sp_export_png, area.x0, area.y0, area.x1, area.y1, width, height, bgcolor, NULL, NULL);
	} else {
		g_warning ("Calculated bitmap dimensions %d %d out of range (16 - 65535)", width, height);
	}
}

#ifdef WITH_POPT
static GSList *
sp_process_args (int argc, const char **argv)
{
	poptContext ctx;
	GSList * fl;
	const gchar ** args, *fn;
	gint i, a;

	ctx = poptGetContext (NULL, argc, argv, options, 0);
	if (!ctx) {
		stop = TRUE;
		return NULL;
	}

	fl = NULL;

	while ((a = poptGetNextOpt (ctx)) >= 0) {
		switch (a) {
		case SP_ARG_FILE:
			fn = poptGetOptArg (ctx);
			if (fn != NULL) {
				fl = g_slist_append (fl, g_strdup (fn));
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

	poptFreeContext (ctx);

	return fl;
}
#else

static int
startcmp (const char *s, const char *t)
{
	int lens, lent, len;
	lens = strlen (s);
	lent = strlen (t);
	len = MIN (lens, lent);
	return strncmp (s, t, len);
}

/* Returns TRUE if parsing is successful */
/* Sets aidx to -1 on error */
static unsigned int
parsearg (int *aidx, int argc, const char **argv, int shortarg, const char *longarg, const char **argval)
{
	const char *curarg;
	unsigned int isthis;
	curarg = argv[*aidx];
	isthis = FALSE;
	if (curarg[0] == '-') {
		if ((curarg[1] == shortarg) && !curarg[2]) {
			/* Is short arg */
			if (argval) {
				/* Needs parameter */
				if ((*aidx + 1) < argc) {
					/* Has parameter */
					*aidx += 1;
					*argval = argv[*aidx];
					isthis = TRUE;
					*aidx += 1;
				} else {
					/* Missing parameter */
					*aidx = -1;
					return FALSE;
				}
			} else {
				/* Parameter not needed */
				isthis = TRUE;
				*aidx += 1;
			}
		} else if ((curarg[1] == '-') && curarg[2]) {
			unsigned int len;
			/* Long arg */
			len = strlen (longarg);
			if (!strncmp (curarg + 2, longarg, len)) {
				/* Is long arg */
				if (argval) {
					/* Needs parameter */
					if (curarg[2 + len] == '=') {
						/* Really is this */
						if (curarg[2 + len + 1]) {
							/* Has parameter */
							*argval = curarg + 2 + len + 1;
							isthis = TRUE;
							*aidx += 1;
						} else {
							/* Missing parameter */
							*aidx = -1;
							return FALSE;
						}
					}
					/* Otherwise is not this */
				} else {
					// Parameter not needed
					isthis = TRUE;
					*aidx += 1;
				}
			}
			/* Otherwise is not this */
		}
	}
	return isthis;
}

static void
printusage (void)
{
	g_print ("Usage: sodipodi [options] [files]\n");
	g_print ("Options:\n");
	g_print ("%s, %s %s\n", "-z", "--without-gui", N_("Do not use X server (only process files from console)"));
	g_print ("%s, %s %s\n", "-x", "--with-gui", N_("Try to use X server even if $DISPLAY is not set)"));
	g_print ("%s, %s %s\n", "-f", "--file=FILE", N_("Open specified document(s) (option string may be excluded)"));
	g_print ("%s, %s %s\n", "-e", "--export-png=FILE", N_("Export document to png file"));
	g_print ("%s, %s %s\n", "-d", "--export-dpi=DPI", N_("The resolution used for converting SVG into bitmap (default 72.0)"));
	g_print ("%s, %s %s\n", "-a", "--export-area=x0:y0:x1:y1", N_("Exported area in millimeters (default is full document, 0,0 is lower-left corner)"));
	g_print ("%s, %s %s\n", "-w", "--export-width=WIDTH", N_("The width of generated bitmap in pixels (overwrites dpi)"));
	g_print ("%s, %s %s\n", "-t", "--export-height=HEIGHT", N_("The height of generated bitmap in pixels (overwrites dpi)"));
	g_print ("%s, %s %s\n", "-b", "--export-background=COLOR", N_("Background color of exported bitmap (any SVG supported color string)"));
	g_print ("%s, %s %s\n", "  ", "--export-svg=FILE", N_("Export document to plain SVG file (no \"xmlns:sodipodi\" namespace)"));
	g_print ("%s, %s %s\n", "  ", "--version", N_("Print sodipodi version information"));
	g_print ("%s, %s %s\n", "-h", "--help", N_("Print help about command-line arguments"));
}

static GSList *
sp_process_args (int argc, const char **argv)
{
	GSList *filelist;
	int aidx;

	filelist = NULL;
	aidx = 1;

	while ((aidx >= 0) && (aidx < argc)) {
		const char *arg, *val;
		arg = argv[aidx];
		if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'z', "without-gui", NULL)) {
			/* --without-gui */
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'x', "with-gui", NULL)) {
			/* --with-gui */
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'f', "file", &val)) {
			/* --file */
			filelist = g_slist_append (filelist, (gpointer) val);
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'e', "export-png", &val)) {
			/* --export-png */
			sp_export_png = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'd', "export-dpi", &val)) {
			/* --export-dpi */
			sp_export_dpi = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'a', "export-area", &val)) {
			/* --export-area */
			sp_export_area = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'w', "export-width", &val)) {
			/* --export-width */
			sp_export_width = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 't', "export-height", &val)) {
			/* --export-height */
			sp_export_height = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'b', "export-background", &val)) {
			/* --export-background */
			sp_export_background = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'b', "export-svg", &val)) {
			/* --export-svg */
			sp_export_svg = val;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'y', "gtk-dialogs", NULL)) {
			/* --gtk-dialogs */
			use_gtk_dialogs = TRUE;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'h', "help", NULL)) {
			/* --help */
			printusage ();
			stop = TRUE;
		} else if ((aidx >= 0) && parsearg (&aidx, argc, argv, 'v', "version", NULL)) {
			/* --help */
			g_print ("Sodipodi version %s\n", VERSION);
			stop = TRUE;
		} else if (aidx >= 0) {
			filelist = g_slist_append (filelist, (gpointer) arg);
			aidx += 1;
		} else {
			g_print ("Invalid optio(s) specified\n");
			printusage ();
			stop = TRUE;
			break;
		}
	}
	return filelist;
}

#endif

#ifndef WIN32

#include <time.h>
#include <ctype.h>
#include <gtk/gtkmessagedialog.h>
#include "sodipodi-private.h"

#define SP_INDENT 8

static void
sodipodi_segv_handler (int signum)
{
	static gint recursion = FALSE;
	GSList *savednames, *failednames;
	const GSList *l;
	const gchar *home;
	gchar *istr, *sstr, *fstr, *b;
	gint count, nllen, len, pos;
	time_t sptime;
	struct tm *sptm;
	char sptstr[256];
	GtkWidget *msgbox;

	/* Kill loops */
	if (recursion) abort ();
	recursion = TRUE;

	g_warning ("Emergency save activated");

	home = g_get_home_dir ();
	sptime = time (NULL);
	sptm = localtime (&sptime);
	strftime (sptstr, 256, "%Y_%m_%d_%H_%M_%S", sptm);

	count = 0;
	savednames = NULL;
	failednames = NULL;
	for (l = sodipodi_get_document_list (); l != NULL; l = l->next) {
		SPDocument *doc;
		SPRepr *repr;
		doc = (SPDocument *) l->data;
		repr = sp_document_repr_root (doc);
		if (sp_repr_get_attr (repr, "sodipodi:modified")) {
			const guchar *docname, *d0, *d;
			gchar n[64], c[1024];
			FILE *file;
			docname = doc->name;
			if (docname) {
				/* fixme: Quick hack to remove emergency file suffix */
				d0 = strrchr (docname, '.');
				if (d0 && (d0 > docname)) {
					d0 = strrchr (d0 - 1, '.');
					if (d0 && (d0 > docname)) {
						d = d0;
						while (isdigit (*d) || (*d == '.') || (*d == '_')) d += 1;
						if (*d) {
							memcpy (n, docname, MIN (d0 - docname - 1, 64));
							n[64] = '\0';
							docname = n;
						}
					}
				}
			}
			if (!docname || !*docname) docname = "emergency";
			g_snprintf (c, 1024, "%s/.sodipodi/%.256s.%s.%d", home, docname, sptstr, count);
			file = fopen (c, "w");
			if (!file) {
				g_snprintf (c, 1024, "%s/sodipodi-%.256s.%s.%d", home, docname, sptstr, count);
				file = fopen (c, "w");
			}
			if (!file) {
				g_snprintf (c, 1024, "/tmp/sodipodi-%.256s.%s.%d", docname, sptstr, count);
				file = fopen (c, "w");
			}
			if (file) {
				sp_repr_doc_write_stream (sp_repr_get_doc (repr), file);
				savednames = g_slist_prepend (savednames, g_strdup (c));
				fclose (file);
			} else {
				docname = sp_repr_get_attr (repr, "sodipodi:docname");
				failednames = g_slist_prepend (failednames, (docname) ? g_strdup (docname) : g_strdup (_("Untitled document")));
			}
			count++;
		}
	}

	savednames = g_slist_reverse (savednames);
	failednames = g_slist_reverse (failednames);
	if (savednames) {
		fprintf (stderr, "\nEmergency save locations:\n");
		for (l = savednames; l != NULL; l = l->next) {
			fprintf (stderr, "  %s\n", (gchar *) l->data);
		}
	}
	if (failednames) {
		fprintf (stderr, "Failed to do emergency save for:\n");
		for (l = failednames; l != NULL; l = l->next) {
			fprintf (stderr, "  %s\n", (gchar *) l->data);
		}
	}

	g_warning ("Emergency save completed, now crashing...");

	/* Show nice dialog box */

	istr = N_("Sodipodi encountered an internal error and will close now.\n");
	sstr = N_("Automatic backups of unsaved documents were done to following locations:\n");
	fstr = N_("Automatic backup of following documents failed:\n");
	nllen = strlen ("\n");
	len = strlen (istr) + strlen (sstr) + strlen (fstr);
	for (l = savednames; l != NULL; l = l->next) {
		len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
	}
	for (l = failednames; l != NULL; l = l->next) {
		len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
	}
	len += 1;
	b = g_new (gchar, len);
	pos = 0;
	len = strlen (istr);
	memcpy (b + pos, istr, len);
	pos += len;
	if (savednames) {
		len = strlen (sstr);
		memcpy (b + pos, sstr, len);
		pos += len;
		for (l = savednames; l != NULL; l = l->next) {
			memset (b + pos, ' ', SP_INDENT);
			pos += SP_INDENT;
			len = strlen ((gchar *) l->data);
			memcpy (b + pos, l->data, len);
			pos += len;
			memcpy (b + pos, "\n", nllen);
			pos += nllen;
		}
	}
	if (failednames) {
		len = strlen (fstr);
		memcpy (b + pos, fstr, len);
		pos += len;
		for (l = failednames; l != NULL; l = l->next) {
			memset (b + pos, ' ', SP_INDENT);
			pos += SP_INDENT;
			len = strlen ((gchar *) l->data);
			memcpy (b + pos, l->data, len);
			pos += len;
			memcpy (b + pos, "\n", nllen);
			pos += nllen;
		}
	}
	*(b + pos) = '\0';
	msgbox = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", b);
	gtk_dialog_run (GTK_DIALOG (msgbox));
	gtk_widget_destroy (msgbox);
	g_free (b);

	(* segv_handler) (signum);
}

#endif
