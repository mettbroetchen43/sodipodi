#define __SP_FILE_C__

/*
 * Basic operations from file menu
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include <libgnome/gnome-paper.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>

#include "xml/repr-private.h"
#include "document.h"
#include "view.h"
#include "dir-util.h"
#include "helper/png-write.h"
#include "sodipodi.h"
#include "desktop.h"
#include "sp-image.h"
#include "interface.h"
#include "file.h"

#include <libgnomeprint/gnome-print-pixbuf.h>

gchar * open_path = NULL;
gchar * save_path = NULL;
gchar * import_path = NULL;
gchar * export_path = NULL;

static void sp_do_file_print_to_printer (SPDocument * doc, GnomePrinter * printer);
static void sp_do_file_print_preview (SPDocument * doc);

void sp_file_new (void)
{
	SPDocument * doc;
	SPViewWidget *dtw;

	doc = sp_document_new (NULL, TRUE);
	g_return_if_fail (doc != NULL);

	dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
	sp_document_unref (doc);
	g_return_if_fail (dtw != NULL);

	sp_create_window (dtw, TRUE);
}

void
sp_file_open (const guchar *uri)
{
	SPDocument *doc;
	SPViewWidget *dtw;

	doc = sp_document_new (uri, TRUE);
	g_return_if_fail (doc != NULL);

	dtw = sp_desktop_widget_new (sp_document_namedview (doc, NULL));
	sp_document_unref (doc);
	g_return_if_fail (dtw != NULL);

	sp_create_window (dtw, TRUE);
}

static void
file_open_ok (GtkWidget * widget, GtkFileSelection * fs)
{
	gchar * filename;

	filename = g_strdup (gtk_file_selection_get_filename (fs));

	gtk_widget_destroy (GTK_WIDGET (fs));

	if (filename == NULL) return;

	if (open_path) g_free (open_path);
	open_path = g_dirname (filename);
	if (open_path) open_path = g_strconcat (open_path, "/", NULL);

	sp_file_open (filename);
	g_free (filename);
}

static void
file_open_cancel (GtkButton *b, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET (fs));
}

void sp_file_open_dialog (gpointer object, gpointer data)
{
	GtkWidget * w;

	w = gtk_file_selection_new (_("Select file to open"));
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (w));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (file_open_ok), w);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (file_open_cancel), w);

	if (open_path) {
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w),open_path);
	}

	gtk_widget_show (w);
}

void
sp_file_save_document (SPDocument *document)
{
	SPRepr * repr;
	const gchar * fn;

	repr = sp_document_repr_root (document);

	fn = sp_repr_attr (repr, "sodipodi:docname");
	if (fn == NULL) {
		sp_file_save_as (NULL);
	} else {
		/* fixme: */
		sp_document_set_undo_sensitive (document, FALSE);
		sp_repr_set_attr (repr, "sodipodi:modified", NULL);
		sp_document_set_undo_sensitive (document, TRUE);
		sp_repr_save_file (sp_document_repr_doc (document), fn);
	}
}

void sp_file_save (GtkWidget *widget)
{
	SPDocument * doc;

	doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT (doc)) return;

	sp_file_save_document (doc);
}

static void
file_save_ok (GtkWidget *widget, GtkFileSelection *fs)
{
	SPDocument *doc;
	SPRepr *repr;
	gchar *filename, *type;
	gboolean spns;

	filename = g_strdup (gtk_file_selection_get_filename (fs));
	type = gtk_object_get_data (GTK_OBJECT (fs), "type");
	spns = !(type && !strcmp (type, "svg"));

	gtk_widget_destroy (GTK_WIDGET (fs));

	if (filename != NULL) {
		const GSList *images, *l;
		SPReprDoc *rdoc;

		doc = SP_ACTIVE_DOCUMENT;
		g_return_if_fail (doc != NULL);

		if (save_path) g_free (save_path);
		save_path = g_dirname (filename);
		if (save_path) save_path = g_strconcat (save_path, "/", NULL);

		if (spns) {
			rdoc = NULL;
			repr = sp_document_repr_root (doc);
			sp_repr_set_attr (repr, "sodipodi:docbase", save_path);
			sp_repr_set_attr (repr, "sodipodi:docname", filename);
		} else {
			rdoc = sp_repr_document_new ("svg");
			repr = sp_repr_document_root (rdoc);
			repr = sp_object_invoke_write (sp_document_root (doc), repr, SP_OBJECT_WRITE_BUILD);
		}

		images = sp_document_get_resource_list (doc, "image");
		for (l = images; l != NULL; l = l->next) {
			SPRepr *ir;
			const guchar *href, *relname;
			ir = SP_OBJECT_REPR (l->data);
			href = sp_repr_attr (ir, "xlink:href");
			if (spns && !g_path_is_absolute (href)) {
				href = sp_repr_attr (ir, "sodipodi:absref");
			}
			if (href && g_path_is_absolute (href)) {
				relname = sp_relative_path_from_path (href, save_path);
				sp_repr_set_attr (ir, "xlink:href", relname);
			}
		}

		/* fixme: */
		sp_document_set_undo_sensitive (doc, FALSE);
		sp_repr_set_attr (repr, "sodipodi:modified", NULL);
		sp_document_set_undo_sensitive (doc, TRUE);

		sp_repr_save_file (sp_repr_document (repr), filename);

		sp_document_set_uri (doc, filename);

		if (!spns) sp_repr_document_unref (rdoc);
	}
}

static void
file_save_cancel (GtkButton *b, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
sp_file_save_type_activate (GtkWidget *widget, GtkObject *dlg)
{
	gtk_object_set_data (dlg, "type", gtk_object_get_data (GTK_OBJECT (widget), "type"));
}

void
sp_file_save_as (GtkWidget * widget)
{
	GtkFileSelection *fsel;
	GtkWidget *dlg, *hb, *l, *om, *m, *mi;

	dlg = gtk_file_selection_new (_("Save file"));
	/* fixme: Remove modality (Lauris) */
	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);

	fsel = GTK_FILE_SELECTION (dlg);

	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (fsel->main_vbox), hb, FALSE, FALSE, 0);

	om = gtk_option_menu_new ();
	gtk_widget_show (om);
	gtk_box_pack_end (GTK_BOX (hb), om, FALSE, FALSE, 0);
	m = gtk_menu_new ();
	gtk_widget_show (m);
	mi = gtk_menu_item_new_with_label (_("SVG with \"xmlns:sodipodi\" namespace"));
	gtk_widget_show (mi);
	gtk_menu_append (GTK_MENU (m), mi);
	gtk_object_set_data (GTK_OBJECT (mi), "type", "sodipodi");
	gtk_object_set_data (GTK_OBJECT (dlg), "type", "sodipodi");
	gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_file_save_type_activate), dlg);
	mi = gtk_menu_item_new_with_label (_("Plain SVG"));
	gtk_widget_show (mi);
	gtk_menu_append (GTK_MENU (m), mi);
	gtk_object_set_data (GTK_OBJECT (mi), "type", "svg");
	gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_file_save_type_activate), dlg);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);
	l = gtk_label_new (_("Save as:"));
	gtk_widget_show (l);
	gtk_box_pack_end (GTK_BOX (hb), l, FALSE, FALSE, 0);

	gtk_signal_connect (GTK_OBJECT (fsel->ok_button), "clicked", GTK_SIGNAL_FUNC (file_save_ok), dlg);
	gtk_signal_connect (GTK_OBJECT (fsel->cancel_button), "clicked", GTK_SIGNAL_FUNC (file_save_cancel), dlg);

	if (save_path) {
		gtk_file_selection_set_filename (fsel, save_path);
	}

	gtk_widget_show (dlg);
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

	gtk_widget_destroy (GTK_WIDGET (fs));

	if (filename == NULL) return;

	import_path = g_dirname (filename);
	if (import_path) import_path = g_strconcat (import_path, "/", NULL);

	rdoc = sp_document_repr_root (doc);

	docbase = sp_repr_attr (rdoc, "sodipodi:docbase");
	relname = sp_relative_path_from_path (filename, docbase);
	/* fixme: this should be implemented with mime types */
	e = sp_extension_from_path (filename);

	if ((e == NULL) || (strcmp (e, "svg") == 0) || (strcmp (e, "xml") == 0)) {
		SPRepr * newgroup;
		const gchar * style;
		SPRepr * child;

		rnewdoc = sp_repr_read_file (filename);
		if (rnewdoc == NULL) return;
		repr = sp_repr_document_root (rnewdoc);
		style = sp_repr_attr (repr, "style");

		newgroup = sp_repr_new ("g");
		sp_repr_set_attr (newgroup, "style", style);

		for (child = repr->children; child != NULL; child = child->next) {
			SPRepr * newchild;
			newchild = sp_repr_duplicate (child);
			sp_repr_append_child (newgroup, newchild);
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
		/* Try pixbuf */
		GdkPixbuf *pb;
		pb = gdk_pixbuf_new_from_file (filename);
		if (pb) {
			/* We are readable */
			repr = sp_repr_new ("image");
			sp_repr_set_attr (repr, "xlink:href", relname);
			sp_repr_set_attr (repr, "sodipodi:absref", filename);
			sp_repr_set_double_attribute (repr, "width", gdk_pixbuf_get_width (pb));
			sp_repr_set_double_attribute (repr, "height", gdk_pixbuf_get_height (pb));
			sp_document_add_repr (doc, repr);
			sp_repr_unref (repr);
			sp_document_done (doc);
			gdk_pixbuf_unref (pb);
		}
	}
}

static void
file_import_cancel (GtkButton *b, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET (fs));
}

void sp_file_import (GtkWidget * widget)
{
        SPDocument * doc;
	GtkWidget * w;

        doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT(doc)) return;

	w = gtk_file_selection_new (_("Select file to import"));
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (w));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (file_import_ok), w);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (file_import_cancel), w);

	if (import_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w), import_path);

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

	gtk_widget_destroy (GTK_WIDGET (fs));

	if (filename == NULL) return;

	if (export_path) g_free (export_path);
	export_path = g_dirname (filename);
	if (export_path) export_path = g_strconcat (export_path, "/", NULL);

	doc = SP_ACTIVE_DOCUMENT;
	g_return_if_fail (doc != NULL);

	sp_item_bbox_desktop (SP_ITEM (sp_document_root (doc)), &bbox);

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

	sp_png_write_rgba (filename, pixbuf->pixels, pixbuf->width, pixbuf->height, pixbuf->rowstride);

	art_pixbuf_free (pixbuf);
#if 0
	sp_desktop_set_title (sp_filename_from_path (filename));
#endif
}

static void
file_export_cancel (GtkButton *b, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET (fs));
}

void sp_file_export (GtkWidget * widget)
{
	GtkWidget * w;

	w = gtk_file_selection_new (_("Export file"));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (file_export_ok), w);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (file_export_cancel), w);

	if (export_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w), export_path);

	gtk_widget_show (w);
}

static void
sp_do_file_print_to_printer (SPDocument * doc, GnomePrinter * printer)
{
        GnomePrintContext * gpc;

        gpc = gnome_print_context_new (printer);

	sp_document_ensure_up_to_date (doc);

	gnome_print_beginpage (gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpc, 0.0, sp_document_height (doc));
	gnome_print_scale (gpc, 0.8, -0.8);
	gnome_print_concat (gpc, SP_ITEM (SP_DOCUMENT_ROOT (doc))->affine);
        sp_item_print (SP_ITEM (sp_document_root (doc)), gpc);
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);

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

static void
sp_do_file_print_preview (SPDocument * doc)
{
        GnomePrintContext * gpc;
        GnomePrintMaster * gpm;
	GnomePrintMasterPreview *gpmp;
	gchar * title;

	sp_document_ensure_up_to_date (doc);

	gpm = gnome_print_master_new();
	gpc = gnome_print_master_get_context (gpm);

	g_return_if_fail (gpm != NULL);
	g_return_if_fail (gpc != NULL);

	gnome_print_beginpage (gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpc, 0.0, sp_document_height (doc));
	gnome_print_scale (gpc, 0.8, -0.8);
	gnome_print_concat (gpc, SP_ITEM (SP_DOCUMENT_ROOT (doc))->affine);
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

void sp_file_exit (void)
{
	sodipodi_exit (SODIPODI);
}

