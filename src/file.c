#define SP_FILE_C

#include <config.h>
#include <gnome.h>
#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>

#include "xml/repr.h"
#include "dir-util.h"
#include "helper/png-write.h"
#include "sodipodi.h"
#include "sp-image.h"
#include "interface.h"
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
	SPDesktop * desktop;

	doc = sp_document_new (NULL);
	g_return_if_fail (doc != NULL);

	desktop = sp_desktop_new (doc, sp_document_namedview (doc, NULL));
	sp_document_unref (doc);
	g_return_if_fail (desktop != NULL);

	sp_create_window (desktop, TRUE);
}

static void
file_open_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	SPDocument * doc;
	SPDesktop * desktop;
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));

	file_selection_destroy (widget, fs);

	if (filename == NULL) return;

	if (open_path) g_free (open_path);
	open_path = g_dirname (filename);
	if (open_path) open_path = g_strconcat (open_path, "/", NULL);

	doc = sp_document_new (filename);
	g_return_if_fail (doc != NULL);

	desktop = sp_desktop_new (doc, sp_document_namedview (doc, NULL));
	sp_document_unref (doc);
	g_return_if_fail (desktop != NULL);

	sp_create_window (desktop, TRUE);
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
	if (!SP_IS_DOCUMENT(doc)) return;
	//	g_return_if_fail (doc != NULL);

	/* fixme: */
	repr = sp_document_repr_root (doc);

	fn = sp_repr_attr (repr, "sodipodi:docname");
	if (fn == NULL) {
		sp_file_save_as (widget);
	} else {
		sp_repr_save_file (sp_document_repr_doc (doc), fn);
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

	repr = sp_document_repr_root (doc);

	if (save_path) g_free (save_path);
	save_path = g_dirname (filename);
	if (save_path) save_path = g_strconcat (save_path, "/", NULL);

	sp_repr_set_attr (repr, "sodipodi:docbase", save_path);
	sp_repr_set_attr (repr, "sodipodi:docname", filename);

	sp_repr_save_file (sp_document_repr_doc (doc), filename);
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
	SPReprDoc * rnewdoc;

	doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT(doc)) return;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	file_selection_destroy (NULL, fs);

	if (filename == NULL) return;

	import_path = g_dirname (filename);
	if (import_path) import_path = g_strconcat (import_path, "/", NULL);

	rdoc = sp_document_repr_root (doc);

	docbase = sp_repr_attr (rdoc, "sodipodi:docbase");
	relname = sp_relative_path_from_path (filename, docbase);
	/* fixme: this should be implemented with mime types */
	e = sp_extension_from_path (filename);

	if ((e == NULL) || (strcmp (e, "svg") == 0) || (strcmp (e, "xml") == 0)) {
		const GList * children, * l;
		SPRepr * newgroup;
		const gchar * style;

		rnewdoc = sp_repr_read_file (filename);
		if (rnewdoc == NULL) return;
		repr = sp_repr_document_root (rnewdoc);
		children = sp_repr_children (repr);
		style = sp_repr_attr (repr, "style");

		newgroup = sp_repr_new ("g");
		sp_repr_set_attr (newgroup, "style", style);

		for (l = children; l != NULL; l = l->next) {
			SPRepr * child;
			child = sp_repr_copy ((SPRepr *) l->data);
			sp_repr_append_child (newgroup, child);
		}

		sp_repr_document_unref (rnewdoc);

		sp_document_add_repr (doc, newgroup);
		sp_repr_unref (newgroup);
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
        SPDocument * doc;
	static GtkWidget * w = NULL;
  
        doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT(doc)) return;

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

	sp_item_bbox (SP_ITEM (sp_document_root (doc)), &bbox);

	width = bbox.x1 - bbox.x0 + 2;
	height = bbox.y1 - bbox.y0 + 2;

	if ((width < 16) || (height < 16)) return;

	pixels = art_new (art_u8, width * height * 4);
	memset (pixels, 0, width * height * 4);
	pixbuf = art_pixbuf_new_rgba (pixels, width, height, width * 4);

	sp_item_i2d_affine (SP_ITEM (sp_document_root (doc)), affine);
	affine[4] -= bbox.x0;
	affine[5] -= bbox.y0;
	art_affine_scale (a, 1.0, -1.0);
	art_affine_multiply (affine, affine, a);
	art_affine_translate (a, 0.0, height);
	art_affine_multiply (affine, affine, a);

	sp_item_paint (SP_ITEM (sp_document_root (doc)), pixbuf, affine);

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
        sp_item_print (SP_ITEM (sp_document_root (doc)), GNOME_PRINT_CONTEXT (frgba));
        gnome_print_showpage (GNOME_PRINT_CONTEXT (frgba));
        gnome_print_context_close (GNOME_PRINT_CONTEXT (frgba));
#else
        sp_item_print (SP_ITEM (sp_document_root (doc)), gpc);
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

static void
sp_print_preview_destroy_cb (GtkObject *obj, gpointer data)
{

}

void sp_do_file_print_preview (SPDocument * doc)
{
        GnomePrintContext * gpc;
        GnomePrintMaster * gpm;
	GnomePrintMasterPreview *gpmp;
	gchar * title;

	gpm = gnome_print_master_new();
	gpc = gnome_print_master_get_context (gpm);

	g_return_if_fail (gpm != NULL);
	g_return_if_fail (gpc != NULL);

	sp_item_print (SP_ITEM (sp_document_root (doc)), GNOME_PRINT_CONTEXT (gpc));
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);

	title = g_strdup_printf (_("Sodipodi (doc name %s..): Print Preview"),"");
	gpmp = gnome_print_master_preview_new (gpm, title);

        /* Conect the signals and display the preview window */
	gtk_signal_connect (GTK_OBJECT(gpmp), "destroy",
			    GTK_SIGNAL_FUNC(sp_print_preview_destroy_cb), NULL);
	gtk_widget_show (GTK_WIDGET(gpmp));

	gnome_print_master_close (gpm);

	g_free (title);

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
	if (!SP_IS_DOCUMENT(doc)) return;

	//	g_return_if_fail (doc != NULL);

	sp_do_file_print (doc);
}

void sp_file_print_preview (GtkWidget * widget)
{
	SPDocument * doc;

	doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT(doc)) return;
	//	g_return_if_fail (doc != NULL);

	sp_do_file_print_preview (doc);
}
