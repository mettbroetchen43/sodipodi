#define __SP_FILE_C__

/*
 * File/Print operations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Chema Celorio <chema@celorio.com>
 *
 * Copyright (C) 1999-2002 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <string.h>
#include <time.h>
#include <libnr/nr-pixops.h>
#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkfilesel.h>

#include "macros.h"
#include "xml/repr-private.h"
#include "document.h"
#include "view.h"
#include "dir-util.h"
#include "helper/png-write.h"
#include "dialogs/export.h"
#include "helper/sp-intl.h"
#include "sodipodi.h"
#include "desktop.h"
#include "sp-image.h"
#include "interface.h"
#include "print.h"
#include "file.h"

gchar * open_path = NULL;
gchar * save_path = NULL;
gchar * import_path = NULL;
gchar * export_path = NULL;

#if 0
static void sp_do_file_print_to_printer (SPDocument * doc, GnomePrinter * printer);
#endif

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

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked", G_CALLBACK (file_open_ok), w);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked", G_CALLBACK (file_open_cancel), w);

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
	type = g_object_get_data (G_OBJECT (fs), "type");
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
sp_file_save_type_activate (GtkWidget *widget, GObject *dlg)
{
	g_object_set_data (dlg, "type", g_object_get_data (G_OBJECT (widget), "type"));
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
	g_object_set_data (G_OBJECT (mi), "type", "sodipodi");
	g_object_set_data (G_OBJECT (dlg), "type", "sodipodi");
	g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (sp_file_save_type_activate), dlg);
	mi = gtk_menu_item_new_with_label (_("Plain SVG"));
	gtk_widget_show (mi);
	gtk_menu_append (GTK_MENU (m), mi);
	g_object_set_data (G_OBJECT (mi), "type", "svg");
	g_signal_connect (G_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_file_save_type_activate), dlg);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);
	l = gtk_label_new (_("Save as:"));
	gtk_widget_show (l);
	gtk_box_pack_end (GTK_BOX (hb), l, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (fsel->ok_button), "clicked", G_CALLBACK (file_save_ok), dlg);
	g_signal_connect (G_OBJECT (fsel->cancel_button), "clicked", G_CALLBACK (file_save_cancel), dlg);

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
		pb = gdk_pixbuf_new_from_file (filename, NULL);
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

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (w)->ok_button), "clicked", G_CALLBACK (file_import_ok), w);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (w)->cancel_button), "clicked", G_CALLBACK (file_import_cancel), w);

	if (import_path)
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (w), import_path);

	gtk_widget_show (w);
}

void
sp_file_print (gpointer object, gpointer data)
{
	SPDocument *doc;

	doc = SP_ACTIVE_DOCUMENT;

	if (doc) {
		sp_print_document (doc);
	}
}

void
sp_file_print_preview (gpointer object, gpointer data)
{
	SPDocument *doc;

	doc = SP_ACTIVE_DOCUMENT;

	if (doc) {
		sp_print_preview_document (doc);
	}
}

void sp_file_exit (void)
{
	if (sp_ui_close_all ()) {
		sodipodi_exit (SODIPODI);
	}
}

void
sp_file_export_dialog (void *widget)
{
	sp_export_dialog ();
}

#include <display/nr-arena-item.h>
#include <display/nr-arena.h>

struct SPEBP {
	int width, height;
	unsigned char r, g, b, a;
	NRArenaItem *root;
	NRBuffer *buf;
};

static int
sp_export_get_rows (const unsigned char **rows, int row, int num_rows, void *data)
{
	struct SPEBP *ebp;
	NRRectL bbox;
	NRGC gc;
	int r, c;

	ebp = (struct SPEBP *) data;

	num_rows = MIN (num_rows, 64);
	num_rows = MIN (num_rows, ebp->height - row);

	g_print ("Rendering %d + %d rows\n", row, num_rows);

	/* Set area of interest */
	bbox.x0 = 0;
	bbox.y0 = row;
	bbox.x1 = ebp->width;
	bbox.y1 = row + num_rows;
	/* Update to renderable state */
	art_affine_identity (gc.affine);
	nr_arena_item_invoke_update (ebp->root, &bbox, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);

	for (r = 0; r < num_rows; r++) {
		unsigned char *p;
		p = ebp->buf->px + r * ebp->buf->rs;
		for (c = 0; c < ebp->width; c++) {
			*p++ = ebp->r;
			*p++ = ebp->g;
			*p++ = ebp->b;
			*p++ = ebp->a;
		}
	}
	/* Render */
	nr_arena_item_invoke_render (ebp->root, &bbox, ebp->buf);

	for (r = 0; r < num_rows; r++) {
		rows[r] = ebp->buf->px + r * ebp->buf->rs;
	}

	return num_rows;
}

void
sp_export_png_file (SPDocument *doc, const unsigned char *filename,
		    double x0, double y0, double x1, double y1,
		    unsigned int width, unsigned int height,
		    unsigned long bgcolor)
{
	NRMatrixF affine;
	gdouble t;
	NRArena *arena;
	struct SPEBP ebp;

	g_return_if_fail (doc != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (doc));
	g_return_if_fail (filename != NULL);
	g_return_if_fail (width >= 16);
	g_return_if_fail (height >= 16);

	sp_document_ensure_up_to_date (doc);

	/* Go to document coordinates */
	t = y0;
	y0 = sp_document_height (doc) - y1;
	y1 = sp_document_height (doc) - t;

	/*
	 * 1) a[0] * x0 + a[2] * y1 + a[4] = 0.0
	 * 2) a[1] * x0 + a[3] * y1 + a[5] = 0.0
	 * 3) a[0] * x1 + a[2] * y1 + a[4] = width
	 * 4) a[1] * x0 + a[3] * y0 + a[5] = height
	 * 5) a[1] = 0.0;
	 * 6) a[2] = 0.0;
	 *
	 * (1,3) a[0] * x1 - a[0] * x0 = width
	 * a[0] = width / (x1 - x0)
	 * (2,4) a[3] * y0 - a[3] * y1 = height
	 * a[3] = height / (y0 - y1)
	 * (1) a[4] = -a[0] * x0
	 * (2) a[5] = -a[3] * y1
	 */

	affine.c[0] = width / ((x1 - x0) * 1.25);
	affine.c[1] = 0.0;
	affine.c[2] = 0.0;
	affine.c[3] = height / ((y1 - y0) * 1.25);
	affine.c[4] = -affine.c[0] * x0 * 1.25;
	affine.c[5] = -affine.c[3] * y0 * 1.25;

	SP_PRINT_MATRIX ("SVG2PNG", &affine);

	ebp.width = width;
	ebp.height = height;
	ebp.r = NR_RGBA32_R (bgcolor);
	ebp.g = NR_RGBA32_G (bgcolor);
	ebp.b = NR_RGBA32_B (bgcolor);
	ebp.a = NR_RGBA32_A (bgcolor);
	/* Get RGBA buffer */
	ebp.buf = nr_buffer_get (NR_IMAGE_R8G8B8A8, width, 64, TRUE, FALSE);

	/* Create new arena */
	arena = g_object_new (NR_TYPE_ARENA, NULL);
	/* Create ArenaItem and set transform */
	ebp.root = sp_item_show (SP_ITEM (sp_document_root (doc)), arena);
	nr_arena_item_set_transform (ebp.root, &affine);


	sp_png_write_rgba_striped (filename, width, height, sp_export_get_rows, &ebp);

	/* Free Arena and ArenaItem */
	sp_item_hide (SP_ITEM (sp_document_root (doc)), arena);
	g_object_unref (G_OBJECT (arena));

	/* Release RGBA buffer */
	nr_buffer_free (ebp.buf);
}

