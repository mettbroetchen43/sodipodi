#define __SPSVGVIEW_C__

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

#include <locale.h>
#include <libxml/tree.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktable.h>
#include <gtk/gtkbutton.h>

#include "helper/sp-intl.h"
#include "document.h"
#include "svg-view.h"

static GtkWidget *sp_svgview_control_show (void);

static int numdocs = 0;

static int
sp_svgview_main_delete (GtkWidget *widget, GdkEvent *event, SPDocument *doc)
{
	numdocs -= 1;
	if (numdocs < 1) gtk_main_quit ();
	return FALSE;
}

static int
sp_svgview_main_key_press (GtkWidget *widget, GdkEventKey *event, SPDocument *doc)
{
	switch (event->keyval) {
	case GDK_f:
	case GDK_F:
		if (event->state & GDK_SHIFT_MASK) {
			gtk_window_unfullscreen ((GtkWindow *) widget);
		} else {
			gtk_window_fullscreen ((GtkWindow *) widget);
		}
		break;
	case GDK_Return:
		sp_svgview_control_show ();
		break;
	default:
		break;
	}
	return FALSE;
}

int
main (int argc, const char **argv)
{
	int i;

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	LIBXML_TEST_VERSION

	gtk_init (&argc, (char ***) &argv);

#ifdef lalaWITH_MODULES
	g_warning ("Have to autoinit modules (lauris)");
	sp_modulesys_init();
#endif /* WITH_MODULES */

	/* We must set LC_NUMERIC to default, or otherwise */
	/* we'll end with localised SVG files :-( */

	setlocale (LC_NUMERIC, "C");

	for (i = 1; i < argc; i++) {
		SPDocument *doc;
		doc = sp_document_new (argv[i], TRUE, TRUE);
		if (doc) {
			GtkWidget *w, *v;
			w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
			gtk_window_set_title (GTK_WINDOW (w), SP_DOCUMENT_NAME (doc));
			gtk_window_set_default_size (GTK_WINDOW (w), sp_document_width (doc), sp_document_height (doc));
			gtk_window_set_policy (GTK_WINDOW (w), TRUE, TRUE, FALSE);

			g_signal_connect (G_OBJECT (w), "delete_event", (GCallback) sp_svgview_main_delete, doc);
			g_signal_connect (G_OBJECT (w), "key_press_event", (GCallback) sp_svgview_main_key_press, doc);

			v = sp_svg_view_widget_new (doc);
			sp_svg_view_widget_set_resize (SP_SVG_VIEW_WIDGET (v), FALSE, sp_document_width (doc), sp_document_height (doc));
			sp_document_unref (doc);
			gtk_widget_show (v);
			gtk_container_add (GTK_CONTAINER (w), v);

			gtk_widget_show (w);
			numdocs += 1;
		}
	}

	if (numdocs > 0) {
		gtk_main ();
	}

	return 0;
}

static GtkWidget *ctrlwin = NULL;

static int
sp_svgview_ctrlwin_delete (GtkWidget *widget, GdkEvent *event, void *data)
{
	ctrlwin = NULL;
	return FALSE;
}

static GtkWidget *
sp_svgview_control_show (void)
{
	if (!ctrlwin) {
		GtkWidget *t, *b;
		ctrlwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		g_signal_connect (G_OBJECT (ctrlwin), "delete_event", (GCallback) sp_svgview_ctrlwin_delete, NULL);
		t = gtk_table_new (1, 4, TRUE);
		gtk_container_add ((GtkContainer *) ctrlwin, t);
		b = gtk_button_new_from_stock (GTK_STOCK_GOTO_FIRST);
		gtk_table_attach ((GtkTable *) t, b, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		b = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
		gtk_table_attach ((GtkTable *) t, b, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		b = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
		gtk_table_attach ((GtkTable *) t, b, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		b = gtk_button_new_from_stock (GTK_STOCK_GOTO_LAST);
		gtk_table_attach ((GtkTable *) t, b, 3, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		gtk_widget_show_all (ctrlwin);
	} else {
		gtk_window_present ((GtkWindow *) ctrlwin);
	}

	return NULL;
}

Sodipodi *sodipodi;

void sodipodi_ref (void) {}
void sodipodi_unref (void) {}
void sodipodi_add_document (SPDocument *document) {}
void sodipodi_remove_document (SPDocument *document) {}
SPRepr *sodipodi_get_repr (Sodipodi *sodipodi, const unsigned char *key) {return NULL;}
#include "widgets/menu.h"
void sp_menu_append (SPMenu *menu, const unsigned char *name, const unsigned char *tip, const void *data) {}
