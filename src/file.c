#define SP_FILE_C

#include <gnome.h>
#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include "xml/repr.h"
#include "dir-util.h"
#include "desktop.h"
#include "sp-image.h"
#include "file.h"

gchar * open_path = NULL;
gchar * save_path = NULL;
gchar * import_path = NULL;
SPDesktop * import_desktop, * save_desktop;

static void
file_selection_destroy (GtkWidget * widget, GtkFileSelection * fs)
{
	gtk_widget_hide (GTK_WIDGET (fs));
}

void sp_file_new (void)
{
	SPDocument * doc;
	SPDesktop * desktop;

	doc = sp_document_new ();
	g_return_if_fail (doc != NULL);

	desktop = sp_desktop_new (SP_APP_MAIN, doc);
	g_return_if_fail (desktop != NULL);
}

void
file_open_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPDesktop * desktop;
	SPDocument * doc;
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));

	file_selection_destroy (widget, fs);

	if (filename == NULL) return;

	if (open_path) g_free (open_path);
	open_path = g_dirname (filename);
	if (open_path) open_path = g_strconcat (open_path, "/", NULL);

	doc = sp_document_new_from_file (filename);
	g_return_if_fail (doc != NULL);

	desktop = sp_desktop_new (SP_APP_MAIN, doc);
	if (desktop == NULL) {
		sp_document_destroy (doc);
		return;
	}
}

void sp_file_open (void)
{
	static GtkWidget * w = NULL;

	if (w == NULL) {
		w = gtk_file_selection_new (_("Select file to open"));
		gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (w));
		gtk_signal_connect (GTK_OBJECT (w), "delete_event",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			GTK_SIGNAL_FUNC (file_open_ok), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
	}
	if (open_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w),open_path);

	gtk_widget_show (w);
}

void sp_file_save (GtkWidget * widget)
{
	SPRepr * repr;
	const gchar * fn;

	save_desktop = SP_WIDGET_DESKTOP (widget);
	g_return_if_fail (save_desktop != NULL);

	repr = SP_ITEM (SP_DT_DOCUMENT (save_desktop))->repr;

	fn = sp_repr_attr (repr, "SP-DOCNAME");
	if (fn == NULL) {
		sp_file_save_as (widget);
	} else {
		sp_repr_save_file (repr, fn);
	}
}

static void
file_save_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPRepr * repr;
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	repr = SP_ITEM (SP_DT_DOCUMENT (save_desktop))->repr;

	if (save_path) g_free (save_path);
	save_path = g_dirname (filename);
	if (save_path) save_path = g_strconcat (save_path, "/", NULL);

	sp_repr_set_attr (repr, "SP-DOCBASE", save_path);
	sp_repr_set_attr (repr, "SP-DOCNAME", filename);

	sp_repr_save_file (repr, filename);
#if 0
	sp_desktop_set_title (sp_filename_from_path (filename));
#endif
}

void sp_file_save_as (GtkWidget * widget)
{
	static GtkWidget * w = NULL;

	save_desktop = SP_WIDGET_DESKTOP (widget);

	if (w == NULL) {
		w = gtk_file_selection_new (_("Save file"));
		gtk_signal_connect (GTK_OBJECT (w), "delete_event",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			GTK_SIGNAL_FUNC (file_save_ok), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
	}
	if (save_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w),save_path);

	gtk_widget_show (w);
}

void sp_file_exit (void)
{
	gtk_main_quit ();
}

static void
file_import_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPDocument * doc;
	SPRepr * rdoc;
	gchar * filename;
	const gchar * e, * n, * docbase, * relname;
	SPRepr * repr;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	import_path = g_dirname (filename);
	if (import_path) import_path = g_strconcat (import_path, "/", NULL);

	doc = SP_DT_DOCUMENT (import_desktop);
	rdoc = SP_ITEM (doc)->repr;

	docbase = sp_repr_attr (rdoc, "SP-DOCBASE");
	relname = sp_relative_path_from_path (filename, docbase);
	/* fixme: this should be implemented with mime types */
	e = sp_extension_from_path (filename);

	if ((e == NULL) || (strcmp (e, "svg") == 0) || (strcmp (e, "xml") == 0)) {
		repr = sp_repr_read_file (filename);
		if (repr == NULL) return;
		sp_repr_set_name (repr, "g");
		sp_repr_append_child (rdoc, repr);
		sp_repr_unref (repr);
		return;
	}

	if ((strcmp (e, "png") == 0) ||
	    (strcmp (e, "jpg") == 0) ||
	    (strcmp (e, "jpeg") == 0) ||
	    (strcmp (e, "bmp") == 0) ||
	    (strcmp (e, "gif") == 0) ||
	    (strcmp (e, "tiff") == 0) ||
	    (strcmp (e, "xpm") == 0)) {
		repr = sp_repr_new ();
		sp_repr_set_name (repr, "image");
		sp_repr_set_attr (repr, "src", relname);
		sp_repr_set_attr (repr, "sp-absolute-path-name", filename);
		sp_repr_append_child (rdoc, repr);
		sp_repr_unref (repr);
	}
}

void sp_file_import (GtkWidget * widget)
{
	static GtkWidget * w = NULL;

	if (w == NULL) {
		w = gtk_file_selection_new (_("Select file to import"));
		gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (w));
		gtk_signal_connect (GTK_OBJECT (w), "delete_event",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			GTK_SIGNAL_FUNC (file_import_ok), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
	}
	if (import_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w),import_path);

	import_desktop = SP_WIDGET_DESKTOP (widget);

	gtk_widget_show (w);
}

void sp_file_print (GtkWidget * widget)
{
	GnomePrinter * printer;
	GnomePrintContext * gpc;
	SPDesktop * desktop;
	SPDocument * doc;

	desktop = SP_WIDGET_DESKTOP (widget);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	doc = SP_DT_DOCUMENT (desktop);
	g_return_if_fail (doc != NULL);

	printer = gnome_printer_dialog_new_modal ();
	if (printer == NULL) return;

	gpc = gnome_print_context_new (printer);

	sp_item_print (SP_ITEM (doc), gpc);

	gnome_print_showpage (gpc);

	gnome_print_context_close (gpc);
#if 0
	gnome_print_context_free (gpc);
#endif
}
