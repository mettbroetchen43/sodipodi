#define __SP_HELP_C__

/*
 * Help/About window
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>

#include "system.h"
#include "document.h"
#include "sp-text.h"
#include "svg-view.h"
#include "help.h"
#include "helper/sp-intl.h"

void
sp_help_about (void)
{
	SPDocument *doc;
	SPObject *title;
	GtkWidget *w, *v;
	char *path;

	path = g_build_filename (SODIPODI_PIXMAPDIR, "about.svg", NULL);
	doc = sodipodi_document_new (path, FALSE, TRUE);
	g_free (path);
	g_return_if_fail (doc != NULL);
	title = sp_document_lookup_id (doc, "title");
	if (title && SP_IS_TEXT (title)) {
		gchar *t;
		t = g_strdup_printf ("Sodipodi %s", VERSION);
		sp_text_set_repr_text_multiline (SP_TEXT (title), t);
		g_free (t);
	}
	sp_document_ensure_up_to_date (doc);

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (w), _("About sodipodi"));
	gtk_window_set_default_size (GTK_WINDOW (w), sp_document_width (doc), sp_document_height (doc));
#if 1
	gtk_window_set_policy (GTK_WINDOW (w), TRUE, TRUE, FALSE);
#endif

	v = sp_svg_view_widget_new (doc);
	sp_svg_view_widget_set_resize (SP_SVG_VIEW_WIDGET (v), FALSE, sp_document_width (doc), sp_document_height (doc));
	sp_document_unref (doc);
	gtk_widget_show (v);
	gtk_container_add (GTK_CONTAINER (w), v);

	gtk_widget_show (w);
}

#include <gtk/gtkmessagedialog.h>

void
sp_help_about_module (const unsigned char *text)
{
	GtkWidget *w;
	w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, text);
	gtk_dialog_run (GTK_DIALOG (w));
	gtk_widget_destroy (w);
}

