#define __SP_EXPORT_C__

/*
 * PNG export dialog
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include <config.h>
#include <string.h>
#include <libart_lgpl/art_affine.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-file-entry.h>
#include <glade/glade.h>
#include "../macros.h"
#include "../helper/png-write.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-units.h"
#include "../desktop-handles.h"
#include "../desktop-affine.h"
#include "../selection.h"
#include "export.h"

#define SP_EXPORT_MIN_SIZE 16.0

static GladeXML * xml = NULL;
static GtkWidget * dialog = NULL;
static gboolean spin = TRUE;
static SPDesktop * desktop = NULL;

void sp_export_drawing (GtkToggleButton * tb);
void sp_export_page (GtkToggleButton * tb);
void sp_export_selection (GtkToggleButton * tb);

void sp_export_area_x0_changed (GtkSpinButton * sb);
void sp_export_area_y0_changed (GtkSpinButton * sb);
void sp_export_area_x1_changed (GtkSpinButton * sb);
void sp_export_area_y1_changed (GtkSpinButton * sb);
void sp_export_area_width_changed (GtkSpinButton * sb);
void sp_export_area_height_changed (GtkSpinButton * sb);

void sp_export_image_width_changed (GtkSpinButton * sb);
void sp_export_image_height_changed (GtkSpinButton * sb);
void sp_export_image_xdpi_changed (GtkSpinButton * sb);
void sp_export_image_ydpi_changed (GtkSpinButton * sb);

static void sp_export_set_image_y (void);

static void sp_export_do_export (SPDesktop * desktop, gchar * filename,
	gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint width, gint height);

static void sp_export_set_area (GladeXML * xml, ArtDRect * bbox);
static void sp_spin_button_set (GladeXML * xml, const gchar * name, gdouble value);
static gdouble sp_spin_button_get (GladeXML * xml, const gchar * name);

void sp_export_dialog (void)
{
	GtkWidget * tb;
	GtkWidget * fe;
	gint b;
	gdouble x0, y0, x1, y1, width, height;
	gchar * filename;

	if (SP_ACTIVE_DESKTOP == NULL) return;
	desktop = SP_ACTIVE_DESKTOP;

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "export");
		g_return_if_fail (xml != NULL);
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "export");
		g_return_if_fail (dialog != NULL);
	}

	tb = glade_xml_get_widget (xml, "export_drawing");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
	sp_export_drawing (GTK_TOGGLE_BUTTON (tb));

	if (!GTK_WIDGET_VISIBLE (dialog))
		gtk_widget_show (dialog);

	b = gnome_dialog_run (GNOME_DIALOG (dialog));

	fe = glade_xml_get_widget (xml, "export_filename");
	filename = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (fe), FALSE);

	if (b == 0) {
		x0 = sp_spin_button_get (xml, "export_area_x0");
		y0 = sp_spin_button_get (xml, "export_area_y0");
		x1 = sp_spin_button_get (xml, "export_area_x1");
		y1 = sp_spin_button_get (xml, "export_area_y1");
		width = sp_spin_button_get (xml, "export_image_width");
		height = sp_spin_button_get (xml, "export_image_height");
		sp_export_do_export (desktop, filename, x0, y0, x1, y1, width, height);
	}

	if (GTK_WIDGET_VISIBLE (dialog))
		gtk_widget_hide (dialog);
}

void
sp_export_area_x0_changed (GtkSpinButton * sb)
{
	gdouble x0, x1, xdpi, width;

	if (!spin) return;

	spin = FALSE;

	x0 = sp_spin_button_get (xml, "export_area_x0");
	x1 = sp_spin_button_get (xml, "export_area_x1");
	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	width = (x1 - x0) / POINTS_PER_INCH * xdpi;

	if (width < SP_EXPORT_MIN_SIZE) {
		width = SP_EXPORT_MIN_SIZE;
		x1 = x0 + width * POINTS_PER_INCH / xdpi;
		sp_spin_button_set (xml, "export_area_x1", x1);
	}

	sp_spin_button_set (xml, "export_area_width", x1 - x0);
	sp_spin_button_set (xml, "export_image_width", width);

	spin = TRUE;
}

void
sp_export_area_y0_changed (GtkSpinButton * sb)
{
	gdouble y0, y1, ydpi, height;

	if (!spin) return;

	spin = FALSE;

	y0 = sp_spin_button_get (xml, "export_area_y0");
	y1 = sp_spin_button_get (xml, "export_area_y1");
	ydpi = sp_spin_button_get (xml, "export_image_ydpi");
	height = (y1 - y0) / POINTS_PER_INCH * ydpi;

	if (height < SP_EXPORT_MIN_SIZE) {
		height = SP_EXPORT_MIN_SIZE;
		y1 = y0 + height * POINTS_PER_INCH / ydpi;
		sp_spin_button_set (xml, "export_area_y1", y1);
	}

	sp_spin_button_set (xml, "export_area_width", y1 - y0);
	sp_spin_button_set (xml, "export_image_width", height);

	spin = TRUE;
}

void
sp_export_area_x1_changed (GtkSpinButton * sb)
{
	gdouble x0, x1, xdpi, width;

	if (!spin) return;

	spin = FALSE;

	x0 = sp_spin_button_get (xml, "export_area_x0");
	x1 = sp_spin_button_get (xml, "export_area_x1");
	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	width = (x1 - x0) / POINTS_PER_INCH * xdpi;

	if (width < SP_EXPORT_MIN_SIZE) {
		width = SP_EXPORT_MIN_SIZE;
		x0 = x1 - width * POINTS_PER_INCH / xdpi;
		sp_spin_button_set (xml, "export_area_x0", x0);
	}

	sp_spin_button_set (xml, "export_area_width", x1 - x0);
	sp_spin_button_set (xml, "export_image_width", width);

	spin = TRUE;
}

void
sp_export_area_y1_changed (GtkSpinButton * sb)
{
	gdouble y0, y1, ydpi, height;

	if (!spin) return;

	spin = FALSE;

	y0 = sp_spin_button_get (xml, "export_area_y0");
	y1 = sp_spin_button_get (xml, "export_area_y1");
	ydpi = sp_spin_button_get (xml, "export_image_ydpi");
	height = (y1 - y0) / POINTS_PER_INCH * ydpi;

	if (height < SP_EXPORT_MIN_SIZE) {
		height = SP_EXPORT_MIN_SIZE;
		y0 = y1 - height * POINTS_PER_INCH / ydpi;
		sp_spin_button_set (xml, "export_area_y0", y0);
	}

	sp_spin_button_set (xml, "export_area_width", y1 - y0);
	sp_spin_button_set (xml, "export_image_width", height);

	spin = TRUE;
}

void
sp_export_area_width_changed (GtkSpinButton * sb)
{
	gdouble width, x0, x1, xdpi, iw;

	if (!spin) return;

	spin = FALSE;

	width = sp_spin_button_get (xml, "export_area_width");
	x0 = sp_spin_button_get (xml, "export_area_x0");
	x1 = sp_spin_button_get (xml, "export_area_x1");
	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	iw = width / POINTS_PER_INCH * xdpi;

	if (iw < SP_EXPORT_MIN_SIZE) {
		iw = SP_EXPORT_MIN_SIZE;
		width = iw * POINTS_PER_INCH / xdpi;
		sp_spin_button_set (xml, "export_area_width", width);
	}

	sp_spin_button_set (xml, "export_area_x1", x0 + width);
	sp_spin_button_set (xml, "export_image_width", iw);

	spin = TRUE;
}

void
sp_export_area_height_changed (GtkSpinButton * sb)
{
	gdouble height, y0, y1, ydpi, ih;

	if (!spin) return;

	spin = FALSE;

	height = sp_spin_button_get (xml, "export_area_height");
	y0 = sp_spin_button_get (xml, "export_area_y0");
	y1 = sp_spin_button_get (xml, "export_area_y1");
	ydpi = sp_spin_button_get (xml, "export_image_ydpi");
	ih = height / POINTS_PER_INCH * ydpi;

	if (ih < SP_EXPORT_MIN_SIZE) {
		ih = SP_EXPORT_MIN_SIZE;
		height = ih * POINTS_PER_INCH / ydpi;
		sp_spin_button_set (xml, "export_area_height", height);
	}

	sp_spin_button_set (xml, "export_area_y1", y0 + height);
	sp_spin_button_set (xml, "export_image_height", ih);

	spin = TRUE;
}

void
sp_export_image_width_changed (GtkSpinButton * sb)
{
	gdouble x0, x1, xdpi, width;

	if (!spin) return;

	spin = FALSE;

	width = sp_spin_button_get (xml, "export_image_width");
	x0 = sp_spin_button_get (xml, "export_area_x0");
	x1 = sp_spin_button_get (xml, "export_area_x1");

	if (width < SP_EXPORT_MIN_SIZE) {
		width = SP_EXPORT_MIN_SIZE;
		sp_spin_button_set (xml, "export_image_width", width);
	}

	xdpi = width * POINTS_PER_INCH / (x1 - x0);

	sp_spin_button_set (xml, "export_image_xdpi", xdpi);

	sp_export_set_image_y ();

	spin = TRUE;
}

void
sp_export_image_xdpi_changed (GtkSpinButton * sb)
{
	gdouble x0, x1, xdpi, width;

	if (!spin) return;

	spin = FALSE;

	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	x0 = sp_spin_button_get (xml, "export_area_x0");
	x1 = sp_spin_button_get (xml, "export_area_x1");

	width = (x1 - x0) / POINTS_PER_INCH * xdpi;

	if (width < SP_EXPORT_MIN_SIZE) {
		width = SP_EXPORT_MIN_SIZE;
		xdpi = width * POINTS_PER_INCH / (x1 - x0);
		sp_spin_button_set (xml, "export_image_xdpi", xdpi);
	}

	sp_spin_button_set (xml, "export_image_width", width);

	sp_export_set_image_y ();

	spin = TRUE;
}

static void
sp_export_set_image_y (void)
{
	gdouble xdpi, y0, y1;

	spin = FALSE;

	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	y0 = sp_spin_button_get (xml, "export_area_y0");
	y1 = sp_spin_button_get (xml, "export_area_y1");

	sp_spin_button_set (xml, "export_image_ydpi", xdpi);
	sp_spin_button_set (xml, "export_image_height", (y1 - y0) / POINTS_PER_INCH * xdpi);

	spin = TRUE;
}

void
sp_export_drawing (GtkToggleButton * tb)
{
	SPDesktop * desktop;
	SPDocument * doc;
	ArtDRect bbox;

	if (!gtk_toggle_button_get_active (tb)) return;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail (desktop != NULL);

	doc = SP_DT_DOCUMENT (desktop);

	sp_item_bbox_desktop (SP_ITEM (sp_document_root (doc)), &bbox);

	sp_export_set_area (xml, &bbox);
}

void
sp_export_page (GtkToggleButton * tb)
{
	SPDesktop * desktop;
	SPDocument * doc;
	ArtDRect bbox;

	if (!gtk_toggle_button_get_active (tb)) return;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail (desktop != NULL);

	doc = SP_DT_DOCUMENT (desktop);

	bbox.x0 = 0.0;
	bbox.y0 = 0.0;
	bbox.x1 = sp_document_width (doc);
	bbox.y1 = sp_document_height (doc);

	sp_export_set_area (xml, &bbox);
}

void
sp_export_selection (GtkToggleButton * tb)
{
	SPDesktop * desktop;
	SPSelection * sel;
	ArtDRect bbox;

	if (!gtk_toggle_button_get_active (tb)) return;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail (desktop != NULL);

	sel = SP_DT_SELECTION (desktop);

	sp_selection_bbox (sel, &bbox);

	sp_export_set_area (xml, &bbox);
}

static void
sp_export_set_area (GladeXML * xml, ArtDRect * bbox)
{
	gdouble xdpi, ydpi;

	g_return_if_fail (xml != NULL);
	g_return_if_fail (bbox != NULL);

	spin = FALSE;

	sp_spin_button_set (xml, "export_area_x0", bbox->x0);
	sp_spin_button_set (xml, "export_area_y0", bbox->y0);
	sp_spin_button_set (xml, "export_area_x1", bbox->x1);
	sp_spin_button_set (xml, "export_area_y1", bbox->y1);
	sp_spin_button_set (xml, "export_area_width", bbox->x1 - bbox->x0);
	sp_spin_button_set (xml, "export_area_height", bbox->y1 - bbox->y0);

	xdpi = sp_spin_button_get (xml, "export_image_xdpi");
	ydpi = sp_spin_button_get (xml, "export_image_ydpi");

	sp_spin_button_set (xml, "export_image_width", (bbox->x1 - bbox->x0) / 72.0 * xdpi);
	sp_spin_button_set (xml, "export_image_height", (bbox->y1 - bbox->y0) / 72.0 * ydpi);

	spin = TRUE;
}

static void
sp_spin_button_set (GladeXML * xml, const gchar * name, gdouble value)
{

	GtkWidget * sb;

	g_return_if_fail (xml != NULL);
	g_return_if_fail (name != NULL);

	sb = glade_xml_get_widget (xml, name);
	g_return_if_fail (sb != NULL);
	g_return_if_fail (GTK_IS_SPIN_BUTTON (sb));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (sb), value);
}

static gdouble
sp_spin_button_get (GladeXML * xml, const gchar * name)
{

	GtkWidget * sb;

	g_return_val_if_fail (xml != NULL, 1.0);
	g_return_val_if_fail (name != NULL, 1.0);

	sb = glade_xml_get_widget (xml, name);
	g_return_val_if_fail (sb != NULL, 1.0);
	g_return_val_if_fail (GTK_IS_SPIN_BUTTON (sb), 1.0);

	return gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (sb));
}

#include <display/nr-arena-item.h>
#include <display/nr-arena.h>

struct SPEBP {
	int width, height;
	NRArenaItem *root;
	NRBuffer *b;
};

static int
sp_export_get_rows (const unsigned char **rows, int row, int num_rows, void *data)
{
	struct SPEBP *ebp;
	NRRectL bbox;
	NRGC gc;
	int r;

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
		memset (ebp->b->px + r * ebp->b->rs, 0x0, 4 * ebp->width);
	}
	/* Render */
	nr_arena_item_invoke_render (ebp->root, &bbox, ebp->b);

	for (r = 0; r < num_rows; r++) {
		rows[r] = ebp->b->px + r * ebp->b->rs;
	}

	return num_rows;
}

static void
sp_export_do_export (SPDesktop *desktop, gchar *filename, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint width, gint height)
{
	SPDocument *doc;
	gdouble affine[6], t;
	NRArena *arena;
	struct SPEBP ebp;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (filename != NULL);
	g_return_if_fail (width >= 16);
	g_return_if_fail (height >= 16);

	doc = SP_DT_DOCUMENT (desktop);

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

	affine[0] = width / ((x1 - x0) * 1.25);
	affine[1] = 0.0;
	affine[2] = 0.0;
	affine[3] = height / ((y1 - y0) * 1.25);
	affine[4] = -affine[0] * x0 * 1.25;
	affine[5] = -affine[3] * y0 * 1.25;

	SP_PRINT_TRANSFORM ("SVG2PNG", affine);

	ebp.width = width;
	ebp.height = height;
	/* Get RGBA buffer */
	ebp.b = nr_buffer_get (NR_IMAGE_R8G8B8A8, width, 64, TRUE, FALSE);

	/* Create new arena */
	arena = gtk_type_new (NR_TYPE_ARENA);
	/* Create ArenaItem and set transform */
	ebp.root = sp_item_show (SP_ITEM (sp_document_root (doc)), arena);
	nr_arena_item_set_transform (ebp.root, affine);


	sp_png_write_rgba_striped (filename, width, height, sp_export_get_rows, &ebp);

	/* Free Arena and ArenaItem */
	sp_item_hide (SP_ITEM (sp_document_root (doc)), arena);
	gtk_object_unref (GTK_OBJECT (arena));

	/* Release RGBA buffer */
	nr_buffer_free (ebp.b);
}

