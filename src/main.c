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
#include <locale.h>

#ifdef WITH_POPT
#include <popt.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <libxml/tree.h>
#include <glib-object.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkbox.h>
#include "helper/sp-intl.h"

#include "macros.h"
#include "file.h"
#include "document.h"
#include "desktop.h"
#include "sp-object.h"
#include "toolbox.h"
#include "interface.h"
#include "print.h"

#if 1
/* fixme: This are required for temporary slideshow hack */
#include "slide-context.h"
#endif

#include "svg/svg.h"

#include "sodipodi-private.h"

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
	SP_ARG_LAST
};
#endif

#ifndef WIN32
int sp_main_gui (int argc, const char **argv);
int sp_main_console (int argc, const char **argv);
static void sp_do_export_png (SPDocument *doc);
#endif

static guchar *sp_global_printer = NULL;
static gboolean sp_global_slideshow = FALSE;
static guchar *sp_export_png = NULL;
static guchar *sp_export_dpi = NULL;
static guchar *sp_export_area = NULL;
static guchar *sp_export_width = NULL;
static guchar *sp_export_height = NULL;
static guchar *sp_export_background = NULL;
static guchar *sp_export_svg = NULL;

#ifdef WITH_POPT
static GSList *sp_process_args (poptContext ctx);
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
	{"export-height", 'h', POPT_ARG_STRING, &sp_export_height, SP_ARG_EXPORT_HEIGHT,
	 N_("The height of generated bitmap in pixels (overwrites dpi)"), N_("HEIGHT")},
	{"export-background", 'b', POPT_ARG_STRING, &sp_export_background, SP_ARG_EXPORT_HEIGHT,
	 N_("Background color of exported bitmap (any SVG supported color string)"), N_("COLOR")},
	{"export-svg", 0, POPT_ARG_STRING, &sp_export_svg, SP_ARG_EXPORT_SVG,
	 N_("Export document to plain SVG file (no \"xmlns:sodipodi\" namespace)"), N_("FILENAME")},
	{"slideshow", 's', POPT_ARG_NONE, &sp_global_slideshow, SP_ARG_SLIDESHOW,
	 N_("Show given files one-by-one, switch to next on any key/mouse event"), NULL},
	POPT_AUTOHELP POPT_TABLEEND
};
#endif

#ifdef WIN32
int WINAPI
WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFS)
{
	gtk_init (&_argc, &_argv);

	LIBXML_TEST_VERSION

	/* We must set LC_NUMERIC to default, or otherwise */
	/* we'll end with localised SVG files :-( */

	setlocale (LC_NUMERIC, "C");

	sodipodi = sodipodi_application_new ();
	sodipodi_load_preferences (sodipodi);
	sp_maintoolbox_create ();
	sodipodi_unref ();

	gtk_main ();

	return 0;
}
#endif

#ifndef WIN32
int
main (int argc, const char **argv)
{
	gboolean use_gui;
	gint result, i;

#ifdef __FreeBSD__
	fpsetmask (fpgetmask() & ~(FP_X_DZ|FP_X_INV));
#endif

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (GETTEXT_PACKAGE);

	LIBXML_TEST_VERSION

	use_gui = (getenv ("DISPLAY") != NULL);

	/* Test whether with/without GUI is forced */
	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-z") ||
		    !strcmp (argv[i], "--without-gui") ||
		    !strcmp (argv[i], "-p") ||
		    !strncmp (argv[i], "--print", 7) ||
		    !strcmp (argv[i], "-e") ||
		    !strncmp (argv[i], "--export-png", 12) ||
		    !strncmp (argv[i], "--export-svg", 12)) {
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
sp_main_gui (int argc, const char **argv)
{
#if WITH_POPT
	poptContext ctx;
#endif
	GSList *fl = NULL;

	gtk_init (&argc, (char ***) &argv);

	/* We must set LC_NUMERIC to default, or otherwise */
	/* we'll end with localised SVG files :-( */

	setlocale (LC_NUMERIC, "C");

#if WITH_POPT
	ctx = poptGetContext (NULL, argc, argv, options, 0);
	g_return_val_if_fail (ctx != NULL, 1);
	/* Collect own arguments */
	fl = sp_process_args (ctx);
	poptFreeContext (ctx);
#endif

#if 0
	/* Set default icon */
	if (g_file_test (GNOME_ICONDIR "/sodipodi.png", G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_SYMLINK)) {
		gnome_window_icon_set_default_from_file (GNOME_ICONDIR "/sodipodi.png");
	} else {
		g_warning ("Could not find %s", GNOME_ICONDIR "/sodipodi.png");
	}
#endif

	if (!sp_global_slideshow) {
		sodipodi = sodipodi_application_new ();
		sodipodi_load_preferences (sodipodi);
		sp_maintoolbox_create ();
		sodipodi_unref ();

		while (fl) {
			SPDocument *doc;
			SPViewWidget *dtw;
			doc = sp_document_new ((const gchar *) fl->data, TRUE, TRUE);
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
		sp_maintoolbox_create ();
		sodipodi_unref ();
		
		while (fl) {
			doc = sp_document_new ((const gchar *) fl->data, FALSE, TRUE);
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
				sp_desktop_set_event_context (SP_DESKTOP_WIDGET (dtw)->desktop, SP_TYPE_SLIDE_CONTEXT, NULL);
				g_object_set_data (G_OBJECT (SP_DESKTOP_WIDGET (dtw)->desktop), "slides", slides);
				sp_create_window (dtw, FALSE);
			}
		}
		
		sodipodi_unref ();
	}

	gtk_main ();

	return 0;
}

int
sp_main_console (int argc, const char **argv)
{
#ifdef WITH_POPT
	poptContext ctx = NULL;
#endif
	GSList * fl = NULL;
	guchar *printer;

	/* We are started in text mode */

	/* We must set LC_NUMERIC to default, or otherwise */
	/* we'll end with localised SVG files :-( */

	setlocale (LC_NUMERIC, "C");

#ifdef WITH_POPT
	ctx = poptGetContext (NULL, argc, argv, options, 0);
	g_return_val_if_fail (ctx != NULL, 1);
	fl = sp_process_args (ctx);
	poptFreeContext (ctx);
#endif

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
			printer = g_build_filename (cwd, sp_global_printer, NULL);
			g_free (cwd);
		} else {
			printer = g_strdup (sp_global_printer);
		}
	}

	/* Start up gtk, without requiring X */
	g_type_init();
	sodipodi = sodipodi_application_new ();

	while (fl) {
		SPDocument *doc;
		doc = sp_document_new ((gchar *) fl->data, FALSE, TRUE);
		if (doc == NULL) {
			g_warning ("Specified document %s cannot be opened (is it valid SVG file?)", (gchar *) fl->data);
		} else {
			if (printer) {
				sp_print_document_to_file (doc, printer);
			}
			if (sp_export_png) {
				sp_do_export_png (doc);
			}
			if (sp_export_svg) {
				SPReprDoc *rdoc;
				SPRepr *repr;
				rdoc = sp_repr_document_new ("svg");
				repr = sp_repr_document_root (rdoc);
				repr = sp_object_invoke_write (sp_document_root (doc), repr, SP_OBJECT_WRITE_BUILD);
				sp_repr_save_file (sp_repr_document (repr), sp_export_svg);
			}
		}
		fl = g_slist_remove (fl, fl->data);
	}

	if (printer) g_free (printer);

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
		if ((width < 16) || (width > 65536)) {
			g_warning ("Export width %d out of range (16 - 65536)", width);
			return;
		}
		dpi = (gdouble) width * 72.0 / (area.x1 - area.x0);
	}

	if (sp_export_height) {
		height = atoi (sp_export_height);
		if ((height < 16) || (height > 65536)) {
			g_warning ("Export height %d out of range (16 - 65536)", width);
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

	if ((width >= 16) || (height >= 16) || (width < 65536) || (height < 65536)) {
#if 1
		sp_export_png_file (doc, sp_export_png, area.x0, area.y0, area.x1, area.y1, width, height, bgcolor);
#else
		ArtPixBuf *pixbuf;
		art_u8 *pixels;
		gdouble affine[6], t;
		gint len, i;

		/* fixme: Move this to helper */
		len = width * height * 4;
		pixels = art_new (art_u8, len);
		for (i = 0; i < len; i+= 4) {
			pixels[i + 0] = (bgcolor >> 24);
			pixels[i + 1] = (bgcolor >> 16) & 0xff;
			pixels[i + 2] = (bgcolor >> 8) & 0xff;
			pixels[i + 3] = (bgcolor & 0xff);
		}
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

		affine[0] = width / ((area.x1 - area.x0) * 1.25);
		affine[1] = 0.0;
		affine[2] = 0.0;
		affine[3] = height / ((area.y1 - area.y0) * 1.25);
		affine[4] = -affine[0] * area.x0 * 1.25;
		affine[5] = -affine[3] * area.y0 * 1.25;

		SP_PRINT_TRANSFORM ("SVG2PNG", affine);

		sp_item_paint (SP_ITEM (sp_document_root (doc)), pixbuf, affine);

		sp_png_write_rgba (sp_export_png, pixbuf->pixels, pixbuf->width, pixbuf->height, pixbuf->rowstride);

		art_pixbuf_free (pixbuf);
#endif
	} else {
		g_warning ("Calculated bitmap dimensions %d %d out of range (16 - 65535)", width, height);
	}
}
#endif /* NOT WIN32 */

#ifdef WITH_POPT
static GSList *
sp_process_args (poptContext ctx)
{
	GSList * fl;
	const gchar ** args, *fn;
	gint i, a;

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

	return fl;
}
#endif

