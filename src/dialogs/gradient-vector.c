#define __SP_GRADIENT_VECTOR_C__

/*
 * Gradient vector editing widget and dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <gtk/gtkvbox.h>
#include <gtk/gtkpreview.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "../widgets/sp-color-selector.h"
#include "../sp-gradient.h"
#include "gradient-vector.h"

#define PAD 4

static void sp_gradient_vector_widget_load_gradient (GtkWidget *widget, SPGradient *gradient);
static void sp_gradient_vector_dialog_close (GtkWidget *widget, GtkWidget *dialog);
static gint sp_gradient_vector_dialog_delete (GtkWidget *widget, GdkEvent *event, GtkWidget *dialog);

GtkWidget *
sp_gradient_vector_widget_new (SPGradient *gradient)
{
	GtkWidget *vb, *w, *f, *csel;

	g_return_val_if_fail (gradient != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT (gradient), NULL);

	vb = gtk_vbox_new (FALSE, PAD);

	w = gtk_preview_new (GTK_PREVIEW_COLOR);
	gtk_object_set_data (GTK_OBJECT (vb), "preview", w);
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vb), w, TRUE, TRUE, PAD);

	f = gtk_frame_new (_("Start color"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, PAD);
	csel = sp_color_selector_new ();
	gtk_object_set_data (GTK_OBJECT (vb), "start", csel);
	gtk_widget_show (csel);
	gtk_container_add (GTK_CONTAINER (f), csel);

	f = gtk_frame_new (_("End color"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, PAD);
	csel = sp_color_selector_new ();
	gtk_object_set_data (GTK_OBJECT (vb), "end", csel);
	gtk_widget_show (csel);
	gtk_container_add (GTK_CONTAINER (f), csel);

	gtk_widget_show (vb);

	sp_gradient_vector_widget_load_gradient (vb, gradient);

	return vb;
}

void
sp_gradient_vector_dialog (SPGradient *gradient)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL) {
		GtkWidget *w;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Gradient vector"));
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (sp_gradient_vector_dialog_delete), dialog);
		w = sp_gradient_vector_widget_new (gradient);
		gtk_object_set_data (GTK_OBJECT (dialog), "gradient-vector-widget", w);
		/* Connect signals */
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	} else {
		GtkWidget *w;
		w = gtk_object_get_data (GTK_OBJECT (dialog), "gradient-vector-widget");
		sp_gradient_vector_widget_load_gradient (w, gradient);
	}

	gtk_widget_show (dialog);
}

static void
sp_gradient_vector_widget_load_gradient (GtkWidget *widget, SPGradient *gradient)
{
	GtkWidget *w;
	guint32 cs, ce;
	gint x, y, width, height;
	gint r0, g0, b0, a0, r1, g1, b1, a1;
	gint dr, dg, db, da, cr, cg, cb, ca;
	guchar *row, *p;

	if (!gradient->stops) {
		cs = 0x00000000;
		ce = 0x00000000;
	} else {
		SPObject *o;
		cs = ((SPStop *) gradient->stops)->color;
		o = gradient->stops;
		while (o->next) o = o->next;
		ce = ((SPStop *) o)->color;
	}

	/* Set color selector values */
	w = gtk_object_get_data (GTK_OBJECT (widget), "start");
	sp_color_selector_set_rgba_uint (SP_COLOR_SELECTOR (w), cs);
	w = gtk_object_get_data (GTK_OBJECT (widget), "end");
	sp_color_selector_set_rgba_uint (SP_COLOR_SELECTOR (w), ce);

	/* Fill preview */
	w = gtk_object_get_data (GTK_OBJECT (widget), "preview");
	width = w->allocation.width;
	height = w->allocation.height;
	row = g_new (guchar, width * 3);
	r0 = (cs >> 16) & 0xff00;
	g0 = (cs >> 8) & 0xff00;
	b0 = cs & 0xff00;
	a0 = (cs << 8) & 0xff00;
	r1 = (ce >> 16) & 0xff00;
	g1 = (ce >> 8) & 0xff00;
	b1 = ce & 0xff00;
	a1 = (ce << 8) & 0xff00;
	dr = (r1 - r0 + 0x7f) / width;
	dg = (r1 - r0 + 0x7f) / width;
	db = (r1 - r0 + 0x7f) / width;
	da = (r1 - r0 + 0x7f) / width;
	/* Gradiented line 2 */
	cr = r0;
	cg = g0;
	cb = b0;
	ca = a0;
	p = row;
	for (x = 0; x < width; x++) {
		gint bg, fc;
		bg = (x & 0x8) ? 0x7f : 0xbf;
		fc = ((cr >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = ((cg >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = ((cb >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		cr += dr;
		cg += dg;
		cb += db;
		ca += da;
	}
	for (y = 0; y < height; y+= 16) {
		gint r;
		for (r = 0; r < 8; r++) {
			if ((y + r) < height) gtk_preview_draw_row (GTK_PREVIEW (w), row, 0, y + r, width);
		}
	}
	/* Gradiented line 2 */
	cr = r0;
	cg = g0;
	cb = b0;
	ca = a0;
	p = row;
	for (x = 0; x < width; x++) {
		gint bg, fc;
		bg = (x & 0x8) ? 0xbf : 0x7f;
		fc = ((cr >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = ((cg >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = ((cb >> 8) - bg) * (ca >> 8);
		*p++ = bg + ((fc + (fc >> 8) + 0x80) >> 8);
		cr += dr;
		cg += dg;
		cb += db;
		ca += da;
	}
	for (y = 0; y < height; y+= 16) {
		gint r;
		for (r = 8; r < 16; r++) {
			if ((y + r) < height) gtk_preview_draw_row (GTK_PREVIEW (w), row, 0, y + r, width);
		}
	}
	g_free (row);
}

static void
sp_gradient_vector_dialog_close (GtkWidget *widget, GtkWidget *dialog)
{
	gtk_widget_hide (dialog);
}

static gint
sp_gradient_vector_dialog_delete (GtkWidget *widget, GdkEvent *event, GtkWidget *dialog)
{
	sp_gradient_vector_dialog_close (widget, dialog);

	return TRUE;
}


