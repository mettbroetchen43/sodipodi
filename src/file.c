#define SP_FILE_C

#include <config.h>
#include <gnome.h>
#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include "xml/repr.h"
#include "dir-util.h"
#include "helper/png-write.h"
#include "sodipodi.h"
#include "mdi.h"
#include "mdi-child.h"
#include "mdi-document.h"
#include "sp-image.h"
#include "file.h"

#include <libgnomeprint/gnome-print-pixbuf.h>
#ifdef ENABLE_FRGBA
#include <libgnomeprint/gnome-print-frgba.h>
#endif

gchar * open_path = NULL;
gchar * save_path = NULL;
gchar * import_path = NULL;
gchar * export_path = NULL;

static void
file_selection_destroy (GtkWidget * widget, GtkFileSelection * fs)
{
	gtk_widget_hide (GTK_WIDGET (fs));
}

void sp_file_new (void)
{
	SPDocument * doc;
	SPMDIChild * child;

	doc = sp_document_new (NULL);
	g_return_if_fail (doc != NULL);

	child = sp_mdi_child_new (doc);

	gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
	gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));
}

void
file_open_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPMDIChild * child;
	SPDocument * doc;
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));

	file_selection_destroy (widget, fs);

	if (filename == NULL) return;

	if (open_path) g_free (open_path);
	open_path = g_dirname (filename);
	if (open_path) open_path = g_strconcat (open_path, "/", NULL);

	doc = sp_document_new (filename);
	g_return_if_fail (doc != NULL);

	child = sp_mdi_child_new (doc);

	gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
	gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));
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
	SPDocument * doc;
	SPRepr * repr;
	const gchar * fn;

	doc = SP_ACTIVE_DOCUMENT;
	g_return_if_fail (doc != NULL);

	/* fixme: */
	repr = SP_OBJECT (doc->root)->repr;

	fn = sp_repr_attr (repr, "sodipodi:docname");
	if (fn == NULL) {
		sp_file_save_as (widget);
	} else {
		sp_repr_save_file (repr, fn);
	}
}

static void
file_save_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPDocument * doc;
	SPRepr * repr;
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	doc = SP_ACTIVE_DOCUMENT;
	g_return_if_fail (doc != NULL);

	repr = SP_OBJECT (doc->root)->repr;

	if (save_path) g_free (save_path);
	save_path = g_dirname (filename);
	if (save_path) save_path = g_strconcat (save_path, "/", NULL);

	sp_repr_set_attr (repr, "sodipodi:docbase", save_path);
	sp_repr_set_attr (repr, "sodipodi:docname", filename);

	sp_repr_save_file (repr, filename);
#if 0
	sp_desktop_set_title (sp_filename_from_path (filename));
#endif
}

void sp_file_save_as (GtkWidget * widget)
{
	static GtkWidget * w = NULL;

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
	const gchar * e, * docbase, * relname;
	SPRepr * repr;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	import_path = g_dirname (filename);
	if (import_path) import_path = g_strconcat (import_path, "/", NULL);

	doc = SP_ACTIVE_DOCUMENT;
	rdoc = SP_OBJECT (doc->root)->repr;

	docbase = sp_repr_attr (rdoc, "sodipodi:docbase");
	relname = sp_relative_path_from_path (filename, docbase);
	/* fixme: this should be implemented with mime types */
	e = sp_extension_from_path (filename);

	if ((e == NULL) || (strcmp (e, "svg") == 0) || (strcmp (e, "xml") == 0)) {
		repr = sp_repr_read_file (filename);
		if (repr == NULL) return;
#if 0
		sp_repr_set_name (repr, "g");
#endif
		sp_document_add_repr (doc, repr);
		sp_repr_unref (repr);
		sp_document_done (doc);
		return;
	}

	if ((strcmp (e, "png") == 0) ||
	    (strcmp (e, "jpg") == 0) ||
	    (strcmp (e, "jpeg") == 0) ||
	    (strcmp (e, "bmp") == 0) ||
	    (strcmp (e, "gif") == 0) ||
	    (strcmp (e, "tiff") == 0) ||
	    (strcmp (e, "xpm") == 0)) {
		repr = sp_repr_new ("image");
		sp_repr_set_attr (repr, "src", relname);
		sp_repr_set_attr (repr, "sodipodi:absref", filename);
		sp_document_add_repr (doc, repr);
		sp_repr_unref (repr);
		sp_document_done (doc);
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

	gtk_widget_show (w);
}

static void
file_export_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPDocument * doc;
	gchar * filename;
	ArtDRect bbox;
	gint width, height;
	art_u8 * pixels;
	ArtPixBuf * pixbuf;
	gdouble affine[6], a[6];

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	if (export_path) g_free (export_path);
	export_path = g_dirname (filename);
	if (export_path) export_path = g_strconcat (export_path, "/", NULL);

	doc = SP_ACTIVE_DOCUMENT;
	g_return_if_fail (doc != NULL);

	sp_item_bbox (SP_ITEM (doc->root), &bbox);

	width = bbox.x1 - bbox.x0 + 2;
	height = bbox.y1 - bbox.y0 + 2;

	if ((width < 16) || (height < 16)) return;

	pixels = art_new (art_u8, width * height * 4);
	memset (pixels, 0, width * height * 4);
	pixbuf = art_pixbuf_new_rgba (pixels, width, height, width * 4);

	sp_item_i2d_affine (SP_ITEM (doc->root), affine);
	affine[4] -= bbox.x0;
	affine[5] -= bbox.y0;
	art_affine_scale (a, 1.0, -1.0);
	art_affine_multiply (affine, affine, a);
	art_affine_translate (a, 0.0, height);
	art_affine_multiply (affine, affine, a);

	sp_item_paint (SP_ITEM (doc->root), pixbuf, affine);

	sp_png_write_rgba (filename, pixbuf);

	art_pixbuf_free (pixbuf);
#if 0
	sp_desktop_set_title (sp_filename_from_path (filename));
#endif
}

void sp_file_export (GtkWidget * widget)
{
	static GtkWidget * w = NULL;

	if (w == NULL) {
		w = gtk_file_selection_new (_("Export file"));
		gtk_signal_connect (GTK_OBJECT (w), "delete_event",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			GTK_SIGNAL_FUNC (file_export_ok), w);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC (file_selection_destroy), w);
	}
	if (export_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w), export_path);

	gtk_widget_show (w);
}

void sp_do_file_print_to_printer (SPDocument * doc, GnomePrinter * printer)
{
        GnomePrintContext * gpc;

#ifdef ENABLE_FRGBA
        GnomePrintFRGBA * frgba;
#endif

        gpc = gnome_print_context_new (printer);

#ifdef ENABLE_FRGBA
	frgba = gnome_print_frgba_new (gpc);
        sp_item_print (SP_ITEM (doc->root), GNOME_PRINT_CONTEXT (frgba));
        gnome_print_showpage (GNOME_PRINT_CONTEXT (frgba));
        gnome_print_context_close (GNOME_PRINT_CONTEXT (frgba));
#else
        sp_item_print (SP_ITEM (doc->root), gpc);
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);
#endif
}

void sp_do_file_print (SPDocument * doc)
{
        GnomePrinter * printer;

        printer = gnome_printer_dialog_new_modal ();
        if (printer == NULL) return;

        sp_do_file_print_to_printer (doc, printer);
}

void sp_do_file_print_to_file (SPDocument * doc, gchar *filename)
{
        GnomePrinter * printer;

        printer = gnome_printer_new_generic_ps (filename);
        if (printer == NULL) return;

        sp_do_file_print_to_printer (doc, printer);
}

void sp_file_print (GtkWidget * widget)
{
	SPDocument * doc;

	doc = SP_ACTIVE_DOCUMENT;
	g_return_if_fail (doc != NULL);

	sp_do_file_print (doc);
}
