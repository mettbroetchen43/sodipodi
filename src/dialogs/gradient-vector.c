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
#include <gtk/gtkframe.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "../widgets/sp-color-selector.h"
#include "../widgets/gradient-image.h"
#include "../sp-gradient.h"
#include "../gradient-chemistry.h"
#include "gradient-vector.h"

#define PAD 4

static void sp_gradient_vector_widget_load_gradient (GtkWidget *widget, SPGradient *gradient);
static void sp_gradient_vector_dialog_close (GtkWidget *widget, GtkWidget *dialog);
static gint sp_gradient_vector_dialog_delete (GtkWidget *widget, GdkEvent *event, GtkWidget *dialog);

static void sp_gradient_vector_widget_destroy (GtkObject *object, gpointer data);
static void sp_gradient_vector_gradient_destroy (SPGradient *gradient, GtkWidget *widget);
static void sp_gradient_vector_gradient_modified (SPGradient *gradient, guint flags, GtkWidget *widget);
static void sp_gradient_vector_color_dragged (SPColorSelector *csel, GtkObject *object);
static void sp_gradient_vector_color_changed (SPColorSelector *csel, GtkObject *object);

static gboolean blocked = FALSE;

GtkWidget *
sp_gradient_vector_widget_new (SPGradient *gradient)
{
	GtkWidget *vb, *w, *f, *csel;

	g_return_val_if_fail (!gradient || SP_IS_GRADIENT (gradient), NULL);

	vb = gtk_vbox_new (FALSE, PAD);
	gtk_signal_connect (GTK_OBJECT (vb), "destroy",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_widget_destroy), NULL);

	w = sp_gradient_image_new (gradient);
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
	gtk_signal_connect (GTK_OBJECT (csel), "dragged",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_dragged), vb);
	gtk_signal_connect (GTK_OBJECT (csel), "changed",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_changed), vb);

	f = gtk_frame_new (_("End color"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, PAD);
	csel = sp_color_selector_new ();
	gtk_object_set_data (GTK_OBJECT (vb), "end", csel);
	gtk_widget_show (csel);
	gtk_container_add (GTK_CONTAINER (f), csel);
	gtk_signal_connect (GTK_OBJECT (csel), "dragged",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_dragged), vb);
	gtk_signal_connect (GTK_OBJECT (csel), "changed",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_changed), vb);

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
		gtk_container_set_border_width (GTK_CONTAINER (dialog), PAD);
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
	SPGradient *old;
	GtkWidget *w;
	guint32 cs, ce;

	old = gtk_object_get_data (GTK_OBJECT (widget), "gradient");
	if (old != gradient) {
		if (old) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (old), widget);
		}
		if (gradient) {
			gtk_signal_connect (GTK_OBJECT (gradient), "destroy",
					    GTK_SIGNAL_FUNC (sp_gradient_vector_gradient_destroy), widget);
			gtk_signal_connect (GTK_OBJECT (gradient), "modified",
					    GTK_SIGNAL_FUNC (sp_gradient_vector_gradient_modified), widget);
		}
	}

	gtk_object_set_data (GTK_OBJECT (widget), "gradient", gradient);

	if (gradient) {
		sp_gradient_ensure_vector (gradient);
		cs = sp_color_get_rgba32_falpha (&gradient->vector->stops[0].color, gradient->vector->stops[0].opacity);
		ce = sp_color_get_rgba32_falpha (&gradient->vector->stops[1].color, gradient->vector->stops[1].opacity);

		/* Set color selector values */
		w = gtk_object_get_data (GTK_OBJECT (widget), "start");
		sp_color_selector_set_rgba_uint (SP_COLOR_SELECTOR (w), cs);
		w = gtk_object_get_data (GTK_OBJECT (widget), "end");
		sp_color_selector_set_rgba_uint (SP_COLOR_SELECTOR (w), ce);

		/* Fixme: Sensitivity */
	}

	/* Fill preview */
	w = gtk_object_get_data (GTK_OBJECT (widget), "preview");
	sp_gradient_image_set_gradient (SP_GRADIENT_IMAGE (w), gradient);
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

/* Widget destroy handler */

static void
sp_gradient_vector_widget_destroy (GtkObject *object, gpointer data)
{
	GtkObject *gradient;

	gradient = gtk_object_get_data (object, "gradient");

	if (gradient) {
		/* Remove signals connected to us */
		/* fixme: may use _connect_while_alive as well */
		gtk_signal_disconnect_by_data (gradient, object);
	}
}

static void
sp_gradient_vector_gradient_destroy (SPGradient *gradient, GtkWidget *widget)
{
	sp_gradient_vector_widget_load_gradient (widget, NULL);
}

static void
sp_gradient_vector_gradient_modified (SPGradient *gradient, guint flags, GtkWidget *widget)
{
	if (!blocked) {
		blocked = TRUE;
		sp_gradient_vector_widget_load_gradient (widget, gradient);
		blocked = FALSE;
	}
}

static void
sp_gradient_vector_color_dragged (SPColorSelector *csel, GtkObject *object)
{
	SPGradient *gradient, *ngr;
	SPGradientVector *vector;
	gdouble c[4];

	if (blocked) return;

	gradient = gtk_object_get_data (object, "gradient");
	if (!gradient) return;

	blocked = TRUE;

	ngr = sp_gradient_ensure_vector_normalized (gradient);
	if (ngr != gradient) {
		/* Our master gradient has changed */
		sp_gradient_vector_widget_load_gradient (GTK_WIDGET (object), ngr);
	}

	sp_gradient_ensure_vector (ngr);

	vector = alloca (sizeof (SPGradientVector) + sizeof (SPGradientStop));
	vector->nstops = 2;
	vector->start = ngr->vector->start;
	vector->end = ngr->vector->end;

	csel = gtk_object_get_data (object, "start");
	sp_color_selector_get_rgba_double (csel, c);
	vector->stops[0].offset = 0.0;
	sp_color_set_rgb_float (&vector->stops[0].color, c[0], c[1], c[2]);
	vector->stops[0].opacity = c[3];
	csel = gtk_object_get_data (object, "end");
	sp_color_selector_get_rgba_double (csel, c);
	vector->stops[1].offset = 1.0;
	sp_color_set_rgb_float (&vector->stops[1].color, c[0], c[1], c[2]);
	vector->stops[1].opacity = c[3];

	sp_gradient_set_vector (ngr, vector);

	blocked = FALSE;
}

static void
sp_gradient_vector_color_changed (SPColorSelector *csel, GtkObject *object)
{
	SPGradient *gradient, *ngr;
	gdouble start, end;
	SPObject *child;
	guint32 color;
	gchar c[256];

	if (blocked) return;

	gradient = gtk_object_get_data (object, "gradient");
	if (!gradient) return;

	blocked = TRUE;

	ngr = sp_gradient_ensure_vector_normalized (gradient);
	if (ngr != gradient) {
		/* Our master gradient has changed */
		sp_gradient_vector_widget_load_gradient (GTK_WIDGET (object), ngr);
	}

	sp_gradient_ensure_vector (ngr);

	start = ngr->vector->start;
	end = ngr->vector->end;

	/* Set start parameters */
	/* We rely on normalized vector, i.e. stops HAVE to exist */
	for (child = ngr->stops; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) break;
	}
	g_return_if_fail (child != NULL);

	csel = gtk_object_get_data (object, "start");
	color = sp_color_selector_get_color_uint (csel);

	sp_repr_set_double_attribute (SP_OBJECT_REPR (child), "offset", start);
	g_snprintf (c, 256, "stop-color:#%06x;stop-opacity:%g;", color >> 8, (gdouble) (color & 0xff) / 255.0);
	sp_repr_set_attr (SP_OBJECT_REPR (child), "style", c);

	for (child = child->next; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) break;
	}
	g_return_if_fail (child != NULL);

	csel = gtk_object_get_data (object, "end");
	color = sp_color_selector_get_color_uint (csel);

	sp_repr_set_double_attribute (SP_OBJECT_REPR (child), "offset", end);
	g_snprintf (c, 256, "stop-color:#%06x;stop-opacity:%g;", color >> 8, (gdouble) (color & 0xff) / 255.0);
	sp_repr_set_attr (SP_OBJECT_REPR (child), "style", c);

	/* Remove other stops */
	while (child->next) {
		if (SP_IS_STOP (child->next)) {
			sp_repr_remove_child (SP_OBJECT_REPR (ngr), SP_OBJECT_REPR (child->next));
		} else {
			child = child->next;
		}
	}

	blocked = FALSE;
}

